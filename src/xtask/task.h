/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   task.h
 * Author: pnookala
 *
 * Created on March 16, 2018, 12:42 PM
 */

#ifndef TASK_H
#define TASK_H

#ifdef __cplusplus
extern "C" {
#endif

        typedef struct xtask_task xtask_task;
        
    struct xtask_task {
        void* (*func)(xtask_task *t); //left sibling of the tree
	void* data;
	void* parent;
        void* sibling;
#ifndef USE_STACK
        void* next; //Used only when queue is full, not required in case of stack
#endif
    };
    
#ifdef __cplusplus
}
#endif

#endif /* TASK_H */

