#!/bin/csh -f
#
# RAID Unit level regression script
#
echo "RAID UNIT REGRESSSION TEST"
echo "RAID UNIT REGRESSSION TEST" >& testresults.log
echo "" >>& testresults.log


echo "TEST SET 1: checkout and build test"
echo "TEST SET 1: checkout and build test" >>& testresults.log
make clean >>& testresults.log
git pull >>& testresults.log
make >>& testresults.log
echo "" >>& testresults.log


echo "TEST SET 2: simple RAID encode, erase and rebuild test"
echo "TEST SET 2: simple RAID encode, erase and rebuild test" >>& testresults.log
./raidtest 1000 >>& testresults.log
echo "" >>& testresults.log


echo "TEST SET 3: REBUILD DIFF CHECK for Chunk4 rebuild"
echo "TEST SET 3: REBUILD DIFF CHECK for Chunk4 rebuild" >>& testresults.log
diff Chunk1.bin Chunk4_Rebuilt.bin >>& testresults.log
diff Chunk2.bin Chunk4_Rebuilt.bin >>& testresults.log
diff Chunk3.bin Chunk4_Rebuilt.bin >>& testresults.log
diff Chunk4.bin Chunk4_Rebuilt.bin >>& testresults.log
echo "" >>& testresults.log


echo "TEST SET 4: RAID performance test"
echo "TEST SET 4: RAID performance test" >>& testresults.log
./raid_perftest 1000 >>& testresults.log
echo "" >>& testresults.log
