
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/wait.h>
#include <libgen.h>
#include <sys/resource.h>
#include <signal.h>

#include "eprintf.h"
#include "snapshot.h"
#include "allocarray.h"
#include "syscall_check.h"


#define BUF_LEN 1024*1024

int snapshot(char *ssname, char *progpath, char *readme) {
  int rc,len,wlen;
  int fin,fout;
  char tar_path[PATH_MAX],readme_path[PATH_MAX],core_path[PATH_MAX],prog_path[PATH_MAX];
  char *argv[10];
  char *buffer;
  struct rlimit corelimit = {RLIM_INFINITY,RLIM_INFINITY};

  //setup ulimits

  // Create Directory
  rc = mkdir(ssname,0777);
  if (rc != 0) {
    if (errno == EEXIST)
      return -1;
    eprintf("Errno on mkdir: %d - %s\n",errno,strerror(errno));
  }

  // Write out readme file
  sprintf(readme_path,"%s/%s",ssname,"README.txt");
  rc = open(readme_path,O_WRONLY|O_CREAT|O_TRUNC,0666);
  if (rc < 0) {
    eprintf("Error on open: %d - %s\n",errno,strerror(errno));
  }
  else {
    len = write(rc,readme,strlen(readme));
    if (len != strlen(readme))
      eprintf("Error writing data, len=%d errno=%d\n",len,errno);
  }
  rc = close(rc);

  //Create coredump
  sprintf(core_path,"%s/%s",ssname,"core");
  setrlimit(RLIMIT_CORE,&corelimit);

  rc = fork();
  if (rc == 0) {
    SYSCALL_CHECK(chdir(ssname));
    kill(getpid(),SIGABRT);
    eprintf("Error killing %d - %s\n", rc, strerror(errno));
    exit(1);
  }
  if (rc == -1) {
    eprintf("Error Forking: %d - %s\n",errno,strerror(errno));
    return -1;
  }
  wait(&rc);

  //collect program image
  buffer = strdup(progpath);
  sprintf(prog_path,"%s/%s",ssname,basename(progpath));
  free(buffer);

  ALLOC_ARRAY(buffer,char,BUF_LEN);

  SYSCALL_CHECK(fin = open("/proc/self/exe",O_RDONLY));
  SYSCALL_CHECK(fout = open(prog_path,O_WRONLY|O_CREAT|O_TRUNC,0777));
  len=1;
  while (len > 0) {
    SYSCALL_CHECK(len=read(fin,buffer,BUF_LEN));
    if (len > 0) {
      SYSCALL_CHECK(wlen=write(fout,buffer,len));
      if (len != wlen)
        eprintf_fail("Read and Write Lengths differ: %d != %d\n",len,wlen);
    }
  }
  FREE_ARRAY(buffer);
  SYSCALL_CHECK(close(fin));
  SYSCALL_CHECK(close(fout));


  //make tarfile
  sprintf(tar_path,"%s.tgz",ssname);
  argv[0] = "/bin/tar";
  argv[1] = "-czf";
  argv[2] = tar_path;
  argv[3] = readme_path;
  argv[4] = prog_path;
  argv[5] = core_path;
  argv[6] = NULL;
  //argv[4] = core_path;
  //argv[5] = prog_path;
  rc = fork();
  if (rc == 0) {
    rc = execv(argv[0],argv);
    eprintf("Error execing %d - %s\n", rc, strerror(errno));
    exit(0);
  }
  if (rc == -1) {
    eprintf("Error Forking: %d - %s\n",errno,strerror(errno));
    return -1;
  }
  wait(&rc);


  //remove directory
  SYSCALL_CHECK(unlink(readme_path));
  SYSCALL_CHECK(unlink(core_path));
  SYSCALL_CHECK(unlink(prog_path));
  SYSCALL_CHECK(rmdir(ssname));
  return 0;
}
