// Главно меню - бутон Y (KEY_YES) или /menu.
// Всичко в играта е достъпно оттук, за да се играе удобно и от таблет/телефон.

enum {
    MENU_ITEM_PROFILE,      // "Статистика"
    MENU_ITEM_DOCS,
    MENU_ITEM_TASKS,
    MENU_ITEM_MARKET,
    MENU_ITEM_GPS,          // гаражът, компанията и домът са в GPS > Моето имущество
    MENU_ITEM_CB,
    MENU_ITEM_SETTINGS,
    MENU_ITEM_HELP,
    MENU_ITEM_ADMIN     // само за админи
}

enum {
    SETTINGS_CB_TOGGLE,
    SETTINGS_CB_CHANNEL,
    SETTINGS_HUD_TOGGLE,
    SETTINGS_SPAWN
}

stock Menu_Show(playerid)
{
    if (PlayerData[playerid][pAdmin] >= 1) {
        ShowPlayerDialog(playerid, DIALOG_MENU, DIALOG_STYLE_LIST, C_BRAND SERVER_NAME C_WHITE " | Меню",
            "Статистика\nДокументи\nЗадачи\nБорса на товарите\nGPS\nCB радио\nНастройки\nПомощ\n" C_ADMIN "Админ панел", "Избери", "Затвори");
    } else {
        ShowPlayerDialog(playerid, DIALOG_MENU, DIALOG_STYLE_LIST, C_BRAND SERVER_NAME C_WHITE " | Меню",
            "Статистика\nДокументи\nЗадачи\nБорса на товарите\nGPS\nCB радио\nНастройки\nПомощ", "Избери", "Затвори");
    }
}

stock Menu_ShowProfile(playerid)
{
    new money[16], toNext[48], lic[24], payday[64], hours = PlayerData[playerid][pPlayTime] / 3600, minutes = (PlayerData[playerid][pPlayTime] / 60) % 60;
    FormatMoney(PlayerData[playerid][pMoney], money);
    format(toNext, sizeof (toNext), "още %d до ниво %d", Level_ExpToNext(playerid), PlayerData[playerid][pLevel] + 1);
    License_Format(License_GetBits(playerid), lic);
    Payday_Status(playerid, payday);

    ShowPlayerDialog(playerid, DIALOG_MENU_PROFILE, DIALOG_STYLE_MSGBOX, C_BRAND "Статистика",
        C_WHITE "Ник: " C_BRAND "%s\n" \
        C_WHITE "Пари: " C_GREEN "$%s\n" \
        C_WHITE "Ниво: " C_BRAND "%d\n" \
        C_WHITE "Опит: %d " C_GREY "(%s)\n" \
        C_WHITE "Категории: " C_BRAND "%s\n" \
        C_WHITE "PayDay: %s\n" \
        C_WHITE "Изиграно време: %d ч. %d мин.\n" \
        C_WHITE "CB канал: %d",
        "Назад", "", PlayerData[playerid][pName], money, PlayerData[playerid][pLevel],
        PlayerData[playerid][pExp], toNext, lic, payday, hours, minutes, PlayerData[playerid][pCBChannel]);
}

stock Menu_ShowSettings(playerid)
{
    new body[320], spawn[64];
    Spawn_Describe(playerid, spawn);
    format(body, sizeof (body), "CB радио\t%s\nCB канал\t" C_BRAND "%d\nHUD (скорост)\t%s\nСпаун (къде се появяваш)\t%s",
        PlayerData[playerid][pCBOn] ? (C_GREEN "ВКЛ") : (C_RED "ИЗК"),
        PlayerData[playerid][pCBChannel],
        PlayerData[playerid][pHudOn] ? (C_GREEN "ВКЛ") : (C_RED "ИЗК"),
        spawn);
    ShowPlayerDialog(playerid, DIALOG_MENU_SETTINGS, DIALOG_STYLE_TABLIST, C_BRAND "Настройки", "%s", "Промени", "Назад", body);
}

stock Menu_ShowCBChannel(playerid)
{
    ShowPlayerDialog(playerid, DIALOG_MENU_CB_CHANNEL, DIALOG_STYLE_INPUT, C_BRAND "CB радио" C_WHITE " | Канал",
        C_WHITE "Сега си на канал " C_CB "%d" C_WHITE ".\n\nВъведи канал от 1 до %d.\n" \
        C_GREY "Канал %d е общият канал на шофьорите.",
        "Смени", "Назад", PlayerData[playerid][pCBChannel], CB_MAX_CHANNEL, CB_DEFAULT_CHANNEL);
}

// ---------- Помощ ----------

static HelpLastTopic[MAX_PLAYERS] = {-1, ...};

stock Menu_ShowHelp(playerid)
{
    ShowPlayerDialog(playerid, DIALOG_MENU_HELP, DIALOG_STYLE_LIST, C_BRAND "Помощ",
        "Информация за проекта\nКоманди на сървъра", "Избери", "Назад");
}

// Информация за проекта - на теми (кратки страници, удобни и на телефон)
static const HelpTopicNames[][] = {
    "Какво е CARGO STATE",
    "Първи стъпки",
    "Курсове и товари",
    "Камиони, гараж и тунинг",
    "Шофьорски категории",
    "Жива икономика и борса",
    "Компании и служители",
    "Жилища, PayDay и нива",
    "Нужди и умора",
    "Правила на пътя"
};

static stock Menu_ShowHelpTopics(playerid)
{
    new body[512];
    body[0] = EOS;
    for (new i = 0; i < sizeof (HelpTopicNames); i++) {
        strcat(body, HelpTopicNames[i]);
        strcat(body, "\n");
    }
    ShowPlayerDialog(playerid, DIALOG_HELP_TOPICS, DIALOG_STYLE_LIST, C_BRAND "Информация за проекта", "%s", "Чети", "Назад", body);
}

static stock Menu_ShowHelpPage(playerid, topic)
{
    new title[64];
    format(title, sizeof (title), C_BRAND "%s", HelpTopicNames[topic]);
    switch (topic) {
        case 0: ShowPlayerDialog(playerid, DIALOG_HELP_PAGE, DIALOG_STYLE_MSGBOX, title,
            C_BRAND "CARGO STATE" C_WHITE " е симулатор на шофьор на камион за SA-MP и open.mp.\n\n" \
            C_WHITE "Започваш с бус в собствен гараж и с всеки курс ставаш по-опитен:\n" \
            C_WHITE "взимаш категории, купуваш по-големи камиони, ремаркета и компании.\n\n" \
            C_WHITE "Светът е жив: компаниите произвеждат и потребяват стоки,\n" \
            C_WHITE "цените следват търсенето, а складовете се пълнят и празнят от шофьорите.\n\n" \
            C_WHITE "Играе се еднакво удобно на компютър и на телефон или таблет -\n" \
            C_WHITE "почти всичко е в менюта (бутон " C_BRAND "Y" C_WHITE ").",
            "Назад", "");
        case 1: ShowPlayerDialog(playerid, DIALOG_HELP_PAGE, DIALOG_STYLE_MSGBOX, title,
            C_WHITE "Пристигаш с влака на гарата на своя град. Там те чака " C_BRAND "Наставникът" C_WHITE " (ALT).\n" \
            C_WHITE "Велосипедите до гарата са безплатни - с тях се движиш без книжка.\n\n" \
            C_WHITE "1. Лична карта - кметството\n" \
            C_WHITE "2. Медицинско - болницата\n" \
            C_WHITE "3. Книжка B - автошколата в San Fierro\n" \
            C_WHITE "4. Първият бус - гаражът ти\n" \
            C_WHITE "5. Първа доставка (" C_BRAND "N" C_WHITE " в буса)\n" \
            C_WHITE "6. Храна и гориво\n" \
            C_WHITE "7. Борсата или кандидатстване в компания\n\n" \
            C_WHITE "Напредъкът е в " C_BRAND "Меню > Задачи" C_WHITE ", документите - в " C_BRAND "Меню > Документи" C_WHITE ".",
            "Назад", "");
        case 2: ShowPlayerDialog(playerid, DIALOG_HELP_PAGE, DIALOG_STYLE_MSGBOX, title,
            C_WHITE "Всяка поръчка е от компания А до компания Б.\n" \
            C_WHITE "Плащането зависи от разстоянието, теглото, ценността на товара\n" \
            C_WHITE "и колко е нужен той на получателя.\n\n" \
            C_CB "Хладилни" C_WHITE " товари имат по-кратък срок - закъснението ги разваля.\n" \
            C_RED "Чупливи" C_WHITE " товари се повреждат двойно и се чупят при силен удар.\n" \
            C_WHITE "Насипните, течните и тежките товари се возят с ремарке (влекач).\n\n" \
            C_WHITE "Навреме = бонус. Закъснение и щети = по-малко пари.",
            "Назад", "");
        case 3: ShowPlayerDialog(playerid, DIALOG_HELP_PAGE, DIALOG_STYLE_MSGBOX, title,
            C_WHITE "Гаражът ти побира 2 превозни средства. Първият бус е безплатен.\n" \
            C_WHITE "Нови камиони - в автокъщите (GPS > Автокъщи), според нивото и категорията.\n\n" \
            C_WHITE "Всеки камион има товароносимост, резервоар и разход.\n" \
            C_WHITE "Зареждаш на бензиностанция с " C_BRAND "ALT" C_WHITE ", при авария - " C_BRAND "/roadside" C_WHITE ".\n\n" \
            C_WHITE "В гаража (" C_BRAND "ALT" C_WHITE " в превозното средство) има ремонт,\n" \
            C_WHITE "боя, неон и подобрения на двигателя, спирачките и бронята.",
            "Назад", "");
        case 4: ShowPlayerDialog(playerid, DIALOG_HELP_PAGE, DIALOG_STYLE_MSGBOX, title,
            C_BRAND "B" C_WHITE " - бусове до 3.5 т (задача 3 за начинаещи)\n" \
            C_BRAND "C" C_WHITE " - камиони (от ниво 2)\n" \
            C_BRAND "CE" C_WHITE " - влекачи с ремарке (от ниво 4, след C)\n" \
            C_BRAND "ADR" C_WHITE " - опасни товари, гориво (от ниво 4, след CE)\n\n" \
            C_WHITE "Взимат се в автошколата в San Fierro (GPS > Важни места).\n" \
            C_WHITE "Изпитът е теория (5 въпроса за правилата на сървъра)\n" \
            C_WHITE "и за C и CE - практика с изпитен камион.",
            "Назад", "");
        case 5: ShowPlayerDialog(playerid, DIALOG_HELP_PAGE, DIALOG_STYLE_MSGBOX, title,
            C_WHITE "Всяка компания има склад. Производителите произвеждат всяка минута,\n" \
            C_WHITE "магазините и строежите изразходват стоки.\n\n" \
            C_WHITE "Празен склад плаща повече (до +60 процента), пълен - по-малко (до -30 процента).\n" \
            C_WHITE "Пълен склад не приема нищо.\n\n" \
            C_BRAND "Борсата на товарите" C_WHITE " (меню) показва къде кой товар е най-търсен\n" \
            C_WHITE "и колко го има - така избираш най-изгодните курсове.",
            "Назад", "");
        case 6: ShowPlayerDialog(playerid, DIALOG_HELP_PAGE, DIALOG_STYLE_MSGBOX, title,
            C_WHITE "От ниво 3 можеш да купиш компания (ALT пеша на базата й).\n" \
            C_WHITE "Тя ти носи процент от всеки курс от и до нея.\n\n" \
            C_WHITE "Можеш да наемаш шофьори: те карат поръчките на компанията с бонус,\n" \
            C_WHITE "а тя взима комисионна. Даваш заплата, карта за гориво,\n" \
            C_WHITE "избираш име, таг, цвят и слоган.\n\n" \
            C_WHITE "Без компания? Кандидатствай в чужда - ALT на базата й.\n" \
            C_WHITE "Фирмен чат: " C_BRAND "/f",
            "Назад", "");
        case 7: ShowPlayerDialog(playerid, DIALOG_HELP_PAGE, DIALOG_STYLE_MSGBOX, title,
            C_WHITE "Курсовете дават опит, а опитът - нива (без горна граница).\n" \
            C_WHITE "Нивата отключват камиони, категории и компании.\n\n" \
            C_BRAND "PayDay" C_WHITE " идва на всеки кръгъл час, ако си изиграл поне 20 минути:\n" \
            C_WHITE "опит, заплата от компанията (ако работиш) и социална помощ, ако нямаш дом.\n\n" \
            C_WHITE "Къщите за продан са в GPS. В дома си имаш хладилник, тоалетна и легло,\n" \
            C_WHITE "но плащаш малък данък на PayDay.",
            "Назад", "");
        case 8: ShowPlayerDialog(playerid, DIALOG_HELP_PAGE, DIALOG_STYLE_MSGBOX, title,
            C_WHITE "Под парите виждаш глад, жажда, тоалетна и умора.\n\n" \
            C_WHITE "Храна и напитки има в магазини, закусвални, кафенета и на бензиностанциите.\n" \
            C_WHITE "В гаража и в дома си ги имаш безплатно.\n\n" \
            C_WHITE "Умората расте, докато караш. Почини си на паркинг, вкъщи или в гаража,\n" \
            C_WHITE "или изпий кафе. Много уморен - картината се люшка.",
            "Назад", "");
        default: ShowPlayerDialog(playerid, DIALOG_HELP_PAGE, DIALOG_STYLE_MSGBOX, title,
            C_WHITE "С товар не превишавай ограничението - бързането разлива и чупи.\n\n" \
            C_BRAND "Кантари:" C_WHITE " с товар спри на платформата за 10 секунди.\n" \
            C_WHITE "Претоварването над товароносимостта носи глоба.\n\n" \
            C_WHITE "Ремаркето се закачва отново с " C_BRAND "H" C_WHITE ", ако се откачи.\n" \
            C_WHITE "Отказан курс носи глоба. Бъди учтив в чата и по CB радиото.",
            "Назад", "");
    }
}

static stock Menu_ShowCommands(playerid)
{
    HelpLastTopic[playerid] = -1;
    ShowPlayerDialog(playerid, DIALOG_HELP_PAGE, DIALOG_STYLE_MSGBOX, C_BRAND "Команди на сървъра",
        C_BRAND "Y" C_WHITE " или " C_BRAND "/menu" C_WHITE " - главното меню\n" \
        C_BRAND "N" C_WHITE " или " C_BRAND "/work" C_WHITE " - работа: поръчки, текущ курс, отказ\n" \
        C_BRAND "CTRL" C_WHITE " или " C_BRAND "/engine" C_WHITE " - двигател\n" \
        C_BRAND "ALT" C_WHITE " пеша - врати, магазини, компании, къщи\n" \
        C_BRAND "H" C_WHITE " в превозно средство - гараж, закачане на ремарке\n" \
        C_BRAND "/gps" C_WHITE " - навигация и моето имущество\n" \
        C_BRAND "/market" C_WHITE " - борса на товарите\n" \
        C_BRAND "/company" C_WHITE " - моята компания\n" \
        C_BRAND "/roadside" C_WHITE " - пътна помощ\n" \
        C_BRAND "/cb [текст]" C_WHITE ", " C_BRAND "/cbch [канал]" C_WHITE " - CB радио\n" \
        C_BRAND "/f [текст]" C_WHITE " - фирмен чат\n" \
        C_BRAND "/pm [ID] [текст]" C_WHITE ", " C_BRAND "/r" C_WHITE " - лични съобщения\n" \
        C_BRAND "/admins" C_WHITE " - администраторите онлайн",
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
                case MENU_ITEM_MARKET:   Economy_ShowMarket(playerid);
                case MENU_ITEM_GPS:      Gps_ShowCategories(playerid);
                case MENU_ITEM_PROFILE:  Menu_ShowProfile(playerid);
                case MENU_ITEM_DOCS:     Docs_ShowMenu(playerid);
                case MENU_ITEM_TASKS:    Tutorial_ShowList(playerid);
                case MENU_ITEM_CB:       Menu_ShowCBChannel(playerid);
                case MENU_ITEM_SETTINGS: Menu_ShowSettings(playerid);
                case MENU_ITEM_HELP:     Menu_ShowHelp(playerid);
                case MENU_ITEM_ADMIN:    Admin_ShowPanel(playerid);
            }
            return 1;
        }
        case DIALOG_MENU_PROFILE, DIALOG_MENU_INFO: {
            if (response) Menu_Show(playerid);
            return 1;
        }
        case DIALOG_MENU_HELP: {
            if (!response) { Menu_Show(playerid); return 1; }
            if (listitem == 0) Menu_ShowHelpTopics(playerid);
            else Menu_ShowCommands(playerid);
            return 1;
        }
        case DIALOG_HELP_TOPICS: {
            if (!response || listitem < 0 || listitem >= sizeof (HelpTopicNames)) { Menu_ShowHelp(playerid); return 1; }
            HelpLastTopic[playerid] = listitem;
            Menu_ShowHelpPage(playerid, listitem);
            return 1;
        }
        case DIALOG_HELP_PAGE: {
            // От темите - обратно към темите; от командите - към помощта
            if (HelpLastTopic[playerid] >= 0) Menu_ShowHelpTopics(playerid);
            else Menu_ShowHelp(playerid);
            HelpLastTopic[playerid] = -1;
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
                case SETTINGS_SPAWN: {
                    Spawn_Cycle(playerid);
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
