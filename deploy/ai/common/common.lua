
-- This is the table where we define all of the different taunts we can say to the player. Each taunt is grouped by
-- "kind" which lets you choose an appropriate taunt based on the situation.
_taunts = {
  -- This taunt is appropriate when joining a game (that is, when
  -- we're sitting the "join game" window waiting for the game to start)
  join_game = {
    "Come on, what are you waiting for?",
    "I'm ready, what about you!?",
    "Hurry up!",
    "Bring it on!"
  }
}

-- this will get set to true once the game is up & running
is_game_started = false

-- "Says" a random taunt that we've defined in the global taunts table.
--   kind tells us which sub-table to get our taunt from (see the comments for the taunts table for a description
-- of each).
--   probability is used to decide whether to say the taunt at all. We choose a random number between 0 and 1 and
-- if that number is grater than probability, we won't say anything (so you can say taunt("unit_killed", 0.2) and
-- we'll say a "unit_killed" taunt 20% of the time).
function taunt(kind, probability)
  if (math.random() < probability) then
    local taunt_list = _taunts[kind]
    local msg = taunt_list[math.random(#taunt_list)]
    player:say(msg)
  end
end

_log_contexts = {}

-- Enables the given context that we'll log to the chat window
--function log.enable_context(context)
--  _log_contexts[context] = true
--end

-- Disables the given context from being logged to the chat window
--function log.disable_context(context)
--  _log_contexts[context] = false
--end

-- This is useful for debugging. You can selectively enable/disable a context in your setup, and then sprinkle
-- log.say() throughout your code. The message is printed in the game's chat log.
--function log.say(context, msg)
--  if (_log_contexts[context] == true) then
--    player:local_say(msg)
--  end
--end


-- This event is fired when the game starts. We just set the is_game_started flag
player:event("game_started", function()
  is_game_started = true
end)
