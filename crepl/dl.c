#include <dlfcn.h>
#include <stdio.h>
#include <assert.h>

int (*dfunc)(void);

int main(){
    void *dhandle = dlopen("./tmpfile.so", RTLD_LAZY|RTLD_GLOBAL);
    assert(dhandle!=NULL);
    dfunc = dlsym(dhandle, "a");
    assert(dfunc!=NULL);
    int a = dfunc();
    printf("%d\n", a);
    dlclose(dhandle);
    return 0;
}
