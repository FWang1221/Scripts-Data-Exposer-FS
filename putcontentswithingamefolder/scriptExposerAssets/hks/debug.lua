
--print(json.encode(paramUtils.getParamRows("LockCamParam")["0"]))
--print(json.encode(paramsDataByName["LockCamParam"]))

--spAtkcategory
-- 100: whirlwind slash
-- 101: nighjar slash
-- 102: ichimonji
-- 103: dragon flash
-- 104: ashina cross
--paramUtils.setParamRowAttribute("LockCamParam", 0, "camDistTarget", 5)
--paramUtils.setParamRowAttribute("LockCamParam", 0, "chrOrgOffset_Y", 2)
--paramUtils.setParamRowAttribute("LockCamParam", 0, "camFovY", 40 + 30 * math.sin(os.clock() * 15))
--paramUtils.setParamRowAttribute("EquipParamWeapon", 5000, "spAtkcategory", 102)
--paramUtils.setParamRowAttribute("CameraParam", 0, "chrOrgOffsetX", 1)
--[[
paramUtils.initRowDetect()
local camDefaultAddress = paramUtils.getParamRowAddress("LockCamParam", 0)
local distOffset = numberToHexString(
    where(
        paramsDataByName["LockCamParam"].fields, "name", "camDistTarget"
    )[1]
.offset
)]]
--writeFloatFunc(addHexStrings(camDefaultAddress, distOffset), 11 + 10 * math.sin(os.clock()))
SetVariable("SwimMoveTwistAngle_Head", os.clock() * -2500)
--SetVariable("SwimMoveTwistAngle_Spine", os.clock() * 234)
--SetVariable("InternalOffset", math.sin(os.clock() * 3))
-- field type 9 is a bit
-- field type 7 is unicode string (wchar_t array)
-- field type 6 is string (char array)
-- field type 4 is a float
-- field type 2 is int (4 bytes)
-- field type 1 is a short (2 bytes)
-- field type 0 is a byte

-- arraysize is for arrays, bitOffset is number of bits to add, bitSize is how many bits to read/write

--playerData.setAnimSpeed(1)

--setWorldSpeed(1)
--print(env(1108, ACTION_ARM_SP_MOVE) > 5000)
--enableOrDisableDynamicParamsBackstep()
dynamicBackstep()
