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