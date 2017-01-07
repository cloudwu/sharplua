#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define DLLEXPORT __declspec(dllexport)

// see sharplua.cs  ShapeLua.max_args
#define MAXRET 256
#define SUBMODNAME "sharplua.cscall"

enum var_type {
	NIL = 0,
	INTEGER = 1,
	INT64 = 2,
	REAL = 3,
	BOOLEAN = 4,
	STRING = 5,
	POINTER = 6,
	LUAOBJ = 7,
	SHARPOBJ = 8,
};

struct var {
	enum var_type type;
	int d;
	int64_t d64;
	double f;
	void * ptr;
};

static int func_proxy = 0;
static int func_object = 0;
static int func_garbage = 0;

struct string_pusher {
	lua_State *L;
	int done;
};

typedef int (*csharp_callback)(int argc, struct var *argv, struct string_pusher *sp);

static int
marshal_vars(lua_State *L, int n, struct var *v) {
	if (n > MAXRET) {
		v[0].type = STRING;
		v[0].ptr = (void *)("Too many results");
		return -1;
	}
	int base = lua_gettop(L) - n + 1;
	int marshal = 0;
	int i;
	for (i=0;i<n;i++) {
		switch(lua_type(L, base+i)) {
		case LUA_TNIL:
			v[i].type = NIL;
			break;
		case LUA_TBOOLEAN:
			v[i].type = BOOLEAN;
			v[i].d = lua_toboolean(L, base+i);
			break;
		case LUA_TNUMBER:
			if (lua_isinteger(L, base+i)) {
				lua_Integer n = lua_tointeger(L, base+i);
				int h = (int)(n>>33);
				if (h == 0 || h == -1) {
					v[i].type = INTEGER;
					v[i].d = (int)(n & 0xffffffff);
				} else {
					v[i].type = INT64;
					v[i].d64 = n;
				}
			} else {
				v[i].type = REAL;
				v[i].f = lua_tonumber(L, base+i);
			}
			break;
		case LUA_TSTRING:
			v[i].type = STRING;
			// todo: encoding
			v[i].ptr = (void*)lua_tostring(L, base+i);
			break;
		case LUA_TLIGHTUSERDATA:
			v[i].type = POINTER;
			v[i].ptr = lua_touserdata(L, base+i);
			break;
		default:
			// lua object
			if (marshal == 0) {
				lua_rawgetp(L, LUA_REGISTRYINDEX, &func_proxy);
				marshal = lua_gettop(L);
			}
			lua_pushvalue(L, marshal);
			lua_pushvalue(L, base+i);
			if (lua_pcall(L, 1, 2, 0) != LUA_OK) {
				v[i].type = STRING;
				v[i].ptr = (void *)lua_tostring(L, -1);
				return -1;
			}
			const char * t = lua_tostring(L, -2);
			if (t == NULL) {
				v[0].type = STRING;
				v[0].ptr = (void *)("Invalid proxy function");
			}
			v[i].d = lua_tointeger(L, -1);
			if (t[0] == 'S') {
				v[i].type = SHARPOBJ;
			} else {
				// t[0] == 'L'
				v[i].type = LUAOBJ;
			}
			lua_pop(L, 2);
			break;
		}
	}
	return n;
}

static int
toluaobject(lua_State *L, struct var *v, int object, int strc, const char **strs) {
	int t = v->type;
	switch(t) {
	case NIL:
		lua_pushnil(L);
		break;
	case INTEGER:
		lua_pushinteger(L, v->d);
		break;
	case INT64:
		lua_pushinteger(L, v->d64);
		break;
	case REAL:
		lua_pushnumber(L, v->f);
		break;
	case BOOLEAN:
		lua_pushboolean(L, v->d);
		break;
	case STRING:
		// todo: add short string cache
		if (strs) {
			if (v->d < 0 || v->d >= strc) {
				return luaL_error(L, "Invalid string id");
			}
			lua_pushstring(L, strs[v->d]);
		} else {
			lua_pushstring(L, (const char *)v->ptr);
		}
		break;
	case POINTER:
		lua_pushlightuserdata(L, v->ptr);
		break;
	case LUAOBJ:
	case SHARPOBJ:
		if (object == 0) {
			lua_rawgetp(L, LUA_REGISTRYINDEX, &func_object);
			lua_replace(L, 1);
			object = 1;
		}
		lua_pushvalue(L, 1);	// func_object
		lua_pushstring(L, (t == LUAOBJ) ? "L" : "S");
		lua_pushinteger(L, v->d);
		lua_call(L, 2, 1);
		break;
	default:
		return luaL_error(L, "Invalid type %d", v->type);
	}
	return object;
}

static int
Lpushstring(lua_State *L) {
	void *str = lua_touserdata(L, 1);
	lua_pushstring(L, (const char *)str);
	return 1;
}

DLLEXPORT int
c_pushstring(struct string_pusher *sp, const char * str) {
	lua_pushcfunction(sp->L, Lpushstring);
	lua_pushlightuserdata(sp->L, (void *)str);
	if (lua_pcall(sp->L, 1, 1, 0) != LUA_OK) {
		lua_pop(sp->L, 1);	// out of memory
		return 0;
	}
	sp->done = 1;
	return 1;
}

static int
Lsharpcall(lua_State *L) {
	if (lua_type(L, lua_upvalueindex(1)) != LUA_TLIGHTUSERDATA) {
		return luaL_error(L, "call c_callback first");
	}
	csharp_callback cb = (csharp_callback)lua_touserdata(L, lua_upvalueindex(1));
	struct var arg[MAXRET];
	int n = lua_gettop(L);
	int ok = marshal_vars(L, n , arg);
	if (ok == -1) {
		return luaL_error(L, "Marshal args error : %s", (const char *)arg[0].ptr);
	}
	struct string_pusher sp = { L, 0 };
	lua_pushnil(L);	// remain one slot
	int rt = cb(n, arg, &sp);
	if (rt == -1) {
		return luaL_error(L, "Raise error : %s", (const char *)arg[0].ptr);
	}
	if (sp.done) {
		// result string
		if (arg[0].type != STRING) {
			return luaL_error(L, "Invalid C sharp function returns string, the type is %s", arg[0].type);
		}
		arg[0].ptr = (void *)lua_tostring(L, -1);
	}
	toluaobject(L, &arg[0], 0, 0, NULL);

	return 1;
}

static int
lib_cscall(lua_State *L) {
	lua_pushnil(L);
	lua_pushcclosure(L, Lsharpcall, 1);
	return 1;
}

static int
Linit(lua_State *L) {
	const char *filename = lua_touserdata(L, 1);
	luaL_openlibs(L);
	luaL_requiref(L, SUBMODNAME, lib_cscall, 0);
	lua_pushvalue(L, 2);	// csharp_callback
	lua_setupvalue(L, -2, 1);
	lua_pop(L, 1);

	if (luaL_dofile(L, filename) != LUA_OK) {
		return lua_error(L);
	}
	if (lua_getglobal(L, "sharplua") != LUA_TTABLE) {
		return luaL_error(L, "Require sharplua");
	}
	if (lua_getfield(L, -1, "_proxy") != LUA_TFUNCTION) {
		return luaL_error(L, "Missing sharplua._proxy");
	}
	lua_rawsetp(L, LUA_REGISTRYINDEX, &func_proxy);

	if (lua_getfield(L, -1, "_object") != LUA_TFUNCTION) {
		return luaL_error(L, "Missing sharplua._object");
	}
	lua_rawsetp(L, LUA_REGISTRYINDEX, &func_object);

	if (lua_getfield(L, -1, "_garbage") != LUA_TFUNCTION) {
		return luaL_error(L, "Missing sharplua._garbage");
	}
	lua_rawsetp(L, LUA_REGISTRYINDEX, &func_garbage);

	return 0;
}

DLLEXPORT lua_State *
c_newvm(const char *filename, csharp_callback cb, void **err) {
	lua_State *L = luaL_newstate();
	if (L == NULL)
		return NULL;
	lua_pushcfunction(L, Linit);
	lua_pushlightuserdata(L, (void *)filename);
	lua_pushlightuserdata(L, (void *)cb);
	if (lua_pcall(L, 2, 0 ,0) != LUA_OK) {
		*err = (void *)lua_tostring(L, -1);
	} else {
		*err = NULL;
	}
	return L;
}

DLLEXPORT void
c_closevm(lua_State *L) {
	if (L) {
		lua_close(L);
	}
}

static int
Lgetfunction(lua_State *L) {
	lua_rawgetp(L, LUA_REGISTRYINDEX, &func_proxy);
	void * key = lua_touserdata(L, 1);
	int t = lua_getglobal(L, (const char *)key);
	if (t == LUA_TNIL) {
		lua_pushinteger(L, 0);
		return 1;
	}
	if (t != LUA_TFUNCTION && t != LUA_TTABLE && t != LUA_TUSERDATA) {
		return luaL_error(L, "Invalid type %s for global[%s]", lua_typename(L, lua_type(L, t)), (const char *)key);
	}
	lua_call(L, 1, 2);
	const char * objtype = lua_tostring(L, -2);
	if (objtype == NULL || objtype[0] != 'L') {
		return luaL_error(L, "Not a lua object");
	}
	return 1;
}

DLLEXPORT const char *
c_getglobal(lua_State *L, const char *str, int *id) {
	if (L == NULL) {
		*id = 0;
		return "L already closed";
	}
	lua_pushcfunction(L, Lgetfunction);
	lua_pushlightuserdata(L, (void *)str);
	if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
		const char * err = lua_tostring(L, -1);
		*id = 0;
		lua_pop(L, 1); // The error string must use immediately, pop doesn't trigger gc.
		return err;
	}
	*id = lua_tointeger(L, -1);
	lua_pop(L, 1);
	return NULL;
}

static int
Lmarshal(lua_State *L) {
	int argc = lua_tointeger(L, 1);
	struct var *argv = (struct var *)lua_touserdata(L, 2);
	int strc = lua_tointeger(L, 3);
	const char **strs = (const char **)lua_touserdata(L, 4);
	int i;
	int object = 0;
	luaL_checkstack(L, argc + 3, NULL);
	for (i=0;i<argc;i++) {
		struct var *v = &argv[i];
		object = toluaobject(L, v, object, strc, strs);
	}
	return argc;
}

static int 
Ltraceback (lua_State *L) {
	luaL_traceback(L, L, NULL, 0);
	return 1;
}

DLLEXPORT int
c_callfunction(lua_State *L, int argc, struct var *argv, int strc, const char **strs) {
	if (L == NULL) {
		argv->type = STRING;
		argv->ptr = (void *)("L already closed");
		return -1;
	}
	if (argc <= 0 || argv->type != LUAOBJ) {
		argv->type = STRING;
		argv->ptr = (void *)("Need Function");
		return -1;
	}
	if (!lua_checkstack(L, argc)) {
		argv->type = STRING;
		argv->ptr = (void *)("Out of memory");
		return -1;
	}
	lua_pushcfunction(L, Ltraceback);
	int top = lua_gettop(L);

	// marshal args from argv to L
	lua_pushcfunction(L, Lmarshal);
	lua_pushinteger(L, argc);
	lua_pushlightuserdata(L, (void *)argv);
	lua_pushinteger(L, strc);
	lua_pushlightuserdata(L, (void *)strs);
	if (lua_pcall(L, 4, argc, 0) != LUA_OK) {
		argv->type = STRING;
		argv->ptr = (void *)lua_tostring(L, -1);
		lua_settop(L, top);
		return -1;
	}
	if (lua_pcall(L, argc-1, LUA_MULTRET, top) != LUA_OK) {
		argv->type = STRING;
		argv->ptr = (void *)lua_tostring(L, -1);
		lua_settop(L, top);
		return -1;
	}
	if (!lua_checkstack(L, 3)) {
		argv->type = STRING;
		argv->ptr = (void *)("Out of memory when recv result");
		return -1;
	}
	int retn = lua_gettop(L) - top;
	int result = marshal_vars(L, retn, argv);
	lua_settop(L, top-1);	// remove luaL_traceback
	return result;
}

DLLEXPORT int
c_collectgarbage(lua_State *L, int n, int *result) {
	if (L == NULL) {
		return 0;
	}
	lua_rawgetp(L, LUA_REGISTRYINDEX, &func_garbage);
	int i;
	int ret = 0;
	for (i=0;i<n;i++) {
		lua_pushvalue(L, -1);
		if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
			lua_pop(L, 1);	// ignore error message
			break;
		}
		if (lua_isnil(L, -1)) {
			lua_pop(L, 1);
			break;
		}
		result[ret] = lua_tointeger(L, -1);
		lua_pop(L, 1);
		++ret;
	}
	lua_pop(L, 1);	// pop func_garbage
	return ret;
}
