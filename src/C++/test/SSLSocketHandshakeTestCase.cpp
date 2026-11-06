/****************************************************************************
** This file is part of the QuickFIX FIX Engine
** See http://www.quickfixengine.org/LICENSE for licensing information.
****************************************************************************/

#ifdef _MSC_VER
#include "stdafx.h"
#else
#include "config.h"
#endif

#if (HAVE_SSL > 0)

#include <UnitTest++.h>
#include <SSLSocketInitiator.h>
#include <MessageStore.h>
#include <Parser.h>
#include <fix44/Logon.h>
#include <fix44/TestRequest.h>
#include <sstream>
#include <stdexcept>

SUITE(SSLSocketHandshakeTests)
{
void requireTLSFixture( bool condition )
{
  if( !condition ) throw std::runtime_error( "TLS peer setup failed" );
}

void waitReadable( int socket )
{
  fd_set ready;
  FD_ZERO( &ready );
  FD_SET( socket, &ready );
  // A failure watchdog for transport setup, never an elapsed-time oracle.
  timeval watchdog = { 5, 0 };
  requireTLSFixture( ::select( socket + 1, &ready, 0, 0, &watchdog ) > 0 );
}

struct HandshakeSocketRuntime
{
  HandshakeSocketRuntime() { FIX::socket_init(); }
  ~HandshakeSocketRuntime() { FIX::socket_term(); }
};

struct HandshakePeer : HandshakeSocketRuntime
{
  HandshakePeer() : listener( -1 ), socket( -1 ), port( 0 ), ssl( 0 ),
                    receivedLogon( false ), receivedHeartbeat( false )
  {
    listener = ::socket( PF_INET, SOCK_STREAM, IPPROTO_TCP );
    requireTLSFixture( listener >= 0 );
    sockaddr_in address = sockaddr_in();
    address.sin_family = PF_INET;
    address.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
    requireTLSFixture( ::bind( listener, reinterpret_cast<sockaddr*>( &address ),
                              sizeof(address) ) == 0 );
    requireTLSFixture( ::listen( listener, 1 ) == 0 );
    socklen_t size = sizeof(address);
    requireTLSFixture( ::getsockname( listener, reinterpret_cast<sockaddr*>( &address ),
                                     &size ) == 0 );
    port = ntohs( address.sin_port );
    FIX::socket_setnonblock( listener );
  }

  ~HandshakePeer()
  {
    if( ssl ) SSL_free( ssl );
    if( socket >= 0 ) FIX::socket_close( socket );
    FIX::socket_close( listener );
  }

  void accept( FIX::Initiator& initiator )
  {
    waitReadable( listener );
    socket = ::accept( listener, 0, 0 );
    requireTLSFixture( socket >= 0 );
    FIX::socket_setnonblock( socket );
    FIX::socket_setsockopt( socket, TCP_NODELAY );
    requireTLSFixture( initiator.poll(0) );
    waitReadable( socket );
  }

  bool closed()
  {
    char buffer[4096];
    int result;
    do
    {
      waitReadable( socket );
      result = ::recv( socket, buffer, sizeof(buffer), 0 );
    } while( result > 0 );
    return result == 0;
  }

  void step( SSL_CTX* context )
  {
    if( !ssl )
    {
      ssl = SSL_new( context );
      requireTLSFixture( ssl && SSL_set_fd( ssl, socket ) == 1 );
    }
    if( !SSL_is_init_finished( ssl ) )
    {
      ERR_clear_error();
      int result = SSL_accept( ssl );
      if( result != 1 )
      {
        int error = SSL_get_error( ssl, result );
        requireTLSFixture( error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE );
        return;
      }
    }

    char buffer[4096];
    ERR_clear_error();
    int result = SSL_read( ssl, buffer, sizeof(buffer) );
    if( result > 0 ) parser.addToStream( buffer, result );
    else
    {
      int error = SSL_get_error( ssl, result );
      requireTLSFixture( error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE );
    }

    std::string wire;
    while( parser.readFixMessage( wire ) )
    {
      FIX::Message message( wire );
      if( message.getHeader().getField( FIX::FIELD::MsgType ) == "A" )
      {
        receivedLogon = true;
        sender = message.getHeader().getField( FIX::FIELD::SenderCompID );
        FIX44::Logon logon( FIX::EncryptMethod(0), FIX::HeartBtInt(30) );
        reply( logon, 1 );
      }
      else if( message.getHeader().getField( FIX::FIELD::MsgType ) == "0" )
        receivedHeartbeat = message.getField( FIX::FIELD::TestReqID ) == "tls-handshake";
    }
    if( !outgoing.empty() )
    {
      ERR_clear_error();
      result = SSL_write( ssl, outgoing.data(), static_cast<int>(outgoing.size()) );
      if( result > 0 ) outgoing.erase( 0, result );
      else
      {
        int error = SSL_get_error( ssl, result );
        requireTLSFixture( error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE );
      }
    }
  }

  void reply( FIX::Message& message, int sequence )
  {
    message.getHeader().setField( FIX::SenderCompID("LOCAL") );
    message.getHeader().setField( FIX::TargetCompID(sender) );
    message.getHeader().setField( FIX::MsgSeqNum(sequence) );
    message.getHeader().setField( FIX::SendingTime() );
    outgoing += message.toString();
  }

  int listener, socket;
  unsigned short port;
  SSL* ssl;
  FIX::Parser parser;
  std::string outgoing, sender;
  bool receivedLogon, receivedHeartbeat;
};

struct HandshakeContext
{
  HandshakeContext() : context( 0 )
  {
    FIX::ssl_init();
    context = SSL_CTX_new( SSLv23_server_method() );
    requireTLSFixture( context != 0 );
    requireTLSFixture( SSL_CTX_use_certificate_file( context,
      "../bin/cfg/certs/127_0_0_1_server.crt", SSL_FILETYPE_PEM ) == 1 );
    requireTLSFixture( SSL_CTX_use_PrivateKey_file( context,
      "../bin/cfg/certs/127_0_0_1_server.key", SSL_FILETYPE_PEM ) == 1 );
  }

  ~HandshakeContext()
  {
    SSL_CTX_free( context );
    FIX::ssl_term();
  }

  SSL_CTX* context;
};

FIX::SessionSettings handshakeSettings( unsigned short port, unsigned short secondPort = 0 )
{
  std::ostringstream config;
  config << "[DEFAULT]\nConnectionType=initiator\nReconnectInterval=2147483647\n"
         << "StartTime=00:00:00\nEndTime=00:00:00\nHeartBtInt=30\n"
         << "UseDataDictionary=N\nSocketConnectHost=127.0.0.1\nSocketNodelay=Y\n"
         << "[SESSION]\nBeginString=FIX.4.4\nSenderCompID=A_SILENT\n"
         << "TargetCompID=LOCAL\nSocketConnectPort=" << port << "\n";
  if( secondPort )
    config << "[SESSION]\nBeginString=FIX.4.4\nSenderCompID=B_READY\n"
           << "TargetCompID=LOCAL\nSocketConnectPort=" << secondPort << "\n";
  std::istringstream input( config.str() );
  return FIX::SessionSettings( input );
}

struct HandshakeApplication : FIX::NullApplication
{
  HandshakeApplication() : logons( 0 ) {}

  void toAdmin( FIX::Message& message, const FIX::SessionID& )
  {
    if( message.getHeader().getField( FIX::FIELD::MsgType ) == "A" ) ++logons;
  }

  void onLogon( const FIX::SessionID& session )
  { loggedOn.insert( session ); }

  int logons;
  std::set<FIX::SessionID> loggedOn;
};

// Native OpenSSL callbacks withhold one real client write. The next writable
// poll must resume it without any additional bytes from the server.
struct HandshakeClient
{
  HandshakeClient() : client( 0 ), writeBIO( 0 ), blocked( false )
  {
    static int index = SSL_get_ex_new_index( 0, 0, capture, 0, 0 );
    requireTLSFixture( index >= 0 );
    active = this;
  }

  ~HandshakeClient()
  {
    active = 0;
    if( writeBIO ) BIO_set_callback( writeBIO, 0 );
  }

  void arm()
  {
    active = 0;
    requireTLSFixture( client != 0 );
    writeBIO = SSL_get_wbio( client );
    BIO_set_callback_arg( writeBIO, reinterpret_cast<char*>(this) );
    BIO_set_callback( writeBIO, retryWrite );
  }

#if OPENSSL_VERSION_NUMBER < 0x10100000L
  static int capture( void* parent, void*, CRYPTO_EX_DATA*, int, long, void* )
#else
  static void capture( void* parent, void*, CRYPTO_EX_DATA*, int, long, void* )
#endif
  {
    if( active && !active->client ) active->client = static_cast<SSL*>(parent);
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    return 1;
#endif
  }

  static long retryWrite( BIO* bio, int operation, const char*, int, long, long result )
  {
    HandshakeClient* retry = reinterpret_cast<HandshakeClient*>(BIO_get_callback_arg(bio));
    if( operation == BIO_CB_FREE ) retry->writeBIO = 0;
    if( operation == BIO_CB_WRITE && !retry->blocked )
    {
      retry->blocked = true;
      BIO_clear_retry_flags( bio );
      BIO_set_retry_write( bio );
      return -1;
    }
    return result;
  }

  static HandshakeClient* active;
  SSL* client;
  BIO* writeBIO;
  bool blocked;
};

HandshakeClient* HandshakeClient::active = 0;

struct ActivationStore : FIX::MemoryStore
{
  ActivationStore() : armed( false ), failed( false ) {}

  FIX::UtcTimeStamp getCreationTime() const EXCEPT( FIX::IOException )
  {
    if( armed )
    {
      failed = true;
      throw FIX::IOException( "activation store failure" );
    }
    return FIX::MemoryStore::getCreationTime();
  }

  bool armed;
  mutable bool failed;
};

struct ActivationStoreFactory : FIX::MemoryStoreFactory
{
  ActivationStoreFactory() : store( 0 ) {}
  FIX::MessageStore* create( const FIX::SessionID& )
  { return store = new ActivationStore; }
  ActivationStore* store;
};

TEST(pendingHandshakeYieldsForSilentPeer)
{
  HandshakePeer peer;
  HandshakeApplication application;
  FIX::MemoryStoreFactory store;
  FIX::SessionSettings settings = handshakeSettings( peer.port );
  {
    FIX::SSLSocketInitiator initiator( application, store, settings );
    CHECK( initiator.poll( 0 ) );
    peer.accept( initiator );
    char recordType = 0;
    CHECK_EQUAL( 1, ::recv( peer.socket, &recordType, 1, MSG_PEEK ) );
    CHECK_EQUAL( 22, static_cast<int>(recordType) );
    CHECK_EQUAL( 0, application.logons );
    CHECK( initiator.poll( 0 ) );
    CHECK_EQUAL( 0, application.logons );
    initiator.stop( true );
    CHECK( initiator.isStopped() );
    CHECK( !initiator.poll( 0 ) );
  }
  // No peer response was needed for stop or for release of its owned socket.
  CHECK( peer.closed() );
}

TEST(pendingHandshakeDoesNotBlockOtherSessionAndResumes)
{
  HandshakeContext context;
  HandshakePeer silent, ready;
  HandshakeApplication application;
  FIX::MemoryStoreFactory store;
  FIX::SessionSettings settings = handshakeSettings( silent.port, ready.port );
  FIX::SSLSocketInitiator initiator( application, store, settings );
  FIX::SessionID first( "FIX.4.4", "A_SILENT", "LOCAL" );
  FIX::SessionID second( "FIX.4.4", "B_READY", "LOCAL" );

  CHECK( initiator.poll( 0 ) );
  silent.accept( initiator );
  ready.accept( initiator );
  for( int turn = 0; turn < 32 && !application.loggedOn.count(second); ++turn )
  {
    ready.step( context.context );
    initiator.poll( 0 );
  }
  CHECK( ready.receivedLogon );
  CHECK_EQUAL( "B_READY", ready.sender );
  CHECK( application.loggedOn.count(second) == 1 );
  CHECK( application.loggedOn.count(first) == 0 );
  CHECK_EQUAL( 1, application.logons );

  FIX44::TestRequest request( FIX::TestReqID("tls-handshake") );
  ready.reply( request, 2 );
  for( int turn = 0; turn < 32 && !ready.receivedHeartbeat; ++turn )
  {
    ready.step( context.context );
    initiator.poll( 0 );
  }
  CHECK( ready.receivedHeartbeat );

  for( int turn = 0; turn < 32 && !application.loggedOn.count(first); ++turn )
  {
    silent.step( context.context );
    initiator.poll( 0 );
  }
  CHECK( silent.receivedLogon );
  CHECK_EQUAL( "A_SILENT", silent.sender );
  CHECK( application.loggedOn.count(first) == 1 );
  CHECK_EQUAL( 2, application.logons );
  initiator.stop( true );
}

TEST(pendingHandshakeResumesOnWriteReadiness)
{
  HandshakeContext context;
  HandshakePeer peer;
  HandshakeApplication application;
  FIX::MemoryStoreFactory store;
  FIX::SessionSettings settings = handshakeSettings( peer.port );
  FIX::SSLSocketInitiator initiator( application, store, settings );
  {
    HandshakeClient retry;
    CHECK( initiator.poll( 0 ) );
    peer.accept( initiator );
    retry.arm();
    for( int turn = 0; turn < 32 && application.loggedOn.empty(); ++turn )
    {
      peer.step( context.context );
      initiator.poll( 0 );
    }
    CHECK( retry.blocked );
    CHECK( peer.receivedLogon );
    CHECK_EQUAL( "A_SILENT", peer.sender );
    CHECK_EQUAL( 1U, application.loggedOn.size() );
    CHECK_EQUAL( 1, application.logons );
  }
  initiator.stop( true );
}

TEST(activationStoreFailureRetiresPendingHandshake)
{
  HandshakeContext context;
  HandshakePeer peer;
  HandshakeApplication application;
  ActivationStoreFactory store;
  FIX::SessionSettings settings = handshakeSettings( peer.port );
  FIX::SSLSocketInitiator initiator( application, store, settings );

  CHECK( initiator.poll( 0 ) );
  peer.accept( initiator );
  // getCreationTime is allowed to throw IOException. Initial session creation
  // succeeds; only deferred activation of the completed TLS connection fails.
  store.store->armed = true;
  for( int turn = 0; turn < 32 && !store.store->failed; ++turn )
  {
    peer.step( context.context );
    initiator.poll( 0 );
  }
  CHECK( store.store->failed );
  store.store->armed = false;
  CHECK_EQUAL( 0, application.logons );
  CHECK( application.loggedOn.empty() );
  CHECK( initiator.poll( 0 ) );
  CHECK( peer.closed() );
  initiator.stop( true );
}

TEST(handshakeCompletionConsumesBufferedFIX)
{
  HandshakeContext context;
#ifdef SSL_OP_NO_TLSv1_3
  SSL_CTX_set_options( context.context, SSL_OP_NO_TLSv1_3 );
#endif
  HandshakePeer peer;
  HandshakeApplication application;
  FIX::MemoryStoreFactory store;
  FIX::SessionSettings settings = handshakeSettings( peer.port );
  FIX::SSLSocketInitiator initiator( application, store, settings );
  {
    HandshakeClient client;
    CHECK( initiator.poll( 0 ) );
    peer.accept( initiator );
    requireTLSFixture( client.client != 0 );
    SSL_set_read_ahead( client.client, 1 );
    peer.sender = "A_SILENT";
    FIX44::Logon logon( FIX::EncryptMethod(0), FIX::HeartBtInt(30) );
    peer.reply( logon, 1 );
    // TLS 1.2 sends its last server flight and the queued FIX bytes in one
    // peer turn. Read-ahead can leave FIX in SSL after the socket is drained.
    for( int turn = 0; turn < 32 && application.loggedOn.empty(); ++turn )
    {
      if( !peer.ssl || !SSL_is_init_finished(peer.ssl) ) peer.step( context.context );
      initiator.poll( 0 );
    }
    CHECK_EQUAL( 1U, application.loggedOn.size() );
    CHECK_EQUAL( 1, application.logons );
  }
  initiator.stop( true );
}

TEST(fatalPendingHandshakeClosesAndFreshAttemptLogsOn)
{
  HandshakeContext context;
  HandshakePeer peer;
  HandshakeApplication application;
  FIX::MemoryStoreFactory store;
  FIX::SessionSettings settings = handshakeSettings( peer.port );
  FIX::Dictionary defaults = settings.get();
  defaults.setInt( "ReconnectInterval", 0 );
  settings.set( defaults );
  FIX::SSLSocketInitiator initiator( application, store, settings );

  CHECK( initiator.poll( 0 ) );
  peer.accept( initiator );
  // TLS fatal handshake_failure alert, before any TLS session is established.
  const char alert[] = "\025\003\003\000\002\002\050";
  CHECK_EQUAL( 7, FIX::socket_send(peer.socket, alert, 7) );
  CHECK( initiator.poll( 0 ) );
  CHECK_EQUAL( 0, application.logons );
  CHECK( peer.closed() );
  FIX::socket_close( peer.socket );
  peer.socket = -1;

  CHECK( initiator.poll( 0 ) );
  peer.accept( initiator );
  for( int turn = 0; turn < 32 && application.loggedOn.empty(); ++turn )
  {
    peer.step( context.context );
    initiator.poll( 0 );
  }
  CHECK( peer.receivedLogon );
  CHECK_EQUAL( "A_SILENT", peer.sender );
  CHECK_EQUAL( 1U, application.loggedOn.size() );
  CHECK_EQUAL( 1, application.logons );
  initiator.stop( true );
}
}

#endif
