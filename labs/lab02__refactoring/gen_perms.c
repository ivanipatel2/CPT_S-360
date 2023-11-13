
#include <stdlib.h>

int level;
#define NOT_DONE -1
int *val;


void recur(int k,
           int nElems,
           void (*handlePerm)(int elems[],
                                int nElems,
                                void *userArg),
              void *userArg)
{
    int i;

    val[k] = level;
    level++;
    if (level == nElems) {
      handlePerm(val,nElems,userArg);
    }
    for (i = 0; i < nElems; i++)
        if (val[i] == NOT_DONE)
            recur(i,nElems,handlePerm,userArg);
    level--;
    val[k] = NOT_DONE;
}

void genPerms(int nElems,
              void (*handlePerm)(int elems[],
                                int nElems,
                                void *userArg),
              void *userArg) {
  int i;
  val = malloc(nElems * sizeof(int));
  level = 0;
  for (i = 0; i < nElems; i++)
    val[i] = NOT_DONE;
  for (i = 0; i < nElems; i++)
    recur(i,nElems,handlePerm,userArg);
  free(val);
}
