/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   xtask.cpp
 * Author: pnookala
 * 
 * Created on February 12, 2018, 3:43 PM
 */


extern "C" {
#include <nautilus/semaphore.h>
#include <nautilus/mm.h>
#include "nt_pthread.h"

typedef struct nk_semaphore* sem_t;
//#define sem_t struct nk_semaphore*
//struct nk_semaphore *  donesignal;
}
 
#include <vector>
#include <stack>
#include "xtask.h"

sem_t donesignal;

struct threadData {
    struct worker *w;
    xtask* currentObj;
    int threadId;
};


xtask::xtask() {
    ms = new master_state();
    //ms = (struct master_state *) malloc(sizeof (struct master_state));
    //ms->workers = std::vector<struct worker*>();
    //std::vector<struct worker *> * test = &ms->workers;
    //test = new std::vector<struct worker *>();
    //struct worker* w = (struct worker*)malloc(sizeof(struct worker));
    //ms->workers.emplace_back(w);
    //printk("Xtask Constructor finished Invocation\n");    
}

xtask::~xtask() {
}

void
sem_init(sem_t & m, int pshared, int value)
{
    m = nk_semaphore_create("xtask-donesignal", value, NK_SEMAPHORE_DEFAULT, NULL);
}

void* xtask_leaf(xtask_task* t) {
    //Check recursively for parents
    xtask_parent* next = (xtask_parent*) t->parent;
    while (next) {
        //get the hold, so no other thread processes it, wait in while loop if some other thread owns it.
        while (!(__sync_bool_compare_and_swap(&((xtask_parent*) next)->onHold, 0, 1))) {
            sched_yield();
        }
        int dc = __sync_sub_and_fetch(&((xtask_parent*) next)->depCounter, 1);
        if (dc != 0) {
            __sync_bool_compare_and_swap(&((xtask_parent*) next)->onHold, 1, 0); //release the hold
            xtask_task *dummy = new xtask_task();

            *dummy = {NULL, NULL, next, NULL};
            return dummy;
        }
        __sync_bool_compare_and_swap(&((xtask_parent*) next)->onHold, 1, 0); //release the hold
        next = (xtask_parent*) next->task->parent;
    }
    nk_semaphore_up(donesignal);
    return NULL;
}

void* worker_handler(void *data, void **out) {
    threadData* tl = (threadData*) data;
    int threadId = tl->threadId; //Also the core ID to which it is to be attached.
    struct worker *w = tl->w;
    while (1) {
        //printk("thread: %d\n", threadId);
        xtask_task *task = w->tasks.pop();
        if (!task) continue;

        xtask_task *retTask = (xtask_task*) task->func(task);
        //If a task is returned, we need to push the child tasks.

        if (retTask) {
            xtask_parent *p;
            if (!retTask->parent) {
                int scnt = 0;
                for (xtask_task* t = retTask; t; t = (xtask_task*) t->sibling, scnt++);
                    p = new xtask_parent();
                    p->task = new xtask_task();
                    *(p->task) = {xtask_leaf, NULL, task->parent, NULL};
                    p->depCounter = scnt;
                    p->onHold = 0;
            }

            if (retTask->func) {
                for (xtask_task* t = retTask; t; t = (xtask_task*) t->sibling) {
                    t->parent = retTask->parent ? retTask->parent : p;
                    tl->currentObj->xtask_realpush(t);
                }
            }
        }//leaf task
        else {
            //Do the post processing here...
            if (task->parent) {
                int dc = __sync_sub_and_fetch(&((xtask_parent*) (task->parent))->depCounter, 1);
                if (dc == 0) {
                    if (((xtask_parent*) task->parent)->task)
                        tl->currentObj->xtask_push(((xtask_parent*) task->parent)->task);
                }
            } else {
                //If no parent, it is the root task
                nk_semaphore_up(donesignal);
                printk("\n I am parent\n");
            }
        }

    }
    return NULL;
}

void xtask::xtask_setup(int workers, int numTasks, int startWorkers) {
    ms->numWorkers = workers;
    ms->numTasks = numTasks;
    ms->nextQueue = 0;
    ms->workers = std::vector<struct worker*>();
    sem_init(donesignal, 0, 0);
    //Change here for multiple single/multiple stacks
    for (int i = 0; i < workers; i++) {
        //struct worker *w = (struct worker*)malloc(sizeof(struct worker));
        struct worker *w = new worker();
        w->tasks.initSemaphores();
        ms->workers.emplace_back(w);
    }

    if (startWorkers) {

        for (int t  = 0; t < workers; t++) {
            //struct threadData *tl = (threadData*) malloc(sizeof (struct threadData));
            struct threadData *tl = new threadData();
            tl->w = ms->workers[t];
            //Change here for single/multiple stacks
            tl->threadId = t;
            tl->currentObj = this;
            pthread_creat(&(ms->workers[t]->t), worker_handler, NULL, (void*) tl, 2);
        }
    }
}

void xtask::xtask_cleanup() {
    free(ms);
}

void xtask::xtask_push(xtask_task *task) {
    xtask_parent *p;
    if (!task->parent) {
        int scnt = 0;
        for (xtask_task* t = task; t; t = (xtask_task*) t->sibling, scnt++);
        p = new xtask_parent();
        p->task = new xtask_task();
        *(p->task) = {xtask_leaf, NULL, NULL, NULL};
        p->depCounter = scnt;
        p->onHold = 0;
    }

    for (xtask_task* t = task; t; t = (xtask_task*) t->sibling) {

        t->parent = task->parent ? task->parent : p;
        xtask_realpush(t);
    }

}

xtask_task* xtask::xtask_pop(int ds) {
    return (ms->workers[ds]->tasks).pop();
}

void xtask::xtask_realpush(xtask_task *task) {

    int next = __sync_fetch_and_add(&ms->nextQueue, 1);

    if (ms->numWorkers == 0) {
        printk("ERROR: non-positive worker count\n");
        return;
    }
    //Change here for single/multiple stacks
    (ms->workers[next % ms->numWorkers]->tasks).push(task);
    //ms->tasks[0].push(task);
    __sync_fetch_and_add(&ms->numTasks, 1);

}

void xtask::xtask_poll(int* totalTasks) {
    nk_semaphore_down(donesignal);
    printk("---------------------------about to return fron xtask_poll\n");
  //  udelay(10);
    for (int i = 0; i < ms->numWorkers; i++) {
        //TODO: Need to implement pthread_cancel in naut we only have kill for now
        pthread_cancel(ms->workers[i]->t);
        // TODO KCH: note that joining on a thread which has been destroyed
        // will produce, at the very least, undesirable effects
        pthread_join(ms->workers[i]->t, NULL);
    }

    *totalTasks = ms->numTasks;
    printk("---------------------------about to return fron xtask_poll\n");
    return;
}

