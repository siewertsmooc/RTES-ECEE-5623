#include "raidtest.h"



int main(int argc, char *argv[])
{
	int idx, LBAidx, numTestIterations, rc;
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
        //
        // This test case computes the XOR parity from 4 test buffers (512 bytes each) and then
        // it rebuilds a missing buffer (buffer 4 in the test case) and we simple time how long this
        // takes.
        //
	printf("\nRAID Operations Performance Test\n");

	gettimeofday(&StartTime, 0);

	for(idx=0;idx<numTestIterations;idx++)
	{
            LBAidx = idx % MAX_LBAS;

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
	}

	gettimeofday(&StopTime, 0);

        microsecs=((StopTime.tv_sec - StartTime.tv_sec)*1000000);

	if(StopTime.tv_usec > StartTime.tv_usec)
		microsecs+=(StopTime.tv_usec - StartTime.tv_usec);
	else
		microsecs-=(StartTime.tv_usec - StopTime.tv_usec);

	printf("Test Done in %u microsecs for %d iterations\n", microsecs, numTestIterations);

	rate=((double)numTestIterations)/(((double)microsecs)/1000000.0);
	printf("%lf RAID ops computed per second\n", rate);
        //
        // END TEST CASE #1


}
