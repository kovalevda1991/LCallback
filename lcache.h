#ifndef LCACHE_H
#define LCACHE_H

#include "lockfree.h"
#ifdef FREERTOS
#include "FreeRTOS.h"
#include "queue.h"
#else
#endif

template<int SZ = 256, typename T = uint8_t>
class LCache {
public:
  LCache(){
      q = xQueueCreate(SZ, sizeof(T));
  }
  virtual ~LCache(){}

#ifdef CORTEX
  int checkISR() {
    return ((SCB->ICSR)&SCB_ICSR_VECTACTIVE_Msk);
  }
#else
  int checkISR() {return 0;}
#endif

#ifdef FREERTOS
  uint32_t push(void *data, int sz) {
    T *pd = (T*)data;
    uint32_t wd = 0;
    while(sz) {
      if(checkISR()) {
        portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
        wd += xQueueSendFromISR(q, pd, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
      } else {
        wd += xQueueSend(q, pd, 10);
      }
      pd++;
      sz--;
    }
    return wd;
  }
  uint32_t take(void *data, int sz) {
    T *pd = (T*)data;
    uint32_t rd = 0;
    while(sz) {
      if(checkISR()) {
        portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
        rd += xQueueReceiveFromISR(q, pd, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
      } else {
        rd += xQueueReceive(q, pd, 10);
      }
      pd++;
      sz--;
    }
    return rd;
  }
  uint32_t count() {
    if(checkISR()) {
      portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
      uint32_t r = uxQueueMessagesWaitingFromISR(q);
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
      return r;
    } else {
      return uxQueueMessagesWaiting(q);
    }
  }

private:
  xQueueHandle q;
#else
  uint32_t push(void *data, int sz) {
    lock.lock();
    uint32_t wr = 0;
    T *pd = (T*)data;
    while(sz) {
      if(size < SZ) {
        *pw = *pd; pw++; pd++; wr++; size++; sz--;
        if((pw - buf)>=SZ) pw = buf;
      } else {
        lock.free();
        return wr;
      }
    }
    lock.free();
    return wr;
  }

  uint32_t take(void *data, int sz) {
    lock.lock();
    uint32_t rd = 0;
    T *pd = (T*)data;
    while(sz) {
      if(size) {
        *pd = *pr; pd++; pr++; rd++; size--; sz--;
        if((pr - buf)>=SZ) pr = buf;
      } else {
        lock.free();
        return rd;
      }
    }
    lock.free();
    return rd;
  }

  uint32_t count() {
    lock.lock();
    uint32_t tmp = size;
    lock.free();
    return tmp;
  }

private:
  LockFree lock;
  T buf[SZ];
  T *pw=buf,*pr=buf;
  uint32_t size = 0;
#endif
};

#endif // LCACHE_H
