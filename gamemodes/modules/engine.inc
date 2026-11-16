// Двигател: пали се и гаси с бутон 2 (KEY_SUBMISSION) или /engine.
// Светлините вървят заедно с двигателя.

#define ENGINE_MIN_HEALTH   300.0       // под това двигателят не пали

stock Engine_Init()
{
    ManualVehicleEngineAndLights();
}

stock bool:Engine_IsOn(vehicleid)
{
    new engine, lights, alarm, doors, bonnet, boot, objective;
    GetVehicleParamsEx(vehicleid, engine, lights, alarm, doors, bonnet, boot, objective);
    return (engine == 1);
}

stock Engine_Set(vehicleid, bool:on)
{
    SetVehicleParamsEx(vehicleid, on ? 1 : 0, on ? 1 : 0);
}

stock Engine_Toggle(playerid)
{
    new vehicleid = GetPlayerVehicleID(playerid);
    if (vehicleid == 0 || GetPlayerState(playerid) != PLAYER_STATE_DRIVER) return 0;

    if (Engine_IsOn(vehicleid)) {
        Engine_Set(vehicleid, false);
        GameTextForPlayer(playerid, "~r~ENGINE OFF", 1500, 4);
        return 1;
    }
    new Float:hp;
    GetVehicleHealth(vehicleid, hp);
    if (hp < ENGINE_MIN_HEALTH) {
        SendError(playerid, "Двигателят е твърде повреден и не пали.");
        return 1;
    }
    Engine_Set(vehicleid, true);
    GameTextForPlayer(playerid, "~g~ENGINE ON", 1500, 4);
    return 1;
}

stock Engine_OnKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
{
    if (!((newkeys & KEY_SUBMISSION) && !(oldkeys & KEY_SUBMISSION))) return 0;
    return Engine_Toggle(playerid);
}

// Подсказка при сядане зад волана, ако двигателят е изгасен
stock Engine_OnPlayerStateChange(playerid, PLAYER_STATE:newstate)
{
    if (newstate != PLAYER_STATE_DRIVER) return;
    new vehicleid = GetPlayerVehicleID(playerid);
    if (!Engine_IsOn(vehicleid)) {
        GameTextForPlayer(playerid, "~w~PRESS ~y~2 ~w~TO START ENGINE", 3000, 4);
    }
}

CMD:engine(playerid, params[])
{
    #pragma unused params
    if (!Engine_Toggle(playerid)) SendError(playerid, "Трябва да си шофьор.");
    return 1;
}
