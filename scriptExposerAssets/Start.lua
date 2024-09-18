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