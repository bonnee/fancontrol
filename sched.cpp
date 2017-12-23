#include "sched.h"

bool sorting_fun (const scheduled first, const scheduled second){
  return first.last_called + first.requested < second.last_called + second.requested;
}
LittleScheduler::LittleScheduler(){
  this->actual_size =0;
}

bool LittleScheduler::add_task(void (*fun)(void*), unsigned long requested, unsigned long predicted, void *args){
  if(this->actual_size+1 > MAX_SCHEDULED){
    return false;
  }
  scheduled tmp = (scheduled){predicted, requested, 0, fun, args };
  for(int i =0; i<this->actual_size; i++){
    if(this->tasks[i].requested > tmp.requested){
      scheduled ttmp = this->tasks[i];
      this->tasks[i] = tmp;
      tmp = ttmp;
    }
  } 
  this->tasks[this->actual_size] = tmp;
  this->actual_size++;
}

bool LittleScheduler::run_sched(unsigned long time){
  #if MAX_PRED_TIME
  unsigned long this_loop_time = 0;
  bool has_to_sort = false;
  #endif
  unsigned int i =0;
  while(i < this->actual_size){ 
    if(time-this->tasks[i].last_called >= this->tasks[i].requested){
      #if MAX_PRED_TIME
      this_loop_time = this_loop_time + this->tasks[i].predicted;
      if(this_loop_time > this->max_loop_time){
        if(has_to_sort){
          std::sort(this->tasks, (this->tasks + this->actual_size), sorting_fun);
        }
        return false;
      }
      #endif
      this->tasks[i].function(this->tasks[i].args);
      this->tasks[i].last_called = time;
      #if MAX_PRED_TIME
      has_to_sort = true;
      #endif
    }
    i++;
  }
  #if MAX_PRED_TIME
  if(has_to_sort){
    std::sort(this->tasks, (this->tasks + this->actual_size),sorting_fun);
  }
  #endif
  return true;
}
