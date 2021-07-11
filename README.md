# iw5-script

Lua scripting support for Plutonium IW5

Works the same as it does in [IW6x](https://github.com/XLabsProject/iw6x-client/wiki/Scripting)

# How

* Download the latest version from the Releases tab
* Copy it to `%localappdata%/Plutonium/storage/iw5/plugins/`
* Create a `__init__.lua` file in a folder with a name of your choice in `%localappdata%/Plutonium/storage/iw5/scripts/`
* Example `%localappdata%/Plutonium/storage/iw5/scripts/myscript/__init__.lua`

Below are some features that are not available or documented in IW6x

# Chat notifies
```lua
level:onnotify("say", function(player, message)
    print(player.name .. " said: " .. message)
end)
```
or
```lua
level:onnotify("connected", function(player)
    player:onnotify("say", function(message)
        print(player.name .. " said: " .. message)
    end)
end)
```

Alternatively you can use the `game:onplayersay` function to handle player messages:

```lua
game:onplayersay(function(player, message, teamchat)
    if (message == "!test") then
        player:tell("Hello world!")
        return true -- Hide the message from chat
    end
end)
```

# Player damage/killed callbacks

Callbacks can be added using the `game:onplayerkilled` or `game:onplayerdamage` functions:

Damage can be changed by returning it

Returning anything other than a number will not do anything (must be an integer)

```lua
game:onplayerdamage(function(_self, inflictor, attacker, damage, dflags, mod, weapon, point, dir, hitloc)
    damage = 0

    return damage
end)
```
```lua
game:onplayerkilled(function(_self, inflictor, attacker, damage, mod, weapon, dir, hitloc, timeoffset, deathanimduration)
    print(attacker.name .. " killed " .. _self.name)
end)
```

# Arrays
GSC arrays are supported and can be accessed similarly to gsc:
```lua
local ents = game:getentarray()

for i = 1, #ents do
    print(ents[i])
end
```
You can also create new arrays to pass them to GSC functions:
```lua
local arr = array.new()
arr:push(6)
arr:push(1)
arr:push(3)
arr:push(7)
arr:push(2)
arr:push(9)

arr = game:scriptcall("maps/mp/_utility", "quickSort", arr)

for i = 1, #arr do
    print(arr[i])
end
```
List of all `array` methods:
| Name | Description |
| --- | --- |
| set(*int*/*string* key, value) | Set an array's value by index (integer) or by string, also works using the `[]` operator |
| get(*int*/*string* key) | Get an array's value by index (integer) or by string, also works using the `[]` operator |
| erase(*int*/*string* key) | Erase an array's value by index (integer) or by string, also works by setting the value to nil |
| size() | Returns array length |
| getkeys() | Returns a table containing all the array's keys |
| push(value) | Push a value to the array |
| pop() | Deletes the last element of the array and returns it |

# Structs
GSC structs are also supported similarly as the arrays.

To get an entity's struct use the `getstruct` method:
```lua
local levelstruct = level:getstruct()

levelstruct.inGracePeriod = 10000
```
Structs in other variables like arrays are automatically converted:
```lua
level:onnotify("connected", function(player)
    player:onnotify("spawned_player", function()
        player.pers.killstreaks[1].streakName = "ac130"
        player.pers.killstreaks[1].available = 1
    end)
end)
```

Note: you cannot create new struct fields but only modify or read existing ones, same thing for arrays

# Functions

You can call functions and methods within the game's gsc scripts using the 

`scriptcall(filename, function, ...)` method:
```lua
level:onnotify("connected", function(player)
    player:onnotify("spawned_player", function()
        local hudelem = player:scriptcall("maps/mp/gametypes/_hud_utils", "createFontString", 1)
        
        hudelem:scriptcall("maps/mp/gametypes/_hud_util", "setPoint", "CENTER", nil, 100, 100)
        hudelem.label = "&Hello world" -- "&Hello world" is the equivalent of doing &"Hello world" in GSC
    end)
end)
```
To 'include' functions from files like in gsc you can do a similar thing:
```lua
local functions = game:getfunctions("maps/mp/killstreaks/_killstreaks") -- Returns a table of functions

level:onnotify("connected", function(player)
    player:notifyonplayercommand("use", "+actionslot 6")

    player:onnotify("use", function()
        functions.giveKillstreak(player, "ac130", false, true, player, false)
    end)
end)
```
Or this:
```lua
game:include("maps/mp/killstreaks/_killstreaks")

level:onnotify("connected", function(player)
    player:notifyonplayercommand("use", "+actionslot 6")

    player:onnotify("use", function()
        -- Included function names are lowercase
        player:givekillstreak("ac130", false, true, player, false)
    end)
end)
```

Functions in variables such as structs or arrays will be automatically converted to a lua function.

The first argument must always be the entity to call the function on (level, player...)
```lua
local levelstruct = level:getstruct()

level:onnotify("connected", function(player)
    player:onnotify("spawned_player", function()
        levelstruct.killstreakFuncs["ac130"](player)
    end)
end)
```

Functions in variables can also be replaced with lua functions:

```lua
local levelstruct = level:getstruct()
local callbackPlayerDamage = levelstruct.callbackPlayerDamage -- Save the original function

-- The first argument (_self) is the entity the function is called on
levelstruct.callbackPlayerDamage = function(_self, inflictor, attacker, damage, flags, mod, weapon, point, dir, hitLoc, timeoffset)
    if (mod == "MOD_FALLING") then
        damage = 0
    end

    callbackPlayerDamage(_self, inflictor, attacker, damage, flags, mod, weapon, point, dir, hitLoc, timeoffset)
end

```

You can also detour existing functions in gsc scripts with lua functions:

```lua
local player_damage_hook = nil
player_damage_hook = game:detour("maps/mp/gametypes/_callbacksetup", "CodeCallback_PlayerDamage", 
    function(_self, inflictor, attacker, damage, flags, mod, weapon, point, dir, hitLoc, timeoffset)
        print(_self.name .. " got " .. damage .. " damage")

        -- Call original function
        player_damage_hook.invoke(_self, inflictor, attacker, damage, flags, mod, weapon, point, dir, hitLoc, timeoffset)
    end
)

player_damage_hook.disable() -- Disable hook
player_damage_hook.enable()  -- Enable hook
```
# HTTP

Http functions can be accessed from the global `http` table:


* `http.request(url[, options[, async]])`: Returns a table, you can replace the request's callbacks (`onprogress`, `onerror`, `onload`) with your own and send the request using the `.send()` method.

  The second parameter is the options table, which should contain two other tables for the headers and parameters for POST requests.
  
  The async parameter specifies wheter the request will be executed on the server thread or on a async thread. If you choose to execute it asynchronously then inside the callbacks you must wrap any calls to game functions with `game:ontimeout` in order to call them from the correct thread.

  ```lua
  local request = http.request("https://example.com", {
      headers = {
          ["Content-type"] = "text/html"
      },
      parameters = {
          foo = "bar"
      }
  }, true)
  
  request.onprogress = function(progress)
      print(progress)
  end
  
  request.onerror = function(error, code)
      print(error, code)
  end
  
  -- Request is done
  request.onload = function(data)
      print(data)
      
      game:ontimeout(function()
          game:iprintln("Done!")
      end, 0)
  end
   
  request.send()
  ```
* `http.get(url, callback[, async])`: This function is simpler than `http.request` and only takes the url and callback as parameters (and eventually the async parameter).
  ```lua
  http.get("https://example.com", function(data)
      print(data)
  end, true)
  ```
