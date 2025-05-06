#include "../include/core.h"

int main(void)
{
    Core c;
    core_init(&c,0,"/Users/adamnaghavi/Dropbox/src/c_code/core_plugin/build/plugins");
    core_run(&c);
    core_shutdown(&c);
    return 0;
}