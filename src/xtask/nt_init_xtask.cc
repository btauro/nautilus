/*
#include "xtask.h"
*/
#include <nautilus/nautilus.h>
#include <nautilus/thread.h>
#include <nautilus/mm.h>
#include <vector>
#include "ConcurrentQueue.h"
#include "ConcurrentStack.h"
#include "common.h"
#include "xtask.h"

#ifndef NAUT_CONFIG_XTASK_RT_DEBUG
#undef DEBUG(fmt, args...)
#define DEBUG(fmt, args...)
#else
#define DEBUG(fmt, args...)    DEBUG_PRINT("XTASK: " fmt, ##args)
#endif

static void 
xtask_thread (void * in, void **out) 
{
    int argc = 4;
    char** argv = (char**) malloc(4 * sizeof(char*));

    for(int i = 0;i < 4;i++) {
        argv[i] = (char*) malloc(3);
    }

    argv[1][0] = '1';                  //type
    argv[1][1] = 0;
    argv[2][0] = '5';                 //N
    argv[2][1] =  0;
    argv[2][2] = 0;
    argv[3][0] = '1';                  //Number of threads
    argv[3][1] = 0;
    xtask_main(argc, argv); 
}


extern "C" int 
xtask_init(void) 
{

    nk_thread_id_t t;

    if (nk_thread_start(xtask_thread,
                NULL,
                0,
                0,
                TSTACK_4KB,
                &t,
                -1)) {
        printk("Could not create xtask main thread\n");
        return -1;
    }

    return 0;
}
