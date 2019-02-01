#include <stdio.h>
#include "aco_assert_override.h"
#include "aco.h"
#ifdef ACO_USE_VALGRIND
#warning "ACO_USE_VALGRIND enabled"
#endif
#include "Coroutine.hpp"

#if 1
void co_fp0(void *p) {
    // Get co->arg. The caller of `aco_get_arg()` must be a non-main co.
    int *iretp = (int *)p;
    // Get current co. The caller of `aco_get_co()` must be a non-main co.
    aco_t* this_co = aco_get_co();
    int ct = *iretp;
    char stack[1024];
    while (ct > 0){

        printf(
            "co:%p save_stack:%p share_stack:%p yield_ct:%d %d\n",
            this_co, this_co->save_stack.ptr,
            this_co->share_stack->ptr, ct, *iretp
        );
        // Yield the execution of current co and resume the execution of
        // `co->main_co`. The caller of `aco_yield()` must be a non-main co.
        memset(stack, 0xff, sizeof(stack));
        Coroutine::yield();
        ct--;
    }

    printf(
        "co:%p save_stack:%p share_stack:%p co_exit() %d\n",
        this_co, this_co->save_stack.ptr,
        this_co->share_stack->ptr, *iretp
    );
}
#else
void co_fp0(void *p) {
    // Get co->arg. The caller of `aco_get_arg()` must be a non-main co.
    int *iretp = (int *)aco_get_arg();
    // Get current co. The caller of `aco_get_co()` must be a non-main co.
    aco_t* this_co = aco_get_co();
    int ct = *iretp;
    char stack[1024];
    MARK("%d", i);

    while (ct > 0){
        /*
        printf(
            "co:%p save_stack:%p share_stack:%p yield_ct:%d %d\n",
            this_co, this_co->save_stack.ptr,
            this_co->share_stack->ptr, ct, *iretp
        );*/
        // Yield the execution of current co and resume the execution of
        // `co->main_co`. The caller of `aco_yield()` must be a non-main co.
        memset(stack, 0xff, sizeof(stack));
        aco_yield();
        ct--;
    }
    /*
    printf(
        "co:%p save_stack:%p share_stack:%p co_exit() %d\n",
        this_co, this_co->save_stack.ptr,
        this_co->share_stack->ptr, *iretp
    );*/
    // In addition do the same as `aco_yield()`, `aco_exit()` also set
    // `co->is_end` to `1` thus to mark the `co` at the status of "END".
    aco_exit();
}
#endif
#define MARK(...) printf("%s %d]", __func__, __LINE__);printf(__VA_ARGS__);printf("\n");
#define CO_NUM 10
int main(int argc, char **argv)
{
#if 1
    CoroutineFactory<int> cr_factory;
    int arg[CO_NUM];
    int i;
    for (i = 0; i < CO_NUM; i++) {
        arg[i] = i;
        if (!cr_factory.cr_create(co_fp0, (void*)&arg[i], 0, i)) {
            MARK("%d", i);
        }
    }
    i = 0;

    while (!cr_factory.empty()) {
        do {
            Coroutine *cr = NULL;
            if (!cr_factory.cr_get(i, &cr)) {
                break;
            }

            if(!cr || cr->is_stop()){
                break;
            }
            // Start or continue the execution of `co`. The caller of this function
            // must be main_co.
            cr->resume();
            if (cr->is_stop()) {
                cr_factory.cr_remove(i);
            }
        } while (0);
        i++;
        if (i == CO_NUM) {
            i = 0;
        }
        //ct++;
    }
#else
    aco_thread_init(NULL);

     // Create a main coroutine whose "share stack" is the default stack
     // of the current thread. And it doesn't need any private save stack
     // since it is definitely a standalone coroutine (which coroutine
     // monopolizes it's share stack).
     aco_t* main_co = aco_create(NULL, NULL, 0, NULL, NULL);

     // Create a share stack with the default size of 2MB and also with a
     // read-only guard page for the detection of stack overflow.
     aco_share_stack_t* sstk = aco_share_stack_new(1024);

     // Create a non-main coroutine whose share stack is `sstk` and has a
     // default 64 bytes size private save stack. The entry function of the
     // coroutine is `co_fp0`. Set `co->arg` to the address of the int
     // variable `co_ct_arg_point_to_me`.
     int arg[CO_NUM];
     aco_t *co_hwnd[CO_NUM] = {NULL};
     int i;
     for (i = 0; i< CO_NUM; i++) {
         arg[i] = i;
         co_hwnd[i] = aco_create(main_co, sstk, 1024, co_fp0, &arg[i]);
     }

    i = 0;
    int alive = 0;
    time_t tp = time(NULL);
    while (time(NULL) - tp < 30) {
        aco_t *co = co_hwnd[i];
        do {
            if(!co || co->is_end){
                break;
            }
            // Start or continue the execution of `co`. The caller of this function
            // must be main_co.
            aco_resume(co);
            if (co->is_end) {
                aco_destroy(co);
                arg[i] += CO_NUM;
                co_hwnd[i] =  aco_create(main_co, sstk, 0, co_fp0, &arg[i]);
                alive++;
            }
        } while (0);
        i++;
        if (i == CO_NUM) {
            i = 0;
        }
        //ct++;
    }
    for (i = 0; i< CO_NUM; i++) {
        aco_destroy(co_hwnd[i]);
    }
    printf("alive %d\n", alive);
    // Destroy the share stack sstk.
    aco_share_stack_destroy(sstk);
    sstk = NULL;
    // Destroy the main_co.
    aco_destroy(main_co);
    main_co = NULL;
#endif
    return 0;
}
