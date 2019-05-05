#include <common.h>

static void mem_test() {
    int base = 0x10000;
    while (1){
        int len = base * (rand()%5)+(rand()%0x100);
        char *str = pmm->alloc(len);
        if (str){
            for (int i =0;i<len;++i){
                str[i] = 'A'+i%24;
            }
            if (rand()%3 = =0)
                pmm->free(str);
        }
    }
}
