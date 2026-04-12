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
#include <TimeRange.h>
#include <FieldConvertors.h>

using namespace FIX;

SUITE(TimeRangeTests)
{

TEST(isInRange)
{
  UtcTimeOnly start( 3, 0, 0 );
  UtcTimeOnly end( 18, 0, 0 );

  UtcTimeStamp now( 10, 0, 0, 10, 10, 2000 );
  CHECK( TimeRange::isInRange( start, end, now ) );

  now = UtcTimeStamp( 18, 0, 0, 10, 10, 2000 );
  CHECK( TimeRange::isInRange( start, end, now ) );

  now = UtcTimeStamp( 2, 0, 0, 10, 10, 2000 );
  CHECK( !TimeRange::isInRange( start, end, now ) );

  now = UtcTimeStamp( 19, 0, 0, 10, 10, 2000 );
  CHECK( !TimeRange::isInRange( start, end, now ) );

  now = UtcTimeStamp( 18, 0, 1, 10, 10, 2000 );
  CHECK( !TimeRange::isInRange( start, end, now ) );

  start = UtcTimeOnly( 18, 0, 0 );
  end = UtcTimeOnly( 3, 0, 0 );
  now = UtcTimeStamp( 18, 0, 0, 10, 10, 2000 );
  CHECK( TimeRange::isInRange( start, end, now ) );

  now = UtcTimeStamp( 3, 0, 0, 10, 10, 2000 );
  CHECK( TimeRange::isInRange( start, end, now ) );

  now = UtcTimeStamp( 4, 0, 0, 10, 10, 2000 );
  CHECK( !TimeRange::isInRange( start, end, now ) );

  now = UtcTimeStamp( 17, 0, 0, 10, 10, 2000 );
  CHECK( !TimeRange::isInRange( start, end, now ) );
}

TEST(isInRangeWithDay)
{
  UtcTimeOnly startTime( 3, 0, 0 );
  UtcTimeOnly endTime( 18, 0, 0 );
  int startDay = 2;
  int endDay = 5;

  UtcTimeStamp now( 2, 0, 0, 28, 7, 2004 );
  CHECK(TimeRange::isInRange(startTime, endTime, startDay, endDay, now));
  now = UtcTimeStamp( 18, 0, 0, 27, 7, 2004 );
  CHECK(TimeRange::isInRange(startTime, endTime, startDay, endDay, now));
  now = UtcTimeStamp( 3, 0, 0, 27, 7, 2004 );
  CHECK(TimeRange::isInRange(startTime, endTime, startDay, endDay, now));
  now = UtcTimeStamp( 2, 59, 59, 26, 7, 2004 );
  CHECK(!TimeRange::isInRange(startTime, endTime, startDay, endDay, now));
  now = UtcTimeStamp( 18, 0, 1, 29, 7, 2004 );
  CHECK(!TimeRange::isInRange(startTime, endTime, startDay, endDay, now));

  startDay = 5;
  endDay = 2;

  now = UtcTimeStamp( 2, 0, 0, 24, 7, 2004 );
  CHECK(TimeRange::isInRange(startTime, endTime, startDay, endDay, now));
  now = UtcTimeStamp( 2, 0, 0, 28, 7, 2004 );
  CHECK(!TimeRange::isInRange(startTime, endTime, startDay, endDay, now));
  now = UtcTimeStamp( 3, 0, 0, 22, 7, 2004 );
  CHECK(TimeRange::isInRange(startTime, endTime, startDay, endDay, now));
  now = UtcTimeStamp( 18, 0, 0, 26, 7, 2004 );
  CHECK(TimeRange::isInRange(startTime, endTime, startDay, endDay, now));
  now = UtcTimeStamp( 2, 59, 59, 22, 7, 2004 );
  CHECK(!TimeRange::isInRange(startTime, endTime, startDay, endDay, now));
  now = UtcTimeStamp( 18, 0, 1, 26, 7, 2004 );
  CHECK(!TimeRange::isInRange(startTime, endTime, startDay, endDay, now));

  startTime = UtcTimeOnly( 9, 1, 0 );
  endTime = UtcTimeOnly( 8, 59, 0 );
  startDay = 1;
  endDay = 1;

  now = UtcTimeStamp( 8, 59, 0, 3, 12, 2006 );
  CHECK(TimeRange::isInRange(startTime, endTime, startDay, endDay, now));
  now = UtcTimeStamp( 8, 59, 1, 3, 12, 2006 );
  CHECK(!TimeRange::isInRange(startTime, endTime, startDay, endDay, now));
  now = UtcTimeStamp( 9, 1, 0, 3, 12, 2006 );
  CHECK(TimeRange::isInRange(startTime, endTime, startDay, endDay, now));
  now = UtcTimeStamp( 9, 0, 0, 3, 12, 2006 );
  CHECK(!TimeRange::isInRange(startTime, endTime, startDay, endDay, now));

  now = UtcTimeStamp( 8, 59, 0, 4, 12, 2006 );
  CHECK(TimeRange::isInRange(startTime, endTime, startDay, endDay, now));
  now = UtcTimeStamp( 8, 59, 1, 4, 12, 2006 );
  CHECK(TimeRange::isInRange(startTime, endTime, startDay, endDay, now));
  now = UtcTimeStamp( 9, 1, 0, 4, 12, 2006 );
  CHECK(TimeRange::isInRange(startTime, endTime, startDay, endDay, now));
  now = UtcTimeStamp( 9, 0, 0, 4, 12, 2006 );
  CHECK(TimeRange::isInRange(startTime, endTime, startDay, endDay, now));
}

TEST(isInSameRange)
{
  // start time is less than end time
  UtcTimeOnly start( 3, 0, 0 );
  UtcTimeOnly end( 18, 0, 0 );

  // same time
  UtcTimeStamp time1( 10, 0, 0, 10, 10, 2000 );
  UtcTimeStamp time2( 10, 0, 0, 10, 10, 2000 );
  CHECK( TimeRange::isInSameRange( start, end, time1, time2 ) );
  CHECK( TimeRange::isInSameRange( start, end, time2, time1 ) );

  // time 2 in same session but greater
  time1 = UtcTimeStamp( 10, 0, 0, 10, 10, 2000 );
  time2 = UtcTimeStamp( 11, 0, 0, 10, 10, 2000 );
  CHECK( TimeRange::isInSameRange( start, end, time1, time2 ) );
  CHECK( TimeRange::isInSameRange( start, end, time2, time1 ) );

  // time 2 in same session but less
  time1 = UtcTimeStamp( 11, 0, 0, 10, 10, 2000 );
  time2 = UtcTimeStamp( 10, 0, 0, 10, 10, 2000 );
  CHECK( TimeRange::isInSameRange( start, end, time1, time2 ) );
  CHECK( TimeRange::isInSameRange( start, end, time2, time1 ) );

  // time 1 not in session
  time1 = UtcTimeStamp( 19, 0, 0, 10, 10, 2000 );
  time2 = UtcTimeStamp( 10, 0, 0, 10, 10, 2000 );
  CHECK( !TimeRange::isInSameRange( start, end, time1, time2 ) );
  CHECK( !TimeRange::isInSameRange( start, end, time2, time1 ) );

  // time 2 not in session
  time1 = UtcTimeStamp( 10, 0, 0, 10, 10, 2000 );
  time2 = UtcTimeStamp( 2, 0, 0, 10, 10, 2000 );
  CHECK( !TimeRange::isInSameRange( start, end, time1, time2 ) );
  CHECK( !TimeRange::isInSameRange( start, end, time2, time1 ) );

  // start time is greater than end time
  start = UtcTimeOnly( 18, 0, 0 );
  end = UtcTimeOnly( 3, 0, 0 );

  // same session same day
  time1 = UtcTimeStamp( 19, 0, 0, 10, 10, 2000 );
  time2 = UtcTimeStamp( 20, 0, 0, 10, 10, 2000 );
  CHECK( TimeRange::isInSameRange( start, end, time1, time2 ) );
  CHECK( TimeRange::isInSameRange( start, end, time2, time1 ) );

  // same session time 2 is in next day
  time1 = UtcTimeStamp( 19, 0, 0, 10, 10, 2000 );
  time2 = UtcTimeStamp( 2, 0, 0, 11, 10, 2000 );
  CHECK( TimeRange::isInSameRange( start, end, time1, time2 ) );
  CHECK( TimeRange::isInSameRange( start, end, time2, time1 ) );

  // same session time 1 is in next day
  time1 = UtcTimeStamp( 2, 0, 0, 11, 10, 2000 );
  time2 = UtcTimeStamp( 19, 0, 0, 10, 10, 2000 );
  CHECK( TimeRange::isInSameRange( start, end, time1, time2 ) );
  CHECK( TimeRange::isInSameRange( start, end, time2, time1 ) );

  // time 1 is 25 hours greater than time 2
  time1 = UtcTimeStamp( 21, 0, 0, 11, 10, 2000 );
  time2 = UtcTimeStamp( 20, 0, 0, 10, 10, 2000 );
  CHECK( !TimeRange::isInSameRange( start, end, time1, time2 ) );
  CHECK( !TimeRange::isInSameRange( start, end, time2, time1 ) );

  // start time is greater than end time
  start = UtcTimeOnly( 6, 0, 0 );
  end = UtcTimeOnly( 6, 0, 0 );
  time1 = UtcTimeStamp( 19, 10, 0, 1, 13, 2004 );
  time2 = UtcTimeStamp( 19, 06, 0, 1, 14, 2004 );
  CHECK( !TimeRange::isInSameRange( start, end, time1, time2 ) );
  CHECK( !TimeRange::isInSameRange( start, end, time2, time1 ) );
}

TEST(isInSameRangeWithDay)
{
  UtcTimeOnly startTime( 3, 0, 0 );
  UtcTimeOnly endTime( 18, 0, 0 );
  int startDay = 2;
  int endDay = 5;

  UtcTimeStamp time1( 3, 0, 0, 27, 7, 2004 );
  UtcTimeStamp time2( 3, 0, 0, 25, 7, 2004 );
  CHECK( !TimeRange::isInSameRange
    ( startTime, endTime, startDay, endDay, time1, time2 ) );

  time1 = UtcTimeStamp( 3, 0, 0, 31, 7, 2004 );
  time2 = UtcTimeStamp( 3, 0, 0, 27, 7, 2004 );
  CHECK( !TimeRange::isInSameRange
    ( startTime, endTime, startDay, endDay, time1, time2 ) );

  time1 = UtcTimeStamp( 3, 0, 0, 27, 7, 2004 );
  time2 = UtcTimeStamp( 3, 0, 0, 27, 7, 2004 );
  CHECK( TimeRange::isInSameRange
    ( startTime, endTime, startDay, endDay, time1, time2 ) );

  time1 = UtcTimeStamp( 10, 0, 0, 26, 7, 2004 );
  time2 = UtcTimeStamp( 3, 0, 0, 27, 7, 2004 );
  CHECK( TimeRange::isInSameRange
    ( startTime, endTime, startDay, endDay, time1, time2 ) );

  time1 = UtcTimeStamp( 10, 0, 0, 27, 7, 2004 );
  time2 = UtcTimeStamp( 2, 0, 0, 29, 7, 2004 );
  CHECK( TimeRange::isInSameRange
    ( startTime, endTime, startDay, endDay, time1, time2 ) );

  time1 = UtcTimeStamp( 10, 0, 0, 27, 7, 2004 );
  time2 = UtcTimeStamp( 3, 0, 0, 20, 7, 2004 );
  CHECK( !TimeRange::isInSameRange
    ( startTime, endTime, startDay, endDay, time1, time2 ) );

  time1 = UtcTimeStamp( 2, 0, 0, 27, 7, 2004 );
  time2 = UtcTimeStamp( 3, 0, 0, 20, 7, 2004 );
  CHECK( !TimeRange::isInSameRange
    ( startTime, endTime, startDay, endDay, time1, time2 ) );

  time1 = UtcTimeStamp( 2, 0, 0, 26, 7, 2004 );
  time2 = UtcTimeStamp( 3, 0, 0, 19, 7, 2004 );
  CHECK( !TimeRange::isInSameRange
    ( startTime, endTime, startDay, endDay, time1, time2 ) );

  // Reset start/end time so that they fall within an hour of midnight
  startTime = UtcTimeOnly(0, 5, 0);
  endTime = UtcTimeOnly(23, 45, 0);

  // Make it a week-long session
  startDay = 1;
  endDay = 7;

  // Check that ST-->DST (Sunday is missing one hour) is handled
  time1 = UtcTimeStamp(0, 0, 0, 4, 4, 2006);
  time2 = UtcTimeStamp(1, 0, 0, 3, 4, 2006);
  CHECK( TimeRange::isInSameRange(startTime, endTime, startDay, endDay, time1, time2) );

  // Check that DST-->ST (Sunday has an extra hour) is handled
  time1 = UtcTimeStamp(0, 0, 0, 30, 10, 2006);
  time2 = UtcTimeStamp(1, 0, 0, 31, 10, 2006);
  CHECK( TimeRange::isInSameRange(startTime, endTime, startDay, endDay, time1, time2) );

  // Check that everything works across a year boundary
  time1 = UtcTimeStamp(10, 10, 10, 31, 12, 2006);
  time2 = UtcTimeStamp(10, 10, 10, 1, 1, 2007);
  CHECK( TimeRange::isInSameRange(startTime, endTime, startDay, endDay, time1, time2) );

  // Check that "missing" start and end days are handled as isSameSession without days
  startDay = -1;
  endDay = -1;
  CHECK( TimeRange::isInSameRange(startTime, endTime, startDay, endDay, time1, time2) );

  // Session days are the same
  startDay = 1;
  endDay = 1;
  startTime = UtcTimeOnly(9, 1, 0);
  endTime = UtcTimeOnly(8, 59, 0);
  time1 = UtcTimeStamp(9, 1, 0, 3, 12, 2006);
  time2 = UtcTimeStamp(9, 1, 0, 3, 12, 2006);
  CHECK( TimeRange::isInSameRange(startTime, endTime, startDay, endDay, time1, time2) );
  time2 = UtcTimeStamp(9, 1, 0, 10, 12, 2006);
  CHECK( !TimeRange::isInSameRange(startTime, endTime, startDay, endDay, time1, time2) );
  time2 = UtcTimeStamp(9, 1, 0, 4, 12, 2006);
  CHECK( TimeRange::isInSameRange(startTime, endTime, startDay, endDay, time1, time2) );
}

TEST(isInRange_Biweekly_MaintenanceFridayIsInactive)
{
  // StartDay=Friday, EndDay=Friday, StartTime=17:05, EndTime=17:00
  // 14-day period. Anchor date 2026-04-10 (a Friday, maintenance day).
  UtcTimeOnly startTime( 17, 5, 0 );
  UtcTimeOnly endTime( 17, 0, 0 );
  int startDay = 6; // Friday
  int endDay = 6;
  int periodDays = 14;
  int anchorJulian = DateTime::julianDate( 2026, 4, 10 );
  TimeRange timeRange( startTime, endTime, startDay, endDay, periodDays, anchorJulian );

  UtcTimeStamp beforeMaint( 16, 59, 59, 10, 4, 2026 );
  CHECK( timeRange.isInRange( beforeMaint ) );
  UtcTimeStamp inGap( 17, 0, 1, 10, 4, 2026 );
  CHECK( !timeRange.isInRange( inGap ) );
  UtcTimeStamp endOfGap( 17, 4, 59, 10, 4, 2026 );
  CHECK( !timeRange.isInRange( endOfGap ) );
  UtcTimeStamp newSession( 17, 5, 0, 10, 4, 2026 );
  CHECK( timeRange.isInRange( newSession ) );
}

TEST(isInRange_Biweekly_NonMaintenanceFridayIsActive)
{
  UtcTimeOnly startTime( 17, 5, 0 );
  UtcTimeOnly endTime( 17, 0, 0 );
  TimeRange timeRange( startTime, endTime, 6, 6, 14, DateTime::julianDate( 2026, 4, 10 ) );

  // 2026-04-17 is 7 days after the anchor, so it's the mid-period Friday.
  UtcTimeStamp midPeriodFriday( 17, 0, 1, 17, 4, 2026 );
  CHECK( timeRange.isInRange( midPeriodFriday ) );
  UtcTimeStamp midPeriodAfterGap( 17, 5, 0, 17, 4, 2026 );
  CHECK( timeRange.isInRange( midPeriodAfterGap ) );
}

TEST(isInRange_Biweekly_MidWeekActive)
{
  UtcTimeOnly startTime( 17, 5, 0 );
  UtcTimeOnly endTime( 17, 0, 0 );
  TimeRange timeRange( startTime, endTime, 6, 6, 14, DateTime::julianDate( 2026, 4, 10 ) );

  // 2026-04-15 (Wednesday) 12:00 — mid-cycle, active
  UtcTimeStamp wed( 12, 0, 0, 15, 4, 2026 );
  CHECK( timeRange.isInRange( wed ) );
}

TEST(isInRange_Biweekly_NextMaintenanceGap)
{
  UtcTimeOnly startTime( 17, 5, 0 );
  UtcTimeOnly endTime( 17, 0, 0 );
  TimeRange timeRange( startTime, endTime, 6, 6, 14, DateTime::julianDate( 2026, 4, 10 ) );

  // 14 days after 2026-04-10 is 2026-04-24 — next maintenance Friday.
  UtcTimeStamp nextGap( 17, 0, 1, 24, 4, 2026 );
  CHECK( !timeRange.isInRange( nextGap ) );
  UtcTimeStamp nextSession( 17, 5, 0, 24, 4, 2026 );
  CHECK( timeRange.isInRange( nextSession ) );
}

TEST(isInRange_Biweekly_DatesBeforeAnchor)
{
  UtcTimeOnly startTime( 17, 5, 0 );
  UtcTimeOnly endTime( 17, 0, 0 );
  TimeRange timeRange( startTime, endTime, 6, 6, 14, DateTime::julianDate( 2026, 4, 10 ) );

  // 2026-03-27 is 14 days before anchor (also Friday, maintenance day)
  UtcTimeStamp beforeGap( 16, 59, 59, 27, 3, 2026 );
  CHECK( timeRange.isInRange( beforeGap ) );
  UtcTimeStamp inGap( 17, 0, 1, 27, 3, 2026 );
  CHECK( !timeRange.isInRange( inGap ) );
  UtcTimeStamp afterGap( 17, 5, 0, 27, 3, 2026 );
  CHECK( timeRange.isInRange( afterGap ) );
  // 2026-04-03 is 7 days before anchor — non-maintenance Friday, should be active
  UtcTimeStamp midPeriodBefore( 17, 0, 1, 3, 4, 2026 );
  CHECK( timeRange.isInRange( midPeriodBefore ) );
}

TEST(isInRange_TriWeekly)
{
  // 21-day period. Anchor 2026-04-10 is a maintenance Friday.
  // Next maintenance: 2026-05-01. 2026-04-17 and 2026-04-24 are NOT maintenance.
  UtcTimeOnly startTime( 17, 5, 0 );
  UtcTimeOnly endTime( 17, 0, 0 );
  TimeRange timeRange( startTime, endTime, 6, 6, 21, DateTime::julianDate( 2026, 4, 10 ) );

  UtcTimeStamp gapOnAnchor( 17, 0, 1, 10, 4, 2026 );
  CHECK( !timeRange.isInRange( gapOnAnchor ) );
  UtcTimeStamp week1Friday( 17, 0, 1, 17, 4, 2026 );
  CHECK( timeRange.isInRange( week1Friday ) );
  UtcTimeStamp week2Friday( 17, 0, 1, 24, 4, 2026 );
  CHECK( timeRange.isInRange( week2Friday ) );
  UtcTimeStamp nextMaintenance( 17, 0, 1, 1, 5, 2026 );
  CHECK( !timeRange.isInRange( nextMaintenance ) );
}

TEST(isInRange_Biweekly_BackwardsCompatibleWhenPeriodIs7)
{
  UtcTimeOnly startTime( 17, 5, 0 );
  UtcTimeOnly endTime( 17, 0, 0 );
  TimeRange timeRange( startTime, endTime, 6, 6 ); // no period/anchor -> defaults to weekly

  UtcTimeStamp inGap( 17, 0, 1, 10, 4, 2026 );
  CHECK( !timeRange.isInRange( inGap ) );
  UtcTimeStamp active( 17, 5, 0, 10, 4, 2026 );
  CHECK( timeRange.isInRange( active ) );
}

TEST(isInRange_Biweekly_YearBoundary)
{
  // Anchor on 2025-12-26 (Friday). Next biweekly maintenance is 2026-01-09.
  UtcTimeOnly startTime( 17, 5, 0 );
  UtcTimeOnly endTime( 17, 0, 0 );
  TimeRange timeRange( startTime, endTime, 6, 6, 14, DateTime::julianDate( 2025, 12, 26 ) );

  UtcTimeStamp anchorGap( 17, 2, 0, 26, 12, 2025 );
  CHECK( !timeRange.isInRange( anchorGap ) );
  // 2026-01-02 17:02 — NOT a maintenance day (mid-period Friday)
  UtcTimeStamp midFriday( 17, 2, 0, 2, 1, 2026 );
  CHECK( timeRange.isInRange( midFriday ) );
  // 2026-01-09 17:02 — next maintenance gap
  UtcTimeStamp nextGap( 17, 2, 0, 9, 1, 2026 );
  CHECK( !timeRange.isInRange( nextGap ) );
  // Year-boundary days — active
  UtcTimeStamp yearEnd( 23, 59, 0, 31, 12, 2025 );
  CHECK( timeRange.isInRange( yearEnd ) );
  UtcTimeStamp newYear( 12, 0, 0, 1, 1, 2026 );
  CHECK( timeRange.isInRange( newYear ) );
}

TEST(isInRange_Biweekly_LeapYearFebruary)
{
  // Anchor 2024-02-23 (Friday, leap year). Next maintenance: 2024-03-08.
  UtcTimeOnly startTime( 17, 5, 0 );
  UtcTimeOnly endTime( 17, 0, 0 );
  TimeRange timeRange( startTime, endTime, 6, 6, 14, DateTime::julianDate( 2024, 2, 23 ) );

  UtcTimeStamp anchorGap( 17, 2, 0, 23, 2, 2024 );
  CHECK( !timeRange.isInRange( anchorGap ) );
  // Leap day, mid-cycle — active
  UtcTimeStamp leapDay( 12, 0, 0, 29, 2, 2024 );
  CHECK( timeRange.isInRange( leapDay ) );
  // 2024-03-01 — 7 days after anchor, non-maintenance Friday
  UtcTimeStamp midFriday( 17, 2, 0, 1, 3, 2024 );
  CHECK( timeRange.isInRange( midFriday ) );
  // 2024-03-08 — next maintenance
  UtcTimeStamp nextGap( 17, 2, 0, 8, 3, 2024 );
  CHECK( !timeRange.isInRange( nextGap ) );
}

TEST(isInSameRange_Biweekly_SameCycleSameDay)
{
  UtcTimeOnly startTime( 17, 5, 0 );
  UtcTimeOnly endTime( 17, 0, 0 );
  TimeRange timeRange( startTime, endTime, 6, 6, 14, DateTime::julianDate( 2026, 4, 10 ) );

  UtcTimeStamp t1( 10, 0, 0, 15, 4, 2026 );
  UtcTimeStamp t2( 14, 30, 0, 15, 4, 2026 );
  CHECK( timeRange.isInSameRange( t1, t2 ) );
}

TEST(isInSameRange_Biweekly_SameCycleDifferentDays)
{
  UtcTimeOnly startTime( 17, 5, 0 );
  UtcTimeOnly endTime( 17, 0, 0 );
  TimeRange timeRange( startTime, endTime, 6, 6, 14, DateTime::julianDate( 2026, 4, 10 ) );

  UtcTimeStamp sat( 10, 0, 0, 11, 4, 2026 );
  UtcTimeStamp wed( 10, 0, 0, 15, 4, 2026 );
  CHECK( timeRange.isInSameRange( sat, wed ) );
}

TEST(isInSameRange_Biweekly_AcrossNonMaintenanceFriday)
{
  UtcTimeOnly startTime( 17, 5, 0 );
  UtcTimeOnly endTime( 17, 0, 0 );
  TimeRange timeRange( startTime, endTime, 6, 6, 14, DateTime::julianDate( 2026, 4, 10 ) );

  // Thursday and Saturday spanning the mid-period Friday (non-maintenance)
  UtcTimeStamp thursday( 10, 0, 0, 16, 4, 2026 );
  UtcTimeStamp saturday( 10, 0, 0, 18, 4, 2026 );
  CHECK( timeRange.isInSameRange( thursday, saturday ) );

  // Times on opposite sides of 17:00 on the mid-period Friday
  UtcTimeStamp fridayBefore( 16, 59, 0, 17, 4, 2026 );
  UtcTimeStamp fridayAfter( 17, 5, 1, 17, 4, 2026 );
  CHECK( timeRange.isInSameRange( fridayBefore, fridayAfter ) );
}

TEST(isInSameRange_Biweekly_AcrossMaintenanceFriday)
{
  UtcTimeOnly startTime( 17, 5, 0 );
  UtcTimeOnly endTime( 17, 0, 0 );
  TimeRange timeRange( startTime, endTime, 6, 6, 14, DateTime::julianDate( 2026, 4, 10 ) );

  // 2026-04-24 is the next maintenance Friday (14 days after anchor)
  UtcTimeStamp before( 16, 59, 0, 24, 4, 2026 );
  UtcTimeStamp after( 17, 5, 1, 24, 4, 2026 );
  CHECK( !timeRange.isInSameRange( before, after ) );

  // Wednesday April 22 vs Wednesday April 29 — span maintenance, different sessions
  UtcTimeStamp wedBefore( 10, 0, 0, 22, 4, 2026 );
  UtcTimeStamp wedAfter( 10, 0, 0, 29, 4, 2026 );
  CHECK( !timeRange.isInSameRange( wedBefore, wedAfter ) );
}

TEST(isInSameRange_Biweekly_ExactlyOnStartOfNewSession)
{
  UtcTimeOnly startTime( 17, 5, 0 );
  UtcTimeOnly endTime( 17, 0, 0 );
  TimeRange timeRange( startTime, endTime, 6, 6, 14, DateTime::julianDate( 2026, 4, 10 ) );

  // inGap is inactive (not in range), so isInSameRange must be false regardless of newSession
  UtcTimeStamp inGap( 17, 2, 0, 24, 4, 2026 );
  UtcTimeStamp newSession( 17, 5, 0, 24, 4, 2026 );
  CHECK( !timeRange.isInSameRange( inGap, newSession ) );
}

TEST(isInSameRange_Biweekly_BackwardsCompatibleWhenPeriodIs7)
{
  UtcTimeOnly startTime( 17, 5, 0 );
  UtcTimeOnly endTime( 17, 0, 0 );
  TimeRange timeRange( startTime, endTime, 6, 6 );

  // Two timestamps in the same week (Wednesday and Thursday) are in the same
  // weekly session.
  UtcTimeStamp wed( 10, 0, 0, 8, 4, 2026 );
  UtcTimeStamp thu( 10, 0, 0, 9, 4, 2026 );
  CHECK( timeRange.isInSameRange( wed, thu ) );

  // Two timestamps a week apart (Thursday April 9 vs Thursday April 16) are in
  // different weekly sessions.
  UtcTimeStamp thu_w1( 10, 0, 0, 9, 4, 2026 );
  UtcTimeStamp thu_w2( 10, 0, 0, 16, 4, 2026 );
  CHECK( !timeRange.isInSameRange( thu_w1, thu_w2 ) );
}

TEST(isInRange_Biweekly_UseLocalTime)
{
  // With UseLocalTime=Y, startTime/endTime are interpreted as local time.
  // Test asserts the LocalTimeOnly constructor path is exercised without crashing.
  LocalTimeOnly startTime( 17, 5, 0 );
  LocalTimeOnly endTime( 17, 0, 0 );
  TimeRange timeRange( startTime, endTime, 6, 6, 14, DateTime::julianDate( 2026, 4, 10 ) );

  UtcTimeStamp wed( 12, 0, 0, 15, 4, 2026 );
  (void)timeRange.isInRange( wed );
}

TEST(isInSameRange_Biweekly_UseLocalTime)
{
  LocalTimeOnly startTime( 17, 5, 0 );
  LocalTimeOnly endTime( 17, 0, 0 );
  TimeRange timeRange( startTime, endTime, 6, 6, 14, DateTime::julianDate( 2026, 4, 10 ) );

  UtcTimeStamp t1( 12, 0, 0, 15, 4, 2026 );
  UtcTimeStamp t2( 14, 0, 0, 15, 4, 2026 );
  (void)timeRange.isInSameRange( t1, t2 );
}

TEST(integration_LMAX_Biweekly_28DayWalkthrough)
{
  // Simulates the LMAX biweekly maintenance scenario.
  // Anchor: 2026-04-10 (Friday) is a maintenance Friday.
  // Next maintenance: 2026-04-24 (Friday). Non-maintenance Friday: 2026-04-17.
  UtcTimeOnly startTime( 17, 5, 0 );
  UtcTimeOnly endTime( 17, 0, 0 );
  int periodDays = 14;
  int anchor = DateTime::julianDate( 2026, 4, 10 );
  TimeRange timeRange( startTime, endTime, 6, 6, periodDays, anchor );

  // Week 1 Friday 2026-04-10 — maintenance day
  UtcTimeStamp w1FriBefore( 16, 59, 59, 10, 4, 2026 );
  UtcTimeStamp w1FriGap( 17, 2, 30, 10, 4, 2026 );
  UtcTimeStamp w1FriAfter( 17, 5, 0, 10, 4, 2026 );
  CHECK( timeRange.isInRange( w1FriBefore ) );
  CHECK( !timeRange.isInRange( w1FriGap ) );
  CHECK( timeRange.isInRange( w1FriAfter ) );

  // Week 1 and Week 2 are a SINGLE session in biweekly mode.
  UtcTimeStamp w1Sat( 10, 0, 0, 11, 4, 2026 );
  UtcTimeStamp w2Mon( 10, 0, 0, 13, 4, 2026 );
  CHECK( timeRange.isInRange( w1Sat ) );
  CHECK( timeRange.isInRange( w2Mon ) );
  CHECK( timeRange.isInSameRange( w1Sat, w2Mon ) );

  // Week 2 Friday 2026-04-17 — NOT a maintenance day
  UtcTimeStamp w2FriBefore( 16, 59, 59, 17, 4, 2026 );
  UtcTimeStamp w2Fri1700( 17, 0, 0, 17, 4, 2026 );
  UtcTimeStamp w2Fri1702( 17, 2, 0, 17, 4, 2026 );
  UtcTimeStamp w2FriAfter( 17, 5, 0, 17, 4, 2026 );
  CHECK( timeRange.isInRange( w2FriBefore ) );
  CHECK( timeRange.isInRange( w2Fri1700 ) );
  CHECK( timeRange.isInRange( w2Fri1702 ) );
  CHECK( timeRange.isInRange( w2FriAfter ) );
  CHECK( timeRange.isInSameRange( w2FriBefore, w2FriAfter ) );

  // Times on opposite sides of week-2 Friday's 17:00 should still be in the same range
  CHECK( timeRange.isInSameRange( w1Sat, w2FriAfter ) );

  // Week 3 Friday 2026-04-24 — maintenance day (14 days after anchor)
  UtcTimeStamp w3FriBefore( 16, 59, 59, 24, 4, 2026 );
  UtcTimeStamp w3FriGap( 17, 2, 30, 24, 4, 2026 );
  UtcTimeStamp w3FriAfter( 17, 5, 0, 24, 4, 2026 );
  CHECK( timeRange.isInRange( w3FriBefore ) );
  CHECK( !timeRange.isInRange( w3FriGap ) );
  CHECK( timeRange.isInRange( w3FriAfter ) );

  // Week 2 (same session as week 1) and Week 3 post-gap (new session) — different sessions
  CHECK( !timeRange.isInSameRange( w2FriAfter, w3FriAfter ) );

  // Week 4 Friday 2026-05-01 — NOT a maintenance day
  UtcTimeStamp w4Fri1702( 17, 2, 0, 1, 5, 2026 );
  UtcTimeStamp w4FriAfter( 17, 5, 0, 1, 5, 2026 );
  CHECK( timeRange.isInRange( w4Fri1702 ) );
  CHECK( timeRange.isInRange( w4FriAfter ) );

  // Week 3 post-gap and Week 4 — same session (second biweekly period)
  CHECK( timeRange.isInSameRange( w3FriAfter, w4FriAfter ) );
}

TEST(isInRange_Biweekly_DistantPastAnchor)
{
  // Anchor in 1970 should still produce correct cycle indices for 2026 dates.
  // 1970-01-02 was a Friday. Verify the floor-division cycle math doesn't break
  // over a 56-year span.
  UtcTimeOnly startTime( 17, 5, 0 );
  UtcTimeOnly endTime( 17, 0, 0 );
  TimeRange timeRange( startTime, endTime, 6, 6, 14, DateTime::julianDate( 1970, 1, 2 ) );

  int jDiff = DateTime::julianDate( 2026, 4, 10 ) - DateTime::julianDate( 1970, 1, 2 );
  bool isMaintenanceAnchorWeek = ( jDiff % 14 == 0 );

  UtcTimeStamp anchorGap( 17, 2, 0, 10, 4, 2026 );
  if( isMaintenanceAnchorWeek )
    CHECK( !timeRange.isInRange( anchorGap ) );
  else
    CHECK( timeRange.isInRange( anchorGap ) );

  UtcTimeStamp nextFriGap( 17, 2, 0, 17, 4, 2026 );
  if( isMaintenanceAnchorWeek )
    CHECK( timeRange.isInRange( nextFriGap ) );
  else
    CHECK( !timeRange.isInRange( nextFriGap ) );
}

TEST(isInRange_Biweekly_LMAX_OneHourMaintenance)
{
  // Realistic LMAX scenario: 1-hour maintenance window every other Friday from
  // 21:00 to 22:00. Active session runs Friday 22:00:00 through next-next
  // Friday 21:00:00, then 1-hour gap, then next active session starts at 22:00.
  UtcTimeOnly startTime( 22, 0, 0 );
  UtcTimeOnly endTime( 21, 0, 0 );
  TimeRange timeRange( startTime, endTime, 6, 6, 14, DateTime::julianDate( 2026, 4, 10 ) );

  // 2026-04-10 — maintenance Friday
  UtcTimeStamp beforeMaint( 16, 59, 59, 10, 4, 2026 );
  CHECK( timeRange.isInRange( beforeMaint ) );

  // Inside the 1-hour gap
  UtcTimeStamp gapStart( 21, 0, 1, 10, 4, 2026 );
  CHECK( !timeRange.isInRange( gapStart ) );
  UtcTimeStamp gapMid( 21, 30, 0, 10, 4, 2026 );
  CHECK( !timeRange.isInRange( gapMid ) );
  UtcTimeStamp gapEnd( 21, 59, 59, 10, 4, 2026 );
  CHECK( !timeRange.isInRange( gapEnd ) );

  // Session resumes at 22:00:00
  UtcTimeStamp sessionResume( 22, 0, 0, 10, 4, 2026 );
  CHECK( timeRange.isInRange( sessionResume ) );

  // 2026-04-17 — mid-period Friday, NOT a maintenance day.
  // The ENTIRE day should be in session, including the 21:00-22:00 window.
  UtcTimeStamp midPeriod1600( 16, 0, 0, 17, 4, 2026 );
  UtcTimeStamp midPeriod1700( 17, 0, 0, 17, 4, 2026 );
  UtcTimeStamp midPeriod1730( 17, 30, 0, 17, 4, 2026 );
  UtcTimeStamp midPeriod2100( 21, 0, 0, 17, 4, 2026 );
  UtcTimeStamp midPeriod2130( 21, 30, 0, 17, 4, 2026 );
  UtcTimeStamp midPeriod2200( 22, 0, 0, 17, 4, 2026 );
  CHECK( timeRange.isInRange( midPeriod1600 ) );
  CHECK( timeRange.isInRange( midPeriod1700 ) );
  CHECK( timeRange.isInRange( midPeriod1730 ) );
  CHECK( timeRange.isInRange( midPeriod2100 ) );
  CHECK( timeRange.isInRange( midPeriod2130 ) );
  CHECK( timeRange.isInRange( midPeriod2200 ) );

  // 2026-04-24 — next maintenance Friday
  UtcTimeStamp nextMaintBefore( 16, 59, 59, 24, 4, 2026 );
  UtcTimeStamp nextMaintGap( 21, 30, 0, 24, 4, 2026 );
  UtcTimeStamp nextMaintResume( 22, 0, 0, 24, 4, 2026 );
  CHECK( timeRange.isInRange( nextMaintBefore ) );
  CHECK( !timeRange.isInRange( nextMaintGap ) );
  CHECK( timeRange.isInRange( nextMaintResume ) );

  // isInSameRange: session boundary IS crossed by the 1-hour gap
  CHECK( !timeRange.isInSameRange( beforeMaint, sessionResume ) );
  // April 10 22:00 through April 24 21:00 — same biweekly session
  UtcTimeStamp w3FriBefore( 20, 59, 59, 24, 4, 2026 );
  CHECK( timeRange.isInSameRange( sessionResume, w3FriBefore ) );
  // Mid-period Friday times (non-maintenance) — same session as Saturday after
  UtcTimeStamp w1Sat( 12, 0, 0, 11, 4, 2026 );
  CHECK( timeRange.isInSameRange( w1Sat, midPeriod1730 ) );
}

}
