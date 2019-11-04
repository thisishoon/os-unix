

/**
 * OS Assignment #1
 **/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/signalfd.h>

#define MSG(x...) fprintf (stderr, x)
#define STRERROR  strerror (errno)

#define ID_MIN 2
#define ID_MAX 8
#define COMMAND_LEN 256
#define ORDER_MAX 4

typedef enum
{
  ACTION_ONCE,
  ACTION_RESPAWN,

} Action;

typedef struct _Task Task;
struct _Task
{
  Task          *next;

  volatile pid_t pid;
  int            piped;
  int            pipe_a[2];
  int            pipe_b[2];

  char           id[ID_MAX + 1];
  char           pipe_id[ID_MAX + 1];
  Action         action;
  char           command[COMMAND_LEN];
  char           order[ORDER_MAX];
};

static Task *tasks;

static volatile int running;

static char * strstrip (char *str)
{
  char  *start;
  size_t len;

  len = strlen (str);
  while (len--)
    {
      if (!isspace (str[len]))
        break;
      str[len] = '\0';  //문자열 뒤에서부터 공백이 존재하지 않을 때 까지 '\0'값으로 변경
    }

  for (start = str; *start && isspace (*start); start++) //앞에서 공백을 제거하는 동일 기능 실행
    ;
  memmove (str, start, strlen (start) + 1);  //start가 가르키는 곳부터 strlen(start)+1 만큼 str이 가르키는 곳으로 이동

  return str;
}

static int
check_valid_id (const char *str)
{
  size_t len;
  int    i;

  len = strlen (str);
  if (len < ID_MIN || ID_MAX < len)     //2개 이상 8개 이하로 구성된 문자열이 아닐 시
    return -1;

  for (i = 0; i < len; i++)     //영문 소문자와 숫자로 이루어지지 않았을 시
    if (!(islower (str[i]) || isdigit (str[i])))
      return -1;

  return 0;
}

static int  //프로그램 실행 순서 order가 유효한 값인지 검증하는 함수
check_valid_order (const char *str)
{
  size_t len;
  int    i;

  len = strlen (str);
  if (ORDER_MAX < len)     // 4자리 이하 숫자가 아닐 시
    return -1;

  for (i = 0; i < len; i++)     //숫자로 이루어지지 않았을 시
    if (!isdigit (str[i]))
      return -1;

  return 0;
}


static Task *
lookup_task (const char *id)
{
  Task *task;
  //tasks의 모든 task들을 순회하며 해당 id값을 가지는 task 확인
  for (task = tasks; task != NULL; task = task->next)
    if (!strcmp (task->id, id))
      return task;

  return NULL;
}

static Task *
lookup_task_by_order (const char *id, const char *order)  //read_config함수에서 pipe_id를 검사 할 떄 order에 의해 검사하기 위한 추가 함수
{
  Task *task;

  for (task = tasks; task != NULL; task = task->next)   //존재하는 task들을 모두 순회하면서 이름이 같은 task가 존재하는지 확인
    if (!strcmp (task->id, id))     //id값과 존재하는 task가 있는지 확인
      if( ( task->order[0]!='\0' && atoi(order) >= atoi(task->order)) || task->order[0]=='\0')  //확인하려는 task의 order가 확인된 task의 order보다 우선순위인지 확인 
        return task;

  return NULL;
}

static Task *
lookup_task_by_pid (pid_t pid)
{
  Task *task;

  for (task = tasks; task != NULL; task = task->next)
    if (task->pid == pid)
      return task;

  return NULL;
}

static void
append_task (Task *task)    //단일 연결리스트(Tasks)의 order기준으로 오름차순을 유지하기위해 
{
  Task *new_task;
  new_task = malloc (sizeof (Task));
  if (!new_task)
    {
      MSG ("failed to allocate a task: %s\n", STRERROR);
      return;
    }

  *new_task = *task;
  new_task->next = NULL;

  if (!tasks) //tasks가 비어있을 때 
    tasks = new_task;
    
  else        //tasks가 존재할 때 
    {
      Task *t;
  
      if(new_task->order[0]=='\0'){  //order가 공백이라면 마지막에 삽입
        for (t = tasks; t->next != NULL; t = t->next);
        t->next = new_task;
      }

      else{   //order가 존재한다면 오름차순 정렬을 유지한 채 삽입
        if(tasks->order[0]=='\0'){  //head의 order가 공백일 때 맨 앞에 삽입
          new_task->next = tasks;
          tasks = new_task;
        }
        else{     //head의 order가 0이 아닐 떄 오름차 순 tasks를 유지하면서 삽입
          t= tasks;
          if(atoi(t->order) > atoi(new_task->order)){   //head의 order가 new_task의 order보다 작다면 맨앞에 삽입
            new_task->next = t;
            tasks = new_task;
          }

          else{
            Task *temp = t;
            
            
            while(t->order[0]!='\0'){           //t가 가르키는 order가 공백이 아닐 때 까지
              if(atoi(t->order) <= atoi(new_task->order)){  //오름차순을 유지하며 삽입할 수 있는 위치 순회
                temp = t;
                t = t->next;
              }
              else break;
            }
            new_task->next = temp->next;       //new_task가 삽입될 수 있는 위치의 앞과 뒤의 task를 연결
            temp->next = new_task;
          }
        }
      }         
    }
}

static int
read_config (const char *filename)      //config.txt파일을 읽어들여 실행시키는 함수
{
  FILE *fp;
  char  line[COMMAND_LEN * 2];  //command의 최대 길이 지정
  int   line_nr;                //command의 line

  fp = fopen (filename, "r");
  if (!fp)  //읽기 실패시 -1 리턴
    return -1;

  tasks = NULL;

  line_nr = 0;
  while (fgets (line, sizeof (line), fp))   //마지막 줄의 command를 읽을 때 까지
    {
      Task   task;
      char  *p;
      char  *s;
      size_t len;      //size_t type= unsigned int type

      line_nr++;
      memset (&task, 0x00, sizeof (task));  //task객체를 0으로 초기화

      len = strlen (line);
      if (line[len - 1] == '\n')    //command 마지막 문자의 개행을 공백으로 변경
        line[len - 1] = '\0';

      if (0)
        MSG ("config[%3d] %s\n", line_nr, line);

      strstrip (line);      //command의 line에서 뒤 공백들을 모두 제거

      /* comment or empty line */
        if (line[0] == '#' || line[0] == '\0')    //command의 첫 문자가 #이거나 공백이면 무시
        continue;

      /* id */
      s = line;
      p = strchr (s, ':');      // 문자열 s에서 ':'로 시작하는 부분 문자열 시작점의 포인터를 반환
      if (!p)                   //command에 ':"를 포함하지 않는다면 무효한 command
        goto invalid_line;
      *p = '\0';                // ':'문자를 '\0'으로 변환
      strstrip (s);             // 다시 공백 제거
      if (check_valid_id (s))
        {
          MSG ("invalid id '%s' in line %d, ignored\n", s, line_nr);
          continue;
        }
      if (lookup_task (s))
        {
          MSG ("duplicate id '%s' in line %d, ignored\n", s, line_nr);
          continue;
        }
      strcpy (task.id, s);

      /* action */
      s = p + 1;
      p = strchr (s, ':');
      if (!p)
        goto invalid_line;
      *p = '\0';
      strstrip (s);
      if (!strcasecmp (s, "once")) //문자열 s와 "once"가 같다면 0 리턴
        task.action = ACTION_ONCE;
      else if (!strcasecmp (s, "respawn"))
        task.action = ACTION_RESPAWN;
      else
        {
          MSG ("invalid action '%s' in line %d, ignored\n", s, line_nr);
          continue;
        }

      /* order */
      s = p + 1;
      p = strchr (s, ':');
      if (!p)
        goto invalid_line;
      *p = '\0';
      strstrip (s);
      if (s[0] != '\0'){
        if (check_valid_order (s))
          {
            MSG ("invalid order '%s' in line %d, ignored\n", s, line_nr);
            continue;
          }
          strcpy (task.order, s);
      }
      
    
      /* pipe-id */
      s = p + 1;
      p = strchr (s, ':');
      if (!p)
        goto invalid_line;
      *p = '\0';
      strstrip (s);
      if (s[0] != '\0')
        {
          Task *t;

          if (check_valid_id (s))   //무효한 이름 확인
            {
              MSG ("invalid pipe-id '%s' in line %d, ignored\n", s, line_nr);
              continue;
            }

          t = lookup_task_by_order(s, task.order);  //order에 의해 중복 task를 확인, t는 task와 파이프로 연결 될 task
          if (!t)
            {
              MSG ("unknown pipe-id '%s' in line %d, ignored\n", s, line_nr);
              continue;
            }
          if (task.action == ACTION_RESPAWN || t->action == ACTION_RESPAWN) //프로그램과 파이프로 연결된 프로그램일 경우 두개 중 하나라도 respawn의 action을 가지면 안된다
            {
              MSG ("pipe not allowed for 'respawn' tasks in line %d, ignored\n", line_nr);
              continue;
            }
          if (t->piped)     //연결된 프로그램의 파이프가 이미 존재할 경우
            {
              MSG ("pipe not allowed for already piped tasks in line %d, ignored\n", line_nr);
              continue;
            }

          strcpy (task.pipe_id, s); //자식 프로세스에게는 pipe_id를 할당하지만 부모프로세스에게는 할당하지않는다
          task.piped = 1;
          t->piped = 1;
        }

      /* command */
      s = p + 1;
      strstrip (s);
      if (s[0] == '\0')
        {
          MSG ("empty command in line %d, ignored\n", line_nr);
          continue;
        }
      strncpy (task.command, s, sizeof (task.command) - 1);
      task.command[sizeof (task.command) - 1] = '\0';   //끝을 알리는 '\0'

      if (0)
        MSG ("id:%s pipe-id:%s action:%d command:%s\n",
             task.id, task.pipe_id, task.action, task.command);

      append_task (&task);  //task 추가
      continue;

    invalid_line:
      MSG ("invalid format in line %d, ignored\n", line_nr);
    }
  
  fclose (fp);

  return 0;
}

static char **
make_command_argv (const char *str)
{
  char      **argv;
  const char *p;
  int         n;

  for (n = 0, p = str; p != NULL; n++)
    {
      char *s;

      s = strchr (p, ' ');
      if (!s)
        break;
      p = s + 1;
    }
  n++;

  argv = calloc (sizeof (char *), n + 1);
  if (!argv)
    {
      MSG ("failed to allocate a command vector: %s\n", STRERROR);
      return NULL;
    }

  for (n = 0, p = str; p != NULL; n++)
    {
      char *s;

      s = strchr (p, ' ');
      if (!s)
        break;
      argv[n] = strndup (p, s - p);
      p = s + 1;
    }
  argv[n] = strdup (p);

  if (0)
    {

      MSG ("command:%s\n", str);
      for (n = 0; argv[n] != NULL; n++)
        MSG ("  argv[%d]:%s\n", n, argv[n]);
    }

  return argv;
}

static void
spawn_task (Task *task)
{
  if (0) MSG ("spawn program '%s'...\n", task->id);

  if (task->piped && task->pipe_id[0] == '\0')
    {
      if (pipe (task->pipe_a))
        {
          task->piped = 0;
          MSG ("failed to pipe() for prgoram '%s': %s\n", task->id, STRERROR);
        }
      if (pipe (task->pipe_b))
        {
          task->piped = 0;
          MSG ("failed to pipe() for prgoram '%s': %s\n", task->id, STRERROR);
        }
    }
  
  task->pid = fork ();  //같은 코드를 공유하는 자식 프로세스를 새로운 메모리에 할당한다
  if (task->pid < 0)    //자식 생성 실패 시
    {
      MSG ("failed to fork() for program '%s': %s\n", task->id, STRERROR);
      return;
    }

  /* child process */
  if (task->pid == 0)   //자식 생성 성공 시, 자식의 코드
    {
      char **argv;

      argv = make_command_argv (task->command);
      if (!argv || !argv[0])
        {
          MSG ("failed to parse command '%s'\n", task->command);
          exit (-1);
        }
        
      if (task->piped)  //task의 piped가 존재한다면 두 프로세스를 양방향으로 이어준다
        {
          if (task->pipe_id[0] == '\0')     //id를 id로가지는 프로세스의 입력,출력 장치 연결 --------- 1번
            {
              dup2 (task->pipe_a[1], 1);
              dup2 (task->pipe_b[0], 0);
              close (task->pipe_a[0]);
              close (task->pipe_a[1]);
              close (task->pipe_b[0]);
              close (task->pipe_b[1]);
            }
          else
            {
              Task *sibling;

              sibling = lookup_task (task->pipe_id); //task->pipe_id를 id로 가지는 프로세스의 입력,출력 장치 연결 ------2번
              if (sibling && sibling->piped)
                {
                  dup2 (sibling->pipe_a[0], 0);
                  dup2 (sibling->pipe_b[1], 1);
                  close (sibling->pipe_a[0]);
                  close (sibling->pipe_a[1]);
                  close (sibling->pipe_b[0]);
                  close (sibling->pipe_b[1]);
                }
            }
        }
      execvp (argv[0], argv);   //자식의 명령어 인수를 포함한 command 실행
      MSG ("failed to execute command '%s': %s\n", task->command, STRERROR);
      exit (-1);
    }
}

static void
spawn_tasks (void)
{
  Task *task;

    for (task = tasks; task != NULL && running; task = task->next){
    spawn_task (task);
    usleep (100000);  //0.1초 지연
    }
      
}



static void
wait_for_children (int signo)
{
  Task *task;
  pid_t pid;

 rewait:
  pid = waitpid (-1, NULL, WNOHANG);  //자식 프로세스가 종료되지 않으면 0, 실패하면 -1을 리턴
  if (pid <= 0)
    return;
    
  task = lookup_task_by_pid (pid);  //종료된 자식 프로세스의 id로 부터 task를 찾음
  if (!task)
    {
      MSG ("unknown pid %d", pid);
      return;
    }

  if (0) MSG ("program[%s] terminated\n", task->id);

    if (running && task->action == ACTION_RESPAWN)  //action이 respawn이라면 다시 실행
    spawn_task (task);
     
  else      //task 종료
    task->pid = 0;

  /* some SIGCHLD signals is lost... */
  goto rewait;
}

static void
terminate_children (int signo)
{
  Task *task;
  if (1) MSG ("terminated by SIGNAL(%d)\n", signo);

  running = 0;

  for (task = tasks; task != NULL; task = task->next)
    if (task->pid > 0)
      {
        if (0) MSG ("kill program[%s] by SIGNAL(%d)\n", task->id, signo);
        kill (task->pid, signo);
      }

  exit (1);
}

int
main (int    argc,
      char **argv)
{
  struct sigaction sa;
  int terminated;

  if (argc <= 1)
    {
      MSG ("usage: %s config-file\n", argv[0]);
      return -1;
    }

  if (read_config (argv[1]))
    {
      MSG ("failed to load config file '%s': %s\n", argv[1], STRERROR);
      return -1;
    }

  running = 1;

  /* SIGCHLD using signalfd(2) */
  sigset_t mask;  
  int fd;   //file descripter
  struct signalfd_siginfo si; //signalfd file descripter read(2) return struct 
  ssize_t s;

  sigemptyset (&mask);
  sigaddset(&mask, SIGCHLD);
  fd = signalfd(-1, &mask, 0);
  sigprocmask(SIG_BLOCK, &mask, NULL);

  /* SIGINT */
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = terminate_children;
  if (sigaction (SIGINT, &sa, NULL))
    MSG ("failed to register signal handler for SIGINT\n");

  /* SIGTERM */
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = terminate_children;
  if (sigaction (SIGTERM, &sa, NULL))
    MSG ("failed to register signal handler for SIGINT\n");

  
  spawn_tasks ();

  terminated = 0;
  while(!terminated){
  { //프로그램이 종료되기 전까지 sigchld 시그널을 확인하기위해 미처리 상태인 시그널을 read(2)를 통해 확인하고 signalfd_siginfo 구조체를 반환
      s = read(fd, &si, sizeof(struct signalfd_siginfo));
      if (si.ssi_signo == SIGCHLD) {  //SIGCHLD 시그널이 발생할 시
          wait_for_children(SIGCHLD);  // 자식 프로세스가 종료될 경우 호출
      }
      else {
          printf("Read unexpected signal\n");
      }

      Task *task;

      terminated = 1;
      for (task = tasks; task != NULL; task = task->next)
        if (task->pid > 0)
          {
            terminated = 0;
            break;
          }

      usleep (100000);
    }
  }
  close(fd);  //close file descripter
  return 0;
}
