/* Copyright (c) 2013 The Squash Authors
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *   Evan Nemerson <evan@coeus-group.com>
 */

#define _POSIX_C_SOURCE 199309L

#include "timer.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define SQUASH_TIMER_METHOD_CLOCK_GETTIME 0
#define SQUASH_TIMER_METHOD_GETRUSAGE 1
#define SQUASH_TIMER_METHOD_CLOCK 2
#define SQUASH_TIMER_METHOD_GETPROCESSTIMES 3

#ifdef _WIN32
#  define SQUASH_TIMER_METHOD SQUASH_TIMER_METHOD_GETPROCESSTIMES
#  include <windows.h>
#endif

#if !defined(SQUASH_TIMER_METHOD) && (_POSIX_TIMERS > 0)
#  ifdef _POSIX_CPUTIME
#    define SQUASH_TIMER_CPUTIME CLOCK_PROCESS_CPUTIME_ID
#    define SQUASH_TIMER_METHOD SQUASH_TIMER_METHOD_CLOCK_GETTIME
#    include <time.h>
#  elif defined(CLOCK_VIRTUAL)
#    define SQUASH_TIMER_CPUTIME CLOCK_VIRTUAL
#    define SQUASH_TIMER_METHOD SQUASH_TIMER_METHOD_CLOCK_GETTIME
#    include <time.h>
#  endif
#endif

#if !defined(SQUASH_TIMER_METHOD)
#  define SQUASH_TIMER_METHOD SQUASH_TIMER_METHOD_GETRUSAGE
#  include <sys/time.h>
#  include <sys/resource.h>
#endif

struct SquashTimer_s {
  double elapsed_cpu;
  double elapsed_wall;

#if SQUASH_TIMER_METHOD == SQUASH_TIMER_METHOD_CLOCK_GETTIME
  struct timespec start_wall;
  struct timespec end_wall;
  struct timespec start_cpu;
  struct timespec end_cpu;
#elif SQUASH_TIMER_METHOD == SQUASH_TIMER_METHOD_GETRUSAGE
  struct timeval start_wall;
  struct timeval end_wall;
  struct rusage start_cpu;
  struct rusage end_cpu;
#elif SQUASH_TIMER_METHOD == SQUASH_TIMER_METHOD_GETPROCESSTIMES
  ULONGLONG start_wall;
  ULONGLONG end_wall;
  FILETIME start_cpu;
  FILETIME end_cpu;
#endif
};

/**
 * @defgroup SquashTimer SquashTimer
 * @brief A cross-platform timer
 *
 * @{
 */

/**
 * @struct _SquashTimer
 * @brief Simple cross-platform timer
 *
 * Provides a timer for measuring the elapsed time between events, in
 * both wall-clock and CPU time.
 */

/**
 * @brief Create a new timer.
 *
 * Note that this function does not actually start timing.
 *
 * @return A new timer instance, or *NULL* on failure.
 * @see squash_timer_free
 */
SquashTimer*
squash_timer_new (void) {
  SquashTimer* timer = (SquashTimer*) malloc (sizeof (SquashTimer));
  squash_timer_reset (timer);
  return timer;
}

/**
 * @brief Free a timer.
 *
 * @param timer The timer.
 */
void
squash_timer_free (SquashTimer* timer) {
  free (timer);
}

/**
 * @brief Begin or continue timing.
 *
 * @param timer The timer.
 */
void
squash_timer_start (SquashTimer* timer) {
#if SQUASH_TIMER_METHOD == SQUASH_TIMER_METHOD_CLOCK_GETTIME
  if (clock_gettime (CLOCK_REALTIME, &(timer->start_wall)) != 0) {
    fputs ("Unable to get wall clock time\n", stderr);
    exit (-1);
  }
  if (clock_gettime (SQUASH_TIMER_CPUTIME, &(timer->start_cpu)) != 0) {
    fputs ("Unable to get CPU clock time\n", stderr);
    exit (-1);
  }
#elif SQUASH_TIMER_METHOD == SQUASH_TIMER_METHOD_GETRUSAGE
  if (gettimeofday(&(timer->start_wall), NULL) != 0) {
    fputs ("Unable to get time\n", stderr);
  }
  if (getrusage(RUSAGE_SELF, &(timer->start_cpu)) != 0) {
    fputs ("Unable to get CPU clock time\n", stderr);
    exit (-1);
  }
#elif SQUASH_TIMER_METHOD == SQUASH_TIMER_METHOD_GETPROCESSTIMES
  #if _WIN32_WINNT >= 0x0600
    timer->start_wall = GetTickCount64 ();
  #else
    timer->start_wall = GetTickCount ();
  #endif
  FILETIME CreationTime, ExitTime, KernelTime;
  if (!GetProcessTimes (GetCurrentProcess(), &CreationTime, &ExitTime, &KernelTime, &(timer->start_cpu))) {
    fputs ("Unable to get CPU clock time\n", stderr);
  }
#endif
}

#if SQUASH_TIMER_METHOD == SQUASH_TIMER_METHOD_CLOCK_GETTIME
static double
squash_timer_timespec_elapsed (struct timespec* start, struct timespec* end) {
  return
    (double) (end->tv_sec - start->tv_sec) +
    (((double) (end->tv_nsec - start->tv_nsec)) / 1000000000);
}
#elif SQUASH_TIMER_METHOD == SQUASH_TIMER_METHOD_GETRUSAGE
static double
squash_timer_timeval_elapsed (struct timeval* start, struct timeval* end) {
  return
    (double) (end->tv_sec - start->tv_sec) +
    (((double) (end->tv_usec - start->tv_usec)) / 1000000);
}

static double
squash_timer_rusage_elapsed (struct rusage* start, struct rusage* end) {
  return
    (double) ((end->ru_utime.tv_sec + end->ru_stime.tv_sec) - (start->ru_utime.tv_sec + start->ru_stime.tv_sec)) +
    (((double) ((end->ru_utime.tv_usec + end->ru_stime.tv_usec) - (start->ru_utime.tv_usec + start->ru_stime.tv_usec))) / 1000000);
}
#endif

/**
 * @brief Stop timing.
 *
 * @param timer The timer.
 */
void
squash_timer_stop (SquashTimer* timer) {
#if SQUASH_TIMER_METHOD == SQUASH_TIMER_METHOD_CLOCK_GETTIME
  if (clock_gettime (SQUASH_TIMER_CPUTIME, &(timer->end_cpu)) != 0) {
    fputs ("Unable to get CPU clock time\n", stderr);
    exit (-1);
  }
  if (clock_gettime (CLOCK_REALTIME, &(timer->end_wall)) != 0) {
    fputs ("Unable to get wall clock time\n", stderr);
    exit (-1);
  }

  timer->elapsed_cpu += squash_timer_timespec_elapsed (&(timer->start_cpu), &(timer->end_cpu));
  timer->elapsed_wall += squash_timer_timespec_elapsed (&(timer->start_wall), &(timer->end_wall));
#elif SQUASH_TIMER_METHOD == SQUASH_TIMER_METHOD_GETRUSAGE
  if (getrusage(RUSAGE_SELF, &(timer->end_cpu)) != 0) {
    fputs ("Unable to get CPU clock time\n", stderr);
    exit (-1);
  }
  if (gettimeofday(&(timer->end_wall), NULL) != 0) {
    fputs ("Unable to get time\n", stderr);
    exit (-1);
  }

  timer->elapsed_cpu += squash_timer_rusage_elapsed (&(timer->start_cpu), &(timer->end_cpu));
  timer->elapsed_wall += squash_timer_timeval_elapsed (&(timer->start_wall), &(timer->end_wall));
#elif SQUASH_TIMER_METHOD == SQUASH_TIMER_METHOD_GETPROCESSTIMES
  FILETIME CreationTime, ExitTime, KernelTime;
  if (!GetProcessTimes (GetCurrentProcess(), &CreationTime, &ExitTime, &KernelTime, &(timer->end_cpu))) {
    fputs ("Unable to get CPU clock time\n", stderr);
  }

  #if _WIN32_WINNT >= 0x0600
    timer->end_wall = GetTickCount64 ();
  #else
    timer->end_wall = GetTickCount ();
    if (timer->end_wall < timer->start_wall) {
      timer->end_wall += 0xffffffff;
    }
  #endif

  uint64_t start_cpu, end_cpu;

  start_cpu   = timer->start_cpu.dwHighDateTime;
  start_cpu <<= sizeof (DWORD) * 8;
  start_cpu  |= timer->start_cpu.dwLowDateTime;

  end_cpu   = timer->end_cpu.dwHighDateTime;
  end_cpu <<= sizeof (DWORD) * 8;
  end_cpu  |= timer->end_cpu.dwLowDateTime;

  timer->elapsed_cpu += ((double) (end_cpu - start_cpu)) / 10000000;
  timer->elapsed_wall += ((double) (timer->end_wall - timer->start_wall)) / 1000;
#endif
}

/**
 * @brief Reset the timer.
 *
 * Resets the elapsed time to 0.
 *
 * @param timer The timer.
 */
void
squash_timer_reset (SquashTimer* timer) {
  timer->elapsed_cpu = 0.0;
  timer->elapsed_wall = 0.0;
}

/**
 * @brief Restart the timer.
 *
 * This is a convenience wrapper for resetting the timer then starting
 * it.
 *
 * @param timer The timer.
 */
void
squash_timer_restart (SquashTimer* timer) {
  squash_timer_reset (timer);
  squash_timer_start (timer);
}

/**
 * @brief Get the elapsed CPU time.
 *
 * @param timer The timer.
 * @return Number of seconds (CPU time) elapsed.
 */
double
squash_timer_get_elapsed_cpu (SquashTimer* timer) {
  return timer->elapsed_cpu;
}

/**
 * @brief Get the elapsed wall-clock time.
 *
 * @param timer The timer.
 * @return Number of seconds (wall-clock time) elapsed.
 */
double
squash_timer_get_elapsed_wall (SquashTimer* timer) {
  return timer->elapsed_wall;
}

/**
 * @}
 */
