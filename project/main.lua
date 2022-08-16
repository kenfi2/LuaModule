Module = { isModule = true }

function Module:checkReload()
  local reload = false
  for file, info in pairs(self.files) do
    local fileTime = getFileTime(file)
    if info.lastEdit ~= fileTime then
      info.lastEdit = fileTime
      reload = true
    end
  end

  if reload then
    self.files = { }

    local env = setmetatable({ }, { __index = _ENV })
    for name, child in pairs(self.list) do
      g_modules.loadModule(tonumber(name) and self.name or name, child, env)
    end

    env.list = self.list
    env.name = self.name

    self = setmetatable(env, { __index = Module })
  end
end

g_modules = { }

function g_modules.loadFile(fileName, env)
  local file, value = loadfile(fileName, "bt", env)
  if not file then
    print(value)
    return false
  end

  local ret, result = pcall(file)
  if not ret then
    print(result)
  end

  if not env.files then
    env.files = { }
  end

  env.files[fileName] = { lastEdit = getFileTime(fileName) }

  return ret
end

function g_modules.loadModule(name, child, env)
  if type(child) == "table" then
    local _env = setmetatable({ }, { __index = env })
    for _name, _child in pairs(child) do
      g_modules.loadModule(tonumber(_name) and name or _name, _child, _env)
    end

    env[name] = _env
  elseif g_modules.loadFile(child, env) then
    print(string.format("[%s] %s loaded", name, child))
  end
end

function g_modules.loadModules(list)
  for name, mainModule in pairs(list) do
    if type(mainModule) == "table" then
      local env = setmetatable({ }, { __index = _ENV })
      for _name, child in pairs(mainModule) do
        g_modules.loadModule(tonumber(_name) and name or _name, child, env)
      end

      env.list = mainModule
      env.name = name

      g_modules[name] = setmetatable(env, { __index = Module })
    end
  end
end

string.split = function(str, sep)
	local res = {}
	for v in str:gmatch("([^" .. sep .. "]+)") do
		res[#res + 1] = v
	end
	return res
end

string.splitTrimmed = function(str, sep)
	local res = {}
	for v in str:gmatch("([^" .. sep .. "]+)") do
		res[#res + 1] = v:trim()
	end
	return res
end

string.trim = function(str)
	return str:match'^()%s*$' and '' or str:match'^%s*(.*%S)'
end

function init()
  print("Lua init system...")
  local files = getFiles("modules", ".lua")
  local list = { }
  for _, file in ipairs(files) do
    local t = file:splitTrimmed("\\")
    local moduleName = t[3]
    if not list[moduleName] then
      list[moduleName] = { }
    end

    local module = list[moduleName]
    local path = t[4]
    if not path:find(".lua") then
      for i = 4, #t do
        if not t[i]:find(".lua") then
          path = t[i]

          if not module[path] then
            module[path] = { }
          end

          module = module[path]
        end
      end
    end

    table.insert(module, file)
  end

  g_modules.loadModules(list)

  local checkModules
  checkModules = function()
    for _, module in pairs(g_modules) do
      if type(module) == "table" and module.isModule then
        module:checkReload()
      end
    end

    g_scheduler.addEvent(createSchedulerTask(5000, checkModules))
  end

  checkModules()
end

function main()
  local status, value = pcall(init)
  if not status then
    print(value)
  end
end

g_dispatcher.addTask(createTask(main))
