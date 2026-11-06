/****************************************************************************
** This file is part of the QuickFIX FIX Engine
** See http://www.quickfixengine.org/LICENSE for licensing information.
****************************************************************************/

#ifdef _MSC_VER
#include "stdafx.h"
#else
#include "config.h"
#endif

#include <UnitTest++.h>
#include <SocketMonitor.h>
#include <Utility.h>

SUITE(SocketMonitorTests)
{
struct monitorSocketRuntime
{
  monitorSocketRuntime() { FIX::socket_init(); }
  ~monitorSocketRuntime() { FIX::socket_term(); }
};

struct monitorFixture : FIX::SocketMonitor::Strategy, monitorSocketRuntime
{
  monitorFixture() : sockets( FIX::socket_createpair() ), writes( 0 ), reads( 0 ),
                     errors( 0 ), byte( 0 ), suppressOnWrite( false ) {}

  ~monitorFixture()
  {
    FIX::socket_close( sockets.first );
    FIX::socket_close( sockets.second );
  }

  void onConnect( FIX::SocketMonitor&, int ) {}
  void onWrite( FIX::SocketMonitor& monitor, int socket )
  {
    ++writes;
    if( suppressOnWrite ) monitor.setReadEnabled( socket, false );
    monitor.unsignal( socket );
  }
  void onEvent( FIX::SocketMonitor&, int socket )
  {
    ++reads;
    FIX::socket_recv( socket, &byte, 1 );
  }
  void onError( FIX::SocketMonitor&, int ) { ++errors; }
  void onError( FIX::SocketMonitor& ) { ++errors; }

  std::pair<int, int> sockets;
  int writes, reads, errors;
  char byte;
  bool suppressOnWrite;
};

TEST_FIXTURE(monitorFixture, readInterestAndRemovalPreserveSocketOwnership)
{
  FIX::SocketMonitor monitor;
  CHECK( sockets.first >= 0 && sockets.second >= 0 );
  CHECK( monitor.addRead(sockets.first) );
  CHECK( monitor.addWrite(sockets.first) );
  CHECK_EQUAL( 1, FIX::socket_send(sockets.second, "r", 1) );

  // Both directions are ready in this select. The write callback changes read
  // interest before dispatch of the already selected read event.
  suppressOnWrite = true;
  monitor.block( *this, true );
  CHECK_EQUAL( 1, writes );
  CHECK_EQUAL( 0, reads );
  CHECK_EQUAL( 1U, monitor.numSockets() );

  CHECK( monitor.addWrite(sockets.first) );
  monitor.block( *this, true );
  CHECK_EQUAL( 2, writes );
  CHECK_EQUAL( 0, reads );
  monitor.setReadEnabled( sockets.first, true );
  monitor.block( *this, true );
  CHECK_EQUAL( 1, reads );
  CHECK_EQUAL( 'r', byte );

  CHECK( monitor.addWrite(sockets.first) );
  monitor.setReadEnabled( sockets.first, false );
  CHECK( monitor.remove(sockets.first) );
  CHECK_EQUAL( 0U, monitor.numSockets() );
  CHECK_EQUAL( 1, FIX::socket_send(sockets.second, "s", 1) );
  monitor.block( *this, true );
  CHECK_EQUAL( 2, writes );
  CHECK_EQUAL( 1, reads );
  CHECK_EQUAL( 0, errors );
  CHECK_EQUAL( 1, FIX::socket_recv(sockets.first, &byte, 1) );
  CHECK_EQUAL( 's', byte );

  CHECK( monitor.addRead(sockets.first) );
  CHECK_EQUAL( 1, FIX::socket_send(sockets.second, "t", 1) );
  monitor.block( *this, true );
  CHECK_EQUAL( 2, reads );
  CHECK_EQUAL( 't', byte );
  CHECK( monitor.remove(sockets.first) );
}
}
