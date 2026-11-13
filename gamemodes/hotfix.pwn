/*
 *  HOTFIX ONLINE – главен файл на gamemode-а
 *
 *  Кодът е в UTF-8. scripts/build.sh го конвертира към Windows-1251
 *  преди компилация, за да се вижда кирилицата в играта.
 *
 *  Структура:
 *    core/     – настройки, цветове, данни на играча, база, помощни функции
 *    modules/  – отделните системи (вход/регистрация, избор на скин, ...)
 */

#include <open.mp>

#undef MAX_PLAYERS
#define MAX_PLAYERS 200

#include "core/config.inc"
#include "core/colors.inc"
#include "core/dialogs.inc"
#include "core/player.inc"
#include "core/utils.inc"
#include "core/database.inc"

#include "modules/promo.inc"
#include "modules/skin_select.inc"
#include "modules/auth.inc"

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

public OnPlayerText(playerid, text[])
{
    #pragma unused text
    if (!PlayerData[playerid][pLogged]) return 0;
    return 1;
}

public OnPlayerCommandText(playerid, cmdtext[])
{
    #pragma unused cmdtext
    if (!PlayerData[playerid][pLogged]) return 1;
    return 0;
}

public OnDialogResponse(playerid, dialogid, response, listitem, inputtext[])
{
    if (Auth_OnDialogResponse(playerid, dialogid, response, listitem, inputtext)) return 1;
    return 0;
}

public OnPlayerClickTextDraw(playerid, Text:clickedid)
{
    if (Skin_OnClickTextDraw(playerid, clickedid)) return 1;
    return 0;
}
