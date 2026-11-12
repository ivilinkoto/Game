// Общи команди за всички играчи.

CMD:help(playerid, params[])
{
    #pragma unused params
    new body[1024];
    body[0] = EOS;
    strcat(body, "{FFC400}Основни{FFFFFF}\n");
    strcat(body, "/stats - твоите статистики\n");
    strcat(body, "/pay [ID] [сума] - даваш пари на играч до теб\n");
    strcat(body, "/admins - админите онлайн\n\n");
    strcat(body, "{FFC400}Чат{FFFFFF}\n");
    strcat(body, "Обикновен текст - говориш на хората около теб\n");
    strcat(body, "/me /do - действия и описания (RP)\n");
    strcat(body, "/s - викаш   /l - говориш тихо\n");
    strcat(body, "/b - локален OOC   /o - глобален OOC\n");
    strcat(body, "/pm [ID] [текст] - лично съобщение   /r - отговор\n\n");
    strcat(body, "{FFC400}Подкрепа{FFFFFF}\n");
    strcat(body, "/shop - магазин   /donate - как да дариш\n");
    strcat(body, "/coins - твоите монети   /vip - VIP статус\n\n");
    strcat(body, "{AFAFAF}Заплата получаваш на всеки час игра. Ниво се вдига с опит от заплатите.");
    ShowPlayerDialog(playerid, DIALOG_INFO, DIALOG_STYLE_MSGBOX, "Помощ", "%s", "Затвори", "", body);
    return 1;
}

CMD:stats(playerid, params[])
{
    #pragma unused params
    new rpName[MAX_PLAYER_NAME + 1], money[16], bank[16], vipName[16], rank[32], body[768];
    GetRPName(playerid, rpName);
    FormatNumber(PlayerData[playerid][pMoney], money);
    FormatNumber(PlayerData[playerid][pBank], bank);
    Vip_GetName(Vip_IsActive(playerid) ? PlayerData[playerid][pVip] : VIP_NONE, vipName);
    Admin_GetRankName(PlayerData[playerid][pAdmin], rank);

    new gender[8];
    if (PlayerData[playerid][pGender] == 1) format(gender, sizeof (gender), "Жена");
    else format(gender, sizeof (gender), "Мъж");

    format(body, sizeof (body),
        "{FFFFFF}Име: {FFC400}%s{FFFFFF}\nПол: %s\n\nНиво: {FFC400}%d{FFFFFF} (опит %d/%d)\nИзиграно време: %d ч. %d мин.\nДо заплата: %d мин.\n\nПари в брой: {33CC33}$%s{FFFFFF}\nБанка: {33CC33}$%s{FFFFFF}\n\nVIP: {FFD700}%s{FFFFFF}\nМонети: {FFD700}%d{FFFFFF}\nРанг: %s",
        rpName,
        gender,
        PlayerData[playerid][pLevel], PlayerData[playerid][pExp], Economy_GetExpNeeded(PlayerData[playerid][pLevel]),
        PlayerData[playerid][pPlayedMinutes] / 60, PlayerData[playerid][pPlayedMinutes] % 60,
        PAYDAY_MINUTES - PlayerData[playerid][pPaydayMinutes],
        money, bank, vipName, PlayerData[playerid][pCoins], rank);
    ShowPlayerDialog(playerid, DIALOG_INFO, DIALOG_STYLE_MSGBOX, "Статистики", "%s", "Затвори", "", body);
    return 1;
}

CMD:pay(playerid, params[])
{
    new targetid, amount;
    if (!GetParamInt(params, 1, amount)) return SendUsage(playerid, "/pay [ID/име] [сума]");
    if (!GetParamPlayer(playerid, params, 0, targetid)) return 1;

    if (targetid == playerid) return SendError(playerid, "Не можеш да даваш пари на себе си.");
    if (amount < 1 || amount > 100000) return SendError(playerid, "Сумата трябва да е между $1 и $100,000.");
    if (amount > PlayerData[playerid][pMoney]) return SendError(playerid, "Нямаш толкова пари.");
    if (!IsPlayerNearPlayer(playerid, targetid, 5.0)) return SendError(playerid, "Играчът трябва да е до теб.");

    GivePlayerCash(playerid, -amount);
    GivePlayerCash(targetid, amount);

    new from[MAX_PLAYER_NAME + 1], to[MAX_PLAYER_NAME + 1], msg[144];
    GetRPName(playerid, from);
    GetRPName(targetid, to);
    SendClientMessage(playerid, COLOR_GREEN, "Даде $%d на %s.", amount, to);
    SendClientMessage(targetid, COLOR_GREEN, "%s ти даде $%d.", from, amount);

    format(msg, sizeof (msg), "* %s вади пари и ги подава на %s.", from, to);
    ProxMessage(playerid, CHAT_RADIUS_ME, COLOR_PURPLE, msg, false);

    new details[32];
    format(details, sizeof (details), "$%d", amount);
    Log_Add("pay", PlayerData[playerid][pName], PlayerData[targetid][pName], details);

    Accounts_Save(playerid, false);
    Accounts_Save(targetid, false);
    return 1;
}

CMD:admins(playerid, params[])
{
    #pragma unused params
    new rank[32], count = 0;
    SendClientMessage(playerid, COLOR_ADMIN, "______ Админи онлайн ______");
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (!IsPlayerConnected(i) || !PlayerData[i][pLogged] || PlayerData[i][pAdmin] == 0) continue;
        Admin_GetRankName(PlayerData[i][pAdmin], rank);
        SendClientMessage(playerid, COLOR_WHITE, "%s [%d] - %s", PlayerData[i][pName], i, rank);
        count++;
    }
    if (count == 0) SendClientMessage(playerid, COLOR_GREY, "Няма админи онлайн.");
    return 1;
}
