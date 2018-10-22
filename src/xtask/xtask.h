/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   xtask.h
 * Author: pnookala
 *
 * Created on February 12, 2018, 3:43 PM
 */

#ifndef XTASK_H
#define XTASK_H

#include "common.h"
using namespace std;

class xtask {
private:
    master_state* ms; 
public:
    xtask();
    virtual ~xtask();
    void xtask_setup(int workers, int numTasks, int startWorkers);
    void xtask_cleanup();
    void xtask_push(xtask_task *task);
    void xtask_realpush(xtask_task *task);
    void xtask_push(vector<xtask_task*> tasks);
    xtask_task* xtask_pop(int ds);
    void xtask_poll(int* totalTasks); //We need to get total tasks processed for benchmarking purposes
};
#endif /* XTASK_H */

int xtask_main(int argc, char** argv); 
