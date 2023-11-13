#define _XOPEN_SOURCE
#include <stdio.h>
#include <math.h>
#include <getopt.h> // for getopt()
#include <assert.h> // for assert()
#include <limits.h>
#include <string.h>
#include <strings.h>
#include <utmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <pwd.h>

#include "syscall_check.h"
#include "eprintf.h"

static char *help_string = "\n\
tattle [-d date] [-f filename] [-t time] [-u login[,login]∗]\n\
   -u option restricts the report to only those logins (e.g., “-u archie,veronica,jughead”).\n\
   -d option restricts the report to logins active on that date (format: mm/dd/yy).\n\
   -t option restricts the report to logins active at that time (format: HH:MM, 24-hour).\n\
   -f option takes data from filename (default: /var/log/wtmp).\n\
";

struct record {
    char *user;
    char *tty;
    time_t login;
    time_t logoff;
    char *host;
};

void show_time(char * prefix, struct tm *tm) {
    printf("%s: %d-%d-%d %d:%d:%d\n", prefix,  tm->tm_mon+1, tm->tm_mday, tm->tm_year+1900, tm->tm_hour,tm->tm_min,tm->tm_sec);
}

int check_users(const char *find, char *names[]) {
    int i;
    if (names[0] == NULL)
        return 1;
    for (i=0;names[i];i++)
        if (strcmp(find,names[i])==0)
            return 1;
    return 0;
}

int in_range(time_t t0, time_t t1, time_t t2, time_t t3) {
    if (t1 == 0)
        t1=INT_MAX;
    //printf("diff: %d\n",t3-t2);
    //printf("Compare: %d < %d || %d < %d\n",t1,t2,t3,t0);
    //printf("Compare: %s < %s || %s < %s\n",strdup(ctime(&t1)),strdup(ctime(&t2)),strdup(ctime(&t3)),strdup(ctime(&t0)));
    if ( t1 < t2 || t3 < t0 )
        //printf("Reject: %d || %d\n",t1<t2,t3<t0);
        return 0;
    return 1;
}

void output_records(struct record *records,int count, char *names[], time_t begin, time_t end) {
    char *fmt="%-15s %-7s %-15s %-20s %-10s\n";
    int i;
    char logoff[128];
    char logon[128];
    printf(fmt,"login","tty","log on","log off","from host");

    for (i=0;i<count;i++) {
        strftime(logon,128,"%D %R",localtime(&records[i].login));
        if (records[i].logoff !=0)
            strftime(logoff,128,"%D %R",localtime(&records[i].logoff));
        else
            strncpy(logoff,"(still logged in)",127);
        if (check_users(records[i].user,names) && in_range(records[i].login,records[i].logoff,begin,end))
            printf(fmt,records[i].user,records[i].tty,logon,logoff,records[i].host);
    }
}

void process_utmp(const char *filename, time_t begin, time_t end, char *names[]) {
    struct utmp *entry;
    int file,len,count,i,j;
    struct utmp *db;
    struct stat stat;
    struct record *rec;


    struct record *records = (struct record *)malloc( 16 * sizeof(struct record));
    int record_count=0;
    int record_limit=16;

    SYSCALL_CHECK(file = open(filename,O_RDONLY));
    SYSCALL_CHECK(fstat(file, &stat));
    db = (struct utmp *)malloc(stat.st_size);
    SYSCALL_CHECK(len=read(file,db,stat.st_size));
    count = len/sizeof(struct utmp);

    for (i=0,entry=db;i<count;i++,entry++) {
        //printf("Entry: type=%d pid=%d user=%s\n",entry->ut_type,entry->ut_pid,entry->ut_user);
        switch (entry->ut_type) {
            case USER_PROCESS:
                if (record_count == record_limit) {
                    record_limit += 16;
                    records = (struct record *)realloc(records, record_limit * sizeof(struct record));
                }
                rec = &records[record_count++];
                rec->user = entry->ut_user;
                rec->tty = entry->ut_line;
                rec->login = entry->ut_time;
                rec->logoff = 0;
                rec->host = entry->ut_host;
                break;
            case DEAD_PROCESS:
                //find the login record, and record time.
                for (j=0;j<record_count;j++) {
                    //printf("Check %s %s\n",records[j].tty, entry->ut_line);
                    if (strcmp(records[j].tty, &entry->ut_line[0])==0 && records[j].logoff==0) {
                        records[j].logoff = entry->ut_time;
                        break;
                    }
                }
                break;
			case BOOT_TIME:
				//log everyone out
                for (j=0;j<record_count;j++) {
					if (records[j].logoff == 0)
						records[j].logoff = entry->ut_time;
				}
                break;
            default:
                //printf("Not handling %d type\n",entry->ut_type);
                break;
        }
    }

    output_records(records,record_count,names,begin,end);
    free(db);
    free(records);
}

int main(int argc, char *argv[])
{
    char filename[PATH_MAX] = "/var/log/wtmp";
    char *users[128]={NULL}; // malloc this?  or home for the best
    char *user,*ptr;
    int opt,count;
    struct tm tm;
    time_t start,end;

	bzero(&tm,sizeof(tm));

    //setup default date
    start = 0;
    end = time(NULL);

    while ((opt = getopt(argc, argv, "hd:f:t:u:")) != -1) {
        switch (opt) {

        case 'd':
            ptr = strptime(optarg,"%D",&tm);
            if (ptr == NULL || *ptr != 0) {
                eprintf_fail("error parsing date: %s\n",optarg);
            }
            //show_time("Date Parse",&tm);
            start = mktime(&tm);
            end = start+24*60*60-1;
            break;

        case 'f':
            strcpy(filename,optarg);
            break;

        case 't':
            if (start == 0) {
                tm = *localtime(&end);
            }
            ptr = strptime(optarg,"%R",&tm);
            if (ptr == NULL) {
                eprintf_fail("error parsing time: %s\n",optarg);
            }
            //show_time("Time parse",&tm);
            start = mktime(&tm);
            end = mktime(&tm);
            break;

        case 'u':
            //parse users
            count=0;
            user = strtok(optarg,",");
            while (user) {
                users[count++]=user;
                //printf("User %s\n",user);
                user = strtok(NULL,",");
            }
            users[count]=NULL;
            break;

        case 'h':
            puts(help_string);
            break;
        default:
            assert(0);
        }
    }
    //show_time("Start",localtime(&start));
    //show_time("End",localtime(&end));
    process_utmp(filename,start,end,users);
    return 0;
}
