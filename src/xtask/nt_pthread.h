#include <nautilus/thread.h>
#define pthread_join(thread_pointer,return_value) nk_join(thread_pointer,return_value)
#define pthread_cancel(thread_pointer) nk_thread_destroy(thread_pointer);
#define sched_yield() nk_yield();

void 
pthread_creat(nk_thread_id_t * thread_pointer, void *(*function_pointer) (void *, void**), void * attribute_pointer ,void * input, int core) 
{
    nk_thread_start((nk_thread_fun_t)function_pointer, input, NULL, 0, TSTACK_DEFAULT, thread_pointer, core);
}
