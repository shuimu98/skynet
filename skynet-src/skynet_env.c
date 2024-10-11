#include "skynet.h"
#include "skynet_env.h"
#include "spinlock.h"

#include <lua.h>
#include <lauxlib.h>

#include <assert.h>
#include <stdlib.h>
#include <time.h>

struct skynet_env {
	struct spinlock lock;
	lua_State *L;
    uint32_t snowflake_starttime;
	int32_t snowflake_sequence;
	int64_t snowflake_lasttime;
};

static struct skynet_env *E = NULL;

const char * 
skynet_getenv(const char *key) {
	SPIN_LOCK(E)

	lua_State *L = E->L;
	
	lua_getglobal(L, key);
	const char * result = lua_tostring(L, -1);
	lua_pop(L, 1);

	SPIN_UNLOCK(E)

	return result;
}

void 
skynet_setenv(const char *key, const char *value) {
	SPIN_LOCK(E)
	
	lua_State *L = E->L;
	lua_getglobal(L, key);
	assert(lua_isnil(L, -1));
	lua_pop(L,1);
	lua_pushstring(L,value);
	lua_setglobal(L,key);

	SPIN_UNLOCK(E)
}

static uint64_t
current_timestamp() {
	struct timespec ti;
	clock_gettime(CLOCK_REALTIME, &ti);
	return ti.tv_sec * 1000 + ti.tv_nsec / 1000000;
}

static uint64_t
next_timestamp(uint64_t lastTimestamp) {
    uint64_t timestamp = current_timestamp();
    while (timestamp <= lastTimestamp) {
        timestamp = current_timestamp();
    }
    return timestamp;
}

int64_t
skynet_snowflake(int machine) {
    SPIN_LOCK(E)
	int64_t uuid = 0;
	int64_t curtime = current_timestamp();
	if (curtime < E->snowflake_lasttime) {
        printf("Clock moved backwards. Refusing to generate id.\n");
        uuid = -1;
    } else if (curtime == E->snowflake_lasttime) {
		// same millisecond
		E->snowflake_sequence = (E->snowflake_sequence + 1) & 0xfff;
		if (E->snowflake_sequence == 0){
			E->snowflake_lasttime = next_timestamp(E->snowflake_lasttime);
		}
	} else{
		E->snowflake_sequence = 0;
		E->snowflake_lasttime = curtime;
	}

	uuid = (curtime - E->snowflake_starttime) << 25 | machine << 10 | E->snowflake_sequence;
    SPIN_UNLOCK(E)
    return uuid;
}

void
skynet_env_init() {
	E = skynet_malloc(sizeof(*E));
	SPIN_INIT(E)
	E->L = luaL_newstate();

	// snowflake 开始时间
	lua_State *L = E->L;
	lua_getglobal(L, "snowflake_starttime");
	int64_t result = lua_tointeger(L, -1);
	lua_pop(L, 1);
	E->snowflake_starttime = result;
	E->snowflake_sequence = 0;
	E->snowflake_lasttime = 0;
}
