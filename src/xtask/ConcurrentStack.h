/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   concurrentstack.h
 * Author: pnookala
 * 
 * Created on February 13, 2018, 10:39 AM
 */

#ifndef CONCURRENTSTACK_H
#define CONCURRENTSTACK_H 


extern "C" {
#include <nautilus/semaphore.h>
#include <nautilus/nautilus.h>
typedef struct nk_semaphore* sem_t;
}
#include <vector>
template<typename T>
class ConcurrentStack {
private:
    //mutable std::mutex m;
    mutable sem_t m = NULL;
    std::vector<T> data;
    int capacity;
public:
    
    ~ConcurrentStack() {
        nk_semaphore_release(m);
    }

    ConcurrentStack() {
        m = nk_semaphore_create("xtask-stack", 1, NK_SEMAPHORE_DEFAULT, NULL);
    }

    ConcurrentStack(int size) {
        printk("Constructor 1 called\n");
        m = nk_semaphore_create("xtask-stack", 1, 0, NULL);
        data.reserve(size);
        capacity = size;
    }

    ConcurrentStack(std::vector<T> d) {
        printk("Constructor 2 called\n");
        m = nk_semaphore_create("xtask-stack", 1, 0, NULL);
        data = d;
    }

    ConcurrentStack(const ConcurrentStack& other) {

        printk("Constructor 3 called\n");
        //std::lock_guard<std::mutex> lock(other.m);
        nk_semaphore_down(other.m);

        // copy in constructor body rather than 
        // the member initializer list 
        // in order to ensure that the mutex is held across the copy.
        data = other.data;
        nk_semaphore_up(other.m);
        m = nk_semaphore_create("xtask-stack", 1, 0, NULL);
    }

    ConcurrentStack& operator=(const ConcurrentStack&) = delete;

    void push(T new_value) {
        printk("Push \n");
        //std::lock_guard<std::mutex> lock(m);
        
        nk_semaphore_down(m);

        printk("Does this really work \n");
        data.push_back(new_value);
        nk_semaphore_up(m);
    }

    T pop() {
        // check for empty before trying to pop value
        if (data.empty()) {
            //pthread_testcancel();
            return NULL;
        }
        
        //std::lock_guard<std::mutex> lock(m);
        nk_semaphore_down(m);

        // allocate return value before modifying stack
        T const res((data.back()));
        data.pop_back();

        nk_semaphore_up(m);
        return res;
    }

    void pop(T& value) {
        //std::lock_guard<std::mutex> lock(m);
        nk_semaphore_down(m);
        if (data.empty()) {
            nk_semaphore_up(m);
            return;
        }
        value = data.back();
        data.pop_back();
        nk_semaphore_up(m);
    }

    bool empty() const {
        //std::lock_guard<std::mutex> lock(m);
        nk_semaphore_down(m);
        bool result = data.empty();
        nk_semaphore_up(m);
        return result;
    }

    bool full() const {
        //std::lock_guard<std::mutex> lock(m);
        nk_semaphore_down(m);
        bool result = (data.size() == capacity);
        nk_semaphore_up(m);
        return result;
    }
};

#endif
