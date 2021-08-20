#include "lcmasmwrapper.h"


int main(void)
{
    int a=146, b=18, gcf=0, lcm=0, quot=0, rem=0;

    gcf=gcfa(a,b);
    printf("C gcf(%d, %d)=%d\n", a, b, gcf);

    quot=divide(a,b);
    rem=remain(a,b);
    printf("C div(%d, %d)=%d, rem=%d\n", a, b, quot, rem);

    quot=divide(a*b, gcfa(a,b));
    rem=remain(a*b, gcfa(a,b));
    printf("C div(%d, %d)=%d, rem=%d\n", a*b, gcfa(a,b), quot, rem);


    lcm=lcma(a,b);
    printf("C lcm(%d, %d)=%d\n", a, b, lcm);
}
