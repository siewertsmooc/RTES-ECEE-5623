/* 
 * This is a very minimal example of two processes in a sync'd
 * relationship.
 *
 * by Sam Siewert
 *
 */
#include	<stdio.h>
#include	<fcntl.h>
#include	<stdlib.h>
#include	<sys/types.h>
#include	<sys/wait.h>
#include	<string.h>
#include	<unistd.h>
#include	<sys/stat.h>
#include	<semaphore.h>

#define TRUE (1)
#define FALSE (0)


int main() 
{
    int chPID;		// Child PID
    int stat;		// Used by parent wait
    int rc;		// return code
    int i=0;
    pid_t thisChPID;
    sem_t *syncSemC, *syncSemP;
    char syncSemCName[]="/twoprocCsync";
    char syncSemPName[]="/twoprocPsync";

    printf("twprocs\n");

    // set up syncrhonizing semaphore before fork
    //
    syncSemC=sem_open(syncSemCName, O_CREAT, 0700, 0);
    printf("twprocs syncSemC done\n");

    syncSemP=sem_open(syncSemPName, O_CREAT, 0700, 0);
    printf("twprocs syncSemP done\n");
    
    printf("twprocs semaphores set up, calling fork\n");

    if((chPID = fork()) == 0) //  This is the child
    {
        while(i < 3) 
        {
            printf("Child: taking syncC semaphore\n");
            if(sem_wait(syncSemC) < 0) perror("sem_wait syncSemC Child");
            printf("Child: got syncC semaphore\n");

            printf("Child: posting syncP Parent semaphore\n");
            if(sem_post(syncSemP) < 0) perror("sem_post syncSemP Child");
            printf("Child: posted syncP Parent semaphore\n");

            i++;
        }

        printf("Child is closing down\n");
        if(sem_close(syncSemP) < 0) perror("sem_close syncSemP Child");
        if(sem_close(syncSemC) < 0) perror("sem_close syncSemC Child");
        exit(0);
    }

    else // This is the parent
    {
        while(i < 3) 
        {
            printf("Parent: posting syncC semaphore\n");
            if(sem_post(syncSemC) < 0) perror("sem_post syncSemC Parent");
            printf("Parent: posted syncC semaphore\n");

            printf("Parent: taking syncP semaphore\n");
            if(sem_wait(syncSemP) < 0) perror("sem_wait syncSemP Parent");
            printf("Parent: got syncP semaphore\n");

            i++;
        }

        // Now wait for the child to terminate
        printf("Parent waiting on child to terminate\n");

        thisChPID = wait(&stat);
            
        printf("Parent is closing down\n");
        if(sem_close(syncSemC) < 0) perror("sem_close syncSemC Parent");
        if(sem_close(syncSemP) < 0) perror("sem_close syncSemC Parent");

        if(sem_unlink(syncSemCName) < 0) perror("sem_unlink syncSemCName");
        if(sem_unlink(syncSemPName) < 0) perror("sem_unlink syncSemPName");

        exit(0);

    }

}
