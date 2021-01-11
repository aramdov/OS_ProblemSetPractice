#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

///////////////////////////////////////////////////////////////////////////
// Problem 2 from Problem set 1 -> Priority Readers and Writers Example

// Readers have priority over writers concerning a shared global variable
    // If any readers are waiting -> they have priority over writer threads
    // Writers can only write when there are NO readers
    // Also adhere to the following constraints
        // multiple readers/writers supported (5 of each is fine)
        // Readers must read shared variable X number of time
        // Writers mmust write the shared variable X number of times
        
        // Readers must print, 1. Value Read and 2. # of readers present when value is read
        // Writers must print, 1. The written value. 2. # of writers when written should be 0

        /* Before reader/writer attempts to access variable it should wait some random
            amount of time to ensure they don't occur all at once*/
            //Use pthreads, mutexes and condition variables to synchronize

    // compiling -> #include pthread.h
    // gcc -o main main.c -pthread
///////////////////////////////////////////////////////////////////////////

// Constants
#define NUM_THREADS 5


// GLOBAL variables
int SHARED_GLOBAL = 2; 
int resource_counter = 0;
int num_wait_readers = 0;

// mutex for a proxy variable that gets updated
// resource_counter with encoding of 0, >0, -1

pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;  	/* mutex lock for buffer */

pthread_cond_t cond_read = PTHREAD_COND_INITIALIZER; /* reader waits on this cond var */
pthread_cond_t cond_write = PTHREAD_COND_INITIALIZER; /* writer waits on this cond var */

void *reader (void *param);
void *writer (void *param);
// void functions we fill in details later


// Main function, create threads and join them here
int main(int argc, char *argv[]) {


	pthread_t readerid[NUM_THREADS];  /* thread identifiers in array of threads*/
    pthread_t writerid[NUM_THREADS]; // 5 of each type of thread
	int i, j, jj;
    int tNum[NUM_THREADS]; // to avoid overwrites to i before 1 thread fetches n uses

	/* create the threads; may be any number, in general */

    // loop to create 5 writer threads
    for (j=0; j < NUM_THREADS; j++) {
        
        if (pthread_create(&writerid[j], NULL, writer, &tNum[j])) {
            fprintf(stderr, "Unable to create writer thread\n");
            exit(1);
        }
    }

    // loop to create 5 reader threads
    for (jj=0; jj < NUM_THREADS; jj++) {
        
        if (pthread_create(&readerid[jj], NULL, reader, &tNum[jj])) {
            fprintf(stderr, "Unable to create reader thread\n");
            exit(1);
        }
    }

	/* wait for created thread to exit */
	// since we used default behavior for pthread_create, we have to join them
		// in the main function.
    for(i = 0; i < NUM_THREADS; i++) { /* wait/join threads. Both in each loop? */ 
		pthread_join(readerid[i], NULL);
	}

    for(i=0; i < NUM_THREADS; i++) {
        pthread_join(writerid[i], NULL);
    }
    // separate loops or combine them into one loops for joins on different types?
    return 0;
}

// Declare function definitions for reader & writer down here

// Delay function to avoid data race, before accessing mutex/lock
void delay (int number_of_seconds) {

    // convert time into ms
    int milli_seconds = 1000 * number_of_seconds;

    // start time rigt now
    clock_t start_time = clock();

    // loop till required time is not achived
    while (clock() < start_time + milli_seconds);
}

/* Writer value(s) */
void *writer(void *param) {

	int i;
	for (i=1; i<=10; i++) {
		
		/* access shared "file" and update random with i iterator as seed# */
        // wait a random amount of time to avoid DATA RACE
            // Where should Delay function go?
                // Before Mutex_Lock?
                // After Mutex unlock?

        delay( rand()%5 ); // wait random time between 0 and 4 seconds

		pthread_mutex_lock (&counter_mutex);
		// start of mutex lock

			while (resource_counter != 0) {  
				pthread_cond_wait (&cond_write, &counter_mutex); 
                }
				// continuously wait until signal comes or broadcast
				// wait on condition variable for producers (cond_write)
				// pass mutex as argument into the wait call, so pthread lib
				// knows what particular mutex is to be freed, use pointer to it

            // while loop ends, then update resource counter as writer is accessing
            resource_counter = -1;
        // end of mutex block by unlocking explicitly
		pthread_mutex_unlock (&counter_mutex);
		

        // use i to seed random update to the shared "file", here its just int var
        int written_value = rand() % i;
        SHARED_GLOBAL = written_value; // any problems?
        printf ("Writer: inserted and updated %d\n", SHARED_GLOBAL);

        // lock to change the resource counter global shared variable
        pthread_mutex_lock (&counter_mutex);

            // after write, we set the resource counter back to 0 and broadcast to threads
                /* that are waiting (reader threads), and signal to writer condition
                but we don't know who gets first access
                How can we create bias/priority to readers over writers here? */
            resource_counter = 0;

            // if readers are waiting, they have priority over writer threads
            if (num_wait_readers > 0) {
                pthread_cond_broadcast (&cond_read);    
            }
            else {
                pthread_cond_signal (&cond_write);    
            }
            // Alternative implicit priority over readers method below
                //first call broadcast means implicit readers have priority over writers
                //but how to have writers only get a signal if there are no readers?

        pthread_mutex_unlock(&counter_mutex);
		// inserted 1 element, no point to wake up multiple consumer threads with broadcast
		// buffer was empty when we performed this insert operation
		// so we signal a thread thats waiting on the condition variable for (c_cons)

	}
	printf("Writer quiting\n");
    pthread_exit(0);
}

/* reader function and printing values */
void *reader(void *param) {

	int i;
    // Reader will read X number of times (not while(1) like other implementations)
	for (i=1; i<=10; i++) {
		
		/* access shared "file" and update random with i iterator as seed# */
        // wait a random amount of time to avoid DATA RACE
            // Where should Delay function go?
                // Before Mutex_Lock?
                // After Mutex unlock?

        delay( rand()%5 ); // wait random time between 0 and 4 seconds

		pthread_mutex_lock (&counter_mutex);
		// start of mutex lock
            num_wait_readers += 1;
			while (resource_counter == -1) {  
				pthread_cond_wait (&cond_read, &counter_mutex); 
                }

            // while loop ends, then update resource counter as writer is accessing
            resource_counter += 1;
            num_wait_readers -= 1;
        // end of mutex block by unlocking explicitly
		pthread_mutex_unlock (&counter_mutex);
		
        int read_value = SHARED_GLOBAL;
	    fprintf(stdout, "reading %u  [readers: %2d]\n", read_value, resource_counter);

        // lock to change the resource counter global shared variable
        pthread_mutex_lock (&counter_mutex);

            resource_counter -= 1;
            // no broadcast -> pthread_cond_broadcast (&cond_read);
            // if no readers, then writer gets a signal
            if (resource_counter == 0) {
                pthread_cond_signal (&cond_write);
            }
        pthread_mutex_unlock(&counter_mutex);
		// inserted 1 element, no point to wake up multiple consumer threads with broadcast
		// buffer was empty when we performed this insert operation
		// so we signal a thread thats waiting on the condition variable for (c_cons)
	}
    printf("Reader quiting\n");
    pthread_exit(0);
}
