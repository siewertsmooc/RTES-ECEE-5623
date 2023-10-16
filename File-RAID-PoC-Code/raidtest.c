#include "raidtest.h"



void modifyBuffer(unsigned char *bufferToModify, int offset)
{
    int idx;

    for(idx=0; idx < SECTOR_SIZE; idx++)
        bufferToModify[idx] = (bufferToModify[idx]+offset) % 100;
}


void printBuffer(char *bufferToPrint)
{
    int idx;

    for(idx=0; idx < SECTOR_SIZE; idx++)
        printf("%c ", bufferToPrint[idx]);

    printf("\n");
}


void dumpBuffer(unsigned char *bufferToDump)
{
    int idx;

    for(idx=0; idx < SECTOR_SIZE; idx++)
        printf("%x ", bufferToDump[idx]);

    printf("\n");
}



int main(int argc, char *argv[])
{
	int idx, LBAidx, numTestIterations, rc;
        int written=0, fd[5];
        int fdrebuild;
	double rate=0.0;
	double totalRate=0.0, aveRate=0.0;
	struct timeval StartTime, StopTime;
	unsigned int microsecs;


        if(argc < 2)
	{
		numTestIterations=TEST_ITERATIONS;
		printf("Will default to %d iterations\n", TEST_ITERATIONS);
        }
        else
        {
            sscanf(argv[1], "%d", &numTestIterations);
       	    printf("Will start %d test iterations\n", numTestIterations);
        }


        // Set all test buffers
	for(idx=0;idx<MAX_LBAS;idx++)
        {
            memcpy(&testLBA1[idx], TEST_RAID_STRING, SECTOR_SIZE);
            memcpy(&testLBA2[idx], TEST_RAID_STRING, SECTOR_SIZE);
            memcpy(&testLBA3[idx], TEST_RAID_STRING, SECTOR_SIZE);
            memcpy(&testLBA4[idx], TEST_RAID_STRING, SECTOR_SIZE);
            memcpy(&testRebuild[idx], NULL_RAID_STRING, SECTOR_SIZE);
        }
            

        // TEST CASE #1
        //
        printf("Architecture validation:\n");
        printf("sizeof(unsigned long long)=%ld\n", sizeof(unsigned long long));
        printf("\n");

        printf("TEST CASE 1:\n");
	// Compute XOR from 4 LBAs for RAID-5
	//
        modifyBuffer(&(testLBA1[0][0]), 7);
        modifyBuffer(&(testLBA2[0][0]), 11);
        modifyBuffer(&(testLBA3[0][0]), 13);
        modifyBuffer(&(testLBA4[0][0]), 23);

        xorLBA(PTR_CAST &testLBA1[0],
	       PTR_CAST &testLBA2[0],
	       PTR_CAST &testLBA3[0],
    	       PTR_CAST &testLBA4[0],
	       PTR_CAST &testPLBA[0]);

	// Now rebuild LBA into test to verify
        rebuildLBA(PTR_CAST &testLBA1[0],
	           PTR_CAST &testLBA2[0],
	           PTR_CAST &testLBA3[0],
	           PTR_CAST &testPLBA[0],
	           PTR_CAST &testRebuild[0]);

        dumpBuffer((char *)&testLBA4[0]);
        printf("\n");
        //getchar();

        dumpBuffer((char *)&testRebuild[0]);
        printf("\n");
        //getchar();

        assert(memcmp(testRebuild, testLBA4, SECTOR_SIZE) ==0);

        // Adding TEST CASE #0 Feature to dump binary files with each 512byte LBA
        // so I can do a "diff" on them, dump with od -x 2, etc.
        //
        fd[0] = open("Chunk1.bin", O_RDWR | O_CREAT, 00644);
        fd[1] = open("Chunk2.bin", O_RDWR | O_CREAT, 00644);
        fd[2] = open("Chunk3.bin", O_RDWR | O_CREAT, 00644);
        fd[3] = open("Chunk4.bin", O_RDWR | O_CREAT, 00644);
        fd[4] = open("ChunkXOR.bin", O_RDWR | O_CREAT, 00644);

        written=write(fd[0], &testLBA1[0], SECTOR_SIZE);
        assert(written == SECTOR_SIZE);
        written=write(fd[1], &testLBA2[0], SECTOR_SIZE);
        assert(written == SECTOR_SIZE);
        written=write(fd[2], &testLBA3[0], SECTOR_SIZE);
        assert(written == SECTOR_SIZE);
        written=write(fd[3], &testLBA4[0], SECTOR_SIZE);
        assert(written == SECTOR_SIZE);
        written=write(fd[4], &testPLBA[0], SECTOR_SIZE);
        assert(written == SECTOR_SIZE);
     
        for(idx=0; idx < 5; idx++) close(fd[idx]); 


        // Now, do the same for the rebult 4th chunk
        fdrebuild = open("Chunk4_Rebuilt.bin", O_RDWR | O_CREAT, 00644);
        written=write(fdrebuild, &testRebuild[0], SECTOR_SIZE);
        assert(written == SECTOR_SIZE);
        close(fdrebuild);



        // TEST CASE #2
        //
        // Verify that the rebuild actually works and passes a data compare
        // for N unique cases with different data.
        //

        printf("TEST CASE 2 (randomized sectors and rebuild):\n");

	for(idx=0;idx<numTestIterations; idx++)
	{
            LBAidx = idx % MAX_LBAS;
            printf("%d ", LBAidx);

	    // Compute XOR from 4 LBAs for RAID-5
            xorLBA(PTR_CAST &testLBA1[LBAidx],
	           PTR_CAST &testLBA2[LBAidx],
	           PTR_CAST &testLBA3[LBAidx],
    	           PTR_CAST &testLBA4[LBAidx],
	           PTR_CAST &testPLBA[LBAidx]);

	    // Now rebuild LBA into test to verify
            rebuildLBA(PTR_CAST &testLBA1[LBAidx],
	               PTR_CAST &testLBA2[LBAidx],
	               PTR_CAST &testLBA3[LBAidx],
	               PTR_CAST &testPLBA[LBAidx],
	               PTR_CAST &testRebuild[LBAidx]);


            // Compare test to see if testRebuild is in fact the same as testLBA4
            //
            // check for miscompare
            assert(memcmp(&testRebuild[LBAidx], &testLBA4[LBAidx], SECTOR_SIZE) ==0);
            //printf("%d ", idx);


            // For the next case modify the contents of testLBA4 each time
            //
            modifyBuffer(&(testLBA4[LBAidx][0]), 17);

	}
        printf("\n");

        //
        // END TEST CASE #2

        printf("FINISHED\n");

        
}
