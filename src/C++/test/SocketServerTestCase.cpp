/****************************************************************************
** Copyright (c) 2001-2014
**
** This file is part of the QuickFIX FIX Engine
**
** This file may be distributed under the terms of the quickfixengine.org
** license as defined by quickfixengine.org and appearing in the file
** LICENSE included in the packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.quickfixengine.org/LICENSE for licensing information.
**
** Contact ask@quickfixengine.org if any conditions of this licensing are
** not clear to you.
**
****************************************************************************/

#ifdef _MSC_VER
#pragma warning( disable : 4503 4355 4786 )
#include "stdafx.h"
#else
#include "config.h"
#endif

#include <UnitTest++.h>
#include <TestHelper.h>
#include <SocketServer.h>
#if defined(__linux__)
#include <sys/stat.h>
#include <unistd.h>
#include <stdexcept>
#endif
#ifdef _MSC_VER
#include <stdlib.h>
#endif

using namespace FIX;

class socketServerFixture : public SocketServer::Strategy
{
public:
  socketServerFixture() 
  : connect( 0 ), write( 0 ), data( 0 ), disconnect( 0 ),
    connectSocket( 0 ), writeSocket( 0 ),
    dataSocket( 0 ), disconnectSocket( 0 ),
    bufLen( 0 ) {}

  void onConnect( SocketServer&, int accept, int socket )
  { connect++; connectSocket = socket; }
  void onWrite( SocketServer&, int socket )
  { write++; writeSocket = socket; }
  bool onData( SocketServer& server, int socket )
  {
    data++; dataSocket = socket;
    bufLen = recv( socket, buf, 1, 0 );
    return bufLen > 0;
  }
  void onDisconnect( SocketServer&, int socket )
  { disconnect++; disconnectSocket = socket; }
  void onError( SocketServer& )
  {}

  int connect, write, data, disconnect;
  int connectSocket, writeSocket;
  int dataSocket, disconnectSocket;
  char buf[ 1 ]; size_t bufLen;
};

SUITE(SocketServerTests)
{

TEST_FIXTURE(socketServerFixture, accept)
{
  SocketServer object( 0 );
  int serverS1 = object.add( TestSettings::port, true, true );
  int clientS1 = createSocket( TestSettings::port, "127.0.0.1" );
  CHECK( clientS1 > 0 );
  process_sleep(0.1);
  int s1 = object.accept( serverS1 );
  CHECK( s1 >= 0 );
  object.block( *this );
  CHECK( object.numConnections() == 1 );

  int clientS2 = createSocket( TestSettings::port, "127.0.0.1" );
  CHECK( clientS2 > 0 );
  process_sleep(0.1);
  int s2 = object.accept( serverS1 );
  CHECK( s2 >= 0 );
  object.block( *this );
  CHECK( object.numConnections() == 2 );

  int clientS3 = createSocket( TestSettings::port, "127.0.0.1" );
  CHECK( clientS3 > 0 );
  process_sleep(0.1);
  int s3 = object.accept( serverS1 );
  CHECK( s3 >= 0 );
  object.block( *this );
  CHECK( object.numConnections() == 3 );

  SocketMonitor& monitor = object.getMonitor();
  CHECK( !monitor.drop( -1 ) );
  CHECK( object.numConnections() == 3 );
  CHECK( monitor.drop( s1 ) );
  CHECK( object.numConnections() == 2 );
  CHECK( monitor.drop( s2 ) );
  CHECK( object.numConnections() == 1 );
  CHECK( monitor.drop( s3 ) );
  CHECK( object.numConnections() == 0 );

  destroySocket( clientS1 );
  destroySocket( clientS2 );
  destroySocket( clientS3 );
}

TEST_FIXTURE(socketServerFixture, block)
{
  SocketServer object( 0 );
  object.add( TestSettings::port, true, true );
  int clientS = createSocket( TestSettings::port, "127.0.0.1" );
  CHECK( clientS >= 0 );

  object.block( *this );
  CHECK_EQUAL( 1, connect );
  CHECK( connectSocket > 0 );

  send( clientS, "1", 1, 0 );
  object.block( *this );
  object.block( *this );
  CHECK_EQUAL( 1, data );
  CHECK_EQUAL( 1U, bufLen );
  CHECK_EQUAL( '1', *buf );
  CHECK( dataSocket > 0 );

  destroySocket( clientS );
  object.block( *this );
  CHECK_EQUAL( 1, disconnect );
  CHECK( disconnectSocket > 0 );
}

TEST_FIXTURE(socketServerFixture, close)
{
  SocketServer object( 0 );
  object.add( TestSettings::port, true, true );
  object.close();
  object.block( *this );
}

}

#if defined(__linux__)
SUITE(SocketMonitorTests)
{
struct MonitorSocketPair
{
  MonitorSocketPair()
  {
    if( socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) )
      throw std::runtime_error( "Cannot create monitor fixture socketpair" );
  }
  ~MonitorSocketPair()
  {
    for( int i = 0; i != 2; ++i ) if( sockets[i] >= 0 ) close( sockets[i] );
  }
  int sockets[2];
};

struct MonitorNotifications : SocketMonitor::Strategy
{
  MonitorNotifications()
  : events(0), errors(0), monitorErrors(0), eventSocket(-1), errorSocket(-1),
    received(0), byte(0) {}
  void onConnect( SocketMonitor&, int ) {}
  void onWrite( SocketMonitor&, int ) {}
  void onEvent( SocketMonitor&, int socket )
  {
    ++events;
    eventSocket = socket;
    received = recv( socket, &byte, 1, 0 );
  }
  void onError( SocketMonitor&, int socket ) { ++errors; errorSocket = socket; }
  void onError( SocketMonitor& ) { ++monitorErrors; }
  int events, errors, monitorErrors, eventSocket, errorSocket;
  ssize_t received;
  char byte;
};

TEST(dropNotificationMode)
{
  for( int notify = 0; notify != 2; ++notify )
  {
    SocketMonitor monitor;
    MonitorSocketPair retired, replacement;
    MonitorNotifications notifications;
    const int descriptor = retired.sockets[0];
    struct stat original, current;
    if( fstat(descriptor, &original) || !monitor.addRead(descriptor) )
      throw std::runtime_error( "Cannot monitor fixture descriptor" );
    retired.sockets[0] = -1; // The monitor now owns this descriptor.
    CHECK( notify ? monitor.drop(descriptor) : monitor.drop(descriptor, false) );
    if( dup2(replacement.sockets[0], descriptor) != descriptor )
      throw std::runtime_error( "Cannot place replacement on retired descriptor" );
    close( replacement.sockets[0] );
    replacement.sockets[0] = -1;
    if( fstat(descriptor, &current) || !monitor.addRead(descriptor) )
    {
      close( descriptor );
      throw std::runtime_error( "Cannot monitor replacement descriptor" );
    }
    CHECK( original.st_ino != current.st_ino );
    const char byte = 'Q';
    CHECK_EQUAL( 1, send(replacement.sockets[1], &byte, 1, MSG_NOSIGNAL) );
    monitor.block( notifications, true, 0.0 );
    CHECK_EQUAL( 0, notifications.monitorErrors );
    CHECK_EQUAL( notify, notifications.errors );
    CHECK_EQUAL( 1 - notify, notifications.events );
    if( notify ) CHECK_EQUAL( descriptor, notifications.errorSocket );
    else
    {
      CHECK_EQUAL( descriptor, notifications.eventSocket );
      CHECK_EQUAL( 1, notifications.received );
      CHECK_EQUAL( byte, notifications.byte );
    }
  }
}
}
#endif
