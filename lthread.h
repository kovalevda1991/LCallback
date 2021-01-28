#ifndef LTHREAD_H
#define LTHREAD_H

#include "stdint.h"
#include "lcallback.h"
#include "lockfree.h"

#ifdef __linux__
#include <pthread.h>
#include <unistd.h>
#elif WIN32
#include "Windows.h"
#elif FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#else
#endif

class LThread {
private:
#ifdef __linux__
  pthread_t pth;
  void createThread(void(*process)(void*), void *params) {
    pthread_create(&pth, 0, (void*(*)(void*))process, params);
  }
  void deleteThread() {
    pthread_join(pth, 0);
  }
  void exitThread() {
  }
public:
  static void msleep(uint32_t ms) {
    if(ms) usleep(ms*1000); else pthread_yield();
  }
  static uint32_t getCurrentThread() {
    return pthread_self();
  }
  static void yield() {
    pthread_yield();
  }
#elif WIN32
  HANDLE pth;
  void createThread(void(*process)(void*), void *params) {
    pth = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)process, params, 0, 0);
  }
  void deleteThread() {
    WaitForSingleObject(pth, INFINITE);
  }
  void exitThread() {
  }
public:
  static void msleep(uint32_t ms) {
    if(ms) {
      HANDLE timer;
      LARGE_INTEGER ft;

      ft.QuadPart = -(10000*(int)ms); // Convert to 100 nanosecond interval, negative value indicates relative time

      timer = CreateWaitableTimer(NULL, TRUE, NULL);
      SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
      WaitForSingleObject(timer, INFINITE);
      CloseHandle(timer);
    }
    else yield();
  }
  static uint32_t getCurrentThread() {
    return (uint32_t)GetCurrentThread();
  }
  static void yield() {
    SwitchToThread();
  }
#elif FREERTOS
  xTaskHandle pth;
  void createThread(void(*process)(void*), void *params, uint16_t stackDepth = configMINIMAL_STACK_SIZE, UBaseType_t priority = tskIDLE_PRIORITY) {
    xTaskCreate(process, "", stackDepth, params, priority, &pth);
  }
  void deleteThread() {
    vTaskDelete(pth);
  }
  void exitThread() {
    vTaskDelete(NULL);
  }
public:
  static void msleep(uint32_t ms) {
    if(ms) vTaskDelay(ms); else taskYIELD();
  }
  static uint32_t getCurrentThread() {
    return (uint32_t)xTaskGetCurrentTaskHandle();
  }
  static void yield() {
    if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
    portYIELD();
  }
#else
#endif


public:
  typedef enum {
    STANDART = 0,
    POLLING = 1
  }WorkMode_t;

  typedef enum {
    RUNNING = 0,
    STOPPED,
    TERMINATED,
    EXITED
  }WorkState_t;

  /**
   * @brief LThread конструктор для object->method
   */
  template<typename Object, typename Method, typename ...Args>
  LThread(Object *o, Method m, Args ...a):process(o,m) {
    init(a...);
  }
  template<typename Method, typename ...Args>
  LThread(Method m, Args ...a):process(m) {
    init(a...);
  }
  virtual ~LThread() {
    lock.lock();
    if(state != EXITED) state = TERMINATED;
    while(state != EXITED) {
      lock.free();
      msleep(1);
      lock.lock();
    }
    deleteThread();
    lock.free();
  }

  /**
   * @brief start
   */
  void start() {
    lock.lock();
    if(workMode != POLLING) {
      lock.free();
      return;
    }
    if(state != STOPPED) {
      lock.free();
      return;
    }
    state = RUNNING;
    lock.free();
  }
  /**
   * @brief stop
   */
  void stop() {
    lock.lock();
    if(workMode != POLLING) {
      lock.free();
      return;
    }
    if(state != RUNNING) {
      lock.free();
      return;
    }
    state = STOPPED;
    lock.free();
  }



private:
  /**
   * @brief Парсинг параметров конструктора
   */
  template<typename ...Args>
  void init(Args ...a) {
    state = EXITED;
    createThread(run, this, a...);
  }
  template<typename ...Args>
  void init(WorkMode_t wmode, Args ...a) {
    workMode = wmode;
    switch (wmode) {
      case STANDART: {
          state = EXITED;
          createThread(run, this, a...);
        }break;
      case POLLING: createThread(poll, this, a...); break;
      default: while(1); //ловушка на неправильные параметры
    }
  }
  template<typename ...Args>
  void init(WorkMode_t wmode, int pollingInterval = 1, Args ...a) {
    workMode = wmode;
    pollInterval = pollingInterval;
    switch (wmode) {
      case STANDART: {
          state = EXITED;
          createThread(run, this, a...);
        }break;
      case POLLING: createThread(poll, this, a...); break;
      default: while(1); //ловушка на неправильные параметры
    }
  }

  /**
   * @brief process - целевая функция потока
   */
  static void run(void *lthread) {
    LThread *th = (LThread*)lthread;
    th->process();
    th->exitThread();
  }
  LCallback<void> process;
  /**
   * @brief все для поллинга
   */
  WorkMode_t workMode = STANDART;
  WorkState_t state = STOPPED;
  LockFree lock;
  uint32_t pollInterval = 0;
  static void poll(void *lthread) {
    LThread *th = (LThread*)lthread;
    th->lock.lock();
    while(th->state != TERMINATED) {
      if(th->state == STOPPED) {
        th->lock.free();
        msleep(th->pollInterval);
        th->lock.lock();
        continue;
      }
      th->lock.free();
      th->process();
      msleep(th->pollInterval);
      th->lock.lock();
    }
    th->state = EXITED;
    th->lock.free();
    th->exitThread();
  }
};

#endif // LTHREAD_H
