/* Name: Samkit Jain
*/
# include  <signal.h>
# include <stdio.h>
# include <sys/types.h>
# include <sys/fcntl.h>
# include <sys/shm.h>
# include <sys/ipc.h>
# include <unistd.h>
# include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define BUFF_SIZE 1024


struct smbuffer{
	char buffer[BUFF_SIZE];
	int emptyspace;
	int occupancy;
	
	pthread_mutex_t mutex;
	pthread_cond_t spaceAvailable;
	pthread_cond_t itemAvailable;
};

// Structure for parameters to function executed by thread
struct param{
	struct smbuffer *ptr;
};


void *threadfn(void *arg){
	struct param *myparam = (struct param *)malloc(sizeof(struct param));
	myparam = (struct param *)arg;
}



int main( int argc, char* argv[] ) {
	/* Child related attributes */
	pid_t conid,prodid;
	int constatus,prodstatus;
	int conretval,prodretval;

	/* Shared memory initialization */
	int sm_id;
	struct smbuffer *sm_ptr;
	key_t key = 1100;
	int size = BUFF_SIZE;


	if((sm_id = shmget(key , size * sizeof(char) , 0666|IPC_CREAT))== -1){	// Allocate shared memory

		perror("SHMGET Failed : \n");
		exit(1);
	}

	if((sm_ptr = shmat(sm_id,NULL,0))==(void *)-1){	// Attach shared memory to your address space

		perror("SHMAT Failed : \n");
		exit(1);
	}

	/* Synchonization Primitive Initialization and shared data initialization */
	sm_ptr->emptyspace = BUFF_SIZE;
	sm_ptr->occupancy = 0;

	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
	pthread_mutexattr_setpshared(&mutexattr,PTHREAD_PROCESS_SHARED);// PTHREAD_PROCESS_SHARED makes mutex shared among all processes and their threads
									// PTHREAD_PROCESS_PRIVATE make mutex available to process and only its thread
	pthread_mutex_init(&sm_ptr->mutex,&mutexattr);

	pthread_condattr_t condattr;
	pthread_condattr_init(&condattr);
	pthread_condattr_setpshared(&condattr,PTHREAD_PROCESS_SHARED);
	pthread_cond_init(&sm_ptr->spaceAvailable,&condattr);
	pthread_cond_init(&sm_ptr->itemAvailable,&condattr);


	if((prodid = fork())== -1){
		perror("Fork Failed :\n");
	}

	if(prodid==0){
		/* Child code */
		struct smbuffer *prod_sm_ptr; 
		int prod_sm_id ;
		if((prod_sm_id = shmget(key,size*sizeof(char),0))==-1){	// Allocate shared memory
			perror("Prod shmget failed :\n");
			exit(2);
		}	
		if((prod_sm_ptr = shmat(prod_sm_id,NULL,0)) == (void *)-1){		// Attach shared memory to your address space
			perror("Prod shmat failed :\n");
			exit(2);
		}
		
		/* Create a thread */
		pthread_t prod;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr , PTHREAD_CREATE_JOINABLE);
		pthread_attr_setscope(&attr,PTHREAD_SCOPE_SYSTEM);
		
		/* Struct for parameters to prod func */
		struct param prodparam;
		prodparam.ptr = prod_sm_ptr;

		int ret;
		if((ret=pthread_create(&prod,&attr,threadfn,(void *) &prodparam))){
			fprintf(stderr,"pthread_create ERROR : %s\n",strerror(ret));
			exit(1);
		}
		/* Wait for all threads to finish */	
		pthread_join(prod,NULL);	


		if(shmdt(prod_sm_ptr)==-1){			// De-attach shared memory
			perror("Prod shmdt failed :\n");
			exit(2);
		}
		
		printf("Prod exit\n");
		return 0;
	}

	else{
		/* Parent code */
		if((conid = fork())== -1){
			perror("Fork Failed :\n");
		}

		if(conid==0){
			/* Child code */
			struct smbuffer *con_sm_ptr; 
			int con_sm_id ;
			if((con_sm_id = shmget(key,size*sizeof(char),0))==-1){	// Allocate shared memory
				perror("Consumer shmget failed :\n");
				exit(2);
			}	
			if((con_sm_ptr = shmat(con_sm_id,NULL,0)) == (void *)-1){		// Attach shared memory to your address space
				perror("Consumer shmat failed :\n");
				exit(2);
			}


			/* Create a thread */
			pthread_t con;
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_attr_setdetachstate(&attr , PTHREAD_CREATE_JOINABLE);
			pthread_attr_setscope(&attr,PTHREAD_SCOPE_SYSTEM);

			/* Struct for parameters to prod func */
			struct param conparam;
			conparam.ptr = con_sm_ptr;

			int ret;
			if((ret=pthread_create(&con,&attr,threadfn,(void *) &conparam))){
				fprintf(stderr,"pthread_create ERROR : %s\n",strerror(ret));
				exit(1);
			}
			/* Wait for all threads to finish */
			pthread_join(con,NULL);	


			if(shmdt(con_sm_ptr)==-1){			// De-attach shared memory
				perror("Consumer shmdt failed :\n");
				exit(2);
			}

			printf("Consumer exit\n");
			return 0;
		}

		else{
			/* Parent code */

			/* Wait for child processes : prod and con to get over */
			while(conretval !=conid || prodretval !=prodid){
				conretval=waitpid(conid,&constatus,0);
				if(WIFEXITED(constatus)){
					if(constatus==0)
						printf("Consumer exited successfully\n");
					else
						printf("Consumer Failed\n");
				}
				prodretval=waitpid(prodid,&prodstatus,0);
				if(WIFEXITED(prodstatus)){
					if(prodstatus==0)
						printf("Producer exited successfully\n");
					else
						printf("Producer Failed\n");
				}
				sleep(1);
			}
		}
	}

	if(shmdt(sm_ptr)==-1){			// De-attach shared memory
		perror("SHMDT failed :\n");
		exit(1);
	}

	// Free shared memory
        if((shmctl(sm_id, IPC_RMID, NULL)) == -1){
         	perror("Shmctl failed \n");
                exit(1);
        }
	
	printf("End of code\n");
	return 0;

} // End of main


















