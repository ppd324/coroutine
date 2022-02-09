//
// Created by 裴沛东 on 2022/2/9.
//
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <stdint-gcc.h>
#include <time.h>
#include "libco.h"

#define STACK_SIZE 1024 * 64
int _co_count = 0;
struct co *current = NULL;
struct co *HEAD = NULL;
struct co *TAIL = NULL;
enum co_status {
    CO_NEW = 1, // 新创建，还未执行过
    CO_RUNNING, // 已经执行过
    CO_WAITING, // 在 co_wait 上等待
    CO_DEAD,    // 已经结束，但还未释放资源
};

typedef struct co {
    char *name;
    void (*func)(void *); // co_start 指定的入口地址和参数
    void *arg;

    enum co_status status;  // 协程的状态
    struct co *    waiter;  // 是否有其他协程在等待当前协程
    jmp_buf        context; // 寄存器现场 (setjmp.h)
    uint8_t        stack[STACK_SIZE]; // 协程的堆栈
    struct co * next;
}co;
void wrapper(void (*func)(void*), void* arg, co* this) {
    this->status = CO_RUNNING;
    ((void(*)())func) (arg);
    this->status = CO_DEAD;
    if (this->waiter) this->waiter->status = CO_RUNNING;
    co_yield();
}
void __attribute__((constructor)) start() {
    _co_count = 1;
    current = malloc(sizeof(co));
    HEAD = TAIL = current;
    current->next = HEAD;
    current->name = "main";
    current->status = CO_RUNNING;
    srand((int)time(0));
}
struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    struct co * task = (struct co *)malloc(sizeof(co));
    task->func = func;
    task->name = (char*)malloc(strlen(name) + 1);
    strcpy(task->name, name);
    task->arg = arg;
    task->status = CO_NEW;
    task->waiter = NULL;
    task->next = HEAD;
    HEAD = task;
    TAIL->next = HEAD;
    ++_co_count;
    return task;
}

struct co *schedule() {
    int ind = rand()%(_co_count);
    struct co *  temp = current->next;
    while((temp->status!=CO_RUNNING && temp->status!=CO_NEW) || ind > 0) {
        --ind;
        temp = temp->next;
    }
    return temp;

}
void  co_yield() {
    int val = setjmp(current->context); //将当前携程寄存器保存下来
    if (val == 0) {
        struct co *next = schedule();
        current = next;
        if(next->status && next->status == CO_RUNNING) {
            longjmp(next->context,1);
        }
        if(next->status && next->status == CO_NEW) {
#if __x86_64__
            asm (
            "movq %[sp], %%rsp\n"
            // "movq %[func], %%rdi\n"
            // "movq %[arg], %%rsi\n"
            // "movq %[cp], %%rdx\n"
            // "jmp *%[wrapper]\n"
            :
            :[sp]"r"((uintptr_t)next->stack+sizeof(next->stack))/*, [func]"r"(c->func), [arg]"r"(c->arg), [cp]"r"(c), [wrapper]"r"(wrapper)*/
            :"rdi", "rsi", "rdx"
            );
            wrapper(next->func, next->arg, next);
#else
            asm (
        "movl %[sp], %%esp\n"
        "movl %[func], 4(%[sp])\n"
        "movl %[arg], 8(%[sp])\n"
        "movl %[cp], 0xc(%[sp])\n"
        "jmp *%[wrapper]\n"
        :
        :[sp]"r"((uintptr_t)next->stack+sizeof(next->stack)-0x10), [func]"r"(next->func), [arg]"r"(next->arg), [cp]"r"(next), [wrapper]"r"(wrapper)
        :
        );
#endif

        }


    } else {
        return;
    }

}

void  co_wait(struct co *co) {
    if(co->status == CO_DEAD) {
        if(HEAD == co) {
            HEAD = co->next;
        }else {
            struct co* tmp = HEAD;
            while(tmp->next!=co) {
                tmp = tmp->next;
            }
            tmp->next = co->next;
            if (TAIL==co) TAIL = tmp;
        }
        free(co->name);
        free(co);
        --_co_count;
        return;
    }
    co->waiter = current;
    current->status = CO_WAITING;
    co_yield();

    if (HEAD==co) HEAD = co->next;
    else {
        struct co* tmp = HEAD;
        while(tmp->next!=co) {
            tmp = tmp->next;
        }
        tmp->next = co->next;
        if (TAIL==co) TAIL = tmp;
    }
    free(co->name);
    free(co);
    _co_count--;
    return;

}