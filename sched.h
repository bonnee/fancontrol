#ifndef SCHED_H
#define SCHED_H 0 
#define MAX_SCHEDULED 10
#define MAX_PRED_TIME 0
#if MAX_PRED_TIME
#include<algorithm>
#endif

struct scheduled{
  unsigned long predicted;
  unsigned long requested;
  unsigned long last_called;
  void (*function) (void *);
  void *args;
};

class LittleScheduler{
private:
  scheduled tasks[MAX_SCHEDULED];
  unsigned int actual_size;
public:
  #if MAX_PRED_TIME
  unsigned long max_loop_time;
  #endif
  LittleScheduler();
  bool add_task(void (*)(void *), unsigned long, unsigned long, void*);
  bool run_sched(unsigned long);
};
#endif
