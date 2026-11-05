/****************************************************************************
** Copyright (c) 2001-2014
**
** This file is part of the QuickFIX FIX Engine and is distributed under the
** quickfixengine.org license as described in the LICENSE file.
****************************************************************************/

#include "config.h"

#if HAVE_SSL && defined(__linux__)

#include <UnitTest++.h>
#include <SSLSocketInitiator.h>
#include <MessageStore.h>
#include <dirent.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <list>
#include <map>
#include <sstream>
#include <stdexcept>

namespace
{
typedef std::map<int, std::string> TLSDescriptors;

std::string tlsDescriptorIdentity( int descriptor )
{
  char path[128], target[512];
  std::sprintf( path, "/proc/self/fd/%d", descriptor );
  const ssize_t length = readlink( path, target, sizeof(target) - 1 );
  if( length < 0 ) return std::string();
  target[length] = 0;
  return target;
}

TLSDescriptors tlsDescriptors()
{
  TLSDescriptors result;
  DIR* directory = opendir( "/proc/self/fd" );
  if( !directory ) throw std::runtime_error( "Cannot enumerate fixture descriptors" );
  const int enumeration = dirfd( directory );
  struct dirent* item;
  while( (item = readdir(directory)) != 0 )
  {
    char* end = 0;
    const long descriptor = std::strtol( item->d_name, &end, 10 );
    if( !*item->d_name || *end || descriptor == enumeration ) continue;
    char path[128], target[512];
    std::sprintf( path, "/proc/self/fd/%ld", descriptor );
    const ssize_t length = readlink( path, target, sizeof(target) - 1 );
    if( length < 0 )
    {
      closedir( directory );
      throw std::runtime_error( "Cannot identify fixture descriptor" );
    }
    target[length] = 0;
    result[static_cast<int>(descriptor)] = target;
  }
  closedir( directory );
  return result;
}

struct TLSLifetime
{
  TLSLifetime() : allocated(0), released(0), tagFailures(0), cancelled(false) {}
  int allocated, released, tagFailures;
  bool cancelled;
};

// OpenSSL can retain a failed baseline SSL past fixture destruction. Both the
// callback state and its tags therefore remain valid until process exit.
struct TLSLedger
{
  TLSLedger() : active(0), index(-1)
  {
    pthread_mutex_init( &mutex, 0 );
    pthread_cond_init( &changed, 0 );
  }
  pthread_mutex_t mutex;
  pthread_cond_t changed;
  TLSLifetime* active;
  TLSLifetime excluded;
  std::list<TLSLifetime> lifetimes;
  int index;
};

#if OPENSSL_VERSION_NUMBER < 0x10100000L
int tlsAllocated( void*, void*, CRYPTO_EX_DATA* data, int index, long, void* argument )
#else
void tlsAllocated( void*, void*, CRYPTO_EX_DATA* data, int index, long, void* argument )
#endif
{
  TLSLedger& ledger = *static_cast<TLSLedger*>( argument );
  pthread_mutex_lock( &ledger.mutex );
  TLSLifetime* lifetime = ledger.active;
  if( lifetime )
  {
    ++lifetime->allocated;
    if( !CRYPTO_set_ex_data(data, index, lifetime) ) ++lifetime->tagFailures;
    pthread_cond_broadcast( &ledger.changed );
  }
  pthread_mutex_unlock( &ledger.mutex );
#if OPENSSL_VERSION_NUMBER < 0x10100000L
  return 1;
#endif
}

void tlsReleased( void*, void* tag, CRYPTO_EX_DATA*, int, long, void* argument )
{
  TLSLedger& ledger = *static_cast<TLSLedger*>( argument );
  TLSLifetime* lifetime = static_cast<TLSLifetime*>( tag );
  pthread_mutex_lock( &ledger.mutex );
  if( lifetime && lifetime != &ledger.excluded ) ++lifetime->released;
  pthread_mutex_unlock( &ledger.mutex );
}

TLSLedger& tlsLedger()
{
  static TLSLedger* ledger = new TLSLedger;
  if( ledger->index < 0 )
  {
    ledger->index = SSL_get_ex_new_index( 0, ledger, tlsAllocated, 0, tlsReleased );
    if( ledger->index < 0 ) throw std::runtime_error( "Cannot observe native SSL lifetime" );
  }
  return *ledger;
}

class TLSListener
{
public:
  TLSListener() : descriptor(-1), port(0)
  {
    descriptor = socket( AF_INET, SOCK_STREAM, 0 );
    struct sockaddr_in address;
    std::memset( &address, 0, sizeof(address) );
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
    socklen_t length = sizeof(address);
    if( descriptor < 0 || ::bind(descriptor, reinterpret_cast<sockaddr*>(&address), length)
        || listen(descriptor, 2)
        || getsockname(descriptor, reinterpret_cast<sockaddr*>(&address), &length) )
    {
      if( descriptor >= 0 ) close( descriptor );
      throw std::runtime_error( "Cannot create loopback TLS listener" );
    }
    port = ntohs( address.sin_port );
  }
  ~TLSListener() { close( descriptor ); }
  int descriptor;
  unsigned short port;
private:
  TLSListener( const TLSListener& );
  TLSListener& operator=( const TLSListener& );
};

FIX::SessionSettings tlsSettings( unsigned short port, bool paired = false )
{
  std::ostringstream text;
  text << "[DEFAULT]\nConnectionType=initiator\nReconnectInterval=2147483647\n"
       << "StartTime=00:00:00\nEndTime=00:00:00\nHeartBtInt=30\nUseDataDictionary=N\n"
       << "SocketConnectHost=127.0.0.1\nSocketConnectPort=" << port << "\n"
       << "[SESSION]\nBeginString=FIX.4.4\nSenderCompID=TLSLOCAL\nTargetCompID="
       << (paired ? "TLSA" : "TLSPEER") << "\n";
  if( paired )
    text << "[SESSION]\nBeginString=FIX.4.4\nSenderCompID=TLSLOCAL\nTargetCompID=TLSB\n";
  std::istringstream input( text.str() );
  return FIX::SessionSettings( input );
}

class TLSApplication : public FIX::NullApplication
{
public:
  TLSApplication() : logons(0), logouts(0) {}
  void onLogon( const FIX::SessionID& ) { ++logons; }
  void onLogout( const FIX::SessionID& ) { ++logouts; }
  int logons, logouts;
};

class TLSConnectOnce : public FIX::SSLSocketInitiator
{
public:
  TLSConnectOnce( FIX::Application& application, FIX::MessageStoreFactory& store,
                  const FIX::SessionSettings& settings ) EXCEPT(FIX::ConfigError)
  : FIX::SSLSocketInitiator(application, store, settings) {}
  bool pending( const FIX::SessionID& id ) { return isPending(id); }
  bool connected( const FIX::SessionID& id ) { return isConnected(id); }
  bool disconnected( const FIX::SessionID& id ) { return isDisconnected(id); }
  TLSDescriptors before;
private:
  void onStart()
  {
    before = tlsDescriptors();
    connect();
  }
};

struct TLSFixture
{
  enum PeerMode { invalidTLS, successfulTLS, reusedSocket };

  TLSFixture()
  : ledger(tlsLedger()), lifetime(0), initiator(0), running(false),
    peerOK(true), accepts(0), mode(invalidTLS), ready(false), released(false),
    serverContext(0), serverSSL(0)
  {
    pthread_mutex_init( &peerMutex, 0 );
    pthread_cond_init( &peerChanged, 0 );
    peerSockets[0] = peerSockets[1] = -1;
    clientSockets[0] = clientSockets[1] = -1;
    ledger.lifetimes.push_back( TLSLifetime() );
    lifetime = &ledger.lifetimes.back();
    fixtureOnly = tlsDescriptors();
    initiator = new TLSConnectOnce( application, store, tlsSettings(listener.port) );
  }

  ~TLSFixture()
  {
    destroyInitiator();
    if( running )
    {
      pthread_mutex_lock( &ledger.mutex );
      lifetime->cancelled = true;
      pthread_cond_broadcast( &ledger.changed );
      pthread_mutex_unlock( &ledger.mutex );
      shutdown( listener.descriptor, SHUT_RDWR );
      pthread_mutex_lock( &peerMutex );
      released = true;
      pthread_cond_broadcast( &peerChanged );
      for( int i = 0; i != 2; ++i )
        if( peerSockets[i] >= 0 ) shutdown( peerSockets[i], SHUT_RDWR );
      pthread_mutex_unlock( &peerMutex );
      FIX::thread_join( thread );
    }
    pthread_mutex_lock( &ledger.mutex );
    ledger.active = 0;
    pthread_mutex_unlock( &ledger.mutex );
    if( serverSSL ) SSL_free( serverSSL );
    if( serverContext ) SSL_CTX_free( serverContext );
    pthread_cond_destroy( &peerChanged );
    pthread_mutex_destroy( &peerMutex );
  }

  void destroyInitiator()
  {
    if( !initiator ) return;
    initiator->stop( true );
    delete initiator;
    initiator = 0;
  }

  void prepareTLS()
  {
    serverContext = SSL_CTX_new( SSLv23_server_method() );
    if( !serverContext
        || SSL_CTX_use_certificate_file(serverContext,
             "../bin/cfg/certs/127_0_0_1_server.crt", SSL_FILETYPE_PEM) != 1
        || SSL_CTX_use_PrivateKey_file(serverContext,
             "../bin/cfg/certs/127_0_0_1_server.key", SSL_FILETYPE_PEM) != 1
        || SSL_CTX_check_private_key(serverContext) != 1 )
      throw std::runtime_error( "Existing TLS fixture certificate/key is incompatible" );
    SSL_CTX_set_verify( serverContext, SSL_VERIFY_NONE, 0 );
    serverSSL = SSL_new( serverContext );
    // This server object is created before client observation starts. Mark it
    // explicitly: the observer must not classify SSL by its allocating thread.
    if( !serverSSL || !SSL_set_ex_data(serverSSL, ledger.index, &ledger.excluded) )
      throw std::runtime_error( "Cannot exclude precreated peer SSL" );
  }

  void startPeer( PeerMode peerMode = invalidTLS )
  {
    mode = peerMode;
    peerOK = true;
    ready = released = false;
    pthread_mutex_lock( &ledger.mutex );
    ledger.active = lifetime;
    pthread_mutex_unlock( &ledger.mutex );
    if( !FIX::thread_spawn(&serve, this, thread) )
      throw std::runtime_error( "Cannot start loopback TLS peer" );
    running = true;
  }

  void waitPeerReady()
  {
    pthread_mutex_lock( &peerMutex );
    while( !ready ) pthread_cond_wait( &peerChanged, &peerMutex );
    pthread_mutex_unlock( &peerMutex );
  }

  void releasePeer()
  {
    pthread_mutex_lock( &peerMutex );
    released = true;
    pthread_cond_broadcast( &peerChanged );
    pthread_mutex_unlock( &peerMutex );
  }

  void joinPeer()
  {
    FIX::thread_join( thread );
    running = false;
  }

  bool acceptPeer( int slot )
  {
    const int accepted = accept( listener.descriptor, 0, 0 );
    pthread_mutex_lock( &peerMutex );
    peerSockets[slot] = accepted;
    pthread_mutex_unlock( &peerMutex );
    if( accepted < 0 ) return false;
    ++accepts;
    peerIdentities[slot] = tlsDescriptorIdentity( accepted );
    struct sockaddr_in local, remote;
    socklen_t length = sizeof(local);
    if( getsockname(accepted, reinterpret_cast<sockaddr*>(&local), &length) ) return false;
    length = sizeof(remote);
    if( getpeername(accepted, reinterpret_cast<sockaddr*>(&remote), &length) ) return false;
    // Find the actual initiating socket from the accepted connection's endpoints.
    // This scan opens no descriptors, so it cannot perturb the reuse ordering.
    for( int descriptor = 0; descriptor < FD_SETSIZE; ++descriptor )
    {
      struct sockaddr_in source, destination;
      length = sizeof(source);
      if( getsockname(descriptor, reinterpret_cast<sockaddr*>(&source), &length)
          || source.sin_family != AF_INET || source.sin_port != remote.sin_port
          || source.sin_addr.s_addr != remote.sin_addr.s_addr ) continue;
      length = sizeof(destination);
      if( getpeername(descriptor, reinterpret_cast<sockaddr*>(&destination), &length)
          || destination.sin_family != AF_INET || destination.sin_port != local.sin_port
          || destination.sin_addr.s_addr != local.sin_addr.s_addr ) continue;
      clientSockets[slot] = descriptor;
      clientIdentities[slot] = tlsDescriptorIdentity( descriptor );
      return !clientIdentities[slot].empty() && !peerIdentities[slot].empty();
    }
    return false;
  }

  bool sendInvalidTLS( int slot )
  {
    char hello[8192];
    const char response[] = "HTTP/1.0 503 Unavailable\r\n\r\n";
    return recv(peerSockets[slot], hello, sizeof(hello), 0) > 0
        && send(peerSockets[slot], response, sizeof(response)-1, MSG_NOSIGNAL)
           == static_cast<ssize_t>(sizeof(response)-1);
  }

  static THREAD_PROC serve( void* argument )
  {
    TLSFixture& fixture = *static_cast<TLSFixture*>( argument );
    fixture.peerOK = fixture.acceptPeer( 0 );
    if( fixture.peerOK && fixture.mode != successfulTLS )
      fixture.peerOK = fixture.sendInvalidTLS( 0 );
    if( fixture.peerOK && fixture.mode == reusedSocket )
    {
      // Keep A's peer open. Accept B only once SSL_new proves that B has
      // already created its client socket, leaving A's retired number for B.
      pthread_mutex_lock( &fixture.ledger.mutex );
      while( fixture.lifetime->allocated < 2 && !fixture.lifetime->cancelled )
        pthread_cond_wait( &fixture.ledger.changed, &fixture.ledger.mutex );
      const bool cancelled = fixture.lifetime->cancelled;
      pthread_mutex_unlock( &fixture.ledger.mutex );
      fixture.peerOK = !cancelled && fixture.acceptPeer( 1 );
    }
    if( fixture.peerOK && fixture.mode != invalidTLS )
    {
      const int slot = fixture.mode == reusedSocket ? 1 : 0;
      fixture.peerOK = SSL_set_fd(fixture.serverSSL, fixture.peerSockets[slot]) == 1
                    && SSL_accept(fixture.serverSSL) == 1;
    }
    if( !fixture.peerOK ) shutdown( fixture.listener.descriptor, SHUT_RDWR );
    pthread_mutex_lock( &fixture.peerMutex );
    fixture.ready = true;
    pthread_cond_broadcast( &fixture.peerChanged );
    while( fixture.peerOK && fixture.mode != invalidTLS && !fixture.released )
      pthread_cond_wait( &fixture.peerChanged, &fixture.peerMutex );
    for( int i = 0; i != 2; ++i )
      if( fixture.peerSockets[i] >= 0 )
      {
        if( !fixture.peerOK ) shutdown( fixture.peerSockets[i], SHUT_RDWR );
        close( fixture.peerSockets[i] );
        fixture.peerSockets[i] = -1;
      }
    pthread_mutex_unlock( &fixture.peerMutex );
    return 0;
  }

  TLSListener listener;
  TLSLedger& ledger;
  TLSLifetime* lifetime;
  TLSApplication application;
  FIX::MemoryStoreFactory store;
  TLSConnectOnce* initiator;
  FIX::thread_id thread;
  bool running;
  pthread_mutex_t peerMutex;
  pthread_cond_t peerChanged;
  int peerSockets[2], clientSockets[2];
  std::string peerIdentities[2], clientIdentities[2];
  bool peerOK;
  int accepts;
  PeerMode mode;
  bool ready, released;
  SSL_CTX* serverContext;
  SSL* serverSSL;
  TLSDescriptors fixtureOnly;
};

SUITE(SSLSocketInitiatorTests)
{
TEST_FIXTURE(TLSFixture, fatalHandshakeReleasesAttempt)
{
  const FIX::SessionID id( "FIX.4.4", "TLSLOCAL", "TLSPEER" );
  for( int attempt = 1; attempt <= 2; ++attempt )
  {
    startPeer();
    initiator->block();
    joinPeer();
    CHECK( peerOK );
    CHECK_EQUAL( attempt, accepts );
    CHECK( initiator->before == tlsDescriptors() );
    CHECK_EQUAL( attempt, lifetime->allocated );
    CHECK_EQUAL( attempt, lifetime->released );
    CHECK_EQUAL( 0, lifetime->tagFailures );
    CHECK( initiator->disconnected(id) );
    CHECK( !initiator->pending(id) );
    CHECK( !initiator->connected(id) );
    CHECK_EQUAL( 0, application.logons );
    CHECK_EQUAL( 0, application.logouts );
  }
}

TEST_FIXTURE(TLSFixture, successfulHandshakeTransfersOwnership)
{
  prepareTLS();
  startPeer( successfulTLS );
  initiator->block();
  waitPeerReady();

  const FIX::SessionID id( "FIX.4.4", "TLSLOCAL", "TLSPEER" );
  CHECK( peerOK );
  CHECK_EQUAL( 1, accepts );
  CHECK( initiator->pending(id) );
  CHECK( !initiator->connected(id) );
  CHECK( !initiator->disconnected(id) );
  CHECK_EQUAL( 1, lifetime->allocated );
  CHECK_EQUAL( 0, lifetime->released );
  CHECK_EQUAL( 0, lifetime->tagFailures );
  TLSDescriptors expected = initiator->before;
  expected[peerSockets[0]] = peerIdentities[0];
  expected[clientSockets[0]] = clientIdentities[0];
  CHECK( expected == tlsDescriptors() );
  CHECK_EQUAL( 0, application.logons );
  CHECK_EQUAL( 0, application.logouts );

  // The peer stays open until after destruction, so its close cannot conceal
  // premature rollback or a client descriptor leaked by BIO_NOCLOSE.
  destroyInitiator();
  releasePeer();
  joinPeer();
  CHECK_EQUAL( 1, lifetime->released );
  CHECK( fixtureOnly == tlsDescriptors() );
}

TEST_FIXTURE(TLSFixture, failedAttemptDoesNotDisconnectReusedSocket)
{
  destroyInitiator();
  initiator = new TLSConnectOnce( application, store, tlsSettings(listener.port, true) );
  prepareTLS();
  const TLSDescriptors before = tlsDescriptors();
  startPeer( reusedSocket );
  CHECK( initiator->poll(0.0) );
  waitPeerReady();

  CHECK( peerOK );
  CHECK_EQUAL( 2, accepts );
  CHECK( clientSockets[0] >= 0 );
  CHECK_EQUAL( clientSockets[0], clientSockets[1] );
  CHECK( clientIdentities[0] != clientIdentities[1] );
  const FIX::SessionID failed( "FIX.4.4", "TLSLOCAL", "TLSA" );
  const FIX::SessionID replacement( "FIX.4.4", "TLSLOCAL", "TLSB" );
  CHECK( initiator->disconnected(failed) );
  CHECK( !initiator->pending(failed) );
  CHECK( !initiator->connected(failed) );
  CHECK( initiator->connected(replacement) );
  CHECK( !initiator->pending(replacement) );
  CHECK( !initiator->disconnected(replacement) );
  CHECK_EQUAL( 2, lifetime->allocated );
  CHECK_EQUAL( 1, lifetime->released );
  CHECK_EQUAL( 0, lifetime->tagFailures );
  TLSDescriptors expected = before;
  expected[peerSockets[0]] = peerIdentities[0];
  expected[peerSockets[1]] = peerIdentities[1];
  expected[clientSockets[1]] = clientIdentities[1];
  CHECK( expected == tlsDescriptors() );
  CHECK_EQUAL( 0, application.logons );
  CHECK_EQUAL( 0, application.logouts );

  destroyInitiator();
  releasePeer();
  joinPeer();
  CHECK_EQUAL( 2, lifetime->released );
  CHECK( fixtureOnly == tlsDescriptors() );
}
}
}

#endif
