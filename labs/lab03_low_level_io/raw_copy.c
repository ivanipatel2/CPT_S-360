#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "allocarray.h"
#include "syscall_check.h"
#include "eprintf.h"

int main(int argc, char** argv) {
  int buf_len,len,wlen;
  int fin,fout;
  char *buffer;
  if (argc != 4)
    eprintf_fail("Argument must be: raw_copy Buffer_size src dest\n");

  buf_len = atoi(argv[1]);
  if (buf_len<=0)
    eprintf_fail("buffer must be greater than 0: %d\n",buf_len);
  ALLOC_ARRAY(buffer,char,buf_len);

  SYSCALL_CHECK(fin = open(argv[2],O_RDONLY));
  SYSCALL_CHECK(fout = open(argv[3],O_WRONLY|O_CREAT|O_TRUNC,0666));
  len=1;
  while (len > 0) {
    SYSCALL_CHECK(len=read(fin,buffer,buf_len));
    if (len > 0) {
      SYSCALL_CHECK(wlen=write(fout,buffer,len));
      if (len != wlen)
        eprintf_fail("Read and Write Lengths differ: %d != %d\n",len,wlen);
    }
  }
  FREE_ARRAY(buffer);
  SYSCALL_CHECK(close(fin));
  SYSCALL_CHECK(close(fout));
  exit(0);
}
