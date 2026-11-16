// Главно меню – бутон Y (KEY_YES) или /menu.
// Всичко в играта е достъпно оттук, за да се играе удобно и от таблет/телефон.

enum {
    MENU_ITEM_GARAGE,
    MENU_ITEM_GPS,
    MENU_ITEM_PROFILE,
    MENU_ITEM_CB,
    MENU_ITEM_SETTINGS,
    MENU_ITEM_HELP,
    MENU_ITEM_ADMIN     // само за админи
}

enum {
    SETTINGS_CB_TOGGLE,
    SETTINGS_CB_CHANNEL,
    SETTINGS_HUD_TOGGLE
}

stock Menu_Show(playerid)
{
    if (PlayerData[playerid][pAdmin] >= 1) {
        ShowPlayerDialog(playerid, DIALOG_MENU, DIALOG_STYLE_LIST, C_BRAND SERVER_NAME C_WHITE " | Меню",
            "Моят гараж\nGPS\nМоят профил\nCB радио\nНастройки\nПомощ\n" C_ADMIN "Админ панел", "Избери", "Затвори");
    } else {
        ShowPlayerDialog(playerid, DIALOG_MENU, DIALOG_STYLE_LIST, C_BRAND SERVER_NAME C_WHITE " | Меню",
            "Моят гараж\nGPS\nМоят профил\nCB радио\nНастройки\nПомощ", "Избери", "Затвори");
    }
}

stock Menu_ShowProfile(playerid)
{
    new money[16], toNext[48], hours = PlayerData[playerid][pPlayTime] / 3600, minutes = (PlayerData[playerid][pPlayTime] / 60) % 60;
    FormatMoney(PlayerData[playerid][pMoney], money);
    new left = Level_ExpToNext(playerid);
    if (left > 0) format(toNext, sizeof (toNext), "още %d до ниво %d", left, PlayerData[playerid][pLevel] + 1);
    else format(toNext, sizeof (toNext), "максимално ниво");

    ShowPlayerDialog(playerid, DIALOG_MENU_PROFILE, DIALOG_STYLE_MSGBOX, C_BRAND "Моят профил",
        C_WHITE "Ник: " C_BRAND "%s\n" \
        C_WHITE "Пари: " C_GREEN "$%s\n" \
        C_WHITE "Ниво: " C_BRAND "%d" C_WHITE " / %d\n" \
        C_WHITE "Опит: %d " C_GREY "(%s)\n" \
        C_WHITE "Изиграно време: %d ч. %d мин.\n" \
        C_WHITE "CB канал: %d",
        "Назад", "", PlayerData[playerid][pName], money, PlayerData[playerid][pLevel], MAX_LEVEL,
        PlayerData[playerid][pExp], toNext, hours, minutes, PlayerData[playerid][pCBChannel]);
}

stock Menu_ShowSettings(playerid)
{
    new body[192];
    format(body, sizeof (body), "CB радио\t%s\nCB канал\t" C_BRAND "%d\nHUD (скорост)\t%s",
        PlayerData[playerid][pCBOn] ? (C_GREEN "ВКЛ") : (C_RED "ИЗК"),
        PlayerData[playerid][pCBChannel],
        PlayerData[playerid][pHudOn] ? (C_GREEN "ВКЛ") : (C_RED "ИЗК"));
    ShowPlayerDialog(playerid, DIALOG_MENU_SETTINGS, DIALOG_STYLE_TABLIST, C_BRAND "Настройки", "%s", "Промени", "Назад", body);
}

stock Menu_ShowCBChannel(playerid)
{
    ShowPlayerDialog(playerid, DIALOG_MENU_CB_CHANNEL, DIALOG_STYLE_INPUT, C_BRAND "CB радио" C_WHITE " | Канал",
        C_WHITE "Сега си на канал " C_CB "%d" C_WHITE ".\n\nВъведи канал от 1 до %d.\n" \
        C_GREY "Канал %d е общият канал на шофьорите.",
        "Смени", "Назад", PlayerData[playerid][pCBChannel], CB_MAX_CHANNEL, CB_DEFAULT_CHANNEL);
}

stock Menu_ShowHelp(playerid)
{
    ShowPlayerDialog(playerid, DIALOG_MENU_HELP, DIALOG_STYLE_MSGBOX, C_BRAND "Помощ",
        C_BRAND "Y" C_WHITE " или " C_BRAND "/menu" C_WHITE " – това меню\n" \
        C_BRAND "N" C_WHITE " или " C_BRAND "/work" C_WHITE " – работа: поръчки, текущ курс, отказ\n" \
        C_BRAND "CTRL" C_WHITE " или " C_BRAND "/engine" C_WHITE " – двигател (пали/гаси)\n" \
        C_BRAND "ALT" C_WHITE " пеша / " C_BRAND "H" C_WHITE " с превозно средство – вход и изход (гараж)\n" \
        C_BRAND "/gps" C_WHITE " – навигация\n" \
        C_BRAND "/cb [текст]" C_WHITE " – CB радио (чуват те всички на твоя канал)\n" \
        C_BRAND "/cbch [канал]" C_WHITE " – смяна на CB канала\n" \
        C_BRAND "/pm [ID] [текст]" C_WHITE " – лично съобщение, " C_BRAND "/r" C_WHITE " – отговор\n" \
        C_BRAND "/admins" C_WHITE " – администраторите онлайн",
        "Назад", "");
}

stock Menu_OnKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
{
    if (!PlayerData[playerid][pLogged] || !PlayerData[playerid][pSpawned]) return 0;
    if ((newkeys & KEY_YES) && !(oldkeys & KEY_YES)) {
        Menu_Show(playerid);
        return 1;
    }
    return 0;
}

stock Menu_OnDialogResponse(playerid, dialogid, response, listitem, const inputtext[])
{
    switch (dialogid) {
        case DIALOG_MENU: {
            if (!response) return 1;
            switch (listitem) {
                case MENU_ITEM_GARAGE:   Garage_ShowMenu(playerid);
                case MENU_ITEM_GPS:      Gps_ShowCategories(playerid);
                case MENU_ITEM_PROFILE:  Menu_ShowProfile(playerid);
                case MENU_ITEM_CB:       Menu_ShowCBChannel(playerid);
                case MENU_ITEM_SETTINGS: Menu_ShowSettings(playerid);
                case MENU_ITEM_HELP:     Menu_ShowHelp(playerid);
                case MENU_ITEM_ADMIN:    Admin_ShowPanel(playerid);
            }
            return 1;
        }
        case DIALOG_MENU_PROFILE, DIALOG_MENU_HELP, DIALOG_MENU_INFO: {
            if (response) Menu_Show(playerid);
            return 1;
        }
        case DIALOG_MENU_SETTINGS: {
            if (!response) {
                Menu_Show(playerid);
                return 1;
            }
            switch (listitem) {
                case SETTINGS_CB_TOGGLE: {
                    PlayerData[playerid][pCBOn] = !PlayerData[playerid][pCBOn];
                    Menu_ShowSettings(playerid);
                }
                case SETTINGS_CB_CHANNEL: Menu_ShowCBChannel(playerid);
                case SETTINGS_HUD_TOGGLE: {
                    Hud_SetEnabled(playerid, !PlayerData[playerid][pHudOn]);
                    Menu_ShowSettings(playerid);
                }
            }
            return 1;
        }
        case DIALOG_MENU_CB_CHANNEL: {
            if (!response) {
                Menu_Show(playerid);
                return 1;
            }
            if (!IsNumeric(inputtext) || !CB_SetChannel(playerid, strval(inputtext))) {
                SendError(playerid, "Каналът трябва да е число от 1 до 40.");
                Menu_ShowCBChannel(playerid);
            }
            return 1;
        }
    }
    return 0;
}

CMD:menu(playerid, params[])
{
    #pragma unused params
    Menu_Show(playerid);
    return 1;
}
