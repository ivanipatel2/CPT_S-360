#include <stdio.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <glib.h>
#include "eprintf.h"
#include "syscall_check.h"

enum cmds {
  CD,DELETE,EXECUTE,LS,READ,SEARCH,WRITE,ENDCMDS
};

#define SREAD 4
#define SWRITE 2
#define SEXEC 1

struct mapping {
  const char *scmd;
  enum cmds cmd;
};

static struct mapping cmdlist[]={
  {      "cd",CD },
  {  "delete",DELETE },
  { "execute",EXECUTE },
  {      "ls",LS },
  {    "read",READ },
  {  "search",SEARCH },
  {   "write",WRITE },
};

void add_user(gpointer hash, int uid) {
  struct passwd *pw;
  //eprintf("add_user: %d\n",uid);
  pw = getpwuid(uid);
  if (pw == NULL)
    perror("Error getting passwd struct for uid\n");
  g_hash_table_add(hash,strdup(pw->pw_name));
  return;
}

void add_everyone_but(gpointer hash, int uid) {
  struct passwd *pw;

  while ((pw = getpwent ()) != NULL) {
    if (pw->pw_uid != uid)
      g_hash_table_add(hash,strdup(pw->pw_name));
  }
  endpwent();
}

void add_group(gpointer hash, int gid, const char *exclude_user) {
  struct group *gr;
  struct passwd *pw;
  int i;
  gr = getgrgid(gid);
  if (gr == NULL)
    perror("Error getting passwd struct for gid\n");
  // Supplemental Groups
  for (i=0;gr->gr_mem[i];i++) {
    if (exclude_user != NULL && strcmp(gr->gr_mem[i],exclude_user)==0 ) {
      //printf("exclude %s\n",gr->gr_mem[i]);
    }
    else
      g_hash_table_add(hash,strdup(gr->gr_mem[i]));
    //printf("-%s-\n",gr->gr_mem[i]);
  }
  // primary groups
  // probably needs error checking
  while ((pw = getpwent())) {
    if (pw->pw_gid == gid ) {
      if (exclude_user != NULL && strcmp(pw->pw_name,exclude_user)==0 ) {
        //printf("exclude %s\n",pw->pw_name);
      }
      else
        g_hash_table_add(hash,strdup(pw->pw_name));
    }
  }
  endpwent();
  return;
}

int can_search_full_path(const char *path, int bits) {
  char *dir;
  int result=0;
  struct stat fsobj;
  int rc;

  //printf("Dirpath: %s\n",path);
  if (strcmp(path,"/") == 0)
    return 1;
  SYSCALL_CHECK(rc = stat(path,&fsobj));
  result = fsobj.st_mode & bits;
  if (result) {
    dir = g_path_get_dirname(path);
    result = can_search_full_path(dir,bits);
    free(dir);
  }
  return result;
}

void print_hash_key(gpointer key,gpointer value,gpointer data) {
  printf("%s\n",(char *)key);
}

void validate_search_path(gpointer key,gpointer value,gpointer data) {
  //char *k = (char *)key;
  char *d = (char *)data;
  //eprintf("validate search path %s %s\n",(char *)key,d);
  can_search_full_path(d,(int)(long)value);
}

int add_if_perms(GHashTable *hash, struct stat fsobj, int bits) {
  struct passwd *pw;
  //printf("mode: %o bits %d\n",fsobj.st_mode,bits);
  pw = getpwuid(fsobj.st_uid);
  if (fsobj.st_mode & (bits << 6))
    add_user(hash,fsobj.st_uid);
  else if (fsobj.st_mode & (bits << 3))
    add_group(hash,fsobj.st_gid,pw->pw_name);
  if (fsobj.st_mode & bits) {
    return 1;
  }
  return 0;
}

int handle_sicky_delete(GHashTable *hash, struct stat fsobj, struct stat dirobj) {
  GHashTable *dirhash;
  guint length;
  char **list;
  struct passwd *pw;
  int every;
  //printf("Sticky!\n");

  dirhash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
  every = add_if_perms(dirhash,dirobj,SWRITE);
  //loop over all writers of the directory, and see if they own the file or dir
  list = (char **)g_hash_table_get_keys_as_array(dirhash,&length);
  for (int i=0;i<length;i++) {
    pw = getpwnam(list[i]);
    if (pw->pw_uid == fsobj.st_uid || pw->pw_uid == dirobj.st_uid ) {
      g_hash_table_add(hash,strdup(list[i]));
    }
  }
  //if every just add the owners of the directory and file
  if (every) {
    pw = getpwuid(fsobj.st_uid);
    g_hash_table_add(hash,strdup(pw->pw_name));
    pw = getpwuid(dirobj.st_uid);
    g_hash_table_add(hash,strdup(pw->pw_name));
  }
  g_hash_table_remove_all(dirhash);
  return 0;
}

int main(int argc, char** argv) {
  enum cmds cmd;
  struct stat fsobj,dirobj;
  int rc,every,isdir;//,uid,gid;
  GHashTable *hash;
  char *fullpath,*dirpath;
  //int style=0;

  if (argc != 3) {
    eprintf_fail("Usage: %s <pathname>\n", argv[0]);
  }

  for (cmd=CD;cmd<ENDCMDS;cmd++){
    if (strcmp(cmdlist[cmd].scmd,argv[1])==0)
      break;
  }

  if (cmd==ENDCMDS) {
    eprintf_fail("Invalid Command Found: %s\n",argv[1]);
  }
  //printf("Cmd: %d - %s\n",cmd,cmdlist[cmd].scmd);
  fullpath = realpath(argv[2],NULL);
  if (fullpath == NULL) {
    perror("Problem calling realpath");
    exit(1);
  }
  //printf("Realpath: %s\n",fullpath);
  SYSCALL_CHECK(rc = stat(fullpath,&fsobj));
  dirpath = g_path_get_dirname(fullpath);
  SYSCALL_CHECK(rc = stat(dirpath,&dirobj));

  isdir = S_ISDIR(fsobj.st_mode);
  //gid = fsobj.st_gid;
  //uid = fsobj.st_uid;
  every = 0;

  hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
  add_user(hash,0);

  switch (cmd) {
    case CD:
      if (isdir) {
        every = add_if_perms(hash,fsobj,SEXEC);
      }
      else
        g_hash_table_remove_all(hash);
      break;
    case DELETE:
    //  style = SEXEC;
      if (dirobj.st_mode & S_ISVTX) {
        every = handle_sicky_delete(hash,fsobj,dirobj);
      }
      else {
        every = add_if_perms(hash,dirobj,SWRITE);
      }
      break;
    case WRITE:
      //style = SWRITE;
      every = add_if_perms(hash,fsobj,SWRITE);
      break;
    case EXECUTE:
      if (isdir) {
        g_hash_table_remove_all(hash);
      } // cant execute directories
      else {
        //style = SEXEC;
        every = add_if_perms(hash,fsobj,SEXEC);
      }
      break;
    case LS:
      if (!isdir) { // We can ls a file if we can read its directory
        SYSCALL_CHECK(rc = stat(dirpath,&fsobj));
        isdir = 1;
        //gid = fsobj.st_gid;
        //uid = fsobj.st_uid;
      }
      //falltrhough and report read ability on directory.
    case READ:
      //style = SREAD;
      every = add_if_perms(hash,fsobj,SREAD);
      break;
    case SEARCH:
      if (isdir) {
        every = add_if_perms(hash,fsobj,SREAD);
      }
      else
        g_hash_table_remove_all(hash);
      break;
    default:
      eprintf_fail("Holy Crap we never should get here!!!!\n");
  }
  //g_hash_table_foreach(hash,print_hash_key,NULL);
  //check paths now.
  //printf("-everyone-\n");
  if (every)
    every = can_search_full_path(dirpath,S_IXOTH);
  g_hash_table_foreach(hash,validate_search_path,dirpath);

  //special case when user is explicitly disallowed, we have to change every to
  // everyone - user
  //printf("Check %o & %o = %o\n",fsobj.st_mode,(style << 6),(fsobj.st_mode & ( style << 6 )));
  //if (every && ((fsobj.st_mode & ( style << 6 )) == 0) )  {
  //  every = 0;
  //  add_everyone_but(hash,fsobj.st_uid);
  //}


  if (every) {
    printf("(everyone)\n");
  }
  else {
    g_hash_table_foreach(hash,print_hash_key,NULL);
  }

  g_hash_table_destroy(hash);
  free(dirpath);
  free(fullpath);
  return 0;
}
