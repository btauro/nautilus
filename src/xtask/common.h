/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   common.h
 * Author: pnookala
 *
 * Created on February 12, 2018, 3:41 PM
 */

#ifndef COMMON_H
#define COMMON_H

//#include <pthread.h>
//#include <semaphore.h>
#include <nautilus/semaphore.h>
#include "ConcurrentStack.h"
#include "ConcurrentQueue.h"
#include "task.h"
#include <vector>
#include <nautilus/thread.h>
#include <nautilus/semaphore.h>
 
#define LEAF_TASK 1
#define NON_LEAF_TASK 0

#define NUM_CPUS 2

typedef long unsigned int ticks;

#ifdef PHI

//get number of ticks, could be problematic on modern CPUs with out of order execution
static __inline__ ticks getticks(void) {
    ticks tsc;
    __asm__ __volatile__(
            "rdtsc;"
            "shl $32, %%rdx;"
            "or %%rdx, %%rax"
            : "=a"(tsc)
            :
            : "%rcx", "%rdx");

    return tsc;
}

#else

static __inline__ ticks getticks(void) {
    ticks tsc;
    __asm__ __volatile__(
            "rdtscp;"
            "shl $32, %%rdx;"
            "or %%rdx, %%rax"
            : "=a"(tsc)
            :
            : "%rcx", "%rdx");

    return tsc;
}
#endif
static inline void spinlock(volatile int *lock)
{
    while(!__sync_bool_compare_and_swap(lock, 0, 1))
    {   
        nk_yield();
    }
}

static inline void spinunlock(volatile int *lock)
{
	//*lock = 0;
    __sync_val_compare_and_swap( lock , 1 , 0 );
}

static inline void spin_tryunlock(volatile int *lock)
{
    __sync_val_compare_and_swap( lock , 1 , 0 ) != 1;
}

struct xtask_parent {
        xtask_task *task;
	int depCounter;
        int onHold; //for concurrency and thread safety
};

struct worker {
	//pthread_t t;
    nk_thread_id_t t;
#ifdef USE_STACK
        ConcurrentStack<xtask_task*> tasks;
#elif USE_LOCKFREE
#else
        ConcurrentQueue<xtask_task*> tasks;
#endif
};

struct master_state {
    std::vector<struct worker*> workers;
        int numWorkers;
        int numTasks;
        int nextQueue;
    };
#endif /* COMMON_H */

