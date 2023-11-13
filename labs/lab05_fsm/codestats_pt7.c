#include <stdio.h>
#include <assert.h> // for assert()
#include <ctype.h>

struct CodeStats {
    int lineCount;
    int linesWithCodeCount;
    int cplusplusCommentCount;
    int cCommentCount;
    int cPreprocessorDirectiveCount;
};


void codeStats_init(struct CodeStats *codeStats)
{
    codeStats->lineCount = 0;
    codeStats->linesWithCodeCount = 0;
    codeStats->cplusplusCommentCount = 0;
    codeStats->cCommentCount = 0;
    codeStats->cPreprocessorDirectiveCount = 0;
}


void codeStats_print(struct CodeStats codeStats, char *fileName)
{
    printf("                   file: %s\n", fileName);
    printf("             line count: %d\n", codeStats.lineCount);
    printf("  lines with code count: %d\n", codeStats.linesWithCodeCount);
    printf("      C++ comment count: %d\n", codeStats.cplusplusCommentCount);
    printf("        C comment count: %d\n", codeStats.cCommentCount);
    printf("preprocessor directives: %d\n", codeStats.cPreprocessorDirectiveCount);
}


#define HANDLE_NEWLINE  do { codeStats->lineCount++; \
                        codeStats->linesWithCodeCount += foundCodeOnLine; \
                        foundCodeOnLine = 0; } while(0)


void codeStats_accumulate(struct CodeStats *codeStats, char *fileName)
{
    FILE *f = fopen(fileName, "r");
    int ch;
    int foundCodeOnLine = 0;

    enum {
        START,SLASH,CPPCOMMENT,CCOMMENT,CSTAR,STRING,SBACKSLASH,CHAR,CBACKSLASH,PREPROCESS,PBACKSLASH
    } state = START;

    assert(f);
    while ((ch = getc(f)) != EOF) {
        //printf("%d %c\n",state,ch);
        switch (state) {

        case START:
            if (ch == '\n') {
                HANDLE_NEWLINE;
            }
            else if (isspace(ch)) {
                // no action
            }
            else if (ch == '/') {
                state = SLASH;
            }
            else if (ch == '"') {
                state = STRING;
                foundCodeOnLine = 1;
            }
            else if (ch == '\'') {
                state = CHAR;
                foundCodeOnLine = 1;
            }
            else if (ch == '#') {
                state = PREPROCESS;
                codeStats->cPreprocessorDirectiveCount++;
            }
            else {
                foundCodeOnLine = 1;
            }
            break;

        case SLASH:
            switch (ch) {
                case '\n':
                    foundCodeOnLine = 1;
                    HANDLE_NEWLINE;
                    state = START;
                    break;
                case '/':
                    codeStats->cplusplusCommentCount++;
                    state = CPPCOMMENT;
                    break;
                case '*':
                    codeStats->cCommentCount++;
                    state = CCOMMENT;
                    break;
                default:
                    foundCodeOnLine = 1;
                    break;
            }
            break;

        case CCOMMENT:
            switch (ch) {
                case '\n':
                    HANDLE_NEWLINE;
                    break;
                case '*':
                    state = CSTAR;
                    break;
                default:
                    break;
            }
            break;

        case CSTAR:
            switch (ch) {
                case '/':
                    state = START;
                    break;
                case '\n':
                    HANDLE_NEWLINE;
                    //fall through
                default:
                    state = CCOMMENT;
            }
            break;

        case CPPCOMMENT:
            switch (ch) {
                case '\n':
                    HANDLE_NEWLINE;
                    state = START;
                    break;
                default:
                    break;
            }
            break;

        case STRING:
            switch (ch) {
                case '"':
                    state = START;
                    break;
                case '\\':
                    state = SBACKSLASH;
                    break;
                default:
                    break;
            }
            break;

        case SBACKSLASH:
            if (ch == '\n') {
                HANDLE_NEWLINE;
                foundCodeOnLine = 1;
            }    
            state = STRING;
            break;
            
        case CHAR:
            switch (ch) {
                case '\'':
                    state = START;
                    break;
                case '\\':
                    state = CBACKSLASH;
                    break;
                default:
                    break;
            }
            break;

        case CBACKSLASH:
            if (ch == '\n') {
                HANDLE_NEWLINE;
                foundCodeOnLine = 1;
            }    
            state = CHAR;
            break;

        case PREPROCESS:
            switch (ch) {
                case '\n':
                    codeStats->lineCount++;
                    state = START;
                    break;
                case '\\':
                    state = PBACKSLASH;
                    break;
                default:
                    break;
            }
            break;

        case PBACKSLASH:
            if (ch == '\n') {
                codeStats->lineCount++;
            }    
            state = PREPROCESS;
            break;
                        
        default:
            assert(0);
            break;
        }
    }
    fclose(f);
    assert(state == START);
}

int main(int argc, char *argv[])
{
    struct CodeStats codeStats;
    int i;

    for (i = 1; i < argc; i++) {
        codeStats_init(&codeStats);
        codeStats_accumulate(&codeStats, argv[i]);
        codeStats_print(codeStats, argv[i]); // no "&" -- see why?
        if (i != argc-1)   // if this wasn't the last file ...
            printf("\n");  // ... print out a separating newline
    }
    return 0;
}
