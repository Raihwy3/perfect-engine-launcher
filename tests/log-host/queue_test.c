#include "pd_log_queue.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
static struct pd_log_queue q;
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static unsigned done,consumed;
static void *produce(void *arg){
 unsigned id=(unsigned)(size_t)arg;
 for(unsigned i=0;i<10000;i++){
  char text[64];int n=snprintf(text,sizeof text,"%u %u",id,i);
  pthread_mutex_lock(&lock);pd_log_push(&q,text,n+1);pthread_mutex_unlock(&lock);
 }
 pthread_mutex_lock(&lock);done++;pthread_mutex_unlock(&lock);return NULL;
}
int main(void){
 char line[PD_LOG_LINE];
 for(unsigned i=0;i<PD_LOG_SLOTS+5;i++){int n=snprintf(line,sizeof line,"%u",i);pd_log_push(&q,line,n+1);}
 assert(q.dropped==5);
 for(unsigned i=5;i<PD_LOG_SLOTS+5;i++){assert(pd_log_pop(&q,line));assert(strtoul(line,NULL,10)==i);}
 assert(!pd_log_pop(&q,line));
 memset(line,'x',sizeof line);pd_log_push(&q,line,sizeof line+100);assert(pd_log_pop(&q,line)==sizeof line);
 memset(&q,0,sizeof q);
 pthread_t a,b;pthread_create(&a,NULL,produce,(void *)0);pthread_create(&b,NULL,produce,(void *)1);
 int previous[2]={-1,-1};
 for(;;){
  pthread_mutex_lock(&lock);size_t n=pd_log_pop(&q,line);unsigned finished=done;pthread_mutex_unlock(&lock);
  if(n){unsigned id,sequence;assert(sscanf(line,"%u %u",&id,&sequence)==2);assert(id<2 && (int)sequence>previous[id]);previous[id]=(int)sequence;consumed++;}
  else if(finished==2)break;
 }
 pthread_join(a,NULL);pthread_join(b,NULL);assert(consumed+q.dropped==20000);
 puts("Bounded log queue: overflow, truncation and concurrent producers passed");
}
