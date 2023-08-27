#ifndef SMW_WRAPPERS
#define SMW_WRAPPERS

#include "smw_lua.h"
#include "funcs.h"

int DamagePlayer_Hurt_wrapper(){
    DamagePlayer_Hurt();
    return 0;
}

int DamagePlayer_Kill_wrapper() {
    DEBUG_PRINT("DamagePlayer_Kill_wrapper\n");
    DamagePlayer_Kill();
    return 0;
}

#endif