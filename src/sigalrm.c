/*
 * Reaver - Sigalrm handler functions
 * Copyright (c) 2011, Tactical Network Solutions, Craig Heffner <cheffner@tacnetsol.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of portions of this program with the
 *  OpenSSL library under certain conditions as described in each
 *  individual source file, and distribute linked combinations
 *  including the two.
 *  You must obey the GNU General Public License in all respects
 *  for all of the code used other than OpenSSL. *  If you modify
 *  file(s) with this exception, you may extend this exception to your
 *  version of the file(s), but you are not obligated to do so. *  If you
 *  do not wish to do so, delete this exception statement from your
 *  version. *  If you delete this exception statement from all source
 *  files in the program, then also delete it here.
 */

#include "sigalrm.h"
#include "send.h"
#include "platform/platform.h"

#ifdef _WIN32
#include <stdlib.h>

static HANDLE timer_queue;
static HANDLE timer_handle;
static CRITICAL_SECTION timer_lock;

static void sigalrm_cleanup(void)
{
        EnterCriticalSection(&timer_lock);
        if (timer_handle) {
                DeleteTimerQueueTimer(timer_queue, timer_handle, NULL);
                timer_handle = NULL;
        }
        LeaveCriticalSection(&timer_lock);

        if (timer_queue) {
                DeleteTimerQueueEx(timer_queue, NULL);
                timer_queue = NULL;
        }
        DeleteCriticalSection(&timer_lock);
}

static VOID CALLBACK timer_callback(PVOID parameter, BOOLEAN TimerOrWaitFired)
{
        (void) parameter;
        (void) TimerOrWaitFired;
        alarm_handler(0);
}
#endif

/* Initializes SIGALRM handler */
void sigalrm_init()
{
#ifdef _WIN32
        InitializeCriticalSection(&timer_lock);
        timer_queue = CreateTimerQueue();
        if (!timer_queue) {
                cprintf(CRITICAL, "[-] Failed to create Windows timer queue\n");
        }
        atexit(sigalrm_cleanup);
#else
        struct sigaction act;
        struct sigevent sev;

        memset(&act, 0, sizeof(struct sigaction));
        act.sa_handler = alarm_handler;

        sigaction (SIGALRM, &act, 0);

        sev.sigev_notify = SIGEV_SIGNAL;
        sev.sigev_signo = SIGALRM;
        sev.sigev_value.sival_ptr = &globule->timer_id;
        timer_create(CLOCK_REALTIME, &sev, &globule->timer_id);
#endif
}

static void rewind_timer() {
        set_out_of_time(0);

#ifdef _WIN32
        unsigned long delay_ms = (unsigned long) (globule->resend_timeout_usec / 1000UL);
        if (delay_ms == 0) {
                delay_ms = 1;
        }

        if (!timer_queue) {
                return;
        }

        EnterCriticalSection(&timer_lock);
        if (timer_handle) {
                DeleteTimerQueueTimer(timer_queue, timer_handle, NULL);
                timer_handle = NULL;
        }
        if (!CreateTimerQueueTimer(&timer_handle, timer_queue, timer_callback, NULL, delay_ms, delay_ms, WT_EXECUTEDEFAULT)) {
                cprintf(CRITICAL, "[-] Failed to create Windows timer queue timer\n");
        }
        LeaveCriticalSection(&timer_lock);
#else
        struct itimerspec its;
        its.it_value.tv_sec = globule->resend_timeout_usec / 1000000;
        its.it_value.tv_nsec = (globule->resend_timeout_usec % 1000000) * 1000;
        its.it_interval.tv_sec = its.it_value.tv_sec;
        its.it_interval.tv_nsec = its.it_value.tv_nsec;
        timer_settime(globule->timer_id, 0, &its, NULL);
#endif
}

static unsigned timeout_ticks;
static unsigned long long timeout_usec;

/* Starts receive timer. Called from send_packet() after a packet is trasmitted */
void start_timer()
{
	struct wps_data *wps = get_wps();
	timeout_ticks = 0;

        /*
	 * The M5 and M7 messages have very fast turn around times -
	 * typically a few hundreths of a second. We don't want to wait
	 * around forever to see if we get them or not, so use a short
	 * timeout value when waiting for those messages.
	 * Ignore this timeout if we know the AP responds with NACKs when
	 * the wrong pin is supplied instead of not responding at all.
         */
        if(get_timeout_is_nack() &&
	  (wps->state == RECV_M5 || wps->state == RECV_M7))
	{
		timeout_usec = get_m57_timeout();
        } else {
                /*
		 * Other messages may take up to 2 seconds to respond -
		 * wait a little longer for them.
		*/
                timeout_usec = get_rx_timeout() * 1000000LL;
        }

	rewind_timer();
}

/* Timer is stopped by process_packet() when any valid EAP packet is received */
void stop_timer()
{
#ifdef _WIN32
        if (!timer_queue) {
                return;
        }

        EnterCriticalSection(&timer_lock);
        if (timer_handle) {
                DeleteTimerQueueTimer(timer_queue, timer_handle, NULL);
                timer_handle = NULL;
        }
        LeaveCriticalSection(&timer_lock);
#else
        struct itimerspec its = {0};

        set_out_of_time(0);

        timer_settime(globule->timer_id, 0, &its, NULL);
#endif
}

/* Handles SIGALRM interrupts */
void alarm_handler(int x)
{
	timeout_ticks++;

	if(timeout_ticks * globule->resend_timeout_usec > timeout_usec) {
		set_out_of_time(1);
		cprintf(VERBOSE, "[!] WARNING: Receive timeout occurred\n");
	} else {
		resend_last_packet();
		rewind_timer();
	}
}

