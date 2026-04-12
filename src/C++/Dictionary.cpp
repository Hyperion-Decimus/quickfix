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

#include "Dictionary.h"
#include "FieldConvertors.h"
#include <algorithm>

namespace FIX
{
std::string Dictionary::getString( const std::string& key, bool capitalize ) const
throw( ConfigError, FieldConvertError )
{
  Data::const_iterator i = m_data.find( string_toUpper(key) );
  if ( i == m_data.end() ) throw ConfigError( key + " not defined" );

  std::string result = i->second;
  if( capitalize )
     std::transform(result.begin(), result.end(), result.begin(), toupper);

  return result;
}

int Dictionary::getInt( const std::string& key ) const
throw( ConfigError, FieldConvertError )
{
  try
  {
    return IntConvertor::convert( getString(key) );
  }
  catch ( FieldConvertError& )
  {
    throw ConfigError( "Illegal value " + getString(key) + " for " + key );
  }
}

double Dictionary::getDouble( const std::string& key ) const
throw( ConfigError, FieldConvertError )
{
  try
  {
    return DoubleConvertor::convert( getString(key) );
  }
  catch ( FieldConvertError& )
  {
    throw ConfigError( "Illegal value " + getString(key) + " for " + key );
  }
}

bool Dictionary::getBool( const std::string& key ) const
throw( ConfigError, FieldConvertError )
{
  try
  {
    return BoolConvertor::convert( getString(key) );
  }
  catch ( FieldConvertError& )
  {
    throw ConfigError( "Illegal value " + getString(key) + " for " + key );
  }
}

int Dictionary::getDay( const std::string& key ) const
throw( ConfigError, FieldConvertError )
{
  try
  {
    std::string value = getString(key);
    if( value.size() < 2 ) throw FieldConvertError();
    std::string abbr = value.substr(0, 2);
    std::transform( abbr.begin(), abbr.end(), abbr.begin(), tolower );
    if( abbr == "su" ) return 1;
    if( abbr == "mo" ) return 2;
    if( abbr == "tu" ) return 3;
    if( abbr == "we" ) return 4;
    if( abbr == "th" ) return 5;
    if( abbr == "fr" ) return 6;
    if( abbr == "sa" ) return 7;
  }
  catch ( FieldConvertError& )
  {
    throw ConfigError( "Illegal value " + getString(key) + " for " + key );
  }
  return -1;
}

int Dictionary::getDate( const std::string& key ) const
throw( ConfigError, FieldConvertError )
{
  std::string value = getString(key);

  if( value.size() != 10 || value[4] != '-' || value[7] != '-' )
    throw ConfigError( "Illegal value " + value + " for " + key + " (expected YYYY-MM-DD)" );

  for( size_t i = 0; i < value.size(); ++i )
  {
    if( i == 4 || i == 7 ) continue;
    if( !std::isdigit( static_cast<unsigned char>( value[i] ) ) )
      throw ConfigError( "Illegal value " + value + " for " + key + " (expected YYYY-MM-DD)" );
  }

  int year = std::atoi( value.substr(0, 4).c_str() );
  int month = std::atoi( value.substr(5, 2).c_str() );
  int day = std::atoi( value.substr(8, 2).c_str() );

  if( month < 1 || month > 12 )
    throw ConfigError( "Illegal value " + value + " for " + key + " (month out of range)" );

  static const int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  int maxDay = daysInMonth[month - 1];
  bool isLeap = ( year % 4 == 0 && year % 100 != 0 ) || ( year % 400 == 0 );
  if( month == 2 && isLeap ) maxDay = 29;
  if( day < 1 || day > maxDay )
    throw ConfigError( "Illegal value " + value + " for " + key + " (day out of range)" );

  return DateTime::julianDate( year, month, day );
}

void Dictionary::setString( const std::string& key, const std::string& value )
{
  m_data[ string_strip(string_toUpper(key)) ] = string_strip(value);
}

void Dictionary::setInt( const std::string& key, int value )
{
  m_data[ string_strip(string_toUpper(key)) ] = IntConvertor::convert( value );
}

void Dictionary::setDouble( const std::string& key, double value )
{
  m_data[ string_strip(string_toUpper(key)) ] = DoubleConvertor::convert( value );
}

void Dictionary::setBool( const std::string& key, bool value )
{
  m_data[ string_strip(string_toUpper(key)) ] = BoolConvertor::convert( value );
}

void Dictionary::setDay( const std::string& key, int value )
{  
    switch( value )
    {
    case 1:
      setString( key, "SU" ); break;
    case 2:
      setString( key, "MO" ); break;
    case 3:
      setString( key, "TU" ); break;
    case 4:
      setString( key, "WE" ); break;
    case 5:
      setString( key, "TH" ); break;
    case 6:
      setString( key, "FR" ); break;
    case 7:
      setString( key, "SA" ); break;
    }
}

bool Dictionary::has( const std::string& key ) const
{
  return m_data.find( string_toUpper(key) ) != m_data.end();
}

void Dictionary::merge( const Dictionary& toMerge )
{
  Data::const_iterator i = toMerge.m_data.begin();
  for ( ; i != toMerge.m_data.end(); ++i )
    if ( m_data.find( i->first ) == m_data.end() )
      m_data[ i->first ] = i->second;
}
}
