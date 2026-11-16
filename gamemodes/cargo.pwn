/*
 *  CARGO STATE – truck simulator за SA-MP / open.mp
 *
 *  Кодът е в UTF-8. scripts/build.sh го конвертира към Windows-1251
 *  преди компилация, за да се вижда кирилицата в играта.
 *
 *  Структура:
 *    core/     – настройки, цветове, данни на играча, база, команди, пароли
 *    modules/  – отделните системи (вход, пари, нива, HUD, чат, меню, GPS, админ)
 */

#include <open.mp>
#include <samp_bcrypt>
#include <streamer>

// Памет за стека/heap-а (в клетки по 4 байта = 256 KB) – за големите диалози и заявки.
#pragma dynamic 65536

#undef MAX_PLAYERS
#define MAX_PLAYERS 200

#include "core/config.inc"
#include "core/colors.inc"
#include "core/dialogs.inc"
#include "core/player.inc"
#include "core/utils.inc"
#include "core/database.inc"
#include "core/passwords.inc"
#include "core/commands.inc"

// Функции с таг (bool:), които се ползват преди дефиницията си
forward bool:Admin_Check(playerid, level);
forward bool:Admin_CheckBan(playerid);
forward bool:Veh_IsOut(playerid, slot);
forward bool:Veh_ReturnToGarage(playerid, slot);
forward bool:Garage_IsInside(playerid);

#include "maps/garage.inc"

#include "modules/money.inc"
#include "modules/levels.inc"
#include "modules/hud.inc"
#include "modules/catalog.inc"
#include "modules/cities.inc"
#include "modules/vehicles.inc"
#include "modules/garage.inc"
#include "modules/skin_select.inc"
#include "modules/auth.inc"
#include "modules/chat.inc"
#include "modules/gps.inc"
#include "modules/admin.inc"
#include "modules/menu.inc"

static gTickTimer;
static gHudTimer;
static gSecondsCounter;

main()
{
    print("CARGO STATE gamemode loaded.");
}

public OnGameModeInit()
{
    if (!Database_Init()) {
        return 0;
    }

    SetGameModeText("Truck Simulator");
    SetWorldTime(12);
    UsePlayerPedAnims();
    EnableStuntBonusForAll(false);
    DisableInteriorEnterExits();
    ShowPlayerMarkers(PLAYER_MARKERS_MODE_GLOBAL);
    SetNameTagDrawDistance(40.0);

    // Нужен е поне един клас, иначе клиентът не може да спаунне.
    AddPlayerClass(133, SPAWN_X, SPAWN_Y, SPAWN_Z, SPAWN_A);

    Garage_CreateMap();
    City_Init();
    Veh_Init();

    gTickTimer = SetTimer("Server_Tick", 1000, true);
    gHudTimer = SetTimer("Hud_Tick", 300, true);
    return 1;
}

public OnGameModeExit()
{
    KillTimer(gTickTimer);
    KillTimer(gHudTimer);
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (IsPlayerLogged(i)) Auth_SaveAccount(i);
    }
    Database_Close();
    return 1;
}

// Веднъж в секунда
forward Server_Tick();
public Server_Tick()
{
    gSecondsCounter++;
    new bool:autosave = (gSecondsCounter % AUTOSAVE_SECONDS == 0);

    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (!IsPlayerLogged(i) || !PlayerData[i][pSpawned]) continue;
        PlayerData[i][pPlayTime]++;
        Money_Sync(i);
        if (autosave) Auth_SaveAccount(i);
    }
    return 1;
}

public OnPlayerConnect(playerid)
{
    ResetPlayerData(playerid);
    Pass_OnPlayerConnect(playerid);
    Gps_Reset(playerid);
    SkinSelect_OnPlayerConnect(playerid);
    Veh_OnPlayerConnect(playerid);
    Garage_OnPlayerConnect(playerid);
    Auth_OnPlayerConnect(playerid);
    Hud_Create(playerid);
    SetPlayerColor(playerid, 0xF5A623FF);
    return 1;
}

public OnPlayerDisconnect(playerid, reason)
{
    #pragma unused reason
    Auth_SaveAccount(playerid);
    Veh_OnPlayerDisconnect(playerid);
    Admin_OnPlayerDisconnect(playerid);
    Hud_Destroy(playerid);
    ResetPlayerData(playerid);
    return 1;
}

public OnPlayerRequestClass(playerid, classid)
{
    #pragma unused classid
    Auth_OnPlayerRequestClass(playerid);
    return 1;
}

public OnPlayerRequestSpawn(playerid)
{
    // Спаунът става само след вход.
    return PlayerData[playerid][pLogged] ? 1 : 0;
}

public OnPlayerSpawn(playerid)
{
    if (!Auth_OnPlayerSpawn(playerid)) return 1;
    Garage_OnPlayerSpawn(playerid);
    Hud_Show(playerid);
    return 1;
}

public OnPlayerDeath(playerid, killerid, WEAPON:reason)
{
    #pragma unused killerid, reason
    Gps_Cancel(playerid);
    return 1;
}

public OnPlayerKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
{
    if (Garage_OnKeyStateChange(playerid, newkeys, oldkeys)) return 1;
    Menu_OnKeyStateChange(playerid, newkeys, oldkeys);
    return 1;
}

public OnPlayerText(playerid, text[])
{
    Chat_OnPlayerText(playerid, text);
    return 0; // винаги 0 – сами изпращаме съобщението
}

public OnPlayerCommandText(playerid, cmdtext[])
{
    return Commands_Process(playerid, cmdtext);
}

public OnDialogResponse(playerid, dialogid, response, listitem, inputtext[])
{
    if (Auth_OnDialogResponse(playerid, dialogid, response, listitem, inputtext)) return 1;
    if (!PlayerData[playerid][pLogged]) return 1;
    if (Menu_OnDialogResponse(playerid, dialogid, response, listitem, inputtext)) return 1;
    if (Gps_OnDialogResponse(playerid, dialogid, response, listitem)) return 1;
    if (Garage_OnDialogResponse(playerid, dialogid, response, listitem)) return 1;
    if (Admin_OnDialogResponse(playerid, dialogid, response, listitem, inputtext)) return 1;
    return 0;
}

public OnPlayerStateChange(playerid, PLAYER_STATE:newstate, PLAYER_STATE:oldstate)
{
    #pragma unused oldstate
    Veh_OnPlayerStateChange(playerid, newstate);
    return 1;
}

public OnVehicleSpawn(vehicleid)
{
    Veh_OnVehicleSpawn(vehicleid);
    return 1;
}

public OnPlayerClickPlayerTextDraw(playerid, PlayerText:playertextid)
{
    if (SkinSelect_OnClickPlayerTD(playerid, playertextid)) return 1;
    return 0;
}

public OnPlayerClickTextDraw(playerid, Text:clickedid)
{
    if (SkinSelect_OnClickTD(playerid, clickedid)) return 1;
    return 0;
}

public OnPlayerEnterCheckpoint(playerid)
{
    Gps_OnEnterCheckpoint(playerid);
    return 1;
}
