/*
 *  RP gamemode за open.mp (съвместим със SA-MP 0.3.7 и мобилни клиенти)
 *
 *  ВАЖНО: файловете се пишат в UTF-8, но SA-MP клиентът очаква Windows-1251.
 *  Не компилирай директно от Qawno! Използвай scripts/build.sh или
 *  scripts/build.ps1 – те конвертират кодировката преди компилация.
 */

#include <open.mp>

// По-малко слотове = по-малко излишни цикли. Трябва да съвпада с config.json.
#undef MAX_PLAYERS
#define MAX_PLAYERS 200

#include "core/config.inc"
#include "core/colors.inc"
#include "core/dialogs.inc"
#include "core/player_data.inc"
#include "core/utils.inc"
#include "core/database.inc"
#include "core/commands.inc"

#include "modules/economy.inc"
#include "modules/bans.inc"
#include "modules/vip.inc"
#include "modules/accounts.inc"
#include "modules/chat.inc"
#include "modules/shop.inc"
#include "modules/donations.inc"
#include "modules/admin.inc"
#include "modules/general.inc"

new gTickCounter;

main()
{
    print("------------------------------------------");
    print("  " SERVER_NAME " v" SERVER_VERSION " е зареден.");
    print("------------------------------------------");
}

public OnGameModeInit()
{
    if (!Database_Init()) {
        return 0;
    }

    SetGameModeText(SERVER_SHORT " v" SERVER_VERSION);
    UsePlayerPedAnims();
    EnableStuntBonusForAll(false);
    ShowPlayerMarkers(PLAYER_MARKERS_MODE_OFF);
    SetNameTagDrawDistance(20.0);

    // Нужен е поне един клас, иначе клиентът не може да спаунне.
    AddPlayerClass(DEFAULT_SKIN_MALE, SPAWN_X, SPAWN_Y, SPAWN_Z, SPAWN_A);

    // Един общ таймер на секунда – модулите си броят сами.
    SetTimer("Server_Tick", 1000, true);
    return 1;
}

public OnGameModeExit()
{
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (IsPlayerConnected(i)) {
            Accounts_Save(i, true);
        }
    }
    Database_Close();
    return 1;
}

forward Server_Tick();
public Server_Tick()
{
    gTickCounter++;

    Economy_OnTick();

    // Всяка минута
    if (gTickCounter % 60 == 0) {
        for (new i = 0; i < MAX_PLAYERS; i++) {
            if (IsPlayerConnected(i) && PlayerData[i][pLogged]) {
                Economy_OnPlayerMinute(i);
                Vip_CheckExpire(i);
            }
        }
    }

    if (gTickCounter % DONATION_CHECK_SECONDS == 0) {
        Donations_Process();
    }

    if (gTickCounter % AUTOSAVE_SECONDS == 0) {
        for (new i = 0; i < MAX_PLAYERS; i++) {
            if (IsPlayerConnected(i)) {
                Accounts_Save(i, true);
            }
        }
    }
    return 1;
}

public OnPlayerConnect(playerid)
{
    Accounts_OnPlayerConnect(playerid);
    return 1;
}

public OnPlayerDisconnect(playerid, reason)
{
    #pragma unused reason
    Accounts_Save(playerid, true);
    Admin_OnPlayerDisconnect(playerid);
    ResetPlayerData(playerid);
    return 1;
}

public OnPlayerRequestClass(playerid, classid)
{
    #pragma unused classid
    Accounts_OnPlayerRequestClass(playerid);
    return 1;
}

public OnPlayerSpawn(playerid)
{
    Accounts_OnPlayerSpawn(playerid);
    return 1;
}

public OnPlayerDeath(playerid, killerid, WEAPON:reason)
{
    #pragma unused killerid, reason
    Accounts_OnPlayerDeath(playerid);
    return 1;
}

public OnPlayerText(playerid, text[])
{
    Chat_OnPlayerText(playerid, text);
    return 0; // Винаги 0 – сами изпращаме RP съобщението.
}

public OnPlayerCommandText(playerid, cmdtext[])
{
    return Commands_Process(playerid, cmdtext);
}

public OnDialogResponse(playerid, dialogid, response, listitem, inputtext[])
{
    if (Accounts_OnDialogResponse(playerid, dialogid, response, listitem, inputtext)) return 1;
    if (Shop_OnDialogResponse(playerid, dialogid, response, listitem, inputtext)) return 1;
    return 0;
}
