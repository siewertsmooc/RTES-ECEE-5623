// The point of this C code is to use nothing built-in as a library at all, just
// the code you see here, which can be compiled into assembly code with no external
// dependencies outside of this file.  This is why it does not even use simple division, 
// which requires a math library or assembly utility function on some architectures.

// Find the greatest common factor between 2 numbers
//
// https://answers.everydaycalculation.com/gcf/

int gcfc(int a, int b)
{
    while(a != b) 
    {
        if(a > b)
            a = a - b;
        else
            b = b - a; 
    }

    return a;
}


// Divide two numbers and return resulting quotient ignoring (truncating)
// the remainder
//
// a is the dividend and b is the divisor in a/b
//
// This could be useful if your CPU does not have division built in and you do not
// want to use a pre-defined math library implementation of division.  It may not be the fastest, but it
// does work.

int divide(int a, int b)
{
    int r=a;
    int cnt=0;

    while(r >= b)
    {
        r=r-b;
	cnt=cnt+1;
    }

    return cnt;
}


// Find the remainder for the division of two numbers
//
int remain(int a, int b)
{
    int r=a;

    while(r >= b)
    {
        r=r-b;
    }

    return r;
}



// Find the least common multiple between 2 numbers
//
// https://answers.everydaycalculation.com/lcm/

int lcmc(int a, int b)
{
    int lcm;

    lcm = divide((a * b), (gcfc(a,b)));
    
    return lcm;
}
