#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <assert.h>

#include "raidlib.h"

#ifdef RAID64
#include "raidlib64.h"
#define PTR_CAST (unsigned long long *)
#else
#include "raidlib.h"
#define PTR_CAST (unsigned char *)
#endif


// RAID-5 encoding
//
// This is 80% capacity with 1/5 LBAs used for parity.
//
// Only handles single faults.
//
// PRECONDITIONS:
// 1) LBA pointeres must have memory allocated for them externally
// 2) Blocks pointer to by LBAs are initialized with data
//
// POST-CONDITIONS:
// 1) Contents of PLBA is modified and contains the computed parity using XOR
//
void xorLBA(unsigned char *LBA1,
	    unsigned char *LBA2,
	    unsigned char *LBA3,
	    unsigned char *LBA4,
	    unsigned char *PLBA)
{
    int idx;

    for(idx=0; idx<SECTOR_SIZE; idx++)
        *(PLBA+idx)=(*(LBA1+idx))^(*(LBA2+idx))^(*(LBA3+idx))^(*(LBA4+idx));
}


// RAID-5 Rebuild
//
// Provide any 3 of the original LBAs and the Parity LBA to rebuild the RLBA
//
// If the Parity LBA was lost, then it can be rebuilt by simply re-encoding.
// 
void rebuildLBA(unsigned char *LBA1,
	        unsigned char *LBA2,
	        unsigned char *LBA3,
	        unsigned char *PLBA,
	        unsigned char *RLBA)
{
    int idx;
    unsigned char checkParity;

    for(idx=0; idx<SECTOR_SIZE; idx++)
    {
        // Parity check word is simply XOR of remaining good LBAs
        checkParity=(*(LBA1+idx))^(*(LBA2+idx))^(*(LBA3+idx));

        // Rebuilt LBA is simply XOR of original parity and parity check word
        // which will preserve original parity computed over the 4 LBAs
        *(RLBA+idx) =(*(PLBA+idx))^(checkParity);
    }
}


int checkEquivLBA(unsigned char *LBA1,
		  unsigned char *LBA2)
{
    int idx;

    for(idx=0; idx<SECTOR_SIZE; idx++)
    {
        if((*(LBA1+idx)) != (*(LBA2+idx)))
	    {
            printf("EQUIV CHECK MISMATCH @ byte %d: LBA1=0x%x, LBA2=0x%x\n", idx, (*LBA1+idx), (*LBA2+idx));
	        return ERROR;
	    }
    }

    return OK;
}


// returns bytes written or ERROR code
// 
int stripeFile(char *inputFileName, int offsetSectors)
{
    int fd[5], idx;
    FILE *fdin;
    unsigned char stripe[5*512];
    int offset=0, bread=0, btoread=(4*512), bwritten=0, btowrite=(512), sectorCnt=0, byteCnt=0;

    fdin = fopen(inputFileName, "r");
    fd[0] = open("StripeChunk1.bin", O_RDWR | O_CREAT, 00644);
    fd[1] = open("StripeChunk2.bin", O_RDWR | O_CREAT, 00644);
    fd[2] = open("StripeChunk3.bin", O_RDWR | O_CREAT, 00644);
    fd[3] = open("StripeChunk4.bin", O_RDWR | O_CREAT, 00644);
    fd[4] = open("StripeChunkXOR.bin", O_RDWR | O_CREAT, 00644);


    do
    {

        // read a stripe or to end of file
        offset=0, bread=0, btoread=(4*512);
        do
        {
            bread=fread(&stripe[offset], 1, btoread, fdin); 
            offset+=bread;
            btoread=(4*512)-bread;
        }
        while (!(feof(fdin)) && (btoread > 0));


        if((offset < (4*512)) && (feof(fdin)))
        {
            printf("hit end of file\n");
            bzero(&stripe[offset], btoread);
            byteCnt+=offset;
        }
        else
        {
            //printf("read full stripe\n");
            assert(offset == (4*512));
            byteCnt+=(4*512);
        };

        // computer xor code for stripe
        //
        xorLBA(PTR_CAST &stripe[0],
               PTR_CAST &stripe[512],
               PTR_CAST &stripe[1024],
               PTR_CAST &stripe[1536],
               PTR_CAST &stripe[2048]);


        // write out the stripe + xor code
        //
        offset=0, bwritten=0, btowrite=(512);
        do
        {
            bwritten=write(fd[0], &stripe[offset], 512); 
            offset+=bwritten;
            btowrite=(512)-bwritten;
        }
        while (btowrite > 0);

        offset=512, bwritten=0, btowrite=(512);
        do
        {
            bwritten=write(fd[1], &stripe[offset], 512); 
            offset+=bwritten;
            btowrite=(512)-bwritten;
        }
        while (btowrite > 0);

        offset=1024, bwritten=0, btowrite=(512);
        do
        {
            bwritten=write(fd[2], &stripe[offset], 512); 
            offset+=bwritten;
            btowrite=(512)-bwritten;
        }
        while (btowrite > 0);

        offset=1536, bwritten=0, btowrite=(512);
        do
        {
            bwritten=write(fd[3], &stripe[offset], 512); 
            offset+=bwritten;
            btowrite=(512)-bwritten;
        }
        while (btowrite > 0);

        offset=2048, bwritten=0, btowrite=(512);
        do
        {
            bwritten=write(fd[4], &stripe[offset], 512); 
            offset+=bwritten;
            btowrite=(512)-bwritten;
        }
        while (btowrite > 0);

        sectorCnt+=4;

    }
    while (!(feof(fdin)));

    fclose(fdin);
    for(idx=0; idx < 5; idx++) close(fd[idx]);

    return(byteCnt);
}


// returns bytes read or ERROR code
// 
// missingChunk = 0 for no missing
//              = 1 ... 4 for missing data chunk
//              = 5 for missing XOR chunk
//
// Ugly repeated code needs to be refactored, but works for now.
//
int restoreFile(char *outputFileName, int offsetSectors, int fileLength, int missingChunk)
{
    int fd[5], idx;
    FILE *fdout;
    unsigned char stripe[5*512];
    int offset=0, bread=0, btoread=(4*512), bwritten=0, btowrite=(512), sectorCnt=fileLength/512;
    int stripeCnt=fileLength/(4*512);
    int lastStripeBytes = fileLength % (4*512);

    fdout = fopen(outputFileName, "w");

    fd[0] = open("StripeChunk1.bin", O_RDWR | O_CREAT, 00644);
    fd[1] = open("StripeChunk2.bin", O_RDWR | O_CREAT, 00644);
    fd[2] = open("StripeChunk3.bin", O_RDWR | O_CREAT, 00644);
    fd[3] = open("StripeChunk4.bin", O_RDWR | O_CREAT, 00644);
    fd[4] = open("StripeChunkXOR.bin", O_RDWR | O_CREAT, 00644);


    for(idx=0; idx < stripeCnt; idx++)
    {
        // read in the stripe + xor code

        if(missingChunk == 1)
	{
		printf("will rebuild chunk 1\n");
	}
	else
	{
            offset=0, bread=0, btoread=(512);
            do
            {
                bread=read(fd[0], &stripe[offset], 512); 
                offset+=bread;
                btoread=(512)-bread;
            }
            while (btoread > 0);
	}


        if(missingChunk == 2)
	{
		printf("will rebuild chunk 2\n");
	}
	else
	{
            offset=512, bread=0, btoread=(512);
            do
            {
                bread=read(fd[1], &stripe[offset], 512); 
                offset+=bread;
                btoread=(512)-bread;
            }
            while (btoread > 0);
	}


        if(missingChunk == 3)
	{
		printf("will rebuild chunk 3\n");
	}
	else
	{
            offset=1024, bread=0, btoread=(512);
            do
            {
                bread=read(fd[2], &stripe[offset], 512); 
                offset+=bread;
                btoread=(512)-bread;
            }
            while (btoread > 0);
	}

        if(missingChunk == 4)
	{
		printf("will rebuild chunk 4\n");
	}
	else
	{
            offset=1536, bread=0, btoread=(512);
            do
            {
                bread=read(fd[3], &stripe[offset], 512); 
                offset+=bread;
                btoread=(512)-bread;
            }
            while (btoread > 0);
	}

        if(missingChunk == 5)
	{
		printf("will rebuild chunk 5\n");
	}
	else
	{
            offset=2048, bread=0, btoread=(512);
            do
            {
                bread=read(fd[4], &stripe[offset], 512); 
                offset+=bread;
                btoread=(512)-bread;
            }
            while (btoread > 0);
	}

        // OPTION - check XOR here or re-compute it
        //
	if(missingChunk == 1)
	{
		rebuildLBA( PTR_CAST &stripe[512], 
			    PTR_CAST &stripe[1024], 
			    PTR_CAST &stripe[1536], 
			    PTR_CAST &stripe[2048], 
			    PTR_CAST &stripe[0]);
	}

	if(missingChunk == 2)
	{
		rebuildLBA( PTR_CAST &stripe[0], 
		  	    PTR_CAST &stripe[1024], 
			    PTR_CAST &stripe[1536], 
			    PTR_CAST &stripe[2048], 
			    PTR_CAST &stripe[512]);
	}
	if(missingChunk == 3)
	{
		rebuildLBA(
			   PTR_CAST &stripe[0],
		           PTR_CAST &stripe[512],
		           PTR_CAST &stripe[1536],
		           PTR_CAST &stripe[2048],
		           PTR_CAST &stripe[1024]
			  );
	}
	if(missingChunk == 4)
	{
		rebuildLBA(
			   PTR_CAST &stripe[0],
		           PTR_CAST &stripe[512],
		           PTR_CAST &stripe[1024],
		           PTR_CAST &stripe[2048],
		           PTR_CAST &stripe[1536]
			  );
	}
	if(missingChunk == 5)
	{
		rebuildLBA(
			   PTR_CAST &stripe[0],
		           PTR_CAST &stripe[512],
		           PTR_CAST &stripe[1024],
		           PTR_CAST &stripe[1536],
		           PTR_CAST &stripe[2048]
			  );
	}
	

       // write a full stripe
        offset=0, bwritten=0, btowrite=(4*512);

        do
        {
            bwritten=fwrite(&stripe[offset], 1, btowrite, fdout); 
            offset+=bwritten;
            btowrite=(4*512)-bwritten;
        }
        while ((btowrite > 0));

    }


    if(lastStripeBytes)
    {
        // read in the parital stripe + xor code
        //
        if(missingChunk == 1)
	{
		printf("will rebuild chunk 1\n");
	}
	else
	{
            offset=0, bread=0, btoread=(512);
            do
            {
                bread=read(fd[0], &stripe[offset], 512); 
                offset+=bread;
                btoread=(512)-bread;
            }
            while (btoread > 0);
	}

        if(missingChunk == 2)
	{
		printf("will rebuild chunk 2\n");
	}
	else
	{
            offset=512, bread=0, btoread=(512);
            do
            {
                bread=read(fd[1], &stripe[offset], 512); 
                offset+=bread;
                btoread=(512)-bread;
            }
            while (btoread > 0);
	}

        if(missingChunk == 3)
	{
		printf("will rebuild chunk 3\n");
	}
	else
	{
            offset=1024, bread=0, btoread=(512);
            do
            {
                bread=read(fd[2], &stripe[offset], 512); 
                offset+=bread;
                btoread=(512)-bread;
            }
            while (btoread > 0);
	}

        if(missingChunk == 4)
	{
		printf("will rebuild chunk 4\n");
	}
	else
	{
            offset=1536, bread=0, btoread=(512);
            do
            {
                bread=read(fd[3], &stripe[offset], 512); 
                offset+=bread;
                btoread=(512)-bread;
            }
            while (btoread > 0);
	}


        if(missingChunk == 5)
	{
		printf("will rebuild chunk 5\n");
	}
	else
	{
            offset=2048, bread=0, btoread=(512);
            do
            {
                bread=read(fd[4], &stripe[offset], 512); 
                offset+=bread;
                btoread=(512)-bread;
            }
            while (btoread > 0);
	}

        // OPTION - check XOR here
        //
	if(missingChunk == 1)
	{
		rebuildLBA( PTR_CAST &stripe[512], 
			    PTR_CAST &stripe[1024], 
			    PTR_CAST &stripe[1536], 
			    PTR_CAST &stripe[2048], 
			    PTR_CAST &stripe[0]);
	}

	if(missingChunk == 2)
	{
		rebuildLBA( PTR_CAST &stripe[0], 
		  	    PTR_CAST &stripe[1024], 
			    PTR_CAST &stripe[1536], 
			    PTR_CAST &stripe[2048], 
			    PTR_CAST &stripe[512]);
	}
	if(missingChunk == 3)
	{
		rebuildLBA(
			   PTR_CAST &stripe[0],
		           PTR_CAST &stripe[512],
		           PTR_CAST &stripe[1536],
		           PTR_CAST &stripe[2048],
		           PTR_CAST &stripe[1024]
			  );
	}
	if(missingChunk == 4)
	{
		rebuildLBA(
			   PTR_CAST &stripe[0],
		           PTR_CAST &stripe[512],
		           PTR_CAST &stripe[1024],
		           PTR_CAST &stripe[2048],
		           PTR_CAST &stripe[1536]
			  );
	}
	if(missingChunk == 5)
	{
		rebuildLBA(
			   PTR_CAST &stripe[0],
		           PTR_CAST &stripe[512],
		           PTR_CAST &stripe[1024],
		           PTR_CAST &stripe[1536],
		           PTR_CAST &stripe[2048]
			  );
	}

       // write a partial stripe
        offset=0, bwritten=0, btowrite=(lastStripeBytes);

        do
        {
            bwritten=fwrite(&stripe[offset], 1, btowrite, fdout); 
            offset+=bwritten;
            btowrite=lastStripeBytes-bwritten;
        }
        while ((btowrite > 0));
    }


    fclose(fdout);
    for(idx=0; idx < 5; idx++) close(fd[idx]);

    return(fileLength);
}

