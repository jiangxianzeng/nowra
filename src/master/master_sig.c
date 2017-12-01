/*
 * @Name: master_sig.c
 *
 */

/* System libraries. */

#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/* Utility library. */

#include "msg.h"

/* Application-specific. */

#include "master.h"


int     master_gotsigchld;
int     master_gotsighup;

/* master_sighup - register arrival of hangup signal */

static void master_sighup(int sig)
{

    /*
     * WARNING WARNING WARNING.
     * 
     * This code runs at unpredictable moments, as a signal handler. Don't put
     * any code here other than for setting a global flag.
     */
    master_gotsighup = sig;
}

/* master_sigchld - register arrival of child death signal */

static void master_sigchld(int sig)
{

    /*
     * WARNING WARNING WARNING.
     * 
     * This code runs at unpredictable moments, as a signal handler. Don't put
     * any code here other than for setting a global flag.
     */
    master_gotsigchld = sig;
}

/* master_sigdeath - die, women and children first */

static void master_sigdeath(int sig)
{
    char   *myname = "master_sigdeath";
    struct sigaction action;
    pid_t   pid = getpid();

    msg_info("terminating on signal %d", sig);

    /*
     * Terminate all processes in our process group, except ourselves.
     */
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_handler = SIG_IGN;

    if (sigaction(SIGTERM, &action, (struct sigaction *) 0) < 0)
        msg_fatal("%s: sigaction: %s", myname, strerror(errno));
    if (kill(-pid, SIGTERM) < 0)
        msg_fatal("%s: kill process group: %s", myname, strerror(errno));

    /*
     * Deliver the signal to ourselves and clean up. XXX We're running as a
     * signal handler and really should not be doing complicated things...
     */
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_handler = SIG_DFL;
    if (sigaction(sig, &action, (struct sigaction *) 0) < 0)
        msg_fatal("%s: sigaction: %s", myname, strerror(errno));
    if (kill(pid, sig) < 0)
        msg_fatal("%s: kill myself: %s", myname, strerror(errno)); 
}

/* master_sigsetup - set up signal handlers */

void master_sigsetup(void)
{
    char   *myname = "master_sigsetup";
    struct sigaction action;
    static int sigs[] = {
        SIGINT, SIGQUIT, SIGILL, SIGBUS, SIGSEGV, SIGTERM,
    };
    unsigned i;

    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    /*
     * Prepare to kill our children when we receive any of the above signals.
     */
    action.sa_handler = master_sigdeath;
    for (i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++)
        if (sigaction(sigs[i], &action, (struct sigaction *) 0) < 0)
            msg_fatal("%s: sigaction(%d): %s", myname, sigs[i], strerror(errno));

    /*
     * Intercept SIGHUP (re-read config file) and SIGCHLD (child exit).
     */
    action.sa_handler = master_sighup;
    if (sigaction(SIGHUP, &action, (struct sigaction *) 0) < 0)
        msg_fatal("%s: sigaction(%d): %m", myname, SIGHUP);

    action.sa_flags |= SA_NOCLDSTOP;
    action.sa_handler = master_sigchld;
    if (sigaction(SIGCHLD, &action, (struct sigaction *) 0) < 0)
        msg_fatal("%s: sigaction(%d): %m", myname, SIGCHLD); 
}
