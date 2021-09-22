#include "ioLib.h"
#include "stdio.h"

static int sout;
static int sin;
static int serr;
static int vf0, vf1, vf2;

void setout(void)
{

  sin = ioGlobalStdGet(0);
  sout = ioGlobalStdGet(1);
  serr = ioGlobalStdGet(2);

  /* set up input to be interlaced with shell */
  if((vf0 = open("/vio/0", O_RDWR, 0)) == ERROR) {
    printf("Error opening virtual channel output\n");
  }
  else {
    ioGlobalStdSet(0,vf0);
    ioGlobalStdSet(1,vf0);
    ioGlobalStdSet(2,vf0);
  }

  logFdSet(1);

  printf("Original setup: sin=%d, sout=%d, serr=%d\n", sin, sout, serr);
  printf("All being remapped to your virtual terminal...\n");
  printf("You should see this message now!!!\n");
  logMsg("You should also see this logMsg\n",0,0,0,0,0,0);

}

void restore(void)
{

  close(vf0);
  close(vf1);
  close(vf2);

  ioGlobalStdSet(0,sin);
  ioGlobalStdSet(1,sout);
  ioGlobalStdSet(2,serr);

  printf("Original setup: sin=%d, sout=%d, serr=%d\n", vf0, vf1, vf2);
  printf("All being remapped to target terminal...\n");
  printf("You should see this message now on target terminal!!!\n");
}
