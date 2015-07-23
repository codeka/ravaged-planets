#pragma once

#include <string>
#include <boost/filesystem.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

namespace fw {

/**
 * This is a helper class for registering classes with Lua. It is based on Lunar.
 *
 * \see http://lua-users.org/wiki/CppBindingWithLunar
 *
 * \example
 * class my_class {
 * public:
 *   static const char[] class_name;
 *   static const lua_registrar<my_class>::method_definition[] methods;
 *
 *   int method_a(lua_State *);
 *   int method_b(lua_State *);
 * }
 *
 * const char my_class::class_name[] = "MyClass";
 * lua_registrar<my_class>::method_definition[] my_class::methods = {
 *   {"method_a", my_class::method_a},
 *   {"method_b", my_class::method_b},
 *   {nullptr, nullptr}
 * };
 *
 * ...
 * lua_registrar<my_class>::register_with_constructor(state);
 * -- or --
 * lua_registrar<my_class>::register_static(state);
 */
template <typename T>
class lua_registrar {
private:
  /** Wrapper object for the registered class. */
  typedef struct { T *wrapped; } wrapper;
public:
  typedef int (T::*member_func_ptr)(lua_State *state);
  typedef struct { const char *name; member_func_ptr mfunc; } method_definition;

  /**
   * Registers the templated class with the given lua_State with a constructor such that you can create instances
   * of this class from your Lua code. For example:
   *
   * \example
   * -- Lua code:
   * local foo = class_name()
   * foo:some_method()
   */
  static void register_with_constructor(lua_State *state) {
    lua_newtable(state);
    int methods = lua_gettop(state);

    luaL_newmetatable(state, T::class_name);
    int metatable = lua_gettop(state);

    // store method table in globals so that scripts can add functions written in Lua.
    lua_pushglobaltable(state);
    lua_pushvalue(state, methods);
    set(state, -3, T::class_name);
    lua_pop(state, 1); // pop the global table.

    // hide metatable from Lua getmetatable()
    lua_pushvalue(state, metatable);
    lua_pushvalue(state, methods);
    set(state, metatable, "__metatable");

    lua_pushvalue(state, methods);
    set(state, metatable, "__index");

    lua_pushcfunction(state, tostring_T);
    set(state, metatable, "__tostring");

    lua_pushcfunction(state, gc_T);
    set(state, metatable, "__gc");

    lua_newtable(state);                // mt for method table
    lua_pushcfunction(state, new_T);
    lua_pushvalue(state, -1);           // dup new_T function
    set(state, methods, "new");         // add new_T to method table
    set(state, -3, "__call");           // mt.__call = new_T
    lua_setmetatable(state, methods);

    // fill method table with methods from class T
    for (method_definition *l = T::methods; l->name; l++) {
      lua_pushstring(state, l->name);
      lua_pushlightuserdata(state, (void*)l);
      lua_pushcclosure(state, thunk, 1);
      lua_settable(state, methods);
    }

    lua_pop(state, 2);  // drop metatable and method table
  }

  /**
   * Registers the templated class with the given lua_State with no constructor. The only way to access this class
   * from Lua code would be if it was returned from another C function. For example:
   *
   * \example
   * -- Lua code:
   * local foo = player:make_foo()
   * foo:some_method()
   */
  static void register_without_constructor(lua_State *state) {
    lua_newtable(state);
    int methods = lua_gettop(state);

    luaL_newmetatable(state, T::class_name);
    int metatable = lua_gettop(state);

    // store method table in globals so that scripts can add functions written in Lua.
    lua_pushglobaltable(state);
    lua_pushvalue(state, methods);
    set(state, -3, T::class_name);
    lua_pop(state, 1); // pop the global table.

    // hide metatable from Lua getmetatable()
    lua_pushvalue(state, metatable);
    lua_pushvalue(state, methods);
    set(state, metatable, "__metatable");

    lua_pushvalue(state, methods);
    set(state, metatable, "__index");

    lua_pushcfunction(state, tostring_T);
    set(state, metatable, "__tostring");

    lua_pushcfunction(state, gc_T);
    set(state, metatable, "__gc");

    lua_setmetatable(state, methods);

    // fill method table with methods from class T
    for (method_definition *l = T::methods; l->name; l++) {
      lua_pushstring(state, l->name);
      lua_pushlightuserdata(state, (void*)l);
      lua_pushcclosure(state, thunk, 1);
      lua_settable(state, methods);
    }

    lua_pop(state, 2);  // drop metatable and method table
  }

  /**
   * Registers the templated class as a static table. You can reference methods on the class directly through the
   * global table like so:
   *
   * \example
   * -- Lua code
   * class_name:some_method()
   */
  static void register_static(lua_State *state) {
    register_without_constructor(state);

    // create a new instance and set it as a global variable with the given class name.
    T *obj = new T(state);
    push(state, obj, true);
    lua_setglobal(state, T::class_name);
  }

  /** Call the named Lua method from userdata method table. */
  static int call(lua_State *state, const char *method, int nargs=0, int nresults=LUA_MULTRET, int errfunc=0) {
    int base = lua_gettop(state) - nargs;  // userdata index
    if (!luaL_checkudata(state, base, T::class_name)) {
      lua_settop(state, base-1);           // drop userdata and args
      lua_pushfstring(state, "not a valid %s userdata", T::class_name);
      return -1;
    }

    lua_pushstring(state, method);         // method name
    lua_gettable(state, base);             // get method from userdata
    if (lua_isnil(state, -1)) {            // no method?
      lua_settop(state, base-1);           // drop userdata and args
      lua_pushfstring(state, "%s missing method '%s'", T::class_name, method);
      return -1;
    }
    lua_insert(state, base);               // put method under userdata, args

    int status = lua_pcall(state, 1+nargs, nresults, errfunc);  // call method
    if (status) {
      const char *msg = lua_tostring(state, -1);
      if (msg == nullptr) msg = "(error with no message)";
      lua_pushfstring(state, "%s:%s status = %d\n%s", T::class_name, method, status, msg);
      lua_remove(state, base);             // remove old message
      return -1;
    }
    return lua_gettop(state) - base + 1;   // number of results
  }

  /** Push onto the Lua stack a userdata containing a pointer to T object. */
  static int push(lua_State *state, T *obj, bool gc=false) {
    if (!obj) {
      lua_pushnil(state);
      return 0;
    }
    luaL_getmetatable(state, T::class_name);  // lookup metatable in Lua registry
    if (lua_isnil(state, -1)) {
      luaL_error(state, "%s missing metatable", T::class_name);
    }
    int mt = lua_gettop(state);
    subtable(state, mt, "userdata", "v");
    wrapper *w = static_cast<wrapper *>(pushuserdata(state, obj, sizeof(wrapper)));
    if (w) {
      w->wrapped = obj;  // store pointer to object in userdata
      lua_pushvalue(state, mt);
      lua_setmetatable(state, -2);
      if (gc == false) {
        lua_checkstack(state, 3);
        subtable(state, mt, "do not trash", "k");
        lua_pushvalue(state, -2);
        lua_pushboolean(state, 1);
        lua_settable(state, -3);
        lua_pop(state, 1);
      }
    }
    lua_replace(state, mt);
    lua_settop(state, mt);
    return mt;  // index of userdata containing pointer to T object
  }

  /** Get userdata from Lua stack and return pointer to T object. */
  static T *check(lua_State *state, int narg) {
    wrapper *w = static_cast<wrapper*>(luaL_checkudata(state, narg, T::class_name));
    if(!w) {
      const char *msg = lua_pushfstring(state, "%s expected, got %s", T::class_name, luaL_typename(state, narg));
      luaL_argerror(state, narg, msg);
      return nullptr;
    }
    return w->wrapped;  // pointer to T object
  }

private:
  lua_registrar();  // hide default constructor

  // member function dispatcher
  static int thunk(lua_State *state) {
    // Stack has userdata, followed by method args
    T *obj = check(state, 1);  // get 'self', or if you prefer, 'this'
    lua_remove(state, 1);  // remove self so member function args start at index 1
    // get member function from upvalue
    method_definition *l = static_cast<method_definition*>(lua_touserdata(state, lua_upvalueindex(1)));
    return (obj->*(l->mfunc))(state);  // call member function
  }

  // create a new T object and push onto the Lua stack a userdata containing a pointer to T object
  static int new_T(lua_State *state) {
    lua_remove(state, 1);   // use classname:new(), instead of classname.new()
    T *obj = new T(state);  // call constructor for T objects
    push(state, obj, true); // gc_T will delete this object
    return 1;           // userdata containing pointer to T object
  }

  // garbage collection metamethod
  static int gc_T(lua_State *state) {
    if (luaL_getmetafield(state, 1, "do not trash")) {
      lua_pushvalue(state, 1);  // dup userdata
      lua_gettable(state, -2);
      if (!lua_isnil(state, -1)) return 0;  // do not delete object
    }
    wrapper *w = static_cast<wrapper*>(lua_touserdata(state, 1));
    T *obj = w->wrapped;
    if (obj) delete obj;  // call destructor for T objects
    return 0;
  }

  static int tostring_T (lua_State *state) {
    char buff[32];
    wrapper *w = static_cast<wrapper*>(lua_touserdata(state, 1));
    T *obj = w->wrapped;
    sprintf(buff, "%p", (void*)obj);
    lua_pushfstring(state, "%s (%s)", T::class_name, buff);

    return 1;
  }

  static void set(lua_State *state, int table_index, const char *key) {
    lua_pushstring(state, key);
    lua_insert(state, -2);  // swap value and key
    lua_settable(state, table_index);
  }

  static void weaktable(lua_State *state, const char *mode) {
    lua_newtable(state);
    lua_pushvalue(state, -1);  // table is its own metatable
    lua_setmetatable(state, -2);
    lua_pushliteral(state, "__mode");
    lua_pushstring(state, mode);
    lua_settable(state, -3);   // metatable.__mode = mode
  }

  static void subtable(lua_State *state, int tindex, const char *name, const char *mode) {
    lua_pushstring(state, name);
    lua_gettable(state, tindex);
    if (lua_isnil(state, -1)) {
      lua_pop(state, 1);
      lua_checkstack(state, 3);
      weaktable(state, mode);
      lua_pushstring(state, name);
      lua_pushvalue(state, -2);
      lua_settable(state, tindex);
    }
  }

  static void *pushuserdata(lua_State *state, void *key, size_t sz) {
    void *ud = nullptr;
    lua_pushlightuserdata(state, key);
    lua_gettable(state, -2);     // lookup[key]
    if (lua_isnil(state, -1)) {
      lua_pop(state, 1);         // drop nil
      lua_checkstack(state, 3);
      ud = lua_newuserdata(state, sz);  // create new userdata
      lua_pushlightuserdata(state, key);
      lua_pushvalue(state, -2);  // dup userdata
      lua_settable(state, -4);   // lookup[key] = userdata
    }
    return ud;
  }
};

/**
 * This class represents the Lua context. It contains the state for a given "instance" of Lua, and holds references
 * to all the global objects, registrations and so on.
 */
class lua_context: boost::noncopyable {
private:
  lua_State *_state;
  std::string _last_error;

  void setup_state();

public:
  lua_context();
  ~lua_context();

  /** Sets the search pattern that Lua will use when searching for scripts via {@code require()}. */
  void set_search_pattern(std::string const &pattern);

  /** Loads a .lua script (and executes it immediately - you should have set up the context ready to go) */
  bool load_script(boost::filesystem::path const &filename);

  lua_State *get_state() const {
    return _state;
  }

  /** If something returns an error, this'll return a string version of the last error that occurred. */
  inline std::string get_last_error() const {
    return _last_error;
  }
};

}
