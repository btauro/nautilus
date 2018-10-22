/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ConcurrentQueue.h
 * Author: pnookala
 *
 * Created on March 12, 2018, 3:50 PM
 */

#ifndef CONCURRENTQUEUE_H
#define CONCURRENTQUEUE_H
/*
#include <type_traits>
#include <nautilus/mm.h>
#include <typeinfo>
#include <vector>
#include <stack>
*/
#include "common.h"
#include <nautilus/semaphore.h>
#include <nautilus/mm.h>
#include <vector>
#include <stack>
typedef struct nk_semaphore* sem_t;
template<typename T>
class ConcurrentQueue {
private:
    mutable sem_t m;
    int capacity;
    T* data; /* all tasks */
    int rear; /* tasks[rear % capacity] is the last item */
    int front; /* tasks[(front+1) % capacity] is the first */
    sem_t task_sem; /* counts available tasks in queue */
    sem_t spaces_sem; /* counts available task slots */
    int numTasks;
public:

    ~ConcurrentQueue() {
        free(data);
        nk_semaphore_release(m);
        nk_semaphore_release(task_sem);
        nk_semaphore_release(spaces_sem);
    }

    ConcurrentQueue(const int& size) : capacity(size) {
        data = (T*) malloc(sizeof (T) * size);
        rear = 0;
        front = 0;
        //sem_init(&task_sem, 0, 0);
        //sem_init(&spaces_sem, 0, capacity);
        m = nk_semaphore_create(NULL, 1, 0, NULL); 
        task_sem = nk_semaphore_create(NULL, 0, 0, NULL); 
        spaces_sem = nk_semaphore_create(NULL, capacity, 0, NULL);
        memset(data, 0, sizeof (T) * size);
    }

    ConcurrentQueue(const ConcurrentQueue& other) {
        //std::lock_guard<std::mutex> lock(other.m);
        nk_semaphore_down(other.m);

        // copy in constructor body rather than 
        // the member initializer list 
        // in order to ensure that the mutex is held across the copy.
        data = other.data;
        capacity = other.capacity;
        rear = other.rear;
        front = other.front;
        //sem_init(&task_sem, 0, 0);
        //sem_init(&spaces_sem, 0, capacity);
        m = nk_semaphore_create(NULL, 1, 0, NULL);
        task_sem = nk_semaphore_create(NULL, 0, 0, NULL); 
        spaces_sem = nk_semaphore_create(NULL, capacity, 0, NULL);
        nk_semaphore_up(other.m);
    }

    void push(T new_value) {
        if (full()) {
            //std::lock_guard<std::mutex> lock(m);
            nk_semaphore_down(m);
            int next = front + 1; //faking pop here
            T cur = data[next % capacity]; //Equivalent to calling pop()
            new_value->next = cur;
            data[next % capacity] = new_value;
            nk_semaphore_up(m);
            return;
        } else {
            //sem_wait(&spaces_sem);
            nk_semaphore_down(spaces_sem);
            //std::lock_guard<std::mutex> lock(m);
            nk_semaphore_down(m);
            data[(++rear) % (capacity)] = new_value;
            //sem_post(&task_sem);
            nk_semaphore_up(task_sem);
            nk_semaphore_up(m);
        }
    }

    T pop() {
        if (full()) {
            //std::lock_guard<std::mutex> lock(m);
            nk_semaphore_down(m);
            int next = front + 1;
            T task = data[next % capacity];
            if(task->next) {
                data[next % capacity] = (T)task->next;
            }
            else {
                front++; //processed all chained tasks, so increment front
                //sem_post(&spaces_sem);
                nk_semaphore_up(spaces_sem);
            }
            nk_semaphore_up(m);
            return task;
        } else if(empty()) {
            //pthread_testcancel();
            return NULL;
        }
        else {

            //sem_wait(&task_sem);
            nk_semaphore_down(task_sem);
            //std::lock_guard<std::mutex> lock(m);
            nk_semaphore_down(m);
            // WARNING: THIS MAY NOT WORK AS I WANT LATER 
            T task = data[(++front) % capacity];
            //sem_post(&spaces_sem);
            nk_semaphore_up(spaces_sem);
            nk_semaphore_up(m);
            return task;
        }
    }

    bool empty() {
        //std::lock_guard<std::mutex> lock(m);
        nk_semaphore_down(m);
        bool result = (front == rear);
        nk_semaphore_up(m);
        return result;
    }

    bool full() {
        //std::lock_guard<std::mutex> lock(m);
        int ne = 0;
        //sem_getvalue(&spaces_sem, &ne);
        ne = spaces_sem->count;
                //if there are no spaces in the queue
        if (ne == 0) {
            return true;
        } else
            return false;
    }
};

#endif /* CONCURRENTQUEUE_H */

