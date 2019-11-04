/**
 * OS Assignment #1 Test Task.
 **/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#define MSG(x...) fprintf (stderr, x)

static char        *name = "Task";
static volatile int looping;

static void
signal_handler (int signo)
{
  if (!looping)
    return;

  looping = 0;
  MSG ("'%s' terminated by SIGNAL(%d)\n", name, signo);
}

int
main (int    argc,
      char **argv)
{
  int   timeout    = 0;
  int   read_stdin = 0;
  char *msg_stdout = NULL;

  /* Parse command line arguments. */
  {
    int opt;

    while ((opt = getopt (argc, argv, "n:t:w:r")) != -1)
      {
	switch (opt)
	  {
	  case 'n':
	    name = optarg;
	    break;
	  case 't':
	    timeout = atoi (optarg);
	    break;
	  case 'r':
	    read_stdin = 1;
	    break;
	  case 'w':
	    msg_stdout = optarg;
	    break;
	  default:
	    MSG ("usage: %s [-n name] [-t timeout] [-r] [-w msg]\n", argv[0]);
	    return -1;
	  }
      }
  }

  /* Register SIGINT / SIGTERM signal handler. */
  {
    struct sigaction sa;

    sigemptyset (&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = signal_handler;
    if (sigaction (SIGINT, &sa, NULL))
      {
	MSG ("'%s' failed to register signal handler for SIGINT\n", name);
	return -1;
      }

    sigemptyset (&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = signal_handler;
    if (sigaction (SIGTERM, &sa, NULL))
      {
	MSG ("'%s' failed to register signal handler for SIGTERM\n", name);
	return -1;
      }
  }

  MSG ("'%s' start (timeout %d)\n", name, timeout);

  looping = 1;

  /* Write the message to standard outout. */
  if (msg_stdout)
    {
      char msg[256];

      snprintf (msg, sizeof (msg), "%s from %s", msg_stdout, name);
      write (1, msg, strlen (msg) + 1);
    }

  /* Read the message from standard input. */
  if (read_stdin)
    {
      char    msg[256];
      ssize_t len;

      len = read (0, msg, sizeof (msg));
      if (len > 0)
	{
	  msg[len] = '\0';
	  MSG ("'%s' receive '%s'\n", name, msg);
	}
    }

  /* Loop */
  while (looping && timeout != 0)
    {
      if (0) MSG ("'%s' timeout %d\n", name, timeout);
      usleep (1000000);
      if (timeout > 0)
	timeout--;
    }

  MSG ("'%s' end\n", name);

  return 0;
}
