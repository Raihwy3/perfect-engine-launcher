#ifndef PD_LOG_QUEUE_H
#define PD_LOG_QUEUE_H
#include <stddef.h>
#include <string.h>
/* Callers serialize access. Never perform file I/O while holding that lock. */
#define PD_LOG_SLOTS 128
#define PD_LOG_LINE 1024
struct pd_log_queue {
 char lines[PD_LOG_SLOTS][PD_LOG_LINE];
 unsigned short lengths[PD_LOG_SLOTS];
 unsigned head,count,dropped;
};
static inline void pd_log_push(struct pd_log_queue *q,const char *line,size_t length) {
 if(length>PD_LOG_LINE)length=PD_LOG_LINE;
 if(q->count==PD_LOG_SLOTS){q->head=(q->head+1)%PD_LOG_SLOTS;q->count--;q->dropped++;}
 unsigned tail=(q->head+q->count)%PD_LOG_SLOTS;
 memcpy(q->lines[tail],line,length);q->lengths[tail]=(unsigned short)length;q->count++;
}
static inline size_t pd_log_pop(struct pd_log_queue *q,char *line) {
 if(!q->count)return 0;
 size_t length=q->lengths[q->head];memcpy(line,q->lines[q->head],length);
 q->head=(q->head+1)%PD_LOG_SLOTS;q->count--;return length;
}
#endif
