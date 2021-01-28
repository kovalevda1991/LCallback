#include "lockfree.h"
#include "lthread.h"

#ifndef FREERTOS
int LockFree::lock() {
    if(checkISR()) {
        return testAndSetAcquire(0,1);
    }
    else {
        while(!testAndSetAcquire(0,1));
        return 0;
    }
}

bool LockFree::testAndSetAcquire(int expectedValue, int newValue)
{
    return l.compare_exchange_strong(expectedValue, newValue, std::memory_order_acquire, std::memory_order_acquire);
}

bool LockFree::testAndSetRelease(int expectedValue, int newValue)
{
    return l.compare_exchange_strong(expectedValue, newValue, std::memory_order_release, std::memory_order_relaxed);
}

void LockFree::free() {
    if(checkISR()) {
        testAndSetAcquire(1,0);
        return;
    } else {
        while(!testAndSetRelease(1,0));
    }
}
#else
LockFree::LockFree() {
    m = xSemaphoreCreateMutex();
}

void LockFree::lock() {
    if(checkISR()) {
        portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreTakeFromISR(m, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        xSemaphoreTakeRecursive(m,-1);
    }
}

void LockFree::free() {
    if(checkISR()) {
        portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(m, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        xSemaphoreGive(m);
    }
}
#endif
