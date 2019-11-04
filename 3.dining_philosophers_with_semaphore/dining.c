/*
 * OS Assignment #3
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <sys/wait.h>
#include <pthread.h>

#define MSG(x...) fprintf (stderr, x)
#define STRERROR  strerror (errno)

#define PHILOSOPHERS_MIN 3
#define PHILOSOPHERS_MAX 10

#define MSEC_MIN 10
#define MSEC_MAX 1000

#define CYCLES_MIN 1
#define CYCLES_MAX 100

void fork_wait(int i);
void fork_signal(int i);
void chair_wait(int i);
void chair_signal(int i);

int philosophers;
int msec;
int cycles;
int chair;

int output[10]={0,};

pthread_mutex_t mutex;
pthread_mutex_t mutex2;
pthread_cond_t ready_queue[10];
pthread_cond_t chair_ready_queue;
int available_fork[10]={1,1,1,1,1,1,1,1,1,1};


int valid_check(char * p, char * m, char * c){
	philosophers = atoi(p);
	msec = atoi(m);
	cycles = atoi(c);
	chair = philosophers - 1;
	if(philosophers < PHILOSOPHERS_MIN || philosophers > PHILOSOPHERS_MAX){
		MSG("invalid philosophers number");
		return 0;
	}
	else if(msec < MSEC_MIN || msec > MSEC_MAX){
		MSG("invalid msec number");
		return 0;
	}
	else if(cycles < CYCLES_MIN || cycles > CYCLES_MAX){
		MSG("invalid cycles number");
		return 0;
	}
	else return 1;
}

void * dining(void *i){
	int ii = (int)i;
	while(1){
		chair_wait(ii);		//의자에 앉기위해 기다리는 행위
		fork_wait(ii);		//왼쪽 포크를 들기위해 기다리는 행위
		fork_wait((ii+1)%philosophers);	//오른쪽 포크를 들기위해 기다리는 행위

		output[ii]=1;
		usleep(msec*1000);	//1000msec동안 생각하는 행위
		
		fork_signal(ii);	//왼쪽 포크를 내려놓고 알려주는 행위
		fork_signal((ii+1)%philosophers);	//오른쪽 포크를 내려놓고 알려주는 행위
		chair_signal(ii);	//의자에서 일어나 알려주는 행위

		output[ii]=0;
		usleep(msec*1000);	//1000msec동안 생각하는 행위
	}
}

void fork_wait(int i){
	pthread_mutex_lock(&mutex);
	{		//critical section
		if(available_fork[i]==1){
			available_fork[i]=0;
		}
		else pthread_cond_wait(&ready_queue[i], &mutex);
	}
	pthread_mutex_unlock(&mutex);
}

void fork_signal(int i){
	pthread_mutex_lock(&mutex);
	{	//critical section
		available_fork[i]=1;
		pthread_cond_signal(&ready_queue[i]);
	}
	pthread_mutex_unlock(&mutex);
}

void chair_wait(int i){
	pthread_mutex_lock(&mutex2);
	{	//critical section
		chair--;
		if(chair < 0){
			pthread_cond_wait(&chair_ready_queue, &mutex2);
		}
	}
	pthread_mutex_unlock(&mutex2);
}

void chair_signal(int i){
	pthread_mutex_lock(&mutex2);
	{	//critical section
		chair++;
		if(chair<=0){
		pthread_cond_signal(&chair_ready_queue);
		}
	}
	pthread_mutex_unlock(&mutex2);
}

int main (int argc, char **argv){
	//input에 대한 오류 처리
	if (argc <= 1){
		MSG ("input parameter must specified");
		return -1;
	}
	else if(argc != 4){
		MSG ("input parameter must be 4");
		return -1;
	}
	if(valid_check(argv[1], argv[2], argv[3]) == 0)	
		return 0;
	
	pthread_mutex_init(&mutex, NULL);	//wait와 signal의 임계구역을 상호배제하기위한 mutex 초기화
	pthread_mutex_init(&mutex2, NULL);

	for(int i=0; i<philosophers; i++)	//각각의 포크를 기다리기위한 레디큐와 의자에 앉기위한 레디큐 초기화
		pthread_cond_init(&ready_queue[i], NULL);
	pthread_cond_init(&chair_ready_queue, NULL);

	pthread_t p_thread[philosophers];
	for(long i=0; i<philosophers; i++){	//dining함수를 실행시키기 위한 철학자 수만큼의 쓰레드 생성
		void * voidp = (void *)i;
		pthread_create(&p_thread[i], NULL, dining, voidp);
	}
	
	for(int i=0; i<philosophers; i++){
		printf("P%d        ", i+1);
	}
	printf("\n");
	
	for(int i=0; i<cycles; i++){
		int error=1;	//모든 철학자가 먹는 상황이거나(error), 
		int deadlock=1;	//모든 철학자가 먹지 못하는 상황(deadlock)의 오류를 확인하기위한 변수)
		usleep(msec*1000);	
		for(int j=0; j<philosophers; j++){
			if(output[j]==1){
				deadlock=0;
				printf("Eating    ");
			}
			else{
				 error=0;	
				 printf("Thinking  ");
			}
		}
		printf("\n");
		//if(deadlock==1) printf("deadlock\n");
		//if(error==1) printf("error\n");
	}

	return 0;
}
