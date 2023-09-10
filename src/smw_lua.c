#include "smw_lua.h"
#include "smw_wrappers.h"
#include "consts.h"
#include "smw_rtl.h"
#include "variables.h"
#include "assets/smw_assets.h"
#include "common_rtl.h"

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include "third_party/sds/sds.h"

#include "../third_party/Nuklear/nuklear.h"

#define NUM_KEYS 256

// Struct to hold the lua state and the mod names
struct lua_mod {
    lua_State *L;
    sds name;
};

struct lua_script {
    bool has_update;
};

lua_State *L;
sds scripts_with_update[3];
sds scripts_on_player_death[2];
sds scripts_on_player_respawn[2];
sds scripts_on_player_powerup[2];
sds scripts_on_misc_1up[2];
sds scripts_on_player_damage[2];
sds scripts_on_level_start[1];

bool lua_key_pressed[NUM_KEYS];

static void dumpstack (lua_State *L) {
  int top=lua_gettop(L);
  for (int i=1; i <= top; i++) {
    printf("%d\t%s\t", i, luaL_typename(L,i));
    switch (lua_type(L, i)) {
      case LUA_TNUMBER:
        printf("%g\n",lua_tonumber(L,i));
        break;
      case LUA_TSTRING:
        printf("%s\n",lua_tostring(L,i));
        break;
      case LUA_TBOOLEAN:
        printf("%s\n", (lua_toboolean(L, i) ? "true" : "false"));
        break;
      case LUA_TNIL:
        printf("%s\n", "nil");
        break;
      default:
        printf("%p\n",lua_topointer(L,i));
        break;
    }
  }
}

enum {EASY, HARD};
static int op = EASY;
static float value = 0.6f;
static int i =  20;

int Nuklear_SdlRenderer_Draw(lua_State *L) {
  if (nk_begin(nk, "Show", nk_rect(5, 0, 110, 110),
    NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE|NK_WINDOW_SCALABLE | NK_WINDOW_NO_SCROLLBAR)) {
    nk_layout_row_begin(nk, NK_STATIC, 30, 2);
    {
        nk_layout_row_push(nk, 40);
        nk_label(nk, "Mario Power Up:", NK_TEXT_LEFT);
        nk_layout_row_push(nk, 40);
        nk_label(nk, "255", NK_TEXT_LEFT);
    }
    nk_layout_row_end(nk);
  }
  nk_end(nk);
  return 0;
}

// SDL Functions
int get_sdl_window_size(lua_State *L) {
    int nargs = lua_gettop(L);
    if (nargs != 1) {
        return luaL_error(L, "get_sdl_window_size expects 0 arguments");
    }

    int width, height;
    SDL_GetWindowSize(g_window, &width, &height);

    lua_pushinteger(L, (lua_Integer)width);
    lua_pushinteger(L, (lua_Integer)height);

    return 2;
}

int set_sdl_window_size(lua_State *L) {
    int nargs = lua_gettop(L);
    if (nargs != 3) {
        return luaL_error(L, "set_sdl_window_size expects 2 arguments");
    }
    lua_Integer width = luaL_checkinteger(L, 2);
    lua_Integer height = luaL_checkinteger(L, 3);

    SDL_SetWindowSize(g_window, width, height);

    return 0;
}


int getPlayerCurrentLifeCount(lua_State *L) {
    lua_pushinteger(L, (lua_Integer)player_current_life_count);
    return 1;
}

int setPlayerCurrentLifeCount(lua_State *L) {
    int nargs = lua_gettop(L);
    if (nargs != 1) {
        return luaL_error(L, "setPlayerCurrentLifeCount expects 1 argument");
    }
    lua_Integer newValue = luaL_checkinteger(L, 1);
    player_current_life_count = (uint8_t)newValue;
    return 0;
}

int getPlayerCurrentState(lua_State *L) {
    lua_pushinteger(L, (lua_Integer)player_current_state);
    return 1;
}

int setPlayerCurrentState(lua_State *L) {
    int nargs = lua_gettop(L);
    if (nargs != 1) {
        return luaL_error(L, "setPlayerCurrentState expects 1 argument");
    }
    lua_Integer newValue = luaL_checkinteger(L, 1);
    player_current_state = (uint8_t)newValue;
    return 0;
}

int getPlayerOnScreenX(lua_State *L) {
    lua_pushinteger(L, (lua_Integer)player_on_screen_pos_x);
    return 1;
}

int getPlayerOnScreenY(lua_State *L) {
    lua_pushinteger(L, (lua_Integer)player_on_screen_pos_y);
    return 1;
}

int getPlayerHasYoshi(lua_State *L) {
    lua_pushboolean(L, (lua_Integer)player_riding_yoshi_flag);
    return 1;
}

// Power Ups
//TODO Add a switch in here to set any powerup
int givePlayerPowerUp(lua_State *L) {
    int nargs = lua_gettop(L);
    if (nargs != 1) {
        return luaL_error(L, "setPlayerCurrentLifeCount expects 1 argument");
    }
    lua_Integer powerup = luaL_checkinteger(L, 1);

    // TODO These need to be passing the right "k"
    switch (powerup) {
        case 0:
            SprXXX_PowerUps_GiveMarioMushroom(11);
            break;
        case 3:
            SprXXX_PowerUps_GiveMarioStar(11);
            break;
        case 4:
            SprXXX_PowerUps_GiveMarioCape(11);
            break;
        case 5:
            SprXXX_PowerUps_GiveMarioFire(11);
            break;
        case 6:
            SprXXX_PowerUps_GiveMario1Up(11);
            break;
        default:
            break;
    }

    return 0;
}

void smw_lua_init_player() {
    // Init the player table
    lua_newtable(L);
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "y");
    lua_setglobal(L, "player");
}

void smw_lua_init_powerups() {
    /*
    PowerUps = {
        Mushroom = 0,
        Star = 3,
        Cape = 4,
        Fire = 5,
        OneUp = 6,
    }
    */

    lua_newtable(L);

    lua_pushstring(L, "Mushroom");
    lua_pushinteger(L, 0);
    lua_settable(L, -3);

    lua_pushstring(L, "Star");
    lua_pushinteger(L, 3);
    lua_settable(L, -3);

    lua_pushstring(L, "Cape");
    lua_pushinteger(L, 4);
    lua_settable(L, -3);

    lua_pushstring(L, "Fire");
    lua_pushinteger(L, 5);
    lua_settable(L, -3);

    lua_pushstring(L, "OneUp");
    lua_pushinteger(L, 6);
    lua_settable(L, -3);

    lua_setglobal(L, "PowerUps");
}

void smw_lua_bind_sdl() {
    // Send enums to lua with the SDL keys

    // Setup the variable that has the SDL key that is pressed
    lua_newtable(L);
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "pressed_key");

    lua_pushcfunction(L, get_sdl_window_size);
    lua_setfield(L, -2, "get_window_size");

    lua_pushcfunction(L, set_sdl_window_size);
    lua_setfield(L, -2, "set_window_size");

    lua_setglobal(L, "sdl");
    DEBUG_PRINT("Set pressed_sdl_key to 0\n");
}

void lua_handle_input(uint32_t key, bool pressed) {
    if (key >= NUM_KEYS)
        return;
    lua_key_pressed[key] = pressed;
    
    // Update the pressed SDL key
    if (!pressed) {
        lua_getglobal(L, "sdl");
        lua_pushinteger(L, (lua_Integer)0);
        lua_setfield(L, -2, "pressed_key");
    } else { 
        lua_getglobal(L, "sdl");
        lua_pushinteger(L, (lua_Integer)key);
        lua_setfield(L, -2, "pressed_key");
    }
}

void lua_nuklear_draw() {
    lua_getglobal(L, "draw_nuklear");
    if (!lua_isfunction(L, -1))
        return;

    if (lua_pcall(L, 0, 1, 0)) {
        fprintf(stderr, "Error calling function: %s\n", lua_tostring(L, -1));
        lua_close(L);
        return;
    }
}

void lua_update_player_pos() {
    // Update the player position
    lua_getglobal(L, "player");
    lua_pushinteger(L, (lua_Integer)player_on_screen_pos_x);
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, (lua_Integer)player_on_screen_pos_y);
    lua_setfield(L, -2, "y");
}

int lua_load_level(lua_State *L) {
    int nargs = lua_gettop(L);
    if (nargs != 1) {
        return luaL_error(L, "load_level expects 1 argument");
    }
    lua_Integer level = luaL_checkinteger(L, 1);

    level_loader = (int) level;
    misc_game_mode = 15;
    return 0;
}

int lua_nk_begin(lua_State *L) {
    int nargs = lua_gettop(L);
    if (nargs != 5) {
        return luaL_error(L, "nk_begin expects 3 arguments");
    }

    //TODO first arg is the table
    const char *name = luaL_checkstring(L, 2);
    
    struct nk_rect rect;
    if (lua_istable(L, 3)) {
        lua_getfield(L, 3, "x");
        lua_Integer x = luaL_checkinteger(L, -1);
        lua_getfield(L, 3, "y");
        lua_Integer y = luaL_checkinteger(L, -1);
        lua_getfield(L, 3, "w");
        lua_Integer w = luaL_checkinteger(L, -1);
        lua_getfield(L, 3, "h");
        lua_Integer h = luaL_checkinteger(L, -1);
        rect = nk_rect(x, y, w, h);
    }

    lua_Integer flags = luaL_checkinteger(L, 4);
    lua_Integer bounds = luaL_checkinteger(L, 5);

    bool ret = nk_begin(nk, name, rect, flags);
    lua_pushboolean(L, ret);

    return 1;
}

int lua_nk_end(lua_State *L) {
    int nargs = lua_gettop(L);
    if (nargs != 1) {
        return luaL_error(L, "nk_end expects 0 arguments");
    }

    nk_end(nk);

    return 0;
}

int lua_nk_layout_row_begin(lua_State *L) {
    int nargs = lua_gettop(L);
    if (nargs != 4) {
        return luaL_error(L, "nk_layout_row_begin expects 3 arguments");
    }
    lua_Integer layout_format = luaL_checkinteger(L, 2);
    lua_Integer row_height = luaL_checkinteger(L, 3);
    lua_Integer cols = luaL_checkinteger(L, 4);
    // lua_Integer ratio = luaL_checkinteger(L, 4); //TODO Come back to ratio

    nk_layout_row_begin(nk, layout_format, row_height, cols);
    // nk_layout_row_push(nk, ratio);

    return 0;
}

int lua_nk_layout_row_end(lua_State *L) {
    int nargs = lua_gettop(L);
    if (nargs != 1) {
        return luaL_error(L, "nk_layout_row_end expects 0 arguments");
    }

    nk_layout_row_end(nk);

    return 0;
}

int lua_nk_layout_row_push(lua_State *L) {
    int nargs = lua_gettop(L);
    if (nargs != 2) {
        return luaL_error(L, "nk_layout_row_push expects 1 argument");
    }
    lua_Integer ratio = luaL_checkinteger(L, 2);

    nk_layout_row_push(nk, ratio);

    return 0;
}

int lua_nk_label(lua_State *L) {
    int nargs = lua_gettop(L);
    if (nargs != 3) {
        return luaL_error(L, "nk_label expects 2 arguments");
    }
    const char *text = luaL_checkstring(L, 2);
    lua_Integer alignment = luaL_checkinteger(L, 3);

    nk_label(nk, text, alignment);

    return 0;
}

void smw_lua_set_variables() {
    // Player lives
    lua_pushcfunction(L, getPlayerCurrentLifeCount);
    lua_setglobal(L, "getPlayerCurrentLifeCount");
    lua_pushcfunction(L, setPlayerCurrentLifeCount);
    lua_setglobal(L, "setPlayerCurrentLifeCount");

    // Player state
    lua_pushcfunction(L, getPlayerCurrentState);
    lua_setglobal(L, "getPlayerCurrentState");

    // Player position
    lua_pushcfunction(L, getPlayerOnScreenX);
    lua_setglobal(L, "getPlayerOnScreenX");
    lua_pushcfunction(L, getPlayerOnScreenY);
    lua_setglobal(L, "getPlayerOnScreenY");

    // Player has yoshi
    lua_pushcfunction(L, getPlayerHasYoshi);
    lua_setglobal(L, "getPlayerHasYoshi");

    // Power Ups
    lua_pushcfunction(L, givePlayerPowerUp);
    lua_setglobal(L, "givePlayerPowerUp");

    lua_pushcfunction(L, lua_load_script);
    lua_setglobal(L, "dep");

    lua_pushcfunction(L, lua_game_reset);
    lua_setglobal(L, "reset");

    lua_pushcfunction(L, lua_scripts_reload);
    lua_setglobal(L, "reload");

    lua_pushcfunction(L, lua_load_level);
    lua_setglobal(L, "load_level");

    lua_pushcfunction(L, DamagePlayer_Hurt_wrapper);
    lua_setglobal(L, "damage_player");

    lua_pushcfunction(L, DamagePlayer_Kill_wrapper);
    lua_setglobal(L, "kill_player");

    // Nuklear
    lua_newtable(L);
    lua_pushcfunction(L, Nuklear_SdlRenderer_Draw);
    lua_setfield(L, -2, "nuklear_demo");

    lua_pushcfunction(L, lua_nk_begin);
    lua_setfield(L, -2, "nk_begin");

    lua_pushcfunction(L, lua_nk_end);
    lua_setfield(L, -2, "nk_end");

    lua_pushcfunction(L, lua_nk_layout_row_begin);
    lua_setfield(L, -2, "nk_layout_row_begin");

    lua_pushcfunction(L, lua_nk_layout_row_push);
    lua_setfield(L, -2, "nk_layout_row_push");

    lua_pushcfunction(L, lua_nk_layout_row_end);
    lua_setfield(L, -2, "nk_layout_row_end");

    lua_pushcfunction(L, lua_nk_label);
    lua_setfield(L, -2, "nk_label");

    lua_setglobal(L, "nk");
}

void lua_init() {
    lua_getglobal(L, "init");
    if (!lua_isfunction(L, -1))
        return;

    if (lua_pcall(L, 0, 1, 0)) {
        fprintf(stderr, "Error calling function: %s\n", lua_tostring(L, -1));
        lua_close(L);
        return;
    }
}

int lua_scripts_reload() {
    smw_lua_init();
    return 0;
}

int lua_game_reset() {
    RtlReset(1);
    smw_lua_init();
    return 0;
}

int lua_load_script(lua_State *L) {
    // Get the script name
    int nargs = lua_gettop(L);
    if (nargs != 1) {
        return luaL_error(L, "load_script expects 1 argument");
    }
    const char *script_name = luaL_checkstring(L, 1);

    lua_newtable(L);
    lua_setglobal(L, "utils");
    luaL_dofile(L, script_name);

    // Check if the script has an update function
    lua_getglobal(L, "utils");
    lua_getfield(L, -1, "update");
    if (!lua_isfunction(L, -1))
        return 1;
    scripts_with_update[2] = sdsnew("utils");

    return 0;
}

void lua_update() {
    // Internal variables update
    lua_update_player_pos();

    // Scripts Update
    for(int i = 0; i < sizeof(scripts_with_update) / sizeof(scripts_with_update[0]); i++) {
        lua_getglobal(L, scripts_with_update[i]);
        lua_getfield(L, -1, "update");
        if (!lua_isfunction(L, -1))
            continue;
        if (lua_pcall(L, 0, 1, 0)) {
            fprintf(stderr, "Error calling function: %s\n", lua_tostring(L, -1));
            lua_close(L);
            return;
        }
    }
}

// Events
void lua_on_level_start(uint8 level) {
    for (int i = 0; i < sizeof(scripts_on_level_start) / sizeof(scripts_on_level_start[0]); i++) {
        lua_getglobal(L, scripts_on_level_start[i]);
        lua_getfield(L, -1, "on_level_load");
        if (!lua_isfunction(L, -1))
            continue;

        lua_pushvalue(L, -2);
        lua_pushinteger(L, (lua_Integer)level);

        DEBUG_PRINT("Calling on_level_start: %d\n", level);
        if (lua_pcall(L, 2, 1, 0)) {
            fprintf(stderr, "Error calling function: %s\n", lua_tostring(L, -1));
            lua_close(L);
            return;
        }
    }
}

bool lua_on_player_damage() {
    bool ret = true;
    for (int i = 0; i < sizeof(scripts_on_player_damage) / sizeof(scripts_on_player_damage[0]); i++) {
        lua_getglobal(L, scripts_on_player_damage[i]);
        lua_getfield(L, -1, "on_player_damage");
        if (!lua_isfunction(L, -1))
            continue;

        if (lua_pcall(L, 0, 1, 0))
            return ret;

        if (lua_isnoneornil(L, -1)) {} 
        else if (lua_isboolean(L, -1)) {
            ret = lua_toboolean(L, -1);
        }
    }
    return ret;
}

void lua_on_misc_1up() {
    for (int i = 0; i < sizeof(scripts_on_misc_1up) / sizeof(scripts_on_misc_1up[0]); i++) {
        lua_getglobal(L, scripts_on_misc_1up[i]);
        lua_getfield(L, -1, "on_misc_1up");
        if (!lua_isfunction(L, -1))
            continue;

        if (lua_pcall(L, 0, 1, 0)) {
            fprintf(stderr, "Error calling function: %s\n", lua_tostring(L, -1));
            lua_close(L);
            return;
        }
    }
}

bool lua_on_player_powerup(int powerup) {
    bool ret = true;
    for (int i = 0; i < sizeof(scripts_on_player_powerup) / sizeof(scripts_on_player_powerup[0]); i++) {
        lua_getglobal(L, scripts_on_player_powerup[i]);
        lua_getfield(L, -1, "on_player_powerup");
        if (!lua_isfunction(L, -1))
            continue;

        lua_pushvalue(L, -2);
        lua_pushinteger(L, (lua_Integer)powerup);

        DEBUG_PRINT("Calling on_player_powerup: %d\n", powerup);
        if (lua_pcall(L, 2, 1, 0)) {
            return ret;
        }
        if (lua_isnoneornil(L, -1)) {} 
        else if (lua_isboolean(L, -1)) {
            ret = lua_toboolean(L, -1);
        }
    }
    return ret;
}

void lua_on_player_death() {
    for (int i = 0; i < sizeof(scripts_on_player_death) / sizeof(scripts_on_player_death[0]); i++) {
        lua_getglobal(L, scripts_on_player_death[i]);
        lua_getfield(L, -1, "on_player_death");
        if (!lua_isfunction(L, -1))
            continue;

        if (lua_pcall(L, 0, 1, 0)) {
            fprintf(stderr, "Error calling function: %s\n", lua_tostring(L, -1));
            lua_close(L);
            return;
        }
    }
}

void lua_on_player_respawn() {
    for(int i = 0; i < sizeof(scripts_on_player_respawn) / sizeof(scripts_on_player_respawn[0]); i++) {
        lua_getglobal(L, scripts_on_player_respawn[i]);
        lua_getfield(L, -1, "on_player_respawn");
        if (!lua_isfunction(L, -1))
            continue;
        if (lua_pcall(L, 0, 1, 0)) {
            fprintf(stderr, "Error calling function: %s\n", lua_tostring(L, -1));
            lua_close(L);
            return;
        }
    }
}

void lua_cleanup() {
    lua_close(L);
}

void smw_lua_init() {
    level_loader = -1;

    L = luaL_newstate();
    luaL_openlibs(L);

    smw_lua_set_variables();

    // TODO Dynamically init these
    scripts_with_update[0] = sdsnew("first");
    scripts_with_update[1] = sdsnew("second");

    scripts_on_player_death[0] = sdsnew("first");
    scripts_on_player_death[1] = sdsnew("second");

    scripts_on_player_respawn[0] = sdsnew("first");
    scripts_on_player_respawn[1] = sdsnew("second");

    scripts_on_player_powerup[0] = sdsnew("first");
    scripts_on_player_powerup[1] = sdsnew("second");

    scripts_on_misc_1up[0] = sdsnew("first");
    scripts_on_misc_1up[1] = sdsnew("second");

    scripts_on_player_damage[0] = sdsnew("first");
    scripts_on_player_damage[1] = sdsnew("second");

    scripts_on_level_start[0] = sdsnew("first");

    // Load all scripts from the scripts directory
    // Subfolders are inidividual mods
    struct dirent *entry;
    DIR *mod_folder;

    mod_folder = opendir("./scripts");
    if (mod_folder == NULL) {
        fprintf(stderr, "Error opening scripts folder\n");
        lua_close(L);
        return;
    }

    int mod_count = 0;
    while((entry = readdir(mod_folder)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        mod_count++;
    }

    char *mod_names[mod_count];
    rewinddir(mod_folder);
    int i = 0;
    while((entry = readdir(mod_folder)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        mod_names[i] = entry->d_name;
        i++;
    }

    // Load the main lua file for each mod
    // TODO Callback when a level is loaded to load the level lua file
    for (int i = 0; i < mod_count; i++) {
        printf("Loading mod: %s...\n", mod_names[i]);
        sds mod_path;
        mod_path = sdsnew("./scripts/");
        mod_path = sdscatprintf(mod_path, "%s/main.lua", mod_names[i]);
        
        lua_newtable(L);
        lua_setglobal(L, mod_names[i]);

        if (luaL_dofile(L, mod_path)) {
            fprintf(stderr, "Error loading mod: %s\n", lua_tostring(L, -1));
            lua_close(L);
            return;
        }
    }

    printf("Loaded %d mods\n", mod_count);
    closedir(mod_folder);
    smw_lua_bind_sdl();
    smw_lua_init_player();
    smw_lua_init_powerups();

    //TODO Init needs to done for every file that gets loaded
    lua_init();
}
