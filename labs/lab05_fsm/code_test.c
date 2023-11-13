#include <stdio.h>

int main(void)
{
    char *s = "\
";
    // these are legal (but uncouth) C
    printf("%x\n", '//');
    printf("%x\n", '/**/');
    return 0;
}


/* Test C Comment *.  */
/* // */
int test_code() {
    printf("/* this is not really a C comment! */\n");
    printf("// this is not a C++ comment, either!\n");
}
