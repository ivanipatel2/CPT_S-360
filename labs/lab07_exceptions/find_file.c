#define _GNU_SOURCE // to get asprintf() prototype
#include <stdio.h>  // this needs to be the first #include in that case

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <getopt.h>

#include "except.h"
#include "eprintf.h"

/*
 * Here's how you declare an exception with the "except" package:
 */
Except_T StatFailed = { "Failed to open file." };
/*
 * ASSIGNMENT
 *
 * Add additional declarations for exceptions TargetFound and
 * MiscellaneousError.
 */
Except_T TargetFound = { "The target file was found." };
Except_T MiscellaneousError = { "There was a strange miscellaneous error" };

char *progname = "*** progname not set ***"; /* should be argv[0] */

int verbose = 0; /* set on command line */

static void explore(char *path, char *target);

static void traverseDirectory(char path[], char *target)
{
    DIR *dirp;
    struct dirent *entry;
    char *subpath;
    int rc;
    /*
     * ASSIGNMENT
     *
     * Implement the following pseudocode:
     */

    //  * open the directory associated with `path` (hint: opendir(3))
    dirp = opendir(path);
    //  * if the open fails,
    if (dirp == NULL)
    //  *     raise the MiscellaneousError exception
        RAISE(MiscellaneousError);

    //  * for each entry in the directory (hint: readdir(3)),
    while ((entry = readdir(dirp)) != NULL) {
    //  *     if the entry's name is "."  or ".."
        if (strcmp(".",entry->d_name)==0 || strcmp("..",entry->d_name)==0)
    //  *         skip that entry
            continue;
    //  *     allocate a string `subpath` concatenatiing `path`, "/", and
    //  *      the entry's name (hint: asprintf())
        rc = asprintf(&subpath,"%s/%s",path,entry->d_name);
        if (rc<0) RAISE(MiscellaneousError);
    //  *     if in the following ... (hint: TRY ... END_TRY)
        TRY
    //  *         call explore on `subpath` and pass `target` as well
            explore(subpath,target);
    //  *     ... the TargetFound exception is raised (hint: EXCEPT())
        EXCEPT(TargetFound);
    //  *         free `subpath`
            free(subpath);
    //  *         close the open directory (hint: closedir(3)
            closedir(dirp);
    //  *         re-raise the exception (hint: RERAISE)
            RERAISE;
    //  *     ... any other exception is raised (hint: ELSE)
        ELSE
    //  *         print a message to stderr that explore() failed
            eprintf("explore() failed!\n");
        END_TRY;
    //  *     free `subpath`
        free(subpath);
    }
    //  * close the directory associated with `path` (hint: closedir(3))
    closedir(dirp);
}


static void explore(char *path, char *target)
/* look at, in, and below `path` for a file named `target` */
{
    struct stat st;
    int rc;
    char *str;
    /*
     * ASSIGNMENT
     *
     * Implement the following pseudocode:
     */

    //eprintf("exploring %s for %s\n",path,target);
    //  * get the status of `path` (hint: stat(2))
    rc = stat(path,&st);
    //  * if it fails,
    if (rc<0)
    //  *     raise the StatFailed exception (hint: RAISE)
        RAISE(StatFailed);
    //  * find the last '/'-delimited component of `path` (or use `path`
    //  *  itself if it contains no '/'s, hint: strrchr())
    str = strrchr(path,'/');
    if (str == NULL)
        str = path;
    else
        str++;//skip the '/'

    //  * if that component is equal to `target`, (hint: strcmp())
    if (strcmp(target,str) == 0) {
    //  *     if `verbose` is set,
        if (verbose)
    //  *         print `path` to standard output, followed by a newline
    //  *          (hint: printf())
        printf("%s\n",path);
    //  *     raise the TargetFound exception (hint: RAISE())
        RAISE(TargetFound);
    }
    //  * if `path` is a directory (hint: S_ISDIR())
    if (S_ISDIR(st.st_mode))
    //  *     traverse it (hint: traverseDirectory())
        traverseDirectory(path,target);

}

void findFile(char *top, char *target)
{
    /*
     * ASSIGNMENT
     *
     * Implement the following pseudocode:
     */

    //  * if in the following ... (hint: TRY ... END_TRY)
    TRY;
    //  *     call explore on `top` and pass `target` as well
        explore(top,target);
    //  * ... the StatFailed exception is raised
    EXCEPT(StatFailed);
    //  *     do nothing (put a ";" here)
        ;
    //  * ... the TargetFound exception is raised
    EXCEPT(TargetFound);
    //  *     exit successfully (hint: exit(3))
        exit(0);
    END_TRY;
}


void usage(void)
{
    printf("usage: %s [-h] [-v] target [directory]*\n", progname);
}


int main(int argc, char *argv[])
{
    int i, ch;
    char *target;
    extern int optind;

    progname = argv[0];
    while ((ch = getopt(argc, argv, "hv")) != -1) {
        switch (ch) {

        case 'v':
            verbose = 1;
            break;

        case 'h':
            usage();
            exit(EXIT_SUCCESS);

        case '?':
            usage();
            exit(EXIT_FAILURE);
        }
    }
    if (optind >= argc) {
        usage();
        exit(EXIT_FAILURE);
    }
    target = argv[optind++];
    if (optind == argc) {
        /* directory name(s) not provided */
        findFile(".", target);
    } else {
        /* directory name(s) provided */
        for (i = optind; i < argc; i++)
            findFile(argv[i], target);
    }
    /*
     * If we find the target, we'll exit immediately (and
     * successfully), so if we get to this point...
     */
    exit(EXIT_FAILURE);
}
