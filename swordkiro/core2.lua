json = require("json")

GAME_BASE = 0
CHR_INS_BASE = 1
TARGET_CHR_INS_BASE = 2
UNSIGNED_BYTE = 0
SIGNED_BYTE = 1
UNSIGNED_SHORT = 2
SIGNED_SHORT = 3
UNSIGNED_INT = 4
SIGNED_INT = 5
FLOAT = 6
BIT = 7
WritePointerChain = 10000
TraversePointerChain = 10000
--Print to this mod's console
--Use ExposePrint func
ExposeDebugPrint = 10001 --args <string>

g_debugMode = true

function debugPrint(v)
    if g_debugMode then
        exposePrint(tostring(v))
    end
end

print = debugPrint
function addHexStrings(hex1, hex2)
    -- Remove the "0x" prefix if present
    hex1 = hex1:gsub("^0x", "")
    hex2 = hex2:gsub("^0x", "")

    -- Make sure hex1 is the longer string
    if #hex2 > #hex1 then
        hex1, hex2 = hex2, hex1
    end

    -- Pad hex2 with leading zeros to match hex1 length
    hex2 = string.rep("0", #hex1 - #hex2) .. hex2

    local result = {}
    local carry = 0

    -- Iterate from the last digit to the first (right to left)
    for i = #hex1, 1, -1 do
        -- Convert each character to a number (base 16)
        local digit1 = tonumber(hex1:sub(i, i), 16)
        local digit2 = tonumber(hex2:sub(i, i), 16)

        -- Add the digits and the carry
        local sum = digit1 + digit2 + carry

        -- Determine the new digit and carry
        local new_digit = sum % 16
        carry = math.floor(sum / 16)

        -- Insert the new digit (in hex form) to the result
        table.insert(result, 1, string.format("%X", new_digit))
    end

    -- If there is a remaining carry, insert it at the start
    if carry > 0 then
        table.insert(result, 1, string.format("%X", carry))
    end

    -- Combine result digits into a final string and prepend "0x"
    return "0x" .. table.concat(result)
end
function numberToHexString( num ) -- tolerance is small numbers, below 2 billion
    num = tonumber( num )
    if num == 0 then return '0' end

   local neg = false
   if num < 0 then
       neg = true
       num = num * -1
   end

   local hexstr    = '0123456789ABCDEF'
   local result    = ''

   while num > 0 do
       local n = num % 16
       result      = string.sub( hexstr, n + 1, n + 1 ) .. result
       num         = math.floor( num / 16 )
   end

   if neg then
       result = '-' .. result
   end

   return "0x" .. result
end

function readPointer(address)
    if type(address) == "string" then
        if string.gmatch(address, "0x") then
            return readPointerFunc(address)
        else
            return readPointerFunc(
                addHexStrings(
                    getScannedAddressStatic(address), getProcessBase()
                )
            )
        end
    end
    print("Invalid type: " .. type(address))
    return nil
end

function readInteger(address)
    if type(address) == "string" then
        if string.gmatch(address, "0x") then
            return readIntegerFunc(address)
        else
            return readIntegerFunc(addHexStrings(getScannedAddressStatic(address), getProcessBase()))
        end
    end
    print("Invalid type: " .. type(address))
    return nil
end

function readSmallInteger(address)
    if type(address) == "string" then
        if string.gmatch(address, "0x") then
            return readSmallIntegerFunc(address)
        else
            return readSmallIntegerFunc(addHexStrings(getScannedAddressStatic(address), getProcessBase()))
        end
    end
    print("Invalid type: " .. type(address))
    return nil
end

function askQuestion(question)
    return getTextInput(question) 
end

function consolePositionAndSize(x, y, width, height)
    setConsolePosSize(x, y, width, height)
end

function bringConsoleToFront()
    focusConsole()
end

function bringConsoleToBack()
    minimizeConsole()
end
function inspectLocalVariables(level)
    local i = 1
    exposePrint("Inspecting local variables at stack level: " .. level)
    while true do
        local name, value = debug.getlocal(level, i)
        if not name then break end
        exposePrint("Local variable [" .. name .. "] = " .. tostring(value))
        i = i + 1
    end
end
function inspectFunctionInfo(level)
    -- Use 'nSluf' to retrieve various information about the function
    local info = debug.getinfo(level, "nSluf")
    
    if info then
        exposePrint("Function info at stack level " .. level .. ":")
        exposePrint("Name: " .. (info.name or "N/A"))
        exposePrint("Source: " .. (info.short_src or "N/A"))
        exposePrint("Current line: " .. (info.currentline or "N/A"))
        exposePrint("Defined in: " .. (info.linedefined or "N/A") .. " to " .. (info.lastlinedefined or "N/A"))
        exposePrint("Number of upvalues: " .. (info.nups or "N/A"))
    else
        exposePrint("No info available at stack level: " .. level)
    end
end
function printFunctionCode(level)
    local info = debug.getinfo(level, "S")

    if not info or not info.source or info.source:sub(1, 1) ~= "@" then
        exposePrint("Cannot retrieve source code for this function.")
        return
    end
    
    local fileName = info.source:sub(2)  -- Remove '@' at the beginning of the file name
    local linedefined = info.linedefined
    local lastlinedefined = info.lastlinedefined

    -- Attempt to open the file
    local file = io.open(fileName, "r")
    if not file then
        exposePrint("Unable to open source file: " .. fileName)
        return
    end

    exposePrint("Printing function code from file: " .. fileName)
    exposePrint("Lines " .. linedefined .. " to " .. lastlinedefined)

    -- Read file line by line and print the code within the specified range
    local lineNumber = 1
    for line in file:lines() do
        if lineNumber >= linedefined and lineNumber <= lastlinedefined then
            exposePrint("Line " .. lineNumber .. ": " .. line)
        end
        lineNumber = lineNumber + 1
        if lineNumber > lastlinedefined then break end
    end

    file:close()
end
-- Define the help message for each debug function
helpMessage = "You may run Lua code to debug. Use print(your thing here) to see what your code returns. g_debugMode turned to true allows for console logging and breakpoints.\n\nThe following debugger functions are available:"

-- Define the debugFuncBank which will contain all the debugger functions
debugFuncBank = {}

-- Define a helper function to add functions to debugFuncBank
function addDebugFunc(name, description, triggers, func)
    table.insert(debugFuncBank, {
        name = name,
        description = description,
        triggers = triggers,
        func = func
    })
end

-- Function to display the help message
function displayHelp()
    exposePrint(helpMessage)
    for _, debugFunc in ipairs(debugFuncBank) do
        local triggerList = table.concat(debugFunc.triggers, ", ")
        exposePrint("Command: " .. debugFunc.name .. " | Triggers: " .. triggerList .. " | Description: " .. debugFunc.description)
    end
end

-- Define the debug functions and add them to debugFuncBank
addDebugFunc("help", "Displays a list of helpful functions for the debugger.", {"help", "h", "helpme", "fuck", "shit"}, function() displayHelp() end)

contTriggers = {"cont", "exit", "stop"}

addDebugFunc("cont", "Continues the game without the console.", contTriggers, function() 
    bringConsoleToBack() 
end)

addDebugFunc("locals", "Prints out local variables at the function level that triggered the breakpoint.", {"locals", "getlocals", "local", "getlocal"}, function()
    inspectLocalVariables(5)  -- Level 5 is the function that triggered the breakpoint
end)

addDebugFunc("info", "Prints out function information at the function level that triggered the breakpoint.", {"info", "getinfo", "function info", "func info", "funcinfo"}, function()
    inspectFunctionInfo(5)
end)

addDebugFunc("code", "Prints out the code of the function that triggered the breakpoint.\n\nMay not be 100 percent accurate if the use of detours/hooks or less straightforwards ways of writing code were used.", {"code", "getcode"}, function()
    printFunctionCode(5)
end)

-- Function to handle user input and execute corresponding debug functions
function runDebugCommand(command)
    for _, debugFunc in ipairs(debugFuncBank) do
        for _, trigger in ipairs(debugFunc.triggers) do
            if command == trigger then
                debugFunc.func()
                return true
            end
        end
    end
    return false
end

-- Modified addCondiBreakpoint function that checks the debugFuncBank for valid commands
function addCondiBreakpoint(condi)
    if not condi then return end
    if not g_debugMode then return end

    local codeThing = ""
    consolePositionAndSize(0, 0, 1200, 800)
    bringConsoleToFront()

    while true do
        codeThing = askQuestion("What code should I run? Type some Lua code, \"cont\" to exit (no quotes), or \"help\" to get a list of helpful functions for the debugger (no quotes).")
        exposePrint("Running " .. codeThing)

        -- Check the debugFuncBank for a valid command
        if not runDebugCommand(codeThing) then
            -- If no valid command was found, try executing the input as Lua code
            local success, result = pcall(loadstring(codeThing))
            if not success then 
                exposePrint("Error: " .. tostring(result)) 
            else
                exposePrint("Result: " .. tostring(result))
            end
        end
        for _, trigger in ipairs(contTriggers) do
            if codeThing == trigger then
                return
            end
        end
    end
end


-- Function to create a detour
function createDetour(originalFunction, newFunction)
    -- Store the original function in the new function's environment
    local original = originalFunction
    -- Create a wrapper that calls the new function first, then the original function
    return function(...)
        -- Call the new function
        newFunction(...)
        -- Call the original function
        return original(...)
    end
end

function runDebugFile()
    pcall(loadfile("swordkiro\\debug.lua"))
end

Update = createDetour(Update, runDebugFile)

os.clock = function() return getOSClockLua() end

baseTickSpeed = 0.0166
currentTickSpeed = baseTickSpeed
lastTime = os.clock()
time = os.clock()
lastRHand = {}
nowRHand = {}

function pythagoreanDistance(x1, y1, z1, x2, y2, z2)
    return math.sqrt((x2 - x1) ^ 2 + (y2 - y1) ^ 2 + (z2 - z1) ^ 2)
end
function adjustForChronoWarp()
    return currentTickSpeed / baseTickSpeed
end
function chronoSpeed(distance, time)
    local speed = (distance / time) / adjustForChronoWarp()
    --exposePrint("Speed: " .. speed .. "\nDistance" .. distance .. "\nTime" .. time)
    return speed
end
function checkTimeBetweenTicks()
    lastTime = time
    time = os.clock()
    local timeBetweenTicks = time - lastTime
    return timeBetweenTicks
end
function checkRHandBetweenTicks()
    lastRHand = nowRHand
    nowRHand = GetRHandBoneCoords()
end
function GetRHandBoneCoords()
    local rightHandModelSpace = hkbGetOldBoneModelSpace("R_Hand")
    local rightHandTranslation = rightHandModelSpace:getTranslation()
    return rightHandTranslation
end
function currentSpeedFormula(speed)
    return baseTickSpeed + (speed / 1000) - 0.0083
end
function chronoMancy()
    local timeBetweenTicks = checkTimeBetweenTicks()
    checkRHandBetweenTicks()
    local distance = pythagoreanDistance(lastRHand[1], lastRHand[2], lastRHand[3], nowRHand[1], nowRHand[2], nowRHand[3])
    local speed = chronoSpeed(distance, timeBetweenTicks)
    currentTickSpeed = currentSpeedFormula(speed)
    setTimeStepSize(currentTickSpeed)
end
function chronoMancyWrapper()
    chronoMancy()
end
Update = createDetour(Update, chronoMancyWrapper)

function getPtr(baseName, ...)
    return traversePointerChain(10000, baseName, ...)
end

function setPtr(baseName, ...)
    return writePointerChain(10000, baseName, ...)
end


local playerData = {
    -- General Data
    ["getHP"] = function()
        return getPtr(SIGNED_INT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x18, 0x130)
    end,
    ["setHP"] = function(hp)
        setPtr(SIGNED_INT, hp, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x18, 0x130)
    end,
    ["getMaxHP"] = function()
        return getPtr(SIGNED_INT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x18, 0x134)
    end,
    ["setMaxHP"] = function(hp)
        setPtr(SIGNED_INT, hp, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x18, 0x134) -- doesn't work, use setBaseHP instead
    end,
    ["getBaseHP"] = function()
        return getPtr(SIGNED_INT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x18, 0x138)
    end,
    ["setBaseHP"] = function(hp)
        setPtr(SIGNED_INT, hp, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x18, 0x138)
    end,
    ["getPosture"] = function()
        return getPtr(SIGNED_INT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x18, 0x148)
    end,
    ["setPosture"] = function(hp)
        setPtr(SIGNED_INT, hp, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x18, 0x148)
    end,
    ["getMaxPosture"] = function()
        return getPtr(SIGNED_INT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x18, 0x14c)
    end,
    ["setMaxPosture"] = function(hp)
        setPtr(SIGNED_INT, hp, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x18, 0x14c) -- doesn't work, use setBasePosture instead
    end,
    ["getBasePosture"] = function()
        return getPtr(SIGNED_INT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x18, 0x150)
    end,
    ["setBasePosture"] = function(hp)
        setPtr(SIGNED_INT, hp, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x18, 0x150)
    end,
    ["getSen"] = function()
        return getPtr(SIGNED_INT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x2000, 0x7c)
    end,
    ["setSen"] = function(sen)
        setPtr(SIGNED_INT, sen, getScannedAddressStatic("WorldChrMan"), 0x88, 0x2000, 0x7c)
    end,
    ["getAttackPower"] = function()
        return getPtr(SIGNED_INT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x2000, 0x48)
    end,
    ["setAttackPower"] = function(atk)
        setPtr(SIGNED_INT, atk, getScannedAddressStatic("WorldChrMan"), 0x88, 0x2000, 0x48)
    end,
    ["getExperience"] = function()
        return getPtr(SIGNED_INT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x2000, 0x160)
    end,
    ["setExperience"] = function(exp)
        setPtr(SIGNED_INT, exp, getScannedAddressStatic("WorldChrMan"), 0x88, 0x2000, 0x160)
    end,
    ["getLevel"] = function()
        return getPtr(SIGNED_INT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x2000, 0x154)
    end,
    ["setLevel"] = function(exp)
        setPtr(SIGNED_INT, exp, getScannedAddressStatic("WorldChrMan"), 0x88, 0x2000, 0x154)
    end,
    -- Status Resistances
    ["getResistancePoison"] = function()
        return getPtr(SIGNED_INT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x20, 0x10)
    end,
    ["setResistancePoison"] = function(val)
        setPtr(SIGNED_INT, val, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x20, 0x10)
    end,
    ["getResistancePoisonMax"] = function()
        return getPtr(SIGNED_INT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x20, 0x24)
    end,
    ["setResistancePoisonMax"] = function(val)
        setPtr(SIGNED_INT, val, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x20, 0x24)
    end,
    ["getResistanceTerror"] = function()
        return getPtr(SIGNED_INT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x20, 0x14)
    end,
    ["setResistanceTerror"] = function(val)
        setPtr(SIGNED_INT, val, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x20, 0x14)
    end,
    ["getResistanceTerrorMax"] = function()
        return getPtr(SIGNED_INT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x20, 0x28)
    end,
    ["setResistanceTerrorMax"] = function(val)
        setPtr(SIGNED_INT, val, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x20, 0x28)
    end,
    ["getResistanceBurn"] = function()
        return getPtr(SIGNED_INT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x20, 0x18)
    end,
    ["setResistanceBurn"] = function(val)
        setPtr(SIGNED_INT, val, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x20, 0x18)
    end,
    ["getResistanceBurnMax"] = function()
        return getPtr(SIGNED_INT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x20, 0x2c)
    end,
    ["setResistanceBurnMax"] = function(val)
        setPtr(SIGNED_INT, val, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x20, 0x2c)
    end,
    ["getResistanceEnfeebled"] = function()
        return getPtr(SIGNED_INT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x20, 0x1c)
    end,
    ["setResistanceEnfeebled"] = function(val)
        setPtr(SIGNED_INT, val, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x20, 0x1c)
    end,
    ["getResistanceEnfeebledMax"] = function()
        return getPtr(SIGNED_INT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x20, 0x30)
    end,
    ["setResistanceEnfeebledMax"] = function(val)
        setPtr(SIGNED_INT, val, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x20, 0x30)
    end,
    ["getResistanceShock"] = function()
        return getPtr(SIGNED_INT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x20, 0x20)
    end,
    ["setResistanceShock"] = function(val)
        setPtr(SIGNED_INT, val, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x20, 0x20)
    end,
    ["getResistanceShockMax"] = function()
        return getPtr(SIGNED_INT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x20, 0x34)
    end,
    ["setResistanceShockMax"] = function(val)
        setPtr(SIGNED_INT, val, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x20, 0x34)
    end,
    -- Positional Data
    ["getX"] = function()
        return getPtr(SIGNED_INT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x68, 0x80)
    end,
    ["setX"] = function(val)
        setPtr(SIGNED_INT, val, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x68, 0x80)
    end,
    ["getY"] = function()
        return getPtr(SIGNED_INT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x68, 0x84)
    end,
    ["setY"] = function(val)
        setPtr(SIGNED_INT, val, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x68, 0x84)
    end,
    ["getZ"] = function()
        return getPtr(SIGNED_INT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x68, 0x88)
    end,
    ["setZ"] = function(val)
        setPtr(SIGNED_INT, val, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x68, 0x88)
    end,
    -- SPEED
    ["getAnimSpeed"] = function()
        return getPtr(FLOAT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x28, 0xd00)
    end,
    ["setAnimSpeed"] = function(val)
        setPtr(FLOAT, val, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x28, 0xd00)
    end,
    ["getAnimID"] = function() -- no setter, also not gonna do the cmsg name coz you can easily find that out by detouring execevent and shit
        return getPtr(SIGNED_INT, getScannedAddressStatic("WorldChrMan"), 0x88, 0x1ff8, 0x10, 0x20)
    end
    -- SpEffect (I'll do em later)
}

local gameData = {
    ["getPlayTime"] = function()
        return getPtr(SIGNED_INT, getScannedAddressStatic("GameData"), 0x9c)
    end,
    ["setPlayTime"] = function(val)
        setPtr(SIGNED_INT, val, getScannedAddressStatic("GameData"), 0x9c)
    end,
    ["getClearCount"] = function() -- ng+ level
        return getPtr(SIGNED_INT, getScannedAddressStatic("GameData"), 0x70)
    end,
    ["setClearCount"] = function(val)
        setPtr(SIGNED_INT, val, getScannedAddressStatic("GameData"), 0x70)
    end,
    ["getClearState"] = function() -- probably whether or not you win? idk
        return getPtr(SIGNED_INT, getScannedAddressStatic("GameData"), 0x74)
    end,
    ["setClearState"] = function(val)
        setPtr(SIGNED_INT, val, getScannedAddressStatic("GameData"), 0x74)
    end,
    ["getDeathNum"] = function() -- probably whether or not you win? idk
        return getPtr(SIGNED_INT, getScannedAddressStatic("GameData"), 0x90)
    end,
    ["setDeathNum"] = function(val)
        setPtr(SIGNED_INT, val, getScannedAddressStatic("GameData"), 0x90)
    end,
    ["getHardmodeUnlocked"] = function() -- charm
        return getPtr(SIGNED_INT, getScannedAddressStatic("GameData"), 0x60, 0x1a)
    end,
    ["setHardmodeUnlocked"] = function(val)
        setPtr(SIGNED_INT, val, getScannedAddressStatic("GameData"), 0x60, 0x1a)
    end

}
truncatedParamsData = true
paramUtils = {}
paramUtilsInit = false
paramsDataByName = {}
local thing1, thing2 = pcall(loadfile("swordkiro\\eladiduParams.lua"))

if not thing1 then
    print("Error: " .. tostring(thing2))
else
    print("Success on loading eladiduParams.lua, now making paramsDataByName...")
    local function escape_quotes(str)
        return str:gsub('"', '\\"')
    end
    local hasStartedStr = (secondEnvGetGlobalJSON("hasStarted"))
    if hasStartedStr == json.encode("false") then
        --paramUtils.initRowDetect()
        if truncatedParamsData then
            paramUtils.initRowDetectQuick()
        else
            -- you're gonna segfault if you do this
            paramUtils.initRowDetect()
        end
        paramUtilsInit = true
        secondEnvRunCode("hasStarted = \"true\"")
        local paramsLoaded = {}
        local ticker = 1
        for i, v in pairs(paramsDataByName) do
            if v.rows ~= nil then
                file = io.open("swordkiro\\temp\\" .. i .. ".lua", "w")
                file:write("paramsDataByName[\"" .. i .. "\"] = json.decode(\"" .. escape_quotes(json.encode(v)) .. "\")")
                file:close()
                paramsLoaded[tostring(ticker)] = i
                ticker = ticker + 1
            end
        end
        secondEnvRunCode("paramsLoaded = json.decode(\"" .. escape_quotes(json.encode(paramsLoaded)) .. "\")")
        print("paramsDataByName has been saved to files. This operation will repeat only once per exe launch.")
    else
        local paramsLoaded = json.decode(secondEnvGetGlobalJSON("paramsLoaded"))
        print("Params to load: " .. json.encode(paramsLoaded))
        for i, v in pairs(paramsLoaded) do
            print("Loading : " .. "swordkiro\\temp\\" .. v .. ".lua")
            local thing3, thing4 = pcall(loadfile("swordkiro\\temp\\" .. v .. ".lua"))
            if not thing3 then
                print("Error: " .. tostring(thing4))
            else
                print("Success on loading " .. v .. ".lua")
            end
        end
        paramUtilsInit = true
        print("paramUtils and paramsDataByName have been loaded from files.")
    end
    --print(json.encode(paramUtils.getParamRows("EquipParamGoods")))
end