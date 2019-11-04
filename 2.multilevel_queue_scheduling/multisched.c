/*
 * OS Assignment #2
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <sys/wait.h>

#define MSG(x...) fprintf (stderr, x)
#define STRERROR  strerror (errno)

#define PROCESS_MAX 520
#define ID      2

#define ARRIVE_TIME_MIN 0
#define ARRIVE_TIME_MAX 30

#define SERVICE_TIME_MIN 1
#define SERVICE_TIME_MAX 30

#define PRIORITY_MIN 1
#define PRIORITY_MAX 10

#define POINT_MAX 31200



typedef struct _Process Process;
struct _Process
{
	int    order;
	int    queue_order;

	char   id[ID + 1];
	char   process_type;
	int    arrive_time;
	int    service_time;
	int    priority;

	int    remain_time;
	int    complete_time;
	int    turnaround_time;
	int    wait_time;
};

static Process  processes[PROCESS_MAX];
static int      process_total;

static Process *queue_h[PROCESS_MAX];
static int      queue_h_len;

static Process *queue_m[PROCESS_MAX];
static int      queue_m_len;

static Process *queue_l[PROCESS_MAX];
static int      queue_l_len;

static char     schedule[PROCESS_MAX][POINT_MAX];

	static char *
strstrip (char *str)
{
	char  *start;
	size_t len;

	len = strlen (str);
	while (len--)
	{
		if (!isspace (str[len]))
			break;
		str[len] = '\0';	//문자열 뒤에서부터 공백이 존재하지 않을 때 까지 '\0'값으로 변경
	}

	for (start = str; *start && isspace (*start); start++)	//앞에서 공백을 제거하는 동일 기능 실행
		;
	memmove (str, start, strlen (start) + 1);	//start가 가르키는 곳부터 strlen(start)+1 만큼 str이 가르키는 곳으로 이동

	return str;
}

	static int
check_valid_id (const char *str)
{
	size_t len;
	int    i;

	len = strlen (str);
	if (len != ID)	//2문자로 구성된 문자열이 아닐 시
		return -1;

	for (i = 0; i < len; i++)	//영문 대문자와 숫자로 이루어지지 않았을 시
		if (!(isupper (str[i]) || isdigit (str[i])))
			return -1;

	return 0;
}

	static Process *
lookup_process (const char *id)
{
	int i;
	//processes의 모든 process 순회하며 해당 id값을 가지는 process 확인
	for (i = 0; i < process_total; i++)
		if (!strcmp (id, processes[i].id))
			return &processes[i];

	return NULL;
}

	static void
append_process (Process *process)
{
	processes[process_total] = *process;
	processes[process_total].order = process_total;
	process_total++;
}

	static int
read_config (const char *filename)
{
	FILE *fp;
	char  line[256];
	int   line_nr;

	fp = fopen (filename, "r");
	if (!fp)
		return -1;

	process_total = 0;

	line_nr = 0;
	while (fgets (line, sizeof (line), fp))
	{
		Process process;
		char  *p;
		char  *s;
		size_t len;

		line_nr++;
		memset (&process, 0x00, sizeof (process));

		len = strlen (line);
		if (line[len - 1] == '\n')
			line[len - 1] = '\0';

		if (0)
			MSG ("config[%3d] %s\n", line_nr, line);

		strstrip (line);

		/* comment or empty line */
		if (line[0] == '#' || line[0] == '\0')
			continue;

		/* id */
		s = line;
		p = strchr (s, ' ');
		if (!p)
			goto invalid_line;
		*p = '\0';
		strstrip (s);
		if (check_valid_id (s))
		{
			MSG ("invalid process id '%s' in line %d, ignored\n", s, line_nr);
			continue;
		}
		if (lookup_process (s))
		{
			MSG ("duplicate process id '%s' in line %d, ignored\n", s, line_nr);
			continue;
		}
		strcpy (process.id, s);

		/* process-type*/
		s = p + 1;
		p = strchr (s, ' ');
		if (!p)
			goto invalid_line;
		*p = '\0';
		strstrip (s);
		if(!strcasecmp(s,"H"))
			process.process_type = 'H';
		else if(!strcasecmp(s,"M"))
			process.process_type = 'M';
		else if(!strcasecmp(s,"L"))
			process.process_type = 'L';
		else goto invalid_line;
		
		/* arrive time */
		s = p + 1;
		p = strchr (s, ' ');
		if (!p)
			goto invalid_line;
		*p = '\0';
		strstrip (s);

		process.arrive_time = atoi(s);

		if (process.arrive_time < ARRIVE_TIME_MIN || ARRIVE_TIME_MAX < process.arrive_time
				|| (process_total > 0 &&
					processes[process_total - 1].arrive_time > process.arrive_time))
		{
			MSG ("invalid arrive-time '%s' in line %d, ignored\n", s, line_nr);
			continue;
		}

		/* service time */
		s = p + 1;
		p = strchr (s, ' ');
		if (!p)
			goto invalid_line;
		*p = '\0';
		strstrip (s);
		process.service_time = atoi(s);
		if (process.service_time < SERVICE_TIME_MIN
				|| SERVICE_TIME_MAX < process.service_time)
		{
			MSG ("invalid service-time '%s' in line %d, ignored\n", s, line_nr);
			continue;
		}
		
		/* priority */
		s = p + 1;
		strstrip (s);
		process.priority = atoi(s);
		if (process.priority < PRIORITY_MIN
				|| PRIORITY_MAX < process.priority)
		{
			MSG ("invalid priority '%s' in line %d, ignored\n", s, line_nr);
			continue;
		}

		append_process (&process);
		continue;

invalid_line:
		MSG ("invalid format in line %d, ignored\n", line_nr);
	}

	fclose (fp);

	return 0;
}

	static void
multischeduling ()
{	
	int      p;
	int      num_completed_process;
	int      cpu_time;
	Process *process;
	Process *process_h;
	Process *process_m;
	Process *process_l;
	int		 cpu_h = 6;
	int		 cpu_m = 4;

	for (int i = 0; i < PROCESS_MAX; i++)
	{
		for (int j = 0; j < POINT_MAX; j++)
			schedule[i][j] = 0;
		queue_h[i] = NULL;
		queue_m[i] = NULL;
		queue_l[i] = NULL;
	}
	
	p = 0;
	num_completed_process = 0;
	queue_h_len = 0;
	queue_m_len = 0;
	queue_l_len = 0;
	process = NULL;
	
	for (cpu_time = 0; num_completed_process < process_total; cpu_time++)
	{	
		for (; p < process_total; p++)
		{
			Process *temp_process;

			temp_process = &processes[p];
			if (temp_process->arrive_time != cpu_time)	
				break;
			temp_process->remain_time = temp_process->service_time;

			if(temp_process->process_type=='H'){
				temp_process->queue_order = queue_h_len;
				queue_h[queue_h_len] = temp_process;
				queue_h_len++;
			}
			else if(temp_process->process_type=='M'){
				temp_process->queue_order = queue_m_len;
				queue_m[queue_m_len] = temp_process;
				queue_m_len++;
			}
			else if(temp_process->process_type=='L'){
				temp_process->queue_order = queue_l_len;
				queue_l[queue_l_len] = temp_process;
				queue_l_len++;
			}
			else continue;
		}

		int		 flag_h = 0;
		int		 flag_m = 0;
		int		 flag_l = 0;

		for(int i=0, min=31; i<queue_h_len; i++){
			if(queue_h[i]->priority < min){
				process_h = queue_h[i];
				min = queue_h[i]->priority;
				flag_h = 1;
			}
		}

		for(int i=0, min=31; i<queue_m_len; i++){
			if(queue_m[i]->service_time < min){
				process_m = queue_m[i];
				min = queue_m[i]->service_time;
				flag_m = 1;
			}
		}

		for(int i=0, min=31; i<queue_l_len; i++){
			if(queue_l[i]->arrive_time < min){
				process_l = queue_l[i];
				min = queue_l[i]->arrive_time;
				flag_l = 1;
				continue;
			}
		}
		
		//선점할 프로세스를 정하는 부분
		if(flag_h==1 && flag_m==1){
			if(cpu_h>0){
				process = process_h;
				cpu_h--;
			}
			else{
				process = process_m;
				cpu_m--;
				if(cpu_m==0) cpu_h=6, cpu_m=4;
			}
		}
		else if(flag_h==1 && flag_m==0){
			process = process_h;
			cpu_h--;
		}
		else if(flag_h==0 && flag_m==1){
			process = process_m;
		}
		else if(flag_h==0 && flag_m==0 && flag_l==1){
			process = process_l;
		}
		else	continue;

		
		schedule[process->order][cpu_time] = 1;
		process->remain_time--;

		if (process->remain_time == 0)	//프로세스 종료시점
		{	
			if(process->process_type=='H'){
				for (int i = process->queue_order; i < (queue_h_len - 1); i++){
				queue_h[i] = queue_h[i + 1];
				queue_h[i]->queue_order = i;
				}
				queue_h_len--;
			}
			else if(process->process_type=='M'){
				for (int i = process->queue_order; i < (queue_m_len - 1); i++){
				queue_m[i] = queue_m[i + 1];
				queue_m[i]->queue_order = i;
				}
				queue_m_len--;
			}
			else if(process->process_type=='L'){
				for (int i = process->queue_order; i < (queue_l_len - 1); i++){
				queue_l[i] = queue_l[i + 1];
				queue_l[i]->queue_order = i;
				}
				queue_l_len--;
			}
			
			process->complete_time = cpu_time + 1;
			process->turnaround_time =
				process->complete_time - process->arrive_time;
			process->wait_time =
				process->turnaround_time - process->service_time;
			process = NULL;
			num_completed_process++;
			
		}
	}

	int      sum_turnaround_time = 0;
	int      sum_waiting_time = 0;
	float    avg_turnaround_time = 0;
	float    avg_waiting_time = 0;
	printf("Multilevel Queue Scheduling\n");
	
	for (p = 0; p < process_total; p++){
		printf ("%s ", processes[p].id);
		for (int point = 0; point <= cpu_time; point++){
			if(schedule[p][point]==1) printf("*");
			else printf(" ");
		}
		printf ("\n");
		sum_turnaround_time += processes[p].turnaround_time;
		sum_waiting_time += processes[p].wait_time;
	}
	printf("\n");
	avg_turnaround_time = (float) sum_turnaround_time / (float) process_total;
	avg_waiting_time = (float) sum_waiting_time / (float) process_total;

	printf ("CPU TIME: %d\n", cpu_time);
	printf ("AVERAGE TURNAROUND TIME: %.2f\n", avg_turnaround_time);
	printf ("AVERAGE WAITING TIME: %.2f\n", avg_waiting_time);
}

	int main (int argc, char **argv){
		if (argc <= 1){
			MSG ("input file must specified");
			return -1;
		}
		
		if (read_config (argv[1])){
			MSG ("failed to load input file");
			return -1;
		}
		multischeduling();
		return 0;
}
