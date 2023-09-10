#ifndef SMW_LUA_H
#define SMW_LUA_H
#include <stdbool.h>
#include "funcs.h"
#include "SDL.h"
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
void lua_on_level_start(uint8 level);

void lua_handle_input(uint32_t key, bool pressed);
void smw_lua_set_variables();

void lua_nuklear_draw();

int lua_load_script(lua_State *L);

extern struct nk_context *nk;
extern SDL_Renderer *g_renderer;
extern SDL_Window *g_window;

#endif
