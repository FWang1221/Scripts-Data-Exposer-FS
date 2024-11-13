json = require("json")
--[[
function addNumTest(a, b)
    return a + b
end
thingy = {["AAA"] = "AAA"}
file = io.open("testthing.txt", "w")
file:write(json.encode(thingy))
file:close()]]

hasStarted = "false"

dynamicBackstepFuncActive = "false"

worldSpeed = "1"

playerSpeed = "1"

paramBackups = {}

function saveParamBackup(paramName, rowID, attributeName, backupDataValue)
    if paramBackups[paramName] == nil then
        paramBackups[paramName] = {}
    end
    if paramBackups[paramName][rowID] == nil then
        paramBackups[paramName][rowID] = {}
    end
    if paramBackups[paramName][rowID][attributeName] == nil then
        paramBackups[paramName][rowID][attributeName] = backupDataValue
    end
end