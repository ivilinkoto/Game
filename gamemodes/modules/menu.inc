// Главно меню – отваря се с бутона Y или с /menu.
// Всичко важно трябва да е достъпно оттук, за да не се пишат команди на телефона.

enum {
    MENU_ITEM_STATS,
    MENU_ITEM_HELP,
    MENU_ITEM_RP,
    MENU_ITEM_ADMINS,
    MENU_ITEM_ADMIN_PANEL   // само за админи – винаги последен
}

stock Menu_OnKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
{
    if (!PlayerData[playerid][pLogged] || PlayerData[playerid][pInSkinSelect]) return;
    if ((newkeys & KEY_YES) && !(oldkeys & KEY_YES)) {
        Menu_Show(playerid);
    }
}

stock Menu_Show(playerid)
{
    new body[512];
    format(body, sizeof (body),
        C_BRAND "»" C_WHITE " Статистика\n" \
        C_BRAND "»" C_WHITE " Помощ и команди\n" \
        C_BRAND "»" C_WHITE " Как се играе RolePlay\n" \
        C_BRAND "»" C_WHITE " Администратори онлайн");
    if (PlayerData[playerid][pAdmin] > 0) {
        strcat(body, "\n" C_RED "»" C_WHITE " Админ панел");
    }
    ShowPlayerDialog(playerid, DIALOG_MENU, DIALOG_STYLE_LIST, C_BRAND "HOTFIX ONLINE " C_WHITE "| Меню", "%s", "Избери", "Затвори", body);
}

// Връща 1, ако диалогът е обработен тук.
stock Menu_OnDialogResponse(playerid, dialogid, response, listitem)
{
    switch (dialogid) {
        case DIALOG_MENU: {
            if (!response) return 1;
            switch (listitem) {
                case MENU_ITEM_STATS:       Menu_ShowStats(playerid);
                case MENU_ITEM_HELP:        Menu_ShowHelp(playerid);
                case MENU_ITEM_RP:          Menu_ShowRPGuide(playerid);
                case MENU_ITEM_ADMINS:      Menu_ShowAdmins(playerid);
                case MENU_ITEM_ADMIN_PANEL: {
                    if (PlayerData[playerid][pAdmin] > 0) Admin_ShowHelp(playerid, true);
                }
            }
            return 1;
        }
        case DIALOG_MENU_BACK: {
            Menu_Show(playerid);
            return 1;
        }
    }
    return 0;
}

stock Menu_ShowStats(playerid)
{
    new body[1500], line[160], rpName[MAX_PLAYER_NAME + 1], money[16], rank[32];
    GetRPName(playerid, rpName);
    FormatNumber(PlayerData[playerid][pMoney], money);

    format(body, sizeof (body), C_WHITE "Име: " C_BRAND "%s\n\n", rpName);

    format(line, sizeof (line), C_GREY "Ниво: " C_GOLD "%d" C_GREY "   Опит: " C_WHITE "%d / %d\n",
        PlayerData[playerid][pLevel], PlayerData[playerid][pExp], Levels_GetExpNeeded(PlayerData[playerid][pLevel]));
    strcat(body, line);
    format(line, sizeof (line), C_GREY "Пари в брой: " C_GREEN "$%s\n", money);
    strcat(body, line);
    format(line, sizeof (line), C_GREY "PayDay: " C_GOLD "+%d опит" C_GREY " след " C_WHITE "%d мин." C_GREY " активна игра\n",
        PAYDAY_EXP, PAYDAY_MINUTES - PlayerData[playerid][pPaydayMinutes]);
    strcat(body, line);
    format(line, sizeof (line), C_GREY "Изиграно време: " C_WHITE "%d ч. %d мин.\n\n",
        PlayerData[playerid][pPlayedMinutes] / 60, PlayerData[playerid][pPlayedMinutes] % 60);
    strcat(body, line);

    if (PlayerData[playerid][pGender] == GENDER_FEMALE) {
        strcat(body, C_GREY "Пол: " C_WHITE "Жена\n");
    } else {
        strcat(body, C_GREY "Пол: " C_WHITE "Мъж\n");
    }
    format(line, sizeof (line), C_GREY "Скин: " C_WHITE "%d\n", PlayerData[playerid][pSkin]);
    strcat(body, line);

    if (PlayerData[playerid][pAdmin] > 0) {
        Admin_GetRankName(PlayerData[playerid][pAdmin], rank);
        format(line, sizeof (line), C_GREY "Ранг: " C_RED "%s\n", rank);
        strcat(body, line);
    }

    strcat(body, "\n" C_GREY "Времето не се брои, докато си AFK.");
    ShowPlayerDialog(playerid, DIALOG_MENU_BACK, DIALOG_STYLE_MSGBOX, C_BRAND "HOTFIX ONLINE " C_WHITE "| Статистика", "%s", "Назад", "", body);
}

stock Menu_ShowHelp(playerid)
{
    new body[2048];
    body[0] = EOS;
    strcat(body, C_BRAND "Основни\n");
    strcat(body, C_GOLD "Y" C_WHITE ", " C_GOLD "/menu" C_WHITE ", " C_GOLD "/mm" C_WHITE " или " C_GOLD "/mn" C_WHITE " - отваря това меню\n");
    strcat(body, C_GOLD "/stats" C_WHITE " - твоята статистика\n");
    strcat(body, C_GOLD "/admins" C_WHITE " - администратори онлайн\n");
    strcat(body, C_GOLD "/afk [причина]" C_WHITE " - отбелязваш, че отсъстваш   " C_GOLD "/afklist" C_WHITE " - кой е AFK\n\n");

    strcat(body, C_BRAND "Чат\n");
    strcat(body, C_WHITE "Обикновен текст - говориш на хората около теб\n");
    strcat(body, C_GOLD "/me" C_WHITE " - действие:  " C_GREY "/me вади телефона си\n");
    strcat(body, C_GOLD "/do" C_WHITE " - описание:  " C_GREY "/do Телефонът е счупен.\n");
    strcat(body, C_GOLD "/s" C_WHITE " - викаш   " C_GOLD "/l" C_WHITE " - говориш тихо\n");
    strcat(body, C_GOLD "/b" C_WHITE " - OOC наблизо   " C_GOLD "/o" C_WHITE " - OOC за всички\n");
    strcat(body, C_GOLD "/pm [ID] [текст]" C_WHITE " - лично съобщение   " C_GOLD "/r" C_WHITE " - отговор\n\n");

    strcat(body, C_BRAND "Нива\n");
    strcat(body, C_WHITE "На всеки " C_GOLD "60 минути" C_WHITE " активна игра получаваш " C_GOLD "PayDay" C_WHITE " - точка опит.\n");
    strcat(body, C_WHITE "С опита вдигаш ниво. Времето, докато си AFK, не се брои.\n\n");

    strcat(body, C_GREY "Скоро: работи, къщи, бизнеси и фракции.");
    ShowPlayerDialog(playerid, DIALOG_MENU_BACK, DIALOG_STYLE_MSGBOX, C_BRAND "HOTFIX ONLINE " C_WHITE "| Помощ", "%s", "Назад", "", body);
}

stock Menu_ShowRPGuide(playerid)
{
    new body[2048];
    body[0] = EOS;
    strcat(body, C_WHITE "В " C_BRAND "HOTFIX ONLINE" C_WHITE " играеш роля - герой с име, история и живот в Los Santos.\n");
    strcat(body, C_WHITE "Сървърът е " C_GREEN "лайт RP" C_WHITE " - важно е да е забавно, а не строго.\n\n");

    strcat(body, C_BRAND "IC и OOC\n");
    strcat(body, C_GOLD "IC" C_WHITE " (в ролята) - всичко, което казва и прави героят ти.\n");
    strcat(body, C_GOLD "OOC" C_WHITE " (извън ролята) - когато говориш като себе си. Използвай " C_GOLD "/b" C_WHITE " или " C_GOLD "/o" C_WHITE ".\n\n");

    strcat(body, C_BRAND "Няколко прости правила\n");
    strcat(body, C_BRAND "»" C_WHITE " Не убивай хора без причина (DM)\n");
    strcat(body, C_BRAND "»" C_WHITE " Не блъскай хора с кола нарочно\n");
    strcat(body, C_BRAND "»" C_WHITE " Не използвай информация извън ролята в играта (MG)\n");
    strcat(body, C_BRAND "»" C_WHITE " Уважавай другите играчи и администраторите\n\n");

    strcat(body, C_GREY "Имаш въпрос? Пиши на администратор с /pm или виж /admins.");
    ShowPlayerDialog(playerid, DIALOG_MENU_BACK, DIALOG_STYLE_MSGBOX, C_BRAND "HOTFIX ONLINE " C_WHITE "| Как се играе", "%s", "Назад", "", body);
}

stock Menu_ShowAdmins(playerid)
{
    new body[1500], line[96], rank[32], count = 0;
    body[0] = EOS;
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (!IsPlayerConnected(i) || !PlayerData[i][pLogged] || PlayerData[i][pAdmin] == 0) continue;
        Admin_GetRankName(PlayerData[i][pAdmin], rank);
        format(line, sizeof (line), C_WHITE "%s " C_GREY "[ID %d]" C_WHITE " - " C_RED "%s\n", PlayerData[i][pName], i, rank);
        strcat(body, line);
        count++;
    }
    if (count == 0) {
        strcat(body, C_GREY "В момента няма администратори онлайн.\n");
    }
    strcat(body, "\n" C_GREY "Пиши им с /pm [ID] [въпрос].");
    ShowPlayerDialog(playerid, DIALOG_MENU_BACK, DIALOG_STYLE_MSGBOX, C_BRAND "HOTFIX ONLINE " C_WHITE "| Администратори", "%s", "Назад", "", body);
}

CMD:menu(playerid, params[])
{
    #pragma unused params
    Menu_Show(playerid);
    return 1;
}

CMD:mm(playerid, params[])
{
    #pragma unused params
    Menu_Show(playerid);
    return 1;
}

CMD:mn(playerid, params[])
{
    #pragma unused params
    Menu_Show(playerid);
    return 1;
}

CMD:stats(playerid, params[])
{
    #pragma unused params
    Menu_ShowStats(playerid);
    return 1;
}

CMD:help(playerid, params[])
{
    #pragma unused params
    Menu_ShowHelp(playerid);
    return 1;
}

CMD:admins(playerid, params[])
{
    #pragma unused params
    Menu_ShowAdmins(playerid);
    return 1;
}
