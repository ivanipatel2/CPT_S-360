#define _GNU_SOURCE // to get asprintf() prototype
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <regex.h>
#include <glib.h>
#include <sys/types.h>
#include <sys/wait.h>


#include "eprintf.h"
#include "syscall_check.h"

#define BLOCK_SIZE 4096
char * progname = "Not Set";

#define HTML_RE   "<[^>]*href=(\"?)(https?:[^\"?>#]*)[^\"]*\\1[^>]*>"

void usage(void)
{
    eprintf("usage: %s [option ]* urlOrFilename\n\n", progname);
    eprintf("checklinks retrieves the contents of a web page or reads a file and scans the result for \n\
URLs (hyperlinks). Each hyperlink is then tested for existence. Finally, checklinks \n\
prints out all of the links, sorted uniquely, with each URL prefaced by either “okay” \n\
if it was accessible or “error” if it was not.\n\n");
    eprintf("  -f treat the urlOrFilename argument as a (local) file name. (default: Treat it as a URL.)\n");
    eprintf("  -h print a help message and exit\n");
    eprintf("  -p run in parallel\n");
}


typedef struct {
  pid_t pid;
  int status;
} UrlNode;

gint string_compare (
  gconstpointer a,
  gconstpointer b,
  gpointer user_data
) {
  char *s1 = (char *)a;
  char *s2 = (char *)b;
  return strcmp(s1,s2);
}

gboolean show_node(gpointer item, gpointer value, gpointer data) {
  UrlNode *node = (UrlNode *)value;
  char *url = (char *)item;
  printf("Pid: %d Status: %d Url: %s\n",node->pid,node->status,url);
  return FALSE;
}

static char *statuses[2]={"error","okay"};

gboolean output_node(gpointer item, gpointer value, gpointer data) {
  UrlNode *node = (UrlNode *)value;
  char *url = (char *)item;
  printf("%-5s %s\n",statuses[node->status==0],url);
  return FALSE;
}

gboolean check_url(gpointer item, gpointer value, gpointer data) {
  UrlNode *node = (UrlNode *)value;
  char *url = (char *)item;
  int dowait = !*((int*)data);
  int rc;

  SYSCALL_CHECK(rc = fork());
  if (rc == 0) {
    execlp("wget","--spider","-q","--delete-after","-T10","-t1",url,NULL);
  }
  else {
    node->pid=rc;
    if (dowait)
      wait(&node->status);
  }

  return FALSE;
}

gboolean set_pid_status(gpointer item, gpointer value, gpointer data) {
  UrlNode *node = (UrlNode *)value;
  //char *url = (char *)item;
  UrlNode *find = (UrlNode *)data;

  if (find->pid == node->pid) {
    node->status = find->status;
    return TRUE;
  }
  return FALSE;
}

void wait_for_children(GTree *urls) {
  int rc=1;
  int status;
  UrlNode node;

  while (rc > 0) {
    rc = wait(&status);
    if (rc >0) {
      node.pid = rc;
      node.status = status;
      // foreach_node would be better, but i dont seem to have it.
      g_tree_foreach(urls,set_pid_status,&node);
    }
  }
  if (errno != ECHILD) {
    perror("wait_for_children");
    eprintf_fail("Error waiting for child: %d\n",rc);
  }
}

int main(int argc, char *argv[])
{
    int len,buflen,ch,rc,pos;
    FILE *f;
    extern int optind;
    int fileMode = 0;
    int parallel = 0;
    int urlcount = INT_MAX;
    char *urlOrFilename = NULL;
    char *cmd;
    char *data = NULL;
    char *url;
    GTree *urls;
    UrlNode *node;

    progname = argv[0];
    while ((ch = getopt(argc, argv, "fhpc:")) != -1) {
        switch (ch) {

        case 'f':
            fileMode = 1;
            break;

        case 'h':
            usage();
            exit(EXIT_SUCCESS);

        case 'p':
            parallel = 1;
            break;

        case 'c':
            urlcount = atoi(optarg);
            break;
        }
    }
    if (optind >= argc) {
        usage();
        exit(EXIT_FAILURE);
    }

    if (optind == argc) {
        /* url/file name(s) not provided */
        eprintf_fail("No urlOrFilename provided");
    }

    urlOrFilename = argv[optind++];

    // open the urlOrFilename
    if (fileMode) {
      f = fopen(urlOrFilename, "r");
    }
    else {
      len = asprintf(&cmd,"wget --no-cache --delete-after -q -O - %s",urlOrFilename);
      assert(len>0);
      f = popen(cmd,"r");
      free(cmd);
    }
    assert (f!=NULL);
    buflen=BLOCK_SIZE;
    pos=0;
    rc=1;
    while (rc > 0) {
      data = realloc(data,buflen);
      rc = fread(data+pos,1,BLOCK_SIZE,f);
      pos += rc;
      if (rc == BLOCK_SIZE) {
        // make buffer larger, and read more
        buflen+=BLOCK_SIZE;
      }
      else {
        // terminate read and set NULL terminator
        data[pos]=0;
        rc=0;
      }
    }
    fclose(f);

    // setup the urls tree
    urls = g_tree_new_full(string_compare, NULL, g_free, g_free);

    // find the urls and process them.
    regex_t http_re;
    regmatch_t http_match[5];
    rc = regcomp(&http_re,HTML_RE,REG_EXTENDED|REG_ICASE);
    if (rc != 0) {
      regerror(rc,&http_re,data,buflen);
      eprintf_fail("RegEx compile Error: %d %s\n",rc,data);
    }

    int count=0;
    pos=0;
    while (1) {
      rc = regexec(&http_re, data + pos, 5, http_match, 0);
      if (rc == REG_NOMATCH) break;
      assert(rc == REG_NOERROR);

      //pos += http_match[0].rm_so;
      //len = http_match[0].rm_eo-http_match[0].rm_so;
      //data[pos+len+1] = 0;
      if (http_match[2].rm_so != -1) {
          len = http_match[2].rm_eo-http_match[2].rm_so;
          data[pos+http_match[2].rm_eo]=0;
          //remove trailing /
          //if (data[pos+http_match[2].rm_eo-1]=='/'){
          //  len--;
          //  data[pos+http_match[2].rm_eo-1]=0;
          //}

          url = data+pos+http_match[2].rm_so;
          //printf("URL: %s\n",url);
          //store the url in the tree;
          node = calloc(1,sizeof(UrlNode));
          g_tree_insert(urls, strdup(url), node);
      }

      pos+=http_match[0].rm_eo;
      count++;
      if (count>urlcount) break;
    }
    // url gather cleanup
    regfree(&http_re);
    free(data);

    //g_tree_foreach(urls,show_node,NULL);
    g_tree_foreach(urls,check_url,&parallel);
    if (parallel)
      wait_for_children(urls);
    g_tree_foreach(urls,output_node,NULL);

    //final cleanup
    g_tree_destroy(urls);
}
