/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: pnookala
 *
 * Created on February 12, 2018, 3:40 PM
 */

extern "C" {
#include <nautilus/nautilus.h>
#include <nautilus/mm.h> 
#include <nautilus/random.h>
#include <nautilus/libccompat.h>

#define DEBUG(fmt, args...)    DEBUG_PRINT("XTASK: " fmt, ##args)
}
#include "xtask.h"

#define RAND() ((uint32_t)rand())
using namespace std;

xtask * xt;

float clockFreq;
typedef long unsigned int ticks;

static int* arr;

static void 
sleep(int time){
    udelay(time);
}
#if defined USE_openmp
#include <omp.h>

static int fib_openmp(int n) {
    if (n <= 1) return n;
    int a, b;
#pragma omp task shared(a)
    a = fib_openmp(n - 1);
#pragma omp task shared(b)
    b = fib_openmp(n - 2);
#pragma omp taskwait
    return a + b;
}
#elif defined USE_single

static int fib(int n) {
    if (n <= 1) return n;
    else return fib(n - 1) + fib(n - 2);
}
#else

struct fibdata {
    int n;
    int *out;
};

struct fulldata {
    fibdata a; //fib(n-1)
    fibdata b; //fib(n-2), both have parents as 
    int aout, bout;
    int* out;
};

void* add_xtask(xtask_task *t) {
    fulldata* data = (fulldata*) t->data;
    *data->out = data->aout + data->bout;
    return NULL;
}

static inline unsigned long long get_current_time_in_micros(void)
{
    struct timespec spec;
    clock_gettime(CLOCK_MONOTONIC, &spec);
    unsigned long long result = (((unsigned long long)spec.tv_sec) * 1000000)
        + (((unsigned long long)spec.tv_nsec)/1000);
    return result;
}

void* fib_xtask(xtask_task *t) {
    fibdata* data = (fibdata*) t->data;
    //printk("Fib of %d\n", data->n);
    if (data->n < 2) {
        *(data->out) = data->n;
        return NULL;
    } else {
        fulldata* fd = new fulldata();
        fd->a = {data->n - 1, &fd->aout};
        fd->b = {data->n - 2, &fd->bout};

        fd->out = data->out;

        xtask_parent *addTask = new xtask_parent();
        addTask->task =  (struct xtask_task *)malloc (sizeof(struct xtask_task));

        *addTask->task = {add_xtask, fd, t->parent, NULL};
        addTask->depCounter = 2;
        addTask->onHold = 0;

        xtask_task *fib2 = (struct xtask_task *)malloc (sizeof(struct xtask_task));
        *fib2 = {fib_xtask, &(fd->b), addTask, NULL};

        xtask_task *fib1 = (struct xtask_task *)malloc(sizeof(struct xtask_task));
        *fib1 = {fib_xtask, &(fd->a), addTask, fib2};

        return fib1;
    }
}
#endif

#if defined USE_openmp
#include <omp.h>

void sleep_openmp(int n) {
    sleep(n);
}
#else

void* sleep_xtask(xtask_task *t) {
    sleep(0);
    return NULL;
}
#endif

struct qdata {
    int lo, hi;
};
int irnd = 1;
int jrnd = 1000;

    
int
random ()
{
    return rand();
}

    
int
srandom (int seed)
{
    srand(seed);
    return random();
}

static int part(int lo, int hi) {
    int p = arr[hi];
    int i = lo - 1;
    for (int j = lo; j < hi; j++)
        if (arr[j] <= p) {
            i++;
            int v = arr[i];
            arr[i] = arr[j];
            arr[j] = v;
        }
    i++;
    int v = arr[i];
    arr[i] = arr[hi];
    arr[hi] = v;
    return i;
}

void* sort_xtask(xtask_task *t) {
    qdata* d = (struct qdata*) t->data;
    int lo = d->lo;
    int hi = d->hi;
    //free(d);

    if (lo < hi) {
        int p = part(lo, hi);
        qdata* a = new qdata();
        qdata* b = new qdata();

        a->lo = lo;
        a->hi = p - 1;

        b->lo = p + 1;
        b->hi = hi;

        xtask_task *atask = new xtask_task();
        *atask = {sort_xtask, a, NULL, NULL};

        xtask_task *btask = new xtask_task();
        *btask = {sort_xtask, b, NULL, atask};

        return btask;
    } else {
        return NULL;
    }
}
#if defined USE_openmp
#include <omp.h>

static void sort(int lo, int hi) {
    if (lo < hi) {
        int p = part(lo, hi);
#pragma task shared(lo, hi, p)
        sort(lo, p - 1);
#pragma task shared(lo, hi, p)
        sort(p + 1, hi);
#pragma taskwait
    }
}
#elif defined USE_single

static void sort(int lo, int hi) {
    if (lo < hi) {
        int p = part(lo, hi);
        sort(lo, p - 1);
        sort(p + 1, hi);
    }
}
#endif

int xtask_main(int argc, char** argv) {
    xt = new xtask();
    if (argc < 2) {
        printk("Please enter the type of test to run.\n");
    }

    int type;
    type = atoi(argv[1]);
    struct timespec ts;
    struct timespec tvstart, tvstop;
    unsigned long long int cycles[2];
    unsigned long microseconds;


    clock_gettime(CLOCK_MONOTONIC, &tvstart);
    cycles[0] = getticks1();
    clock_gettime(CLOCK_MONOTONIC, &tvstart);
    udelay(250000);

    clock_gettime(CLOCK_MONOTONIC, &tvstop);
    cycles[1] = getticks1();
    clock_gettime(CLOCK_MONOTONIC, &tvstop);
    microseconds = ((tvstop.tv_sec - tvstart.tv_sec) * 1000000) + ((tvstop.tv_nsec - tvstart.tv_nsec) / 1000);

    clockFreq = ((cycles[1] - cycles[0])*1.0) / (microseconds * 1000);

    printk("Clock Freq Obtained: %d\n", clockFreq);

    switch (type) {
        case 1: //Fibonacci
            if (argc != 4) {
                printk("Usage: <task type>, <n>, <num threads>\n");
            } else {
                int n, numThreads;
                char* arg = argv[2];
                n = atoi(arg);
                char* arg2 = argv[3];
                numThreads = atoi(arg2);
                printk("\n Bot running, num threads: %d\n", numThreads);
#ifdef USE_xtask

                xt->xtask_setup(numThreads, 0, 1);
#endif
                //printk("Size of task %zu size of parent %zu\n", sizeof(xtask_task), sizeof(xtask_parent));

                ticks start_tick = (ticks) 0;
                ticks end_tick = (ticks) 0;

                ticks diff_tick = (ticks) 0;
                start_tick = getticks1();
                //            clock_gettime(CLOCK_MONOTONIC, &ts);

                int* fib_output = (int*) malloc(sizeof(int));
                int totalTasksRun = 0;
#if defined USE_single
                *fib_output = fib(n);
#elif defined USE_openmp
#pragma omp parallel num_threads(numThreads)
                {
                    *fib_output = fib_openmp(n);

                }
#else

                  fibdata d = {n, fib_output};
                  xtask_task mainTask = {fib_xtask, &d, NULL, NULL};
                  xt->xtask_push(&mainTask);
                  xt->xtask_poll(&totalTasksRun);
#endif
                  end_tick = getticks1();
                  diff_tick = end_tick - start_tick;

                  double totalTime = (diff_tick * 1.0 * 1E-9) / clockFreq;

                  printk("Fib(%d) = %d\n", n, *fib_output);
                  DEBUG("%d %d %f %f\n", numThreads, totalTasksRun, totalTime, (totalTasksRun * 1.0 / totalTime));
            }
            break;
        case 2://Quick sort
        {
            if (argc != 4) {
                printk("Usage: <task type> <numThreads> <num elements>\n");
            }

            int type;
            type = atoi(argv[1]);
            int size = atoi(argv[3]);

            arr = (int*) malloc(size * sizeof (int));

            srandom(8472847);
            for (int i = 0; i < size; i++) arr[i] = random();
#ifdef DEBUG              
            for (int i = 0; i < size; i++)
                printk("%d ", arr[i]);

            printk("\n");
#endif
            int numThreads;
            char* arg2 = argv[2];
            numThreads = atoi(arg2);

            struct timespec tstart, tend;
            int totalTasksRun;
            
#ifdef USE_xtask

                xt->xtask_setup(numThreads, 0, 1);
#endif

              ticks start_tick = (ticks) 0;
              ticks end_tick = (ticks) 0;

              ticks diff_tick = (ticks) 0;
              start_tick = getticks1();
#if defined USE_single
            sort(0, size - 1);
#elif defined USE_openmp
#pragma omp parallel
            {
#pragma omp single
                {
                    sort(0, size - 1);
                }
            }
#else
            qdata sd = {0, size - 1};
            xtask_task mainTask = {sort_xtask, &sd, NULL, NULL};
            xt->xtask_push(&mainTask);
            xt->xtask_poll(&totalTasksRun);
#endif
              end_tick = getticks1();
              diff_tick = end_tick - start_tick;
 
              double totalTime = (diff_tick * 1E-9) / clockFreq;

              DEBUG(" Value %d %d %f %f\n", numThreads, totalTasksRun, totalTime, (totalTasksRun * 1.0 / totalTime));

#ifdef DEBUG
            for (int i = 0; i < size; i++)
                printk("%d ", arr[i]);

            printk("\n");
#endif
            for (int i = 1; i < size; i++)
                if (arr[i - 1] > arr[i]) {
                    printk("Sorting out of order!\n");
                    return 1;
                }

            free(arr);
        }
            break;
    }

    xt->xtask_cleanup();
    return 0;
}
