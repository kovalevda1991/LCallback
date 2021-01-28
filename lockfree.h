#ifndef LOCKFREE_H
#define LOCKFREE_H


#ifdef FREERTOS
#include "FreeRTOS.h"
#include "semphr.h"
#else
#include "atomic"
#endif
#include "lockfreeconf.h"

class LockFree {
public:
  LockFree();

#ifdef CORTEX
  int checkISR() {
    return ((SCB->ICSR)&SCB_ICSR_VECTACTIVE_Msk);
  }
#else
  int checkISR() {return 0;}
#endif


  void lock();
  void free();
#ifdef FREERTOS
private:
    xSemaphoreHandle m;
#else
private:
  bool testAndSetAcquire(int expectedValue, int newValue);
  bool testAndSetRelease(int expectedValue, int newValue);
  std::atomic_int l;
#endif
};

#endif // LOCKFREE_H
