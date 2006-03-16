/*

Copyright (C) 1999 John W. Eaton

This file is part of Octave.

Octave is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

Octave is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Octave; see the file COPYING.  If not, write to the Free
Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <climits>
#include <cmath>

#ifdef HAVE_UNISTD_H
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <unistd.h>
#endif

#if defined (OCTAVE_USE_WINDOWS_API)
#include <windows.h>
#endif

#include "lo-error.h"
#include "lo-utils.h"
#include "oct-time.h"

octave_time::octave_time (const octave_base_tm& tm)
{
  struct tm t;
  
  t.tm_sec = tm.sec ();
  t.tm_min = tm.min ();
  t.tm_hour = tm.hour ();
  t.tm_mday = tm.mday ();
  t.tm_mon = tm.mon ();
  t.tm_year = tm.year ();
  t.tm_wday = tm.wday ();
  t.tm_yday = tm.yday ();
  t.tm_isdst = tm.isdst ();

#if defined (HAVE_STRUCT_TM_TM_ZONE)
  std::string s = tm.zone ();
  char *ps = strsave (s.c_str ());
  t.tm_zone = ps;
#endif

  ot_unix_time = mktime (&t);

#if defined (HAVE_STRUCT_TM_TM_ZONE)
  delete [] ps;
#endif

  ot_usec = tm.usec ();
}

std::string
octave_time::ctime (void) const
{
  return octave_localtime (*this) . asctime ();
}

void
octave_time::stamp (void)
{
#if defined (HAVE_GETTIMEOFDAY)

  struct timeval tp;

#if defined  (GETTIMEOFDAY_NO_TZ)
  gettimeofday (&tp);
#else
  gettimeofday (&tp, 0);
#endif

  ot_unix_time = tp.tv_sec;
  ot_usec = tp.tv_usec;

#elif defined (OCTAVE_USE_WINDOWS_API)

  // Loosely based on the code from Cygwin
  // Copyright 1996-2002 Red Hat, Inc.
  // Licenced under the GPL.

  const LONGLONG TIME_OFFSET = 0x19db1ded53e8000LL;

  static int init = 1;
  static LARGE_INTEGER base;
  static LARGE_INTEGER t0;
  static double dt;

  if (init)
    {
      LARGE_INTEGER ifreq;

      if (QueryPerformanceFrequency (&ifreq))
        {
	  // Get clock frequency
	  dt = (double) 1000000.0 / (double) ifreq.QuadPart;

	  // Get base time as microseconds from Jan 1. 1970
	  int priority = GetThreadPriority (GetCurrentThread ());
	  SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	  if (QueryPerformanceCounter (&base))
	    {
	      FILETIME f;

	      GetSystemTimeAsFileTime (&f);

	      t0.HighPart = f.dwHighDateTime;
	      t0.LowPart = f.dwLowDateTime;
	      t0.QuadPart -= TIME_OFFSET;
	      t0.QuadPart /= 10;

	      init = 0;
	    }

	  SetThreadPriority (GetCurrentThread (), priority);
	}

      if (! init)
	{
	  ot_unix_time = time (0);
	  ot_usec = 0;

	  return;
	}
    }

  LARGE_INTEGER now;

  if (QueryPerformanceCounter (&now))
    {
      now.QuadPart = (LONGLONG) (dt * (double)(now.QuadPart - base.QuadPart));
      now.QuadPart += t0.QuadPart;

      ot_unix_time = now.QuadPart / 1000000LL;
      ot_usec = now.QuadPart % 1000000LL;
    }
  else
    {
      ot_unix_time = time (0);
      ot_usec = 0;
    }

#else

  ot_unix_time = time (0);

#endif
}

// From the mktime() manual page:
//
//     The  mktime()  function converts a broken-down time structure,
//     expressed as local time, to calendar time representation.
//
//     <snip>
//
//     If structure members are outside  their	legal interval, they
//     will be normalized (so that, e.g., 40 October is changed into
//     9 November).
//
// So, we no longer check limits here.

#if 0
#define DEFINE_SET_INT_FIELD_FCN(f, lo, hi) \
  octave_base_tm& \
  octave_base_tm::f (int v) \
  { \
    if (v < lo || v > hi) \
      (*current_liboctave_error_handler) \
	("invalid value specified for " #f); \
 \
    tm_ ## f = v; \
 \
    return *this; \
  }
#else
#define DEFINE_SET_INT_FIELD_FCN(f, lo, hi) \
  octave_base_tm& \
  octave_base_tm::f (int v) \
  { \
    tm_ ## f = v; \
 \
    return *this; \
  }
#endif

DEFINE_SET_INT_FIELD_FCN (usec, 0, 1000000)
DEFINE_SET_INT_FIELD_FCN (sec, 0, 61)
DEFINE_SET_INT_FIELD_FCN (min, 0, 59)
DEFINE_SET_INT_FIELD_FCN (hour, 0, 23)
DEFINE_SET_INT_FIELD_FCN (mday, 1, 31)
DEFINE_SET_INT_FIELD_FCN (mon, 0, 11)
DEFINE_SET_INT_FIELD_FCN (year, INT_MIN, INT_MAX)
DEFINE_SET_INT_FIELD_FCN (wday, 0, 6)
DEFINE_SET_INT_FIELD_FCN (yday, 0, 365)
DEFINE_SET_INT_FIELD_FCN (isdst, 0, 1)

octave_base_tm&
octave_base_tm::zone (const std::string& s)
{
  tm_zone = s;
  return *this;
}

#if !defined STRFTIME_BUF_INITIAL_SIZE
#define STRFTIME_BUF_INITIAL_SIZE 128
#endif

std::string
octave_base_tm::strftime (const std::string& fmt) const
{
  std::string retval;

  if (! fmt.empty ())
    {
      struct tm t;
  
      t.tm_sec = tm_sec;
      t.tm_min = tm_min;
      t.tm_hour = tm_hour;
      t.tm_mday = tm_mday;
      t.tm_mon = tm_mon;
      t.tm_year = tm_year;
      t.tm_wday = tm_wday;
      t.tm_yday = tm_yday;
      t.tm_isdst = tm_isdst;

#if defined (HAVE_STRUCT_TM_TM_ZONE)
      char *ps = strsave (tm_zone.c_str ());
      t.tm_zone = ps;
#endif

      const char *fmt_str = fmt.c_str ();

      char *buf = 0;
      size_t bufsize = STRFTIME_BUF_INITIAL_SIZE;
      size_t chars_written = 0;

      while (chars_written == 0)
	{
	  delete [] buf;
	  buf = new char[bufsize];
	  buf[0] = '\0';

	  chars_written = ::strftime (buf, bufsize, fmt_str, &t);

	  bufsize *= 2;
	}

#if defined (HAVE_STRUCT_TM_TM_ZONE)
      delete [] ps;
#endif

      retval = buf;

      delete [] buf;
    }

  return retval;
}

void
octave_base_tm::init (void *p)
{
  struct tm *t = static_cast<struct tm*> (p);
  
  tm_sec = t->tm_sec;
  tm_min = t->tm_min;
  tm_hour = t->tm_hour;
  tm_mday = t->tm_mday;
  tm_mon = t->tm_mon;
  tm_year = t->tm_year;
  tm_wday = t->tm_wday;
  tm_yday = t->tm_yday;
  tm_isdst = t->tm_isdst;

#if defined (HAVE_STRUCT_TM_TM_ZONE)
  tm_zone = t->tm_zone;
#elif defined (HAVE_TZNAME)
  if (t->tm_isdst == 0 || t->tm_isdst == 1)
    tm_zone = tzname[t->tm_isdst];
#endif
}

void
octave_localtime::init (const octave_time& ot)
{
  tm_usec = ot.usec ();

  time_t t = ot;

  octave_base_tm::init (localtime (&t));
}

void
octave_gmtime::init (const octave_time& ot)
{
  tm_usec = ot.usec ();

  time_t t = ot;

  octave_base_tm::init (gmtime (&t));
}

void
octave_strptime::init (const std::string& str, const std::string& fmt)
{
  struct tm t;

  t.tm_sec = 0;
  t.tm_min = 0;
  t.tm_hour = 0;
  t.tm_mday = 0;
  t.tm_mon = 0;
  t.tm_year = 0;
  t.tm_wday = 0;
  t.tm_yday = 0;
  t.tm_isdst = 0;

#if defined (HAVE_STRUCT_TM_TM_ZONE)
  char *ps = strsave ("");
  t.tm_zone = ps;
#endif

  char *p = strsave (str.c_str ());

  char *q = oct_strptime (p, fmt.c_str (), &t);

  if (q)
    nchars = q - p + 1;
  else
    nchars = 0;

  delete [] p;

  octave_base_tm::init (&t);

#if defined (HAVE_STRUCT_TM_TM_ZONE)
  delete ps;
#endif
}

/*
;;; Local Variables: ***
;;; mode: C++ ***
;;; End: ***
*/
