#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <setjmp.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <linux/sched.h>
#include <netdb.h>
#include <err.h>

#include <dynamic.h>
#include <clo.h>
#include <reactor.h>

static int realtime_scheduler(void)
{
  struct sched_param param;
  struct rlimit rlim;
  int e;

  e = sched_getparam(0, &param);
  if (e == -1)
    return -1;

  param.sched_priority = sched_get_priority_max(SCHED_FIFO);
  rlim = (struct rlimit) {.rlim_cur = param.sched_priority, .rlim_max = param.sched_priority};
  e = prlimit(0, RLIMIT_RTPRIO, &rlim, NULL);
  if (e == -1)
    return -1;

  e = sched_setscheduler(0, SCHED_FIFO, &param);
  if (e == -1)
    return -1;

  return 0;
}

void client_event(void *state, int type, void *data)
{
  reactor_stream *stream = state;
  reactor_stream_data *read = data;

  (void) data;
  switch (type)
    {
    case REACTOR_STREAM_READ:
      reactor_stream_write(stream, read->base, read->size);
      reactor_stream_consume(read, read->size);
      break;
    case REACTOR_STREAM_ERROR:
    case REACTOR_STREAM_SHUTDOWN:
      reactor_stream_close(stream);
      break;
    case REACTOR_STREAM_CLOSE:
      free(stream);
      break;
    }
}


void event(void *state, int type, void *data)
{
  reactor_tcp *tcp = state;
  reactor_stream *stream;

  (void) state;
  switch (type)
    {
    case REACTOR_TCP_ACCEPT:
      stream = malloc(sizeof *stream);
      reactor_stream_init(stream, client_event, stream);
      reactor_stream_open(stream, *(int *) data);
      break;
    case REACTOR_TCP_ERROR:
    case REACTOR_TCP_SHUTDOWN:
      reactor_tcp_close(tcp);
      break;
    }
}

void server(void)
{
  reactor_tcp tcp;
  int e;

  (void) realtime_scheduler();
  reactor_core_open();
  reactor_tcp_init(&tcp, event, &tcp);
  reactor_tcp_open(&tcp, NULL, "8080", REACTOR_TCP_SERVER);
  e = reactor_core_run();
  if (e == -1)
    err(1, "reactor_core_run");
  reactor_core_close();
}

int main()
{
  long i, n;
  pid_t cpid;

  (void) realtime_scheduler();
  signal(SIGPIPE, SIG_IGN);
  n = MAX(1, sysconf(_SC_NPROCESSORS_ONLN) / 2);
  for (i = 0; i < n; i ++)
    {
      cpid = fork();
      if (cpid == -1)
        err(1, "fork");
      if (cpid == 0)
        {
          server();
          exit(0);
        }
    }
  wait(NULL);
}
