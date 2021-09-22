#include "stdio.h"

#ifndef OK
#define OK 1
#endif

#if defined(VXWORKS)
#include "excLib.h"

void myExcHook(int taskID, int excVec, void *excStackFrame)
{
	logMsg("Trapped an Exception for task 0x%x\n", taskID);
}
#endif


int someInt;
int *badPtr = (int *)0xffffffff;
int *goodPtr = &someInt;

int diverror(int arg)
{
    int someNum;
    someNum = 1/arg;
    return OK;
}

int buserror(int *badPtr)
{
    int someData;
    someData = *badPtr;
    return OK;
}

#if defined(VXWORKS)
int testExc(void)
#else
int main(void)
#endif
{
    char selection;

#if defined(VXWORKS)
    excHookAdd((FUNCPTR)myExcHook);
#endif
	
    printf("choose and exception [b=bus error, d=divide by zero]:");
    selection = getchar();

    if(selection == 'b')
    {
        printf("Generating bus error segfault exception\n");
        buserror(badPtr);
    }
    else
    {
        printf("Generating divide by zero exception\n");
        diverror(0);
    }
}
