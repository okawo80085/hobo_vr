// (c) 2021 Okawo
// This code is licensed under MIT license (see LICENSE for details)


#include "driverlog.h"

#include <stdio.h>
#include <stdarg.h>

static vr::IVRDriverLog * s_pLogFile = NULL;

#if !defined( WIN32)
#define vsnprintf_s vsnprintf
#endif

bool InitDriverLog( vr::IVRDriverLog *pDriverLog )
{
  if( s_pLogFile )
    return false;
  s_pLogFile = pDriverLog;
  return s_pLogFile != NULL;
}

void CleanupDriverLog()
{
  s_pLogFile = NULL;
}

static void DriverLogVarArgs( const char *pMsgFormat, va_list args )
{
  char buf[1024];
  vsnprintf_s( buf, sizeof(buf), pMsgFormat, args );

  if( s_pLogFile )
    s_pLogFile->Log( buf );
}


void DriverLog( const char *pMsgFormat, ... )
{
  va_list args;
  va_start( args, pMsgFormat );

  DriverLogVarArgs( pMsgFormat, args );

  va_end(args);
}


void DebugDriverLog( const char *pMsgFormat, ... )
{
#ifdef _DEBUG
  va_list args;
  va_start( args, pMsgFormat );

  DriverLogVarArgs( pMsgFormat, args );

  va_end(args);
#else
  (void)pMsgFormat; // because its not used in this case
#endif
}
