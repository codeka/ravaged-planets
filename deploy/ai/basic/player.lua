
-- common.lua contains a bunch of helper methods and goodies
require ("common")

-- include our functions.lua and other LUA scripts which contains the definition of the classes/functions we'll
-- need for this AI
require ("functions")
require ("factory")

-- first thing we do is set ourselves ready
player:set_ready()

-- Enable some logging contexts so that we can see these messages in the chat log (for debugging).
--log.enable_context("attack")

-- set up the timer that runs while we're waiting for the game to start.
player:timer(10, joingame_waiting_for_start)

-- Bind to the "game_started" event so we know when, well, the game has started.
player:event("game_started", game_started)
