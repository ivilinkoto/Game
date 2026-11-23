// PayDay: всеки кръгъл час (по часовника на сървъра) играчите, изиграли поне 20 минути
// от последния си PayDay, получават опит. Без жилище - социална помощ; с жилище - плащат данък.
// Изиграните минути се трупат: 10 мин в единия час + 10 мин в следващия също стигат.

#define PAYDAY_MIN_SECONDS      (20 * 60)
#define PAYDAY_EXP              25
#define PAYDAY_SOCIAL_AID       100

static PaydaySeconds[MAX_PLAYERS];
static PaydayLastHour = -1;

stock Payday_OnPlayerConnect(playerid)
{
    PaydaySeconds[playerid] = 0;
}

stock bool:Payday_HasHome(playerid)
{
    return House_HasHome(playerid);
}

// Вика се всяка секунда за всеки спаунат играч
stock Payday_PlayerTick(playerid)
{
    PaydaySeconds[playerid]++;
}

// Вика се всяка секунда веднъж - на кръгъл час раздава
stock Payday_Tick()
{
    new hour, minute, second;
    gettime(hour, minute, second);
    if (minute != 0 || hour == PaydayLastHour) return;
    PaydayLastHour = hour;
    Payday_RunAll();
}

stock Payday_RunAll()
{
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (!IsPlayerLogged(i) || !PlayerData[i][pSpawned]) continue;
        Payday_Give(i);
    }
}

static stock Payday_Give(playerid)
{
    if (PaydaySeconds[playerid] < PAYDAY_MIN_SECONDS) {
        SendClientMessage(playerid, COLOR_GREY, "[PAYDAY] " C_WHITE "Няма PayDay за теб този час: изиграл си %d от нужните %d минути.",
            PaydaySeconds[playerid] / 60, PAYDAY_MIN_SECONDS / 60);
        return;
    }
    PaydaySeconds[playerid] -= PAYDAY_MIN_SECONDS;
    if (PaydaySeconds[playerid] > PAYDAY_MIN_SECONDS) PaydaySeconds[playerid] = PAYDAY_MIN_SECONDS;

    PlayerPlaySound(playerid, 1054, 0.0, 0.0, 0.0);
    GameTextForPlayer(playerid, "~y~PAYDAY", 4000, 1);
    SendClientMessage(playerid, COLOR_BRAND, "[PAYDAY] " C_WHITE "Време е за заплата!");
    SendClientMessage(playerid, COLOR_BRAND, "[PAYDAY] " C_WHITE "Опит: " C_BRAND "+%d", PAYDAY_EXP);
    if (!Payday_HasHome(playerid)) {
        Money_Give(playerid, PAYDAY_SOCIAL_AID);
        SendClientMessage(playerid, COLOR_BRAND, "[PAYDAY] " C_WHITE "Социална помощ (нямаш жилище): " C_GREEN "$%d", PAYDAY_SOCIAL_AID);
    } else {
        House_ChargeTax(playerid);
    }
    Firm_PaySalary(playerid);
    Level_GiveExp(playerid, PAYDAY_EXP);
}

// За профила: "след 23 мин (изиграни 12/20 мин)"
stock Payday_Status(playerid, dest[], size = sizeof (dest))
{
    new hour, minute, second;
    gettime(hour, minute, second);
    new played = PaydaySeconds[playerid] / 60;
    if (played > PAYDAY_MIN_SECONDS / 60) played = PAYDAY_MIN_SECONDS / 60;
    format(dest, size, "след %d мин " C_GREY "(изиграни %d/%d мин)", 60 - minute, played, PAYDAY_MIN_SECONDS / 60);
}

CMD:payday(playerid, params[])
{
    #pragma unused params
    if (!Admin_Check(playerid, 5)) return 1;
    SendClientMessageToAll(COLOR_ADMIN, "[A] %s пусна PayDay.", PlayerData[playerid][pName]);
    Payday_RunAll();
    return 1;
}
