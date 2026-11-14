/*
 *  HOTFIX ONLINE – главен файл на gamemode-а
 *
 *  Кодът е в UTF-8. scripts/build.sh го конвертира към Windows-1251
 *  преди компилация, за да се вижда кирилицата в играта.
 *
 *  Структура:
 *    core/     – настройки, цветове, данни на играча, база, помощни функции
 *    modules/  – отделните системи (вход, админи, чат, меню, нива, ...)
 */

#include <open.mp>
#include <streamer>

// Памет за стека/heap-а на скрипта (в клетки по 4 байта = 256 KB).
// Стандартните 16 KB не стигат за големите диалози и SQL заявки (напр. репортите).
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

#include "maps/hospital.inc"

#include "modules/money.inc"
#include "modules/afk.inc"
#include "modules/levels.inc"
#include "modules/hospital.inc"
#include "modules/promo.inc"
#include "modules/skin_select.inc"
#include "modules/admin.inc"
#include "modules/auth.inc"
#include "modules/chat.inc"
#include "modules/report.inc"
#include "modules/menu.inc"

new gTickCounter;

main()
{
    print("HOTFIX ONLINE gamemode loaded.");
}

public OnGameModeInit()
{
    if (!Database_Init()) {
        return 0;
    }
    Report_CreateTables();

    SetGameModeText("HOTFIX ONLINE");
    SetWeather(WORLD_WEATHER);
    SetWorldTime(WORLD_HOUR);
    UsePlayerPedAnims();
    EnableStuntBonusForAll(false);
    ShowPlayerMarkers(PLAYER_MARKERS_MODE_OFF);
    SetNameTagDrawDistance(20.0);

    // Нужен е поне един клас, иначе клиентът не може да спаунне.
    AddPlayerClass(26, SPAWN_X, SPAWN_Y, SPAWN_Z, SPAWN_A);

    Skin_CreateTextDraws();
    Hospital_Init();

    SetTimer("Server_Tick", 1000, true);
    return 1;
}

public OnGameModeExit()
{
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (IsPlayerConnected(i)) Auth_SaveAccount(i);
    }
    Database_Close();
    return 1;
}

forward Server_Tick();
public Server_Tick()
{
    gTickCounter++;

    Money_OnTick();
    Report_OnTick(gTickCounter);

    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (IsPlayerConnected(i) && PlayerData[i][pLogged]) {
            Afk_OnPlayerTick(i);
        }
    }

    // Всяка минута
    if (gTickCounter % 60 == 0) {
        for (new i = 0; i < MAX_PLAYERS; i++) {
            if (IsPlayerConnected(i) && PlayerData[i][pLogged]) {
                Levels_OnPlayerMinute(i);
            }
        }
    }

    if (gTickCounter % AUTOSAVE_SECONDS == 0) {
        for (new i = 0; i < MAX_PLAYERS; i++) {
            if (IsPlayerConnected(i)) Auth_SaveAccount(i);
        }
    }
    return 1;
}

public OnPlayerConnect(playerid)
{
    Auth_OnPlayerConnect(playerid);
    return 1;
}

public OnPlayerDisconnect(playerid, reason)
{
    #pragma unused reason
    Auth_SaveAccount(playerid);
    Skin_OnPlayerDisconnect(playerid);
    Admin_OnPlayerDisconnect(playerid);
    Afk_OnPlayerDisconnect(playerid);
    ResetPlayerData(playerid);
    return 1;
}

public OnPlayerRequestClass(playerid, classid)
{
    #pragma unused classid
    Auth_OnPlayerRequestClass(playerid);
    return 1;
}

public OnPlayerSpawn(playerid)
{
    Auth_OnPlayerSpawn(playerid);
    return 1;
}

public OnPlayerDeath(playerid, killerid, WEAPON:reason)
{
    #pragma unused killerid, reason
    Hospital_OnPlayerDeath(playerid);
    return 1;
}

public OnPlayerUpdate(playerid)
{
    Afk_OnPlayerUpdate(playerid);
    return 1;
}

public OnPlayerKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
{
    Afk_OnKeyStateChange(playerid);
    Menu_OnKeyStateChange(playerid, newkeys, oldkeys);
    return 1;
}

public OnPlayerText(playerid, text[])
{
    Chat_OnPlayerText(playerid, text);
    return 0; // винаги 0 – сами изпращаме RP съобщението
}

public OnPlayerCommandText(playerid, cmdtext[])
{
    return Commands_Process(playerid, cmdtext);
}

public OnDialogResponse(playerid, dialogid, response, listitem, inputtext[])
{
    if (Auth_OnDialogResponse(playerid, dialogid, response, listitem, inputtext)) return 1;
    if (Menu_OnDialogResponse(playerid, dialogid, response, listitem)) return 1;
    if (Admin_OnDialogResponse(playerid, dialogid, response, inputtext)) return 1;
    if (Report_OnDialogResponse(playerid, dialogid, response, listitem, inputtext)) return 1;
    return 0;
}

public OnPlayerPickUpPickup(playerid, pickupid)
{
    Hospital_OnPickUp(playerid, pickupid);
    return 1;
}

public OnPlayerClickTextDraw(playerid, Text:clickedid)
{
    if (Skin_OnClickTextDraw(playerid, clickedid)) return 1;
    return 0;
}
