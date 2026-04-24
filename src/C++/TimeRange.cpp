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
#include "stdafx.h"
#else
#include "config.h"
#endif

#include "TimeRange.h"
#include "Utility.h"

namespace FIX
{
  TimeRange::TimeRange( const UtcTimeOnly& startTime,
                        const UtcTimeOnly& endTime,
                        int startDay,
                        int endDay,
                        int periodDays,
                        int anchorJulianDate )
  : m_startTime( startTime ), m_endTime( endTime ),
    m_startDay( startDay ), m_endDay( endDay ),
    m_useLocalTime( false ),
    m_periodDays( periodDays ), m_anchorJulianDate( anchorJulianDate )
  {
    if( startDay > 0
        && endDay > 0
        && startDay == endDay
        && endTime > startTime )
    { m_endTime = m_startTime; }
  }

  TimeRange::TimeRange( const LocalTimeOnly& startTime,
                        const LocalTimeOnly& endTime,
                        int startDay,
                        int endDay,
                        int periodDays,
                        int anchorJulianDate )
  : m_startTime( startTime ), m_endTime( endTime ),
    m_startDay( startDay ), m_endDay( endDay ),
    m_useLocalTime( true ),
    m_periodDays( periodDays ), m_anchorJulianDate( anchorJulianDate )
  {
    if( startDay > 0
        && endDay > 0
        && startDay == endDay
        && endTime > startTime )
    { m_endTime = m_startTime; }
  }

  bool TimeRange::isInRange( const DateTime& start,
                             const DateTime& end,
                             const DateTime& time )
  {
    UtcTimeOnly timeOnly (time);

    if( start < end )
      return( timeOnly >= start && timeOnly <= end );
    else
      return( timeOnly >= start || timeOnly <= end );
  }

  bool TimeRange::isInRange( const DateTime& startTime,
                             const DateTime& endTime,
                             int startDay,
                             int endDay,
                             const DateTime& time,
                             int day )
  {
    UtcTimeOnly timeOnly (time);

    if( startDay == endDay )
    {
      if( day != startDay )
        return true;
      return isInRange( startTime, endTime, time );
    }
    else if( startDay < endDay )
    {
      if( day < startDay || day > endDay )
        return false;
      else if( day == startDay && timeOnly < startTime )
        return false;
      else if( day == endDay && timeOnly > endTime )
        return false;
    }
    else if( startDay > endDay )
    {
      if( day < startDay && day > endDay )
        return false;
      else if( day == startDay && timeOnly < startTime )
        return false;
      else if( day == endDay && timeOnly > endTime )
        return false;
    }
    return true;
  }

  bool TimeRange::isInRange( const DateTime& startTime,
                             const DateTime& endTime,
                             int startDay,
                             int endDay,
               const DateTime& time ) 
  {
    return isInRange( startTime, endTime, startDay, endDay, time, time.getWeekDay() );
  }

  bool TimeRange::isInSameRange( const DateTime& start,
                                 const DateTime& end,
                                 const DateTime& time1,
                                 const DateTime& time2 )
  {
    if( !isInRange( start, end, time1 ) ) return false;
    if( !isInRange( start, end, time2 ) ) return false;

    if( time1 == time2 ) return true;

    if( start < end || start == end )
    {
      UtcDate time1Date( time1 );
      UtcDate time2Date( time2 );
 
      return time1Date == time2Date;
    }
    else
    {
      int sessionLength = DateTime::SECONDS_PER_DAY - (start - end);

      if( time1 > time2 )
      {
        UtcTimeOnly time2TimeOnly = UtcTimeOnly(time2);

        long delta = time2TimeOnly - start;
        if( delta < 0 )
          delta = DateTime::SECONDS_PER_DAY - labs(delta);

        return (time1 - time2) < (sessionLength - delta);
      }
      else
      {
        return (time2 - time1) < sessionLength;
      }
    }
  }

  bool TimeRange::isInSameRange( const DateTime& startTime,
                                 const DateTime& endTime,
                                 int startDay,
                                 int endDay,
                                 const DateTime& time1,
                                 const DateTime& time2 )
  {
    if( !isInRange( startTime, endTime, startDay, endDay, time1, time1.getWeekDay() ) )
      return false;

    if( !isInRange( startTime, endTime, startDay, endDay, time2, time2.getWeekDay() ) )
      return false;

    int absoluteDay1 = time1.getJulianDate() - time1.getWeekDay();
    int absoluteDay2 = time2.getJulianDate() - time2.getWeekDay();
    return absoluteDay1 == absoluteDay2;
  }

  bool TimeRange::isInRangeMultiWeek( const DateTime& startTime,
                                      const DateTime& endTime,
                                      int startDay,
                                      int endDay,
                                      int periodDays,
                                      int anchorJulianDate,
                                      const DateTime& time,
                                      int day )
  {
    // Multi-week scheduling only supports startDay == endDay (enforced at
    // SessionFactory::create). Fall through safely otherwise.
    if( startDay != endDay )
      return isInRange( startTime, endTime, startDay, endDay, time, day );

    // If today is NOT the cycle weekday, the session is always active — the
    // maintenance gap only occurs on the cycle weekday.
    if( day != startDay )
      return true;

    // Today IS the cycle weekday. Is this particular occurrence a maintenance
    // day? Maintenance days occur at julian dates of the form
    // (anchor + k*periodDays) for integer k.
    int daysDiff = time.getJulianDate() - anchorJulianDate;
    int mod = daysDiff % periodDays;
    if( mod < 0 ) mod += periodDays;
    bool isMaintenanceDay = ( mod == 0 );

    if( !isMaintenanceDay )
    {
      // Non-maintenance cycle weekday (e.g. mid-period Friday in biweekly).
      return true;
    }

    // Maintenance day: apply the same wrap-around gap logic as the weekly path.
    UtcTimeOnly timeOnly( time );
    UtcTimeOnly startTimeOnly( startTime );
    UtcTimeOnly endTimeOnly( endTime );
    return timeOnly >= startTimeOnly || timeOnly <= endTimeOnly;
  }

  bool TimeRange::isInSameRangeMultiWeek( const DateTime& startTime,
                                          const DateTime& endTime,
                                          int startDay,
                                          int endDay,
                                          int periodDays,
                                          int anchorJulianDate,
                                          const DateTime& time1,
                                          const DateTime& time2 )
  {
    // Both times must be in range for the session to be "the same".
    if( !isInRangeMultiWeek( startTime, endTime, startDay, endDay,
                             periodDays, anchorJulianDate, time1, time1.getWeekDay() ) )
      return false;
    if( !isInRangeMultiWeek( startTime, endTime, startDay, endDay,
                             periodDays, anchorJulianDate, time2, time2.getWeekDay() ) )
      return false;

    // Two timestamps are in the same session iff they share a cycle index.
    // Cycle boundary is at startTime on the anchor weekday; a time that's on
    // the anchor weekday BEFORE startTime belongs to the previous cycle.
    int daysSinceAnchor1 = time1.getJulianDate() - anchorJulianDate;
    if( time1.getWeekDay() == startDay && startDay == endDay )
    {
      if( UtcTimeOnly( time1 ) < UtcTimeOnly( startTime ) )
        daysSinceAnchor1 -= 1;
    }
    int daysSinceAnchor2 = time2.getJulianDate() - anchorJulianDate;
    if( time2.getWeekDay() == startDay && startDay == endDay )
    {
      if( UtcTimeOnly( time2 ) < UtcTimeOnly( startTime ) )
        daysSinceAnchor2 -= 1;
    }

    // Floor division (C++ integer division truncates toward zero, wrong for negatives).
    int cycleIndex1;
    if( daysSinceAnchor1 >= 0 )
      cycleIndex1 = daysSinceAnchor1 / periodDays;
    else
      cycleIndex1 = -( ( -daysSinceAnchor1 + periodDays - 1 ) / periodDays );

    int cycleIndex2;
    if( daysSinceAnchor2 >= 0 )
      cycleIndex2 = daysSinceAnchor2 / periodDays;
    else
      cycleIndex2 = -( ( -daysSinceAnchor2 + periodDays - 1 ) / periodDays );

    return cycleIndex1 == cycleIndex2;
  }
}
