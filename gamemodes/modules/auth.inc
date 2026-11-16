// Регистрация и вход.
//
// Поток: свързване -> камера над града -> вход ИЛИ регистрация
//        (парола -> потвърждение -> пол -> скин) -> спаун.

// Скинът се избира визуално – виж modules/skin_select.inc

// ---------- Свързване ----------

stock Auth_OnPlayerConnect(playerid)
{
    new name[MAX_PLAYER_NAME + 1];
    GetPlayerName(playerid, name, sizeof (name));

    if (!IsValidNickname(name)) return;

    new q[160];
    format(q, sizeof (q), "SELECT id, password FROM accounts WHERE name = '%s' LIMIT 1", name);
    new DBResult:r = Database_Query(q);
    if (r != DBResult:0) {
        if (DB_GetRowCount(r) > 0) {
            PlayerData[playerid][pHasAccount] = true;
            PlayerData[playerid][pID] = DB_GetFieldIntByName(r, "id");
            DB_GetFieldStringByName(r, "password", PlayerData[playerid][pPassHash], BCRYPT_HASH_LENGTH);
        }
        DB_FreeResultSet(r);
    }
}

stock Auth_OnPlayerRequestClass(playerid)
{
    if (PlayerData[playerid][pLogged] || PlayerData[playerid][pAuthShown]) return 1;

    PlayerData[playerid][pAuthShown] = true;
    TogglePlayerSpectating(playerid, true);
    // Камерата не се задава надеждно веднага след spectate – изчакваме малко.
    SetTimerEx("Auth_Start", 300, false, "ii", playerid, PlayerData[playerid][pConnSeq]);
    return 1;
}

forward Auth_Start(playerid, seq);
public Auth_Start(playerid, seq)
{
    if (!IsPlayerConnected(playerid) || PlayerData[playerid][pConnSeq] != seq || PlayerData[playerid][pLogged]) return 0;

    SetPlayerCameraPos(playerid, AUTH_CAM_X, AUTH_CAM_Y, AUTH_CAM_Z);
    SetPlayerCameraLookAt(playerid, AUTH_LOOK_X, AUTH_LOOK_Y, AUTH_LOOK_Z);
    ClearChat(playerid);

    if (!IsValidNickname(PlayerData[playerid][pName])) {
        ShowPlayerDialog(playerid, DIALOG_KICK_INFO, DIALOG_STYLE_MSGBOX, C_BRAND SERVER_NAME C_WHITE " | Невалиден ник",
            C_WHITE "Никът ти " C_RED "%s" C_WHITE " не е позволен.\n\n" \
            "Разрешено: 3–20 символа – латински букви, цифри, " C_BRAND "_" C_WHITE " и " C_BRAND "." C_WHITE "\n" \
            "Смени ника си в лаунчера и влез отново.",
            "Разбрах", "", PlayerData[playerid][pName]);
        KickEx(playerid);
        return 1;
    }

    if (Admin_CheckBan(playerid)) return 1;

    if (PlayerData[playerid][pHasAccount]) {
        Auth_ShowLogin(playerid, "");
    } else {
        Auth_ShowWelcome(playerid);
    }
    return 1;
}

// "\n\n{червено}Грешка" или празно
static stock Auth_ErrorSuffix(const error[], dest[], size = sizeof (dest))
{
    if (error[0] == EOS) {
        dest[0] = EOS;
    } else {
        format(dest, size, "\n\n" C_RED "%s", error);
    }
}

// ---------- Вход ----------

stock Auth_ShowLogin(playerid, const error[])
{
    new err[96];
    Auth_ErrorSuffix(error, err);
    ShowPlayerDialog(playerid, DIALOG_LOGIN, DIALOG_STYLE_PASSWORD, C_BRAND SERVER_NAME C_WHITE " | Вход",
        C_WHITE "Добре дошъл отново, " C_BRAND "%s" C_WHITE "!\n\nВъведи паролата си:%s",
        "Вход", "Изход", PlayerData[playerid][pName], err);
}

stock Auth_OnLoginResult(playerid, bool:success)
{
    if (PlayerData[playerid][pLogged]) return;

    if (!success) {
        PlayerData[playerid][pLoginAttempts]++;
        if (PlayerData[playerid][pLoginAttempts] >= MAX_LOGIN_ATTEMPTS) {
            ShowPlayerDialog(playerid, DIALOG_KICK_INFO, DIALOG_STYLE_MSGBOX, C_BRAND SERVER_NAME C_WHITE " | Вход",
                C_RED "Грешна парола %d пъти." C_WHITE "\nВръзката е прекъсната.", "Изход", "", MAX_LOGIN_ATTEMPTS);
            KickEx(playerid);
            return;
        }
        new msg[64];
        format(msg, sizeof (msg), "Грешна парола. Остават %d опита.", MAX_LOGIN_ATTEMPTS - PlayerData[playerid][pLoginAttempts]);
        Auth_ShowLogin(playerid, msg);
        return;
    }

    Auth_LoadAccount(playerid);

    new ip[16], q[160];
    GetPlayerIp(playerid, ip, sizeof (ip));
    format(q, sizeof (q), "UPDATE accounts SET last_login = %d, last_ip = '%s' WHERE id = %d", gettime(), ip, PlayerData[playerid][pID]);
    Database_Exec(q);

    ClearChat(playerid);
    SendClientMessage(playerid, COLOR_BRAND, "[CARGO] " C_WHITE "Добре дошъл отново, " C_BRAND "%s" C_WHITE "! Менюто е на " C_BRAND "Y" C_WHITE " или " C_BRAND "/menu" C_WHITE ".", PlayerData[playerid][pName]);
    if (!PlayerData[playerid][pStarterGiven]) Veh_GiveStarter(playerid);
    Auth_SpawnPlayer(playerid);
}

stock Auth_LoadAccount(playerid)
{
    new q[96];
    format(q, sizeof (q), "SELECT * FROM accounts WHERE id = %d LIMIT 1", PlayerData[playerid][pID]);
    new DBResult:r = Database_Query(q);
    if (r == DBResult:0) return;
    if (DB_GetRowCount(r) > 0) {
        PlayerData[playerid][pGender]    = DB_GetFieldIntByName(r, "gender");
        PlayerData[playerid][pSkin]      = DB_GetFieldIntByName(r, "skin");
        PlayerData[playerid][pMoney]     = DB_GetFieldIntByName(r, "money");
        PlayerData[playerid][pExp]       = DB_GetFieldIntByName(r, "exp");
        PlayerData[playerid][pLevel]     = DB_GetFieldIntByName(r, "level");
        PlayerData[playerid][pPlayTime]  = DB_GetFieldIntByName(r, "playtime");
        PlayerData[playerid][pAdmin]     = DB_GetFieldIntByName(r, "admin");
        DB_GetFieldStringByName(r, "admin_password", PlayerData[playerid][pAdminHash], BCRYPT_HASH_LENGTH);
        PlayerData[playerid][pPosX]      = DB_GetFieldFloatByName(r, "pos_x");
        PlayerData[playerid][pPosY]      = DB_GetFieldFloatByName(r, "pos_y");
        PlayerData[playerid][pPosZ]      = DB_GetFieldFloatByName(r, "pos_z");
        PlayerData[playerid][pPosA]      = DB_GetFieldFloatByName(r, "pos_a");
        PlayerData[playerid][pHasPos]    = (DB_GetFieldIntByName(r, "has_pos") != 0);
        PlayerData[playerid][pCBChannel] = DB_GetFieldIntByName(r, "cb_channel");
        PlayerData[playerid][pCBOn]      = (DB_GetFieldIntByName(r, "cb_on") != 0);
        PlayerData[playerid][pHudOn]     = (DB_GetFieldIntByName(r, "hud_on") != 0);
        PlayerData[playerid][pGarageCity] = DB_GetFieldIntByName(r, "garage_city");
        PlayerData[playerid][pStarterGiven] = (DB_GetFieldIntByName(r, "starter_given") != 0);
    }
    DB_FreeResultSet(r);

    if (PlayerData[playerid][pGarageCity] < 0 || PlayerData[playerid][pGarageCity] >= MAX_CITIES) {
        PlayerData[playerid][pGarageCity] = CITY_LS;
    }
    Veh_Load(playerid);

    // Нивото винаги следва опита (ако праговете се сменят)
    PlayerData[playerid][pLevel] = Level_ForExp(PlayerData[playerid][pExp]);
    if (PlayerData[playerid][pCBChannel] < 1 || PlayerData[playerid][pCBChannel] > CB_MAX_CHANNEL) {
        PlayerData[playerid][pCBChannel] = CB_DEFAULT_CHANNEL;
    }
}

// ---------- Регистрация ----------

stock Auth_ShowWelcome(playerid)
{
    ShowPlayerDialog(playerid, DIALOG_REG_WELCOME, DIALOG_STYLE_MSGBOX, C_BRAND SERVER_NAME C_WHITE " | Добре дошъл",
        C_WHITE "Здравей, " C_BRAND "%s" C_WHITE "!\n\n" \
        C_BRAND SERVER_NAME C_WHITE " е truck simulator в целия San Andreas.\n" \
        "Возиш товари между индустриите, трупаш опит, отключваш\n" \
        "по-големи камиони и печелиш пари.\n\n" \
        "Регистрацията отнема по-малко от минута.",
        "Напред", "Изход", PlayerData[playerid][pName]);
}

stock Auth_ShowPassword(playerid, const error[])
{
    new err[96];
    Auth_ErrorSuffix(error, err);
    ShowPlayerDialog(playerid, DIALOG_REG_PASSWORD, DIALOG_STYLE_PASSWORD, C_BRAND "Регистрация" C_WHITE " | Парола (1/5)",
        C_WHITE "Измисли парола (" C_BRAND "%d–%d" C_WHITE " символа):%s",
        "Напред", "Назад", PASSWORD_MIN, PASSWORD_MAX, err);
}

stock Auth_ShowConfirm(playerid, const error[])
{
    new err[96];
    Auth_ErrorSuffix(error, err);
    ShowPlayerDialog(playerid, DIALOG_REG_CONFIRM, DIALOG_STYLE_PASSWORD, C_BRAND "Регистрация" C_WHITE " | Парола (2/5)",
        C_WHITE "Въведи паролата още веднъж:%s",
        "Напред", "Назад", err);
}

stock Auth_ShowGender(playerid)
{
    ShowPlayerDialog(playerid, DIALOG_REG_GENDER, DIALOG_STYLE_LIST, C_BRAND "Регистрация" C_WHITE " | Пол (3/5)",
        "Мъж\nЖена", "Напред", "Назад");
}

// Визуален избор на скин (решетка с модели). Вика се и при ESC от решетката – назад към пола.
stock Auth_ShowSkin(playerid)
{
    SkinSelect_Show(playerid, PlayerData[playerid][pRegGender]);
}

stock Auth_OnSkinChosen(playerid, skin)
{
    if (PlayerData[playerid][pLogged] || PlayerData[playerid][pRegPass][0] == EOS) return;
    PlayerData[playerid][pGender] = PlayerData[playerid][pRegGender];
    PlayerData[playerid][pSkin] = skin;
    Auth_ShowCity(playerid);
}

stock Auth_ShowCity(playerid)
{
    ShowPlayerDialog(playerid, DIALOG_REG_CITY, DIALOG_STYLE_TABLIST_HEADERS, C_BRAND "Регистрация" C_WHITE " | Начален град (5/5)",
        "Град\tНачални товари\tПътища\n" \
        C_BRAND "Los Santos\t" C_WHITE "пристанище, контейнери, кутии\t" C_GREY "град, къси курсове\n" \
        C_BRAND "San Fierro\t" C_WHITE "дърва, строителни материали\t" C_GREY "хълмове и завои\n" \
        C_BRAND "Las Venturas\t" C_WHITE "кариера, руда, гориво\t" C_GREY "пустиня, дълги прави",
        "Избери", "Назад");
}

stock Auth_OnSkinCancelled(playerid)
{
    if (PlayerData[playerid][pLogged]) return;
    Auth_ShowGender(playerid);
}

stock Auth_OnRegisterHashed(playerid, const hash[])
{
    if (PlayerData[playerid][pLogged]) return;

    new ip[16], q[400];
    GetPlayerIp(playerid, ip, sizeof (ip));
    format(q, sizeof (q),
        "INSERT INTO accounts (name, password, gender, skin, money, garage_city, register_ip, last_ip, registered_at, last_login) " \
        "VALUES ('%s', '%s', %d, %d, %d, %d, '%s', '%s', %d, %d)",
        PlayerData[playerid][pName], hash, PlayerData[playerid][pGender], PlayerData[playerid][pSkin],
        START_MONEY, PlayerData[playerid][pRegCity], ip, ip, gettime(), gettime());
    Database_Exec(q);

    format(q, sizeof (q), "SELECT id FROM accounts WHERE name = '%s' LIMIT 1", PlayerData[playerid][pName]);
    new DBResult:r = Database_Query(q);
    new id = 0;
    if (r != DBResult:0) {
        if (DB_GetRowCount(r) > 0) id = DB_GetFieldIntByName(r, "id");
        DB_FreeResultSet(r);
    }
    if (id <= 0) {
        SendError(playerid, "Грешка при създаване на акаунта. Опитай отново.");
        KickEx(playerid);
        return;
    }

    PlayerData[playerid][pID] = id;
    PlayerData[playerid][pHasAccount] = true;
    format(PlayerData[playerid][pPassHash], BCRYPT_HASH_LENGTH, "%s", hash);
    PlayerData[playerid][pMoney] = START_MONEY;
    PlayerData[playerid][pExp] = 0;
    PlayerData[playerid][pLevel] = 1;
    PlayerData[playerid][pRegPass][0] = EOS;
    PlayerData[playerid][pGarageCity] = PlayerData[playerid][pRegCity];

    printf("[AUTH] New account: %s (id %d)", PlayerData[playerid][pName], id);
    ClearChat(playerid);
    Veh_Load(playerid);
    Veh_GiveStarter(playerid);
    Auth_SpawnPlayer(playerid);

    new money[16];
    FormatMoney(START_MONEY, money);
    ShowPlayerDialog(playerid, DIALOG_WELCOME, DIALOG_STYLE_MSGBOX, C_BRAND SERVER_NAME C_WHITE " | Готово!",
        C_WHITE "Акаунтът ти е създаден. Получаваш " C_GREEN "$%s" C_WHITE " и безплатен бус " C_BRAND "Rumpo" C_WHITE ".\n" \
        "Намираш се в своя гараж в " C_BRAND "%s" C_WHITE ". Качи се в буса, запали с " C_BRAND "2" C_WHITE " и натисни " C_BRAND "H" C_WHITE " до вратата.\n" \
        C_BRAND "N" C_WHITE " – работа и поръчки.\n\n" \
        C_BRAND "Y" C_WHITE " или " C_BRAND "/menu" C_WHITE " – главното меню (работа, GPS, профил, настройки)\n" \
        C_BRAND "/cb [текст]" C_WHITE " – CB радио за шофьорите (канал 19)\n" \
        C_BRAND "/pm [ID] [текст]" C_WHITE " – лично съобщение\n\n" \
        "Приятен път!",
        "Да карам!", "", money, CityNames[PlayerData[playerid][pGarageCity]]);
}

// ---------- Спаун ----------

stock Auth_SpawnPlayer(playerid)
{
    PlayerData[playerid][pLogged] = true;
    SetPlayerScore(playerid, PlayerData[playerid][pLevel]);

    // Спаунът е винаги в личния гараж (виж Garage_OnPlayerSpawn).
    Auth_SetGarageSpawn(playerid);
    TogglePlayerSpectating(playerid, false);
}

stock Auth_SetGarageSpawn(playerid)
{
    SetSpawnInfo(playerid, NO_TEAM, PlayerData[playerid][pSkin],
        GARAGE_X + GARAGE_FOOT_X, GARAGE_Y + GARAGE_FOOT_Y, GARAGE_Z + 1.0, 0.0);
}

stock Auth_OnPlayerSpawn(playerid)
{
    if (!PlayerData[playerid][pLogged]) {
        // Не би трябвало да се случи – спаун без вход.
        Kick(playerid);
        return 0;
    }

    PlayerData[playerid][pSpawned] = true;
    SetPlayerSkin(playerid, PlayerData[playerid][pSkin]);
    Money_Sync(playerid);

    // И следващите спауни (след смърт) са в гаража.
    Auth_SetGarageSpawn(playerid);
    return 1;
}

// ---------- Запис ----------

stock Auth_SaveAccount(playerid)
{
    if (!PlayerData[playerid][pLogged] || PlayerData[playerid][pID] <= 0) return;

    if (PlayerData[playerid][pSpawned]) {
        new PLAYER_STATE:st = GetPlayerState(playerid);
        if (st != PLAYER_STATE_WASTED && st != PLAYER_STATE_SPECTATING && GetPlayerInterior(playerid) == 0) {
            GetPlayerPos(playerid, PlayerData[playerid][pPosX], PlayerData[playerid][pPosY], PlayerData[playerid][pPosZ]);
            GetPlayerFacingAngle(playerid, PlayerData[playerid][pPosA]);
            PlayerData[playerid][pHasPos] = true;
        }
    }

    new q[512];
    format(q, sizeof (q),
        "UPDATE accounts SET skin = %d, money = %d, exp = %d, level = %d, playtime = %d, " \
        "pos_x = %f, pos_y = %f, pos_z = %f, pos_a = %f, has_pos = %d, " \
        "cb_channel = %d, cb_on = %d, hud_on = %d WHERE id = %d",
        PlayerData[playerid][pSkin], PlayerData[playerid][pMoney], PlayerData[playerid][pExp],
        PlayerData[playerid][pLevel], PlayerData[playerid][pPlayTime],
        PlayerData[playerid][pPosX], PlayerData[playerid][pPosY], PlayerData[playerid][pPosZ], PlayerData[playerid][pPosA],
        PlayerData[playerid][pHasPos] ? 1 : 0,
        PlayerData[playerid][pCBChannel], PlayerData[playerid][pCBOn] ? 1 : 0, PlayerData[playerid][pHudOn] ? 1 : 0,
        PlayerData[playerid][pID]);
    Database_Exec(q);
}

// ---------- Диалози ----------

stock Auth_OnDialogResponse(playerid, dialogid, response, listitem, const inputtext[])
{
    switch (dialogid) {
        case DIALOG_KICK_INFO: {
            Kick(playerid);
            return 1;
        }
        case DIALOG_LOGIN: {
            if (!response) {
                Kick(playerid);
                return 1;
            }
            Pass_Verify(playerid, PASS_LOGIN, inputtext, PlayerData[playerid][pPassHash]);
            return 1;
        }
        case DIALOG_REG_WELCOME: {
            if (!response) {
                Kick(playerid);
                return 1;
            }
            Auth_ShowPassword(playerid, "");
            return 1;
        }
        case DIALOG_REG_PASSWORD: {
            if (!response) {
                Auth_ShowWelcome(playerid);
                return 1;
            }
            new len = strlen(inputtext);
            if (len < PASSWORD_MIN || len > PASSWORD_MAX) {
                Auth_ShowPassword(playerid, "Паролата е с грешна дължина.");
                return 1;
            }
            if (strfind(inputtext, PlayerData[playerid][pName], true) != -1) {
                Auth_ShowPassword(playerid, "Паролата не може да съдържа ника ти.");
                return 1;
            }
            format(PlayerData[playerid][pRegPass], PASSWORD_MAX + 1, "%s", inputtext);
            Auth_ShowConfirm(playerid, "");
            return 1;
        }
        case DIALOG_REG_CONFIRM: {
            if (!response) {
                Auth_ShowPassword(playerid, "");
                return 1;
            }
            if (strcmp(inputtext, PlayerData[playerid][pRegPass], false) != 0 || inputtext[0] == EOS) {
                Auth_ShowPassword(playerid, "Паролите не съвпадат. Започни отначало.");
                return 1;
            }
            Auth_ShowGender(playerid);
            return 1;
        }
        case DIALOG_REG_GENDER: {
            if (!response) {
                Auth_ShowConfirm(playerid, "");
                return 1;
            }
            PlayerData[playerid][pRegGender] = (listitem == 1) ? 1 : 0;
            Auth_ShowSkin(playerid);
            return 1;
        }
        case DIALOG_REG_CITY: {
            if (!response) {
                Auth_ShowSkin(playerid);
                return 1;
            }
            if (listitem < 0 || listitem >= MAX_CITIES) listitem = CITY_LS;
            PlayerData[playerid][pRegCity] = listitem;
            Pass_Hash(playerid, PASS_REGISTER, PlayerData[playerid][pRegPass]);
            return 1;
        }
        case DIALOG_WELCOME: {
            return 1;
        }
    }
    return 0;
}
