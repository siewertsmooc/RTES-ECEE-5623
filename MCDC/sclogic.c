#include <stdio.h>


int function_A(void)
{
    static int toggle_A=0;

    printf("function_A\n");
    if(toggle_A == 0)
        toggle_A=1;
    else
        toggle_A=0;

    return toggle_A;
}

int function_B(void)
{
    static int toggle_B=0;

    printf("function_B\n");
    if(toggle_B == 0)
        toggle_B=1;
    else
        toggle_B=0;

    return toggle_B;
}

void function_C(void)
{
    printf("do function_C\n");
}

void function_D(void)
{
    printf("do function_D\n");
}


int main(void)
{
    int rc;

    // Test Case #1 - Call all functions
    rc=function_A();
    rc=function_B();
    function_C();
    function_D();

    // Test Case #2, Test use in logic
    if((rc=(function_A() && function_B())))
        function_C();
    else
        function_D();

    return(1);
}
