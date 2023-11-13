#include <stdlib.h>    // for exit()
#include <stdio.h>     // for the usual printf(), etc.
#include <getopt.h>    // for getopt()
/*
 * ASSIGNMENT
 *
 * - "#include" any other necessary headers (as indicated by "man"
 *    pages)
 */
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "eprintf.h"


// To get `getopt_long()` to work, you need to provide a static
// (usually) array of `struct option` structures.  There are four
// members to be filled in:

// 1. `name` is a (char *) string containing the "long" option name
// (e.g. "--help" or "--format=pdf").

// 2. `has_arg` has one of these values that describe the
// corresponding option:
enum {
    NO_ARG  = 0, // the option takes no argument
    REQ_ARG = 1, // the option must have an argument
    OPT_ARG = 2  // the option takes an optional argument
};

// 3. The "flag" is an int pointer that determines how the function
// will return its value. If it is NULL, getopt_long() will return
// "val" (the fourth member) as its function return. If it is not
// NULL, getopt_long() will return 0 and set "*flag" to "val".

// 4. "val" is an int which is either a character to denote a "short"
// (e.g. "-h" or "-f pdf") option or 0, to denote an option which does
// not have a "short" form.

// The array is terminated by an entry with a NULL name (first
// element).

static struct option options[] = {
    // elements are:
    // name       has_arg   flag   val
    { "children", OPT_ARG,  NULL,  'c'},
    { "help",     NO_ARG,   NULL,  'h'},
    { "nosync",   NO_ARG,   NULL,  'n'},
    { "pgid",     NO_ARG,   NULL,  'g'},
    { "ppid",     NO_ARG,   NULL,  'p'},
    { NULL }  // end of options table
};

/*
 * These globals are set by command line options. Here, they are set
 * to their default values.
 */
int showPpids = 0;   // show parent process IDs
int showPgids = 0;   // show process group IDs
int synchronize = 1; // synchronize outputs (don't use until Part 3)


enum { IN_PARENT = -1 }; // must be negative
/*
 * In the parent, this value is IN_PARENT. In the children, it's set
 * to the order in which they were spawned, starting at 0.
 */
int siblingIndex = IN_PARENT;


// This is a global count of signals received.
int signalCount = 0;


void writeLog(char message[], const char *fromWithin)
// print identifying information about the current process to stdout
{
    char processName[128];
    int colonIndent;
    //  * ASSIGNMENT
    //  *
    //  * - if `siblingIndex` is IN_PARENT,
    if (siblingIndex == IN_PARENT) {
    //  *     + set a string buffer `processName` to "parent" (hint: strcpy())
        strcpy(processName,"parent");
    //  *     + set `colonIndent` to 20
        colonIndent = 20;
    }
    //  * - otherwise,
    else {
    //  *     + set a string buffer `processName` to a string of the form
    //  *       "child #" where "#" is `siblingIndex` (hint: snprintf())
        snprintf(processName,128,"child %d",siblingIndex);
    //  *     + set `colonIndent` to 30
        colonIndent = 20;
    }
    //  * - use `colonIndent` to set the indent up to the ":"
    //  *   using this example, which prints `processName`:
    //  *
    //  *     printf("%*s: %s\n", colonIndent, "process name", processName);
    //  *
    printf("%*s: %s\n", colonIndent, "process name", processName);
    //  * - print the process ID (hint: getpid(2)) indented as above
    printf("%*s: %d\n", colonIndent,"process ID",getpid());
    //  * - if `showPpids` is true,
    if (showPpids)
    //  *     + print the parent process ID (hint: getppid(2)) indented as above
        printf("%*s: %d\n", colonIndent,"parent process ID",getppid());
    //  * - if `showPgids` is true,
    if (showPgids)
    //  *     + print the process group ID (hint: getpgrp(2)) indented as above
        printf("%*s: %d\n", colonIndent,"process group ID",getpgrp());
    //  * - print `signalCount` indented as above (with a "%d" format, of course)
    printf("%*s: %d\n", colonIndent,"signal count",signalCount);
    //  * - print `message` indented as above
    printf("%*s: %s\n", colonIndent,"message",message);
    //  * - print `fromWithin` indented as above
    printf("%*s: %s\n", colonIndent,"from within",fromWithin);
    //  * - print a blank line to separate this from other log entries
    printf("\n");
    //  *
    //  * (Note: The second argument to this function, `fromWithin`, should
    //  *  always be `__func__` (no quotes, just an identifier `__func__`).)

}


void handler(int sigNum)
// handle signal `sigNum`
{
    char message[128];
    //  *
    //  * ASSIGNMENT
    //  *
    //  * - increment signalCount
    signalCount++;
    //  * - create a message that includes `sigNum` and its string
    //  *   equivalent (hint: snprintf(3) and strsignal(3))
    snprintf(message,sizeof(message),"%d - %s",sigNum,strsignal(sigNum));
    //  * - add an entry to the log that includes that message (hint:
    //  *   writeLog())
    writeLog(message,__func__);
    //  *
    //  * Note: Although you'll be doing it here, It is not a safe
    //  * practice to call printf() or any other standard I/O function
    //  * from within (or below) a signal handler, for a reason that will
    //  * be made clear in the lab.
    //  *
}


void initSignals(void)
// initialize all signals
{
    int i;
    sig_t sig;
    char buf[64];
    //  * ASSIGNMENT
    //  *
    //  * - for every signal from 1 to _NSIG (NSIG on MacOS)
    for (i=1;i<_NSIG;i++) {
    //  *   except SIGTRAP and SIGQUIT (to make debugging easier),
        if ( i==SIGTRAP || i==SIGQUIT ) continue;
    //  *     - try to set its handler to handler() (hint: signal(2))
        sig = signal(i,handler);
    //  *       if this fails,
        if (sig == SIG_ERR) {
    //  *         - add an entry to the log with the signal number and
    //  *           its string equivalent (hint: strsignal(3) and
    //  *           writeLog())
            snprintf(buf,sizeof(buf),"Error Setting Handler for %d: %s",i,strsignal(i));
            writeLog(buf,__func__);
        }
    }
}


static void usage(char *progname)
// issue a usage error message
{
    eprintf("usage: %s [{args}]*\n", progname);
    eprintf("%s\n", " {args} are:");
    eprintf("%s",
        "  -g or --pgid                     list process group IDs\n"
        "  -n or --nosync                   turn off synchronization\n"
        "  -p or --ppid                     list parent PIDs (default: no)\n"
        "  -h or --help                     help (this message) and exit\n"
        );
    return;
}

int main(int argc, char **argv)
{
    int ch;
    static char *progname = "**UNSET**";

    /*
     * Parse the command line arguments.
     */
    progname = argv[0];
    for (;;) {
        ch = getopt_long(argc, argv, "c::ghnp", options, NULL);
        if (ch == -1)
            break;

        switch (ch) {

        case 'c':
            break;

        case 'g':
            showPgids = 1;
            break;

        case 'h':
            usage(progname);
            exit(0);

        case 'n':
            synchronize = 0;
            break;

        case 'p':
            showPpids = 1;
            break;

        default:
            printf("?? getopt returned character code 0x%02x ??\n", ch);
            exit(1);
        }
    }

    //  * ASSIGNMENT
    //  *
    //  * - initialize all signals (hint: initSignals())
    initSignals();
    //  * - add an entry to the log that the parent is pause()'d for a
    //  *   signal (hint: writeLog())
    writeLog("Parent Pausing",__func__);
    //  * - await a signal (hint: pause())
    pause();

    return 0;
}
