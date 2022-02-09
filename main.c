#include <stdio.h>
#include "libco.h"

int count = 1; // 协程之间共享

void entry1(void *arg) {
    int i = 0;
    while(1) {
        printf("thread1 %d\n",i);
        ++i;
        co_yield();
    }
}

void entry2(void *arg) {
    int i = 1024;
    while(1) {
        printf("thread2 %d\n",i);
        ++i;
        co_yield();
    }
}

int main() {
    struct co *co1 = co_start("co1", entry1, "a");
    struct co *co2 = co_start("co2", entry2, "b");
    co_wait(co1);
    co_wait(co2);
    printf("Done\n");
}
