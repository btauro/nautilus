/*
#include "xtask.h"
*/
#include <nautilus/nautilus.h>
#include <vector>
#include <nautilus/mm.h>
#include "ConcurrentQueue.h"
#include "ConcurrentStack.h"
#include "common.h"
#include "xtask.h"
#define DEBUG(fmt, args...)    DEBUG_PRINT("MLX3: " fmt, ##args)
extern "C" void xtask(){
    printk(" Xtask Init \n");
    int argc = 4;
    char** argv = (char**) malloc(4 * sizeof(char*));
    for(int i = 0;i < 4;i++) {
        argv[i] = (char*) malloc(3);
    }
    argv[1][0] = '1';                  //type
    argv[1][1] = 0;
    argv[2][0] = '1';                 //N
    argv[2][1] = '0';
    argv[2][2] = 0;
    argv[3][0] = '1';                  //Number of threads
    argv[3][1] = 0;
    xtask_main(argc, argv); 
}
