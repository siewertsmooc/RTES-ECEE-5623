/*******************************************************************

  Unix serial interface test shell

  Sam Siewert

********************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "serialutil.h"

int main(int argc, char** argv)
{
   int type = 0;

   if(argc < 2)
   {
     printf("Usage: serial_test </dev/serial>\n");
     exit(-1);
   }

   /* assume third argument is session type if present and ignore any others */
   else if(argc > 2)
   {
     sscanf(argv[2], "%d", &type);
   }

   initialize_serial_device(argv[1]);
   initiate_serial_session(argv[0], type);

}
