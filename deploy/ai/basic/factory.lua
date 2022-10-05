
Factory = {}

Factory.__init = function(self)
  log:debug("Factory:__init "..self.state)
end

-- register the Factory class as the class to handle 'factory' units
player:register_unit("factory", Factory)
