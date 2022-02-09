//
// Created by 裴沛东 on 2022/2/9.
//

#ifndef COROUTINE_LIBCO_H
#define COROUTINE_LIBCO_H
struct co *co_start(const char *name, void (*func)(void *), void *arg);
void co_yield();
void co_wait(struct co *co);
#endif //COROUTINE_LIBCO_H
