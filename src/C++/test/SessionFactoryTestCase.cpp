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
#include <SessionFactory.h>
#include <Application.h>
#include <MessageStore.h>

using namespace FIX;

SUITE(SessionFactoryTests)
{

TEST(validConfiguration)
{
  NullApplication application;
  MemoryStoreFactory messageStoreFactory;
  SessionFactory object(application, messageStoreFactory, 0);

  SessionID sessionID("FIX.4.2", "SENDER", "TARGET");
  Dictionary settings;
  settings.setString(CONNECTION_TYPE, "initiator");
  settings.setString(USE_DATA_DICTIONARY, "N");
  settings.setString(START_TIME, "12:00:00");
  settings.setString(END_TIME, "12:00:00");
  settings.setString(HEARTBTINT, "30");
  object.destroy(object.create(sessionID, settings));
}

TEST(startDayAndEndDayAreDifferent)
{
  NullApplication application;
  MemoryStoreFactory messageStoreFactory;
  SessionFactory object(application, messageStoreFactory, 0);

  SessionID sessionID("FIX.4.2", "SENDER", "TARGET");
  Dictionary settings;
  settings.setString(CONNECTION_TYPE, "initiator");
  settings.setString(USE_DATA_DICTIONARY, "N");
  settings.setString(START_TIME, "12:00:00");
  settings.setString(END_TIME, "12:00:00");
  settings.setString(START_DAY, "Sun");
  settings.setString(END_DAY, "Mon");
  settings.setString(HEARTBTINT, "30");
  object.destroy(object.create(sessionID, settings));
}

TEST(sessionPeriodDaysValid_Biweekly)
{
  NullApplication application;
  MemoryStoreFactory messageStoreFactory;
  SessionFactory object(application, messageStoreFactory, 0);

  SessionID sessionID("FIX.4.2", "SENDER", "TARGET");
  Dictionary settings;
  settings.setString(CONNECTION_TYPE, "initiator");
  settings.setString(USE_DATA_DICTIONARY, "N");
  settings.setString(START_DAY, "Fri");
  settings.setString(START_TIME, "17:05:00");
  settings.setString(END_DAY, "Fri");
  settings.setString(END_TIME, "17:00:00");
  settings.setString(SESSION_PERIOD_DAYS, "14");
  settings.setString(SESSION_ANCHOR_DATE, "2026-04-10"); // Friday
  settings.setString(HEARTBTINT, "30");
  object.destroy(object.create(sessionID, settings));
}

TEST(sessionPeriodDaysNotMultipleOf7)
{
  NullApplication application;
  MemoryStoreFactory messageStoreFactory;
  SessionFactory object(application, messageStoreFactory, 0);

  SessionID sessionID("FIX.4.2", "SENDER", "TARGET");
  Dictionary settings;
  settings.setString(CONNECTION_TYPE, "initiator");
  settings.setString(USE_DATA_DICTIONARY, "N");
  settings.setString(START_DAY, "Fri");
  settings.setString(START_TIME, "17:05:00");
  settings.setString(END_DAY, "Fri");
  settings.setString(END_TIME, "17:00:00");
  settings.setString(SESSION_PERIOD_DAYS, "13");
  settings.setString(SESSION_ANCHOR_DATE, "2026-04-10");
  settings.setString(HEARTBTINT, "30");
  CHECK_THROW(object.create(sessionID, settings), ConfigError);
}

TEST(sessionPeriodDaysZero)
{
  NullApplication application;
  MemoryStoreFactory messageStoreFactory;
  SessionFactory object(application, messageStoreFactory, 0);

  SessionID sessionID("FIX.4.2", "SENDER", "TARGET");
  Dictionary settings;
  settings.setString(CONNECTION_TYPE, "initiator");
  settings.setString(USE_DATA_DICTIONARY, "N");
  settings.setString(START_DAY, "Fri");
  settings.setString(START_TIME, "17:05:00");
  settings.setString(END_DAY, "Fri");
  settings.setString(END_TIME, "17:00:00");
  settings.setString(SESSION_PERIOD_DAYS, "0");
  settings.setString(HEARTBTINT, "30");
  CHECK_THROW(object.create(sessionID, settings), ConfigError);
}

TEST(sessionPeriodDaysWithoutAnchor)
{
  NullApplication application;
  MemoryStoreFactory messageStoreFactory;
  SessionFactory object(application, messageStoreFactory, 0);

  SessionID sessionID("FIX.4.2", "SENDER", "TARGET");
  Dictionary settings;
  settings.setString(CONNECTION_TYPE, "initiator");
  settings.setString(USE_DATA_DICTIONARY, "N");
  settings.setString(START_DAY, "Fri");
  settings.setString(START_TIME, "17:05:00");
  settings.setString(END_DAY, "Fri");
  settings.setString(END_TIME, "17:00:00");
  settings.setString(SESSION_PERIOD_DAYS, "14");
  settings.setString(HEARTBTINT, "30");
  CHECK_THROW(object.create(sessionID, settings), ConfigError);
}

TEST(sessionAnchorDateWithoutPeriodDays)
{
  NullApplication application;
  MemoryStoreFactory messageStoreFactory;
  SessionFactory object(application, messageStoreFactory, 0);

  SessionID sessionID("FIX.4.2", "SENDER", "TARGET");
  Dictionary settings;
  settings.setString(CONNECTION_TYPE, "initiator");
  settings.setString(USE_DATA_DICTIONARY, "N");
  settings.setString(START_DAY, "Fri");
  settings.setString(START_TIME, "17:05:00");
  settings.setString(END_DAY, "Fri");
  settings.setString(END_TIME, "17:00:00");
  settings.setString(SESSION_ANCHOR_DATE, "2026-04-10");
  settings.setString(HEARTBTINT, "30");
  CHECK_THROW(object.create(sessionID, settings), ConfigError);
}

TEST(sessionAnchorDateWrongWeekday)
{
  NullApplication application;
  MemoryStoreFactory messageStoreFactory;
  SessionFactory object(application, messageStoreFactory, 0);

  SessionID sessionID("FIX.4.2", "SENDER", "TARGET");
  Dictionary settings;
  settings.setString(CONNECTION_TYPE, "initiator");
  settings.setString(USE_DATA_DICTIONARY, "N");
  settings.setString(START_DAY, "Fri");
  settings.setString(START_TIME, "17:05:00");
  settings.setString(END_DAY, "Fri");
  settings.setString(END_TIME, "17:00:00");
  settings.setString(SESSION_PERIOD_DAYS, "14");
  settings.setString(SESSION_ANCHOR_DATE, "2026-04-11"); // Saturday, not Friday
  settings.setString(HEARTBTINT, "30");
  CHECK_THROW(object.create(sessionID, settings), ConfigError);
}

TEST(sessionPeriodDaysWithStartDayNotEqualEndDay)
{
  NullApplication application;
  MemoryStoreFactory messageStoreFactory;
  SessionFactory object(application, messageStoreFactory, 0);

  SessionID sessionID("FIX.4.2", "SENDER", "TARGET");
  Dictionary settings;
  settings.setString(CONNECTION_TYPE, "initiator");
  settings.setString(USE_DATA_DICTIONARY, "N");
  settings.setString(START_DAY, "Mon");
  settings.setString(START_TIME, "00:00:00");
  settings.setString(END_DAY, "Fri");
  settings.setString(END_TIME, "23:00:00");
  settings.setString(SESSION_PERIOD_DAYS, "14");
  settings.setString(SESSION_ANCHOR_DATE, "2026-04-06"); // a Monday
  settings.setString(HEARTBTINT, "30");
  CHECK_THROW(object.create(sessionID, settings), ConfigError);
}

TEST(sessionPeriodDays7Explicit)
{
  // SessionPeriodDays=7 is the default and should work without an anchor.
  NullApplication application;
  MemoryStoreFactory messageStoreFactory;
  SessionFactory object(application, messageStoreFactory, 0);

  SessionID sessionID("FIX.4.2", "SENDER", "TARGET");
  Dictionary settings;
  settings.setString(CONNECTION_TYPE, "initiator");
  settings.setString(USE_DATA_DICTIONARY, "N");
  settings.setString(START_DAY, "Fri");
  settings.setString(START_TIME, "17:05:00");
  settings.setString(END_DAY, "Fri");
  settings.setString(END_TIME, "17:00:00");
  settings.setString(SESSION_PERIOD_DAYS, "7");
  settings.setString(HEARTBTINT, "30");
  object.destroy(object.create(sessionID, settings));
}

TEST(sessionAnchorDateInvalidFormat)
{
  NullApplication application;
  MemoryStoreFactory messageStoreFactory;
  SessionFactory object(application, messageStoreFactory, 0);

  SessionID sessionID("FIX.4.2", "SENDER", "TARGET");
  Dictionary settings;
  settings.setString(CONNECTION_TYPE, "initiator");
  settings.setString(USE_DATA_DICTIONARY, "N");
  settings.setString(START_DAY, "Fri");
  settings.setString(START_TIME, "17:05:00");
  settings.setString(END_DAY, "Fri");
  settings.setString(END_TIME, "17:00:00");
  settings.setString(SESSION_PERIOD_DAYS, "14");
  settings.setString(SESSION_ANCHOR_DATE, "04/10/2026");
  settings.setString(HEARTBTINT, "30");
  CHECK_THROW(object.create(sessionID, settings), ConfigError);
}

TEST(sessionPeriodDaysRequiresWrapAroundWindow)
{
  // StartTime < EndTime (normal business-hours window) is not a valid biweekly
  // configuration — the multi-week branch requires StartTime > EndTime so the
  // maintenance gap wraps around the anchor weekday.
  NullApplication application;
  MemoryStoreFactory messageStoreFactory;
  SessionFactory object(application, messageStoreFactory, 0);

  SessionID sessionID("FIX.4.2", "SENDER", "TARGET");
  Dictionary settings;
  settings.setString(CONNECTION_TYPE, "initiator");
  settings.setString(USE_DATA_DICTIONARY, "N");
  settings.setString(START_DAY, "Fri");
  settings.setString(START_TIME, "08:00:00");
  settings.setString(END_DAY, "Fri");
  settings.setString(END_TIME, "17:00:00");
  settings.setString(SESSION_PERIOD_DAYS, "14");
  settings.setString(SESSION_ANCHOR_DATE, "2026-04-10");
  settings.setString(HEARTBTINT, "30");
  CHECK_THROW(object.create(sessionID, settings), ConfigError);
}

TEST(sessionPeriodDaysRejectsEqualStartEndTimes)
{
  // StartTime == EndTime has no defined maintenance gap semantics for
  // multi-week scheduling — reject it.
  NullApplication application;
  MemoryStoreFactory messageStoreFactory;
  SessionFactory object(application, messageStoreFactory, 0);

  SessionID sessionID("FIX.4.2", "SENDER", "TARGET");
  Dictionary settings;
  settings.setString(CONNECTION_TYPE, "initiator");
  settings.setString(USE_DATA_DICTIONARY, "N");
  settings.setString(START_DAY, "Fri");
  settings.setString(START_TIME, "17:00:00");
  settings.setString(END_DAY, "Fri");
  settings.setString(END_TIME, "17:00:00");
  settings.setString(SESSION_PERIOD_DAYS, "14");
  settings.setString(SESSION_ANCHOR_DATE, "2026-04-10");
  settings.setString(HEARTBTINT, "30");
  CHECK_THROW(object.create(sessionID, settings), ConfigError);
}

}
