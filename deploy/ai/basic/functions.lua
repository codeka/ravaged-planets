
-- This is called every couple of second while we're sitting in the "join game" window waiting for the game
-- to start. We'll just taunt the player randomly.
function joingame_waiting_for_start()
  -- if the game has already started, don't do anything
  if (is_game_started) then
    return
  end

  -- Taunt the players in the game, just for fun :)
  taunt("join_game", 1)

  -- Schedule ourselves to run again
  player:timer(math.random(10,20), joingame_waiting_for_start)
end

-- This is called when the game starts. we set up our "in-game" timers and event handlers and otherwise get the show
-- on the road
function game_started(event_name, params)
  debug:log("game started, setting up handlers")

  player:timer(2, check_attack)
end

-- This is called every few seconds to check whether it's time to attack the enemy. we look at how many idle units
-- we have and if there's enough, we'll launch the attack.
function check_attack()
  debug:say("attack", "check attack")

  local units = player:find_units({ unit_type="factory" })
  for _,u in ipairs(units) do
    debug:say("attack", "unit: " .. u.kind .. " owner: " .. u.player_no .. " state: " .. u.state)
  end
  player:issue_order(units, { order="build", build_unit="simple-tank" })

  player:timer(7, check_attack)
end