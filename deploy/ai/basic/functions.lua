
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
  log:debug(event_name .. ", setting up handlers")

  player:timer(2, check_attack)
end

-- This is called every few seconds to check whether it's time to attack the enemy. we look at how many idle units
-- we have and if there's enough, we'll launch the attack.
function check_attack()
  player:say("check attack")

  local units = player:find_units({ unit_type="factory", state="idle" })
  for _,u in ipairs(units) do
    log:debug("  unit: " .. u:get_kind() .. " owner: " .. u:get_player_no() .. " state: " .. u:get_state())
  end
  player:issue_order(units, { order="build", build_unit="simple-tank" })

  units = player:find_units({ unit_type="simple-tank", state="idle" })
  player:say("found " .. #units .. " idle tanks")
  if #units >= 3 then
    local enemy = player:find_units({ player=1 }) -- todo: find out our enemy player id?
    player:issue_order(units, { order="attack", target=enemy[1] })
  end

  player:timer(7, check_attack)
end
