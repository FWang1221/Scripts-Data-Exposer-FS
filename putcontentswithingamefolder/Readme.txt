Installation:



Put the things in your Game folder (where sekiro.exe is, usually C:\Program Files (x86)\Steam\steamapps\common\Sekiro). Comes pre-packaged with Modengine1 (Katalash) and uses LazyLoader (Church Guard) to run.



Merging Instructions:



Put the things in your Game folder. Leave everything intact (important!) but divert your Modengine ini's modOverrideDirectory="\swordkiro" to whatever your mod is.

If your mod contains action\script\c0000.hks, open it and in function Initialize() add:

pcall(loadfile("swordkiro\\core.lua"))

so it'd probably look like:

function Initialize()
    collectgarbage("setstepmul", 100)
    g_isUpperAction = FALSE
    g_wallJumpCount = 0
    g_airSubAttackCount = 0
    g_airSpecialAttackCount = 0
    g_AddElectroCharge = FALSE
    g_EndSubWeaponChange = FALSE
    g_beforeMoveSpeedIndex = 0
    g_beforeMoveDirection = 0
    g_enableTransitSprint = FALSE
    g_beforeFireLand = -1
    g_beforeSubAttackType = 0
    g_beforeSpAttackNum = 0
    g_beforeItemAnmType = 0
    g_enableSpAttaclkJump = TRUE
    g_forceCrouch = FALSE
    g_autoAimTime = 1
    g_autoAimFlag = FALSE
    g_behaviorValidateOrderByStyle = {}
    g_addBehaviorActionValidateOrderByStyle = {}
    g_addBehaviorReactionValidateOrderByStyle = {}
    ValidateOrderTableInit()
    pcall(loadfile("swordkiro\\core.lua"))
end

or something.

If your mod doesn't have the action/script/c0000.hks then just move the action folder over to your new mod so your new mod can load the hks.



Usage:



In swordkiro\debug.lua there's the formula for editing how sharp the speed is. Remember that 1 frame is 1/60 so the normal step time is 0.0167. You may alter the formula at whim and simply save the file for the changes to take place.



Credits:

Script Exposer made by Eladidu. All of this is possible thanks to him.

Pointer for World Time discovered by Zullie the Witch. The pointer that makes the mod tick.

Code to take advantage of said pointers made by Tmsrise. Super nice code for me to steal.

Sekiro Superhot by f_wang. Some shitty Lua scripts and stuff.