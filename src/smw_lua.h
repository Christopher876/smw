#ifndef SMW_LUA_H
#define SMW_LUA_H
#include <stdbool.h>
#include "funcs.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define DEBUG 3
#if defined(DEBUG) && DEBUG > 0
#define DEBUG_PRINT(fmt, args...) fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt, \
    __FILE__, __LINE__, __func__, ##args)
#else
 #define DEBUG_PRINT(fmt, args...) /* Don't do anything in release builds */
#endif


void smw_lua_init();
int lua_game_reset();
int lua_scripts_reload();
void lua_update();
void lua_cleanup();
void lua_on_misc_1up();
bool lua_on_player_damage();
void lua_on_player_death();
void lua_on_player_respawn();
bool lua_on_player_powerup(int powerup);
void lua_send_sdl_pressed_key(uint32_t key);
void smw_lua_set_variables();

int lua_load_script(lua_State *L);

#endif
