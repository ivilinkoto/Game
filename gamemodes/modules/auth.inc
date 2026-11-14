// Регистрация и вход.
//
// Ред на регистрацията:
//   Добре дошъл -> Парола (+ потвърждение) -> Имейл -> Пол -> Възраст
//   -> Откъде научи -> Покана (по избор) -> Промокод (по избор)
//   -> Обобщение -> Избор на скин (skin_select.inc) -> създаване на акаунта

#define REG_TOTAL_STEPS 7

static const SourceNames[][] = {
    "TikTok",
    "YouTube",
    "Instagram / Facebook",
    "Discord",
    "От приятел",
    "От друг сървър",
    "Друго"
};

// ============================================================
//  Свързване
// ============================================================

stock Auth_OnPlayerConnect(playerid)
{
    ResetPlayerData(playerid);
    Pass_OnPlayerConnect(playerid);
    GetPlayerName(playerid, PlayerData[playerid][pName], MAX_PLAYER_NAME + 1);
    ClearChat(playerid);

    if (!IsValidRPName(PlayerData[playerid][pName])) {
        return 1; // ще покажем обяснение в Auth_ShowAuth
    }

    if (Bans_CheckPlayer(playerid)) {
        return 1; // ще покажем прозорец за бана в Auth_ShowAuth
    }

    new q[160];
    format(q, sizeof (q), "SELECT password, salt FROM accounts WHERE name = '%s' LIMIT 1", PlayerData[playerid][pName]);
    new DBResult:result = Database_Query(q);
    if (result != DBResult:0) {
        if (DB_GetRowCount(result) > 0) {
            PlayerData[playerid][pHasAccount] = true;
            DB_GetFieldStringByName(result, "password", PlayerData[playerid][pPassHash], 65);
            DB_GetFieldStringByName(result, "salt", PlayerData[playerid][pSalt], 17);
        }
        DB_FreeResultSet(result);
    }
    return 1;
}

stock Auth_OnPlayerRequestClass(playerid)
{
    if (PlayerData[playerid][pLogged] || PlayerData[playerid][pInSkinSelect]) return 1;
    if (PlayerData[playerid][pAuthShown]) return 1;

    PlayerData[playerid][pAuthShown] = true;
    TogglePlayerSpectating(playerid, true);
    // Камерата не се задава надеждно веднага след spectate – изчакваме малко.
    SetTimerEx("Auth_ShowAuth", 300, false, "i", playerid);
    return 1;
}

forward Auth_ShowAuth(playerid);
public Auth_ShowAuth(playerid)
{
    if (!IsPlayerConnected(playerid) || PlayerData[playerid][pLogged]) return 0;

    // Изглед над Los Santos
    SetPlayerCameraPos(playerid, 1500.0, -1700.0, 120.0);
    SetPlayerCameraLookAt(playerid, 1750.0, -1850.0, 30.0);

    if (!IsValidRPName(PlayerData[playerid][pName])) {
        Auth_ShowBadName(playerid);
    } else if (PlayerData[playerid][pBanned]) {
        Bans_ShowDialog(playerid);
    } else if (PlayerData[playerid][pHasAccount]) {
        Auth_ShowLogin(playerid, "");
    } else {
        Auth_ShowRegWelcome(playerid);
    }
    return 1;
}

// ============================================================
//  Диалози
// ============================================================

stock Auth_AppendError(body[], size, const error[])
{
    if (error[0] != EOS) {
        new line[160];
        format(line, sizeof (line), "\n\n" C_RED "» %s", error);
        strcat(body, line, size);
    }
}

stock Auth_ShowBadName(playerid)
{
    new body[1024];
    format(body, sizeof (body),
        C_WHITE "Здравей! За съжаление името " C_RED "%s" C_WHITE " не може да се използва.\n\n" \
        "В " C_BRAND "HOTFIX ONLINE" C_WHITE " всеки играч е герой с истинско име и фамилия.\n\n" \
        C_BRAND "»" C_WHITE " Формат: " C_GOLD "Име_Фамилия\n" \
        C_BRAND "»" C_WHITE " Само латински букви, с главни начални\n" \
        C_BRAND "»" C_WHITE " Пример: " C_GOLD "Ivan_Petrov" C_WHITE ", " C_GOLD "Maria_Georgieva\n\n" \
        C_GREY "Смени името си в лаунчера и влез отново. Очакваме те!",
        PlayerData[playerid][pName]);
    ShowPlayerDialog(playerid, DIALOG_INFO_KICK, DIALOG_STYLE_MSGBOX, C_BRAND "HOTFIX ONLINE " C_WHITE "| Невалидно име", "%s", "Разбрах", "", body);
}

stock Auth_ShowLogin(playerid, const error[])
{
    new body[1024], rpName[MAX_PLAYER_NAME + 1];
    GetRPName(playerid, rpName);
    format(body, sizeof (body),
        C_WHITE "Добре дошъл отново, " C_BRAND "%s" C_WHITE "!\n\n" \
        "Радваме се да те видим пак в " C_BRAND "HOTFIX ONLINE" C_WHITE ".\n" \
        "Въведи паролата си, за да продължиш оттам, докъдето стигна.\n\n" \
        C_GREY "Забравена парола? Пиши на администрацията в Discord: %s",
        rpName, SERVER_DISCORD);
    Auth_AppendError(body, sizeof (body), error);
    ShowPlayerDialog(playerid, DIALOG_LOGIN, DIALOG_STYLE_PASSWORD, C_BRAND "HOTFIX ONLINE " C_WHITE "| Вход", "%s", "Вход", "Изход", body);
}

stock Auth_ShowRegWelcome(playerid)
{
    new body[1500], rpName[MAX_PLAYER_NAME + 1];
    GetRPName(playerid, rpName);
    format(body, sizeof (body),
        C_WHITE "Здравей, " C_BRAND "%s" C_WHITE "!\n\n" \
        "Добре дошъл в " C_BRAND "HOTFIX ONLINE" C_WHITE " – лайт RolePlay сървър,\n" \
        "създаден специално за игра от телефон.\n\n" \
        C_BRAND "»" C_WHITE " Започваш нов живот в " C_GOLD "Los Santos\n" \
        C_BRAND "»" C_WHITE " Изкарвай пари, купувай имоти и се развивай\n" \
        C_BRAND "»" C_WHITE " Скоро: работи, бизнеси, къщи и фракции\n" \
        C_BRAND "»" C_WHITE " Приятелска общност и активна администрация\n\n" \
        C_GREY "Регистрацията отнема по-малко от минута.\n" \
        C_GREY "Продължавайки, приемаш правилата на сървъра.",
        rpName);
    ShowPlayerDialog(playerid, DIALOG_REG_WELCOME, DIALOG_STYLE_MSGBOX, C_BRAND "HOTFIX ONLINE " C_WHITE "| Добре дошъл", "%s", "Напред", "Изход", body);
}

stock Auth_ShowRegPassword(playerid, const error[])
{
    new body[1500];
    format(body, sizeof (body),
        C_BRAND "СТЪПКА 1 от %d " C_GREY "| Парола\n\n" \
        C_WHITE "Измисли парола за акаунта " C_BRAND "%s" C_WHITE ".\n\n" \
        C_BRAND "»" C_WHITE " Между " C_GOLD "%d" C_WHITE " и " C_GOLD "%d" C_WHITE " символа\n" \
        C_BRAND "»" C_WHITE " Не използвай същата парола като в други игри\n" \
        C_BRAND "»" C_WHITE " Никога не я казвай на никого – дори на администратор\n\n" \
        C_GREY "Паролата се пази криптирана и никой не може да я види.",
        REG_TOTAL_STEPS, PlayerData[playerid][pName], MIN_PASSWORD_LENGTH, MAX_PASSWORD_LENGTH);
    Auth_AppendError(body, sizeof (body), error);
    ShowPlayerDialog(playerid, DIALOG_REG_PASSWORD, DIALOG_STYLE_PASSWORD, C_BRAND "Регистрация " C_WHITE "| Парола", "%s", "Напред", "Назад", body);
}

stock Auth_ShowRegPasswordConfirm(playerid)
{
    new body[512];
    format(body, sizeof (body),
        C_BRAND "СТЪПКА 1 от %d " C_GREY "| Потвърждение\n\n" \
        C_WHITE "Въведи паролата " C_BRAND "още веднъж" C_WHITE ", за да сме сигурни,\n" \
        "че няма грешка при писането.",
        REG_TOTAL_STEPS);
    ShowPlayerDialog(playerid, DIALOG_REG_PASSWORD_CONFIRM, DIALOG_STYLE_PASSWORD, C_BRAND "Регистрация " C_WHITE "| Парола", "%s", "Напред", "Назад", body);
}

stock Auth_ShowRegEmail(playerid, const error[])
{
    new body[1500];
    format(body, sizeof (body),
        C_BRAND "СТЪПКА 2 от %d " C_GREY "| Имейл\n\n" \
        C_WHITE "Въведи своя имейл адрес.\n\n" \
        C_BRAND "»" C_WHITE " Ще ти трябва, ако някога забравиш паролата си\n" \
        C_BRAND "»" C_WHITE " Използваме го само за неща, свързани с акаунта ти\n" \
        C_BRAND "»" C_WHITE " Никога не го показваме на други играчи\n\n" \
        C_GREY "Пример: ivan.petrov@gmail.com",
        REG_TOTAL_STEPS);
    Auth_AppendError(body, sizeof (body), error);
    ShowPlayerDialog(playerid, DIALOG_REG_EMAIL, DIALOG_STYLE_INPUT, C_BRAND "Регистрация " C_WHITE "| Имейл", "%s", "Напред", "Назад", body);
}

stock Auth_ShowRegGender(playerid)
{
    new title[96];
    format(title, sizeof (title), C_BRAND "Стъпка 3 от %d " C_WHITE "| Пол на героя", REG_TOTAL_STEPS);
    ShowPlayerDialog(playerid, DIALOG_REG_GENDER, DIALOG_STYLE_LIST, title,
        C_BLUE "Мъж\n{FF8AD8}Жена", "Избери", "Назад");
}

stock Auth_ShowRegAge(playerid, const error[])
{
    new body[1024];
    format(body, sizeof (body),
        C_BRAND "СТЪПКА 4 от %d " C_GREY "| Възраст\n\n" \
        C_WHITE "На колко години си?\n\n" \
        C_GREY "Питаме, за да организираме събития и правила,\n" \
        C_GREY "подходящи за нашата общност. Въведи само число.",
        REG_TOTAL_STEPS);
    Auth_AppendError(body, sizeof (body), error);
    ShowPlayerDialog(playerid, DIALOG_REG_AGE, DIALOG_STYLE_INPUT, C_BRAND "Регистрация " C_WHITE "| Възраст", "%s", "Напред", "Назад", body);
}

stock Auth_ShowRegSource(playerid)
{
    new body[512], title[96];
    body[0] = EOS;
    for (new i = 0; i < sizeof (SourceNames); i++) {
        strcat(body, SourceNames[i]);
        if (i < sizeof (SourceNames) - 1) strcat(body, "\n");
    }
    format(title, sizeof (title), C_BRAND "Стъпка 5 от %d " C_WHITE "| Откъде научи за нас?", REG_TOTAL_STEPS);
    ShowPlayerDialog(playerid, DIALOG_REG_SOURCE, DIALOG_STYLE_LIST, title, "%s", "Избери", "Назад", body);
}

stock Auth_ShowRegReferral(playerid, const error[])
{
    new body[1500];
    format(body, sizeof (body),
        C_BRAND "СТЪПКА 6 от %d " C_GREY "| Покана от приятел\n\n" \
        C_WHITE "Покани ли те някой в " C_BRAND "HOTFIX ONLINE" C_WHITE "?\n\n" \
        "Въведи името на играча във формат " C_GOLD "Име_Фамилия" C_WHITE ".\n" \
        "Когато се развиеш в сървъра, " C_GREEN "и двамата ще получите награда" C_WHITE ".\n\n" \
        C_GREY "Ако никой не те е поканил, натисни \"Пропусни\".",
        REG_TOTAL_STEPS);
    Auth_AppendError(body, sizeof (body), error);
    ShowPlayerDialog(playerid, DIALOG_REG_REFERRAL, DIALOG_STYLE_INPUT, C_BRAND "Регистрация " C_WHITE "| Покана", "%s", "Напред", "Пропусни", body);
}

stock Auth_ShowRegPromo(playerid, const error[])
{
    new body[1500];
    format(body, sizeof (body),
        C_BRAND "СТЪПКА 7 от %d " C_GREY "| Промокод\n\n" \
        C_WHITE "Имаш ли промокод?\n\n" \
        "Промокодовете дават " C_GREEN "бонус пари" C_WHITE " за добър старт.\n" \
        "Търси ги в нашия Discord, при стриймъри и по време на събития.\n\n" \
        C_GREY "Ако нямаш код, натисни \"Пропусни\".",
        REG_TOTAL_STEPS);
    Auth_AppendError(body, sizeof (body), error);
    ShowPlayerDialog(playerid, DIALOG_REG_PROMO, DIALOG_STYLE_INPUT, C_BRAND "Регистрация " C_WHITE "| Промокод", "%s", "Напред", "Пропусни", body);
}

stock Auth_ShowRegSummary(playerid)
{
    new body[2048], line[160], rpName[MAX_PLAYER_NAME + 1], money[16];
    GetRPName(playerid, rpName);

    format(body, sizeof (body), C_WHITE "Провери данните си, преди да продължиш:\n\n");

    format(line, sizeof (line), C_GREY "Име: " C_WHITE "%s\n", rpName);
    strcat(body, line);
    format(line, sizeof (line), C_GREY "Имейл: " C_WHITE "%s\n", PlayerData[playerid][pEmail]);
    strcat(body, line);
    if (PlayerData[playerid][pGender] == GENDER_FEMALE) {
        strcat(body, C_GREY "Пол: " C_WHITE "Жена\n");
    } else {
        strcat(body, C_GREY "Пол: " C_WHITE "Мъж\n");
    }
    format(line, sizeof (line), C_GREY "Възраст: " C_WHITE "%d\n", PlayerData[playerid][pAge]);
    strcat(body, line);
    format(line, sizeof (line), C_GREY "Откъде научи: " C_WHITE "%s\n", PlayerData[playerid][pSource]);
    strcat(body, line);

    if (PlayerData[playerid][pReferrerID] > 0) {
        new refName[MAX_PLAYER_NAME + 1];
        Auth_GetAccountName(PlayerData[playerid][pReferrerID], refName);
        format(line, sizeof (line), C_GREY "Поканен от: " C_WHITE "%s\n", refName);
    } else {
        format(line, sizeof (line), C_GREY "Поканен от: " C_WHITE "няма\n");
    }
    strcat(body, line);

    FormatNumber(START_MONEY, money);
    format(line, sizeof (line), "\n" C_GREY "Начални пари: " C_GREEN "$%s\n", money);
    strcat(body, line);

    if (PlayerData[playerid][pPromoCode][0] != EOS) {
        FormatNumber(PlayerData[playerid][pPromoMoney], money);
        format(line, sizeof (line), C_GREY "Промокод " C_GOLD "%s" C_GREY ": " C_GREEN "+$%s\n",
            PlayerData[playerid][pPromoCode], money);
        strcat(body, line);
    }

    strcat(body, "\n" C_WHITE "Последна стъпка – " C_BRAND "избери как ще изглежда героят ти" C_WHITE "!");
    ShowPlayerDialog(playerid, DIALOG_REG_SUMMARY, DIALOG_STYLE_MSGBOX, C_BRAND "Регистрация " C_WHITE "| Обобщение", "%s", "Към скина", "Назад", body);
}

stock Auth_ShowRegComplete(playerid)
{
    new body[1500], rpName[MAX_PLAYER_NAME + 1], money[16];
    GetRPName(playerid, rpName);
    FormatNumber(PlayerData[playerid][pMoney], money);
    format(body, sizeof (body),
        C_GREEN "Регистрацията е успешна!\n\n" \
        C_WHITE "Добре дошъл в " C_GOLD "Los Santos" C_WHITE ", " C_BRAND "%s" C_WHITE "!\n\n" \
        C_BRAND "»" C_WHITE " Получи " C_GREEN "$%s" C_WHITE " за начало\n" \
        C_BRAND "»" C_WHITE " Намираш се на гара " C_GOLD "Unity" C_WHITE " – оттук започва всичко\n" \
        C_BRAND "»" C_WHITE " Натисни " C_GOLD "Y" C_WHITE ", за да отвориш менюто с всичко важно\n" \
        C_BRAND "»" C_WHITE " Сървърът е в активна разработка – нови системи идват редовно\n\n" \
        C_GREY "Приятна игра от екипа на HOTFIX ONLINE!",
        rpName, money);
    ShowPlayerDialog(playerid, DIALOG_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "HOTFIX ONLINE " C_WHITE "| Добре дошъл", "%s", "Да играем!", "", body);
}

// ============================================================
//  Отговори на диалозите
// ============================================================

// Връща 1, ако диалогът е обработен тук.
stock Auth_OnDialogResponse(playerid, dialogid, response, listitem, const inputtext[])
{
    switch (dialogid) {
        case DIALOG_INFO_KICK: {
            KickEx(playerid);
            return 1;
        }

        case DIALOG_LOGIN: {
            if (!response) {
                KickEx(playerid);
                return 1;
            }

            if (Pass_IsLegacy(PlayerData[playerid][pSalt])) {
                // Стара SHA-256 парола: проверяваме веднага и я прехвърляме към BCrypt.
                if (Pass_LegacyCheck(inputtext, PlayerData[playerid][pSalt], PlayerData[playerid][pPassHash])) {
                    Pass_Hash(playerid, PASS_REHASH_ACCOUNT, inputtext);
                    Auth_OnLoginResult(playerid, true);
                } else {
                    Auth_OnLoginResult(playerid, false);
                }
            } else {
                Pass_Verify(playerid, PASS_LOGIN, inputtext, PlayerData[playerid][pPassHash]);
            }
            return 1;
        }

        case DIALOG_REG_WELCOME: {
            if (!response) {
                KickEx(playerid);
                return 1;
            }
            Auth_ShowRegPassword(playerid, "");
            return 1;
        }

        case DIALOG_REG_PASSWORD: {
            if (!response) {
                Auth_ShowRegWelcome(playerid);
                return 1;
            }

            new len = strlen(inputtext);
            if (len < MIN_PASSWORD_LENGTH || len > MAX_PASSWORD_LENGTH) {
                Auth_ShowRegPassword(playerid, "Паролата е твърде къса или твърде дълга.");
                return 1;
            }
            // Пазим я само в паметта, докато бъде потвърдена.
            format(PlayerData[playerid][pTempPass], MAX_PASSWORD_LENGTH + 1, "%s", inputtext);
            Auth_ShowRegPasswordConfirm(playerid);
            return 1;
        }

        case DIALOG_REG_PASSWORD_CONFIRM: {
            if (!response) {
                Auth_ShowRegPassword(playerid, "");
                return 1;
            }

            if (inputtext[0] == EOS || strcmp(inputtext, PlayerData[playerid][pTempPass], false) != 0) {
                PlayerData[playerid][pTempPass][0] = EOS;
                Auth_ShowRegPassword(playerid, "Паролите не съвпадат. Опитай отново.");
                return 1;
            }
            // Хешира се асинхронно – продължаваме в Auth_OnRegisterHashed.
            Pass_Hash(playerid, PASS_REGISTER, PlayerData[playerid][pTempPass]);
            PlayerData[playerid][pTempPass][0] = EOS;
            return 1;
        }

        case DIALOG_REG_EMAIL: {
            if (!response) {
                Auth_ShowRegPassword(playerid, "");
                return 1;
            }

            if (!IsValidEmail(inputtext)) {
                Auth_ShowRegEmail(playerid, "Невалиден имейл адрес. Провери за грешки.");
                return 1;
            }
            format(PlayerData[playerid][pEmail], 65, "%s", inputtext);
            Auth_ShowRegGender(playerid);
            return 1;
        }

        case DIALOG_REG_GENDER: {
            if (!response) {
                Auth_ShowRegEmail(playerid, "");
                return 1;
            }

            PlayerData[playerid][pGender] = (listitem == 1) ? GENDER_FEMALE : GENDER_MALE;
            Auth_ShowRegAge(playerid, "");
            return 1;
        }

        case DIALOG_REG_AGE: {
            if (!response) {
                Auth_ShowRegGender(playerid);
                return 1;
            }

            new age = IsNumeric(inputtext) && strlen(inputtext) <= 3 ? strval(inputtext) : 0;
            if (age < MIN_PLAYER_AGE || age > MAX_PLAYER_AGE) {
                new err[80];
                format(err, sizeof (err), "Въведи число между %d и %d.", MIN_PLAYER_AGE, MAX_PLAYER_AGE);
                Auth_ShowRegAge(playerid, err);
                return 1;
            }
            PlayerData[playerid][pAge] = age;
            Auth_ShowRegSource(playerid);
            return 1;
        }

        case DIALOG_REG_SOURCE: {
            if (!response) {
                Auth_ShowRegAge(playerid, "");
                return 1;
            }

            if (listitem < 0 || listitem >= sizeof (SourceNames)) listitem = sizeof (SourceNames) - 1;
            format(PlayerData[playerid][pSource], 32, "%s", SourceNames[listitem]);
            Auth_ShowRegReferral(playerid, "");
            return 1;
        }

        case DIALOG_REG_REFERRAL: {
            PlayerData[playerid][pReferrerID] = 0;
            if (!response || inputtext[0] == EOS) {
                Auth_ShowRegPromo(playerid, "");
                return 1;
            }
            if (!IsValidRPName(inputtext)) {
                Auth_ShowRegReferral(playerid, "Името трябва да е във формат Име_Фамилия.");
                return 1;
            }
            new refId = Auth_GetAccountID(inputtext);
            if (refId == 0) {
                Auth_ShowRegReferral(playerid, "Няма играч с такова име. Провери как се пише.");
                return 1;
            }
            PlayerData[playerid][pReferrerID] = refId;
            Auth_ShowRegPromo(playerid, "");
            return 1;
        }

        case DIALOG_REG_PROMO: {
            PlayerData[playerid][pPromoCode][0] = EOS;
            PlayerData[playerid][pPromoMoney] = 0;
            if (!response || inputtext[0] == EOS) {
                Auth_ShowRegSummary(playerid);
                return 1;
            }
            new money;
            if (!Promo_Check(inputtext, money)) {
                Auth_ShowRegPromo(playerid, "Този промокод не съществува или е изтекъл.");
                return 1;
            }
            format(PlayerData[playerid][pPromoCode], 33, "%s", inputtext);
            PlayerData[playerid][pPromoMoney] = money;
            Auth_ShowRegSummary(playerid);
            return 1;
        }

        case DIALOG_REG_SUMMARY: {
            if (!response) {
                Auth_ShowRegPromo(playerid, "");
                return 1;
            }
            Skin_Start(playerid);
            return 1;
        }
    }
    return 0;
}

// ============================================================
//  Резултати от BCrypt (виж core/passwords.inc)
// ============================================================

stock Auth_OnLoginResult(playerid, bool:success)
{
    if (PlayerData[playerid][pLogged]) return;

    if (success) {
        Auth_LoadAccount(playerid);
        return;
    }

    PlayerData[playerid][pLoginAttempts]++;
    if (PlayerData[playerid][pLoginAttempts] >= MAX_LOGIN_ATTEMPTS) {
        SendError(playerid, "Твърде много грешни опити. Опитай отново по-късно.");
        KickEx(playerid);
    } else {
        new err[80];
        format(err, sizeof (err), "Грешна парола! Остават опити: %d",
            MAX_LOGIN_ATTEMPTS - PlayerData[playerid][pLoginAttempts]);
        Auth_ShowLogin(playerid, err);
    }
}

stock Auth_OnRegisterHashed(playerid, const hash[])
{
    format(PlayerData[playerid][pPassHash], 65, "%s", hash);
    PlayerData[playerid][pSalt][0] = EOS;
    Auth_ShowRegEmail(playerid, "");
}

// Стара парола е прехвърлена към BCrypt.
stock Auth_OnRehashed(playerid, const hash[])
{
    format(PlayerData[playerid][pPassHash], 65, "%s", hash);
    PlayerData[playerid][pSalt][0] = EOS;
    if (PlayerData[playerid][pID] <= 0) return;

    new q[256];
    format(q, sizeof (q), "UPDATE accounts SET password = '%s', salt = '' WHERE id = %d", hash, PlayerData[playerid][pID]);
    Database_Exec(q);
    printf("[ACCOUNT] %s: password upgraded to BCrypt", PlayerData[playerid][pName]);
}

// ============================================================
//  Акаунти в базата
// ============================================================

// ID на акаунт по име, 0 ако няма такъв.
stock Auth_GetAccountID(const name[])
{
    new eName[64], q[160], id = 0;
    SQL_Escape(name, eName);
    format(q, sizeof (q), "SELECT id FROM accounts WHERE name = '%s' LIMIT 1", eName);
    new DBResult:result = Database_Query(q);
    if (result != DBResult:0) {
        if (DB_GetRowCount(result) > 0) id = DB_GetFieldIntByName(result, "id");
        DB_FreeResultSet(result);
    }
    return id;
}

stock Auth_GetAccountName(id, dest[], size = sizeof (dest))
{
    new q[96];
    dest[0] = EOS;
    format(q, sizeof (q), "SELECT name FROM accounts WHERE id = %d LIMIT 1", id);
    new DBResult:result = Database_Query(q);
    if (result != DBResult:0) {
        if (DB_GetRowCount(result) > 0) DB_GetFieldStringByName(result, "name", dest, size);
        DB_FreeResultSet(result);
    }
}

// Извиква се от skin_select.inc след натискане на SELECT.
stock Auth_FinishRegistration(playerid)
{
    if (Auth_GetAccountID(PlayerData[playerid][pName]) != 0) {
        SendError(playerid, "Някой току-що регистрира това име. Влез с друго име.");
        KickEx(playerid);
        return 0;
    }

    new ip[16], eEmail[140], eSource[70], q[1500];
    GetPlayerIp(playerid, ip, sizeof (ip));
    SQL_Escape(PlayerData[playerid][pEmail], eEmail);
    SQL_Escape(PlayerData[playerid][pSource], eSource);

    PlayerData[playerid][pMoney] = START_MONEY + PlayerData[playerid][pPromoMoney];
    PlayerData[playerid][pLevel] = 1;
    PlayerData[playerid][pPosX] = SPAWN_X;
    PlayerData[playerid][pPosY] = SPAWN_Y;
    PlayerData[playerid][pPosZ] = SPAWN_Z;
    PlayerData[playerid][pPosA] = SPAWN_A;
    PlayerData[playerid][pInterior] = 0;
    PlayerData[playerid][pVirtualWorld] = 0;

    format(q, sizeof (q),
        "INSERT INTO accounts (name, password, salt, email, gender, age, source, referrer_id, promo_code, skin, money, level, " \
        "pos_x, pos_y, pos_z, pos_a, register_ip, last_ip, registered_at, last_login) VALUES " \
        "('%s', '%s', '%s', '%s', %d, %d, '%s', %d, '%s', %d, %d, 1, %f, %f, %f, %f, '%s', '%s', %d, %d)",
        PlayerData[playerid][pName], PlayerData[playerid][pPassHash], PlayerData[playerid][pSalt],
        eEmail, PlayerData[playerid][pGender], PlayerData[playerid][pAge], eSource,
        PlayerData[playerid][pReferrerID], PlayerData[playerid][pPromoCode],
        PlayerData[playerid][pSkin], PlayerData[playerid][pMoney],
        SPAWN_X, SPAWN_Y, SPAWN_Z, SPAWN_A, ip, ip, gettime(), gettime());
    Database_Exec(q);

    PlayerData[playerid][pID] = Auth_GetAccountID(PlayerData[playerid][pName]);
    if (PlayerData[playerid][pID] == 0) {
        SendError(playerid, "Регистрацията не успя. Свържи се с администрацията.");
        KickEx(playerid);
        return 0;
    }

    if (PlayerData[playerid][pPromoCode][0] != EOS) {
        Promo_Use(PlayerData[playerid][pPromoCode]);
    }

    PlayerData[playerid][pHasAccount] = true;
    PlayerData[playerid][pLogged] = true;
    PlayerData[playerid][pFirstSpawnDone] = true;
    printf("[ACCOUNT] New registration: %s (ID %d)", PlayerData[playerid][pName], PlayerData[playerid][pID]);

    // От избора на скин – директно на гарата.
    SetSpawnInfo(playerid, NO_TEAM, PlayerData[playerid][pSkin], SPAWN_X, SPAWN_Y, SPAWN_Z, SPAWN_A);
    SetPlayerVirtualWorld(playerid, 0);
    SetPlayerInterior(playerid, 0);
    SetPlayerPos(playerid, SPAWN_X, SPAWN_Y, SPAWN_Z);
    SetPlayerFacingAngle(playerid, SPAWN_A);
    SetPlayerSkin(playerid, PlayerData[playerid][pSkin]);
    TogglePlayerControllable(playerid, true);
    SetCameraBehindPlayer(playerid);
    Money_Sync(playerid);

    ClearChat(playerid);
    Auth_ShowRegComplete(playerid);
    return 1;
}

stock Auth_LoadAccount(playerid)
{
    new q[160];
    format(q, sizeof (q), "SELECT * FROM accounts WHERE name = '%s' LIMIT 1", PlayerData[playerid][pName]);
    new DBResult:result = Database_Query(q);
    if (result == DBResult:0 || DB_GetRowCount(result) == 0) {
        if (result != DBResult:0) DB_FreeResultSet(result);
        SendError(playerid, "Акаунтът не може да се зареди. Свържи се с администрацията.");
        KickEx(playerid);
        return 0;
    }

    PlayerData[playerid][pID]           = DB_GetFieldIntByName(result, "id");
    PlayerData[playerid][pGender]       = DB_GetFieldIntByName(result, "gender");
    PlayerData[playerid][pAge]          = DB_GetFieldIntByName(result, "age");
    PlayerData[playerid][pSkin]         = DB_GetFieldIntByName(result, "skin");
    PlayerData[playerid][pMoney]        = DB_GetFieldIntByName(result, "money");
    PlayerData[playerid][pLevel]        = DB_GetFieldIntByName(result, "level");
    PlayerData[playerid][pExp]          = DB_GetFieldIntByName(result, "exp");
    PlayerData[playerid][pAdmin]        = DB_GetFieldIntByName(result, "admin");
    PlayerData[playerid][pPlayedMinutes] = DB_GetFieldIntByName(result, "played_minutes");
    PlayerData[playerid][pPaydayMinutes] = DB_GetFieldIntByName(result, "payday_minutes");
    PlayerData[playerid][pPosX]         = DB_GetFieldFloatByName(result, "pos_x");
    PlayerData[playerid][pPosY]         = DB_GetFieldFloatByName(result, "pos_y");
    PlayerData[playerid][pPosZ]         = DB_GetFieldFloatByName(result, "pos_z");
    PlayerData[playerid][pPosA]         = DB_GetFieldFloatByName(result, "pos_a");
    PlayerData[playerid][pInterior]     = DB_GetFieldIntByName(result, "interior");
    PlayerData[playerid][pVirtualWorld] = DB_GetFieldIntByName(result, "virtual_world");
    DB_GetFieldStringByName(result, "email", PlayerData[playerid][pEmail], 65);
    DB_GetFieldStringByName(result, "admin_password", PlayerData[playerid][pAdminPass], 65);
    DB_GetFieldStringByName(result, "admin_salt", PlayerData[playerid][pAdminSalt], 17);
    PlayerData[playerid][pReportBlockedUntil] = DB_GetFieldIntByName(result, "report_block_until");
    DB_FreeResultSet(result);

    if (PlayerData[playerid][pPosX] == 0.0 && PlayerData[playerid][pPosY] == 0.0) {
        PlayerData[playerid][pPosX] = SPAWN_X;
        PlayerData[playerid][pPosY] = SPAWN_Y;
        PlayerData[playerid][pPosZ] = SPAWN_Z;
        PlayerData[playerid][pPosA] = SPAWN_A;
        PlayerData[playerid][pInterior] = 0;
        PlayerData[playerid][pVirtualWorld] = 0;
    }

    PlayerData[playerid][pLogged] = true;

    new ip[16];
    GetPlayerIp(playerid, ip, sizeof (ip));
    format(q, sizeof (q), "UPDATE accounts SET last_login = %d, last_ip = '%s' WHERE id = %d",
        gettime(), ip, PlayerData[playerid][pID]);
    Database_Exec(q);

    ClearChat(playerid);
    new rpName[MAX_PLAYER_NAME + 1];
    GetRPName(playerid, rpName);
    SendClientMessage(playerid, COLOR_BRAND, "[HOTFIX] {FFFFFF}Успешен вход. Добре дошъл отново, {FF6A00}%s{FFFFFF}!", rpName);

    SetSpawnInfo(playerid, NO_TEAM, PlayerData[playerid][pSkin],
        PlayerData[playerid][pPosX], PlayerData[playerid][pPosY], PlayerData[playerid][pPosZ], PlayerData[playerid][pPosA]);
    TogglePlayerSpectating(playerid, false);
    return 1;
}

stock Auth_SaveAccount(playerid)
{
    if (!PlayerData[playerid][pLogged] || PlayerData[playerid][pID] <= 0) return 0;

    if (PlayerData[playerid][pFirstSpawnDone]) {
        new PLAYER_STATE:pState = GetPlayerState(playerid);
        if (pState != PLAYER_STATE_WASTED && pState != PLAYER_STATE_SPECTATING) {
            new Float:x, Float:y, Float:z, Float:a;
            GetPlayerPos(playerid, x, y, z);
            GetPlayerFacingAngle(playerid, a);
            PlayerData[playerid][pPosX] = x;
            PlayerData[playerid][pPosY] = y;
            PlayerData[playerid][pPosZ] = z;
            PlayerData[playerid][pPosA] = a;
            PlayerData[playerid][pInterior] = GetPlayerInterior(playerid);
            PlayerData[playerid][pVirtualWorld] = GetPlayerVirtualWorld(playerid);
        }
    }

    new q[512];
    format(q, sizeof (q),
        "UPDATE accounts SET skin = %d, money = %d, level = %d, exp = %d, admin = %d, played_minutes = %d, payday_minutes = %d, " \
        "pos_x = %f, pos_y = %f, pos_z = %f, pos_a = %f, interior = %d, virtual_world = %d WHERE id = %d",
        PlayerData[playerid][pSkin], PlayerData[playerid][pMoney], PlayerData[playerid][pLevel],
        PlayerData[playerid][pExp], PlayerData[playerid][pAdmin], PlayerData[playerid][pPlayedMinutes], PlayerData[playerid][pPaydayMinutes],
        PlayerData[playerid][pPosX], PlayerData[playerid][pPosY], PlayerData[playerid][pPosZ], PlayerData[playerid][pPosA],
        PlayerData[playerid][pInterior], PlayerData[playerid][pVirtualWorld], PlayerData[playerid][pID]);
    Database_Exec(q);
    return 1;
}


// ============================================================
//  Спаун
// ============================================================

stock Auth_ShowLoginTip(playerid)
{
    SendClientMessage(playerid, COLOR_BRAND, "[HOTFIX] {FFFFFF}Натисни {FFD54A}Y{FFFFFF}, за да отвориш менюто.");
    if (PlayerData[playerid][pAdmin] > 0) {
        new rank[32];
        Admin_GetRankName(PlayerData[playerid][pAdmin], rank);
        if (PlayerData[playerid][pAdminPass][0] == EOS) {
            SendClientMessage(playerid, COLOR_ADMIN, "[Админ] Ти си %s. Създай си админ парола с /alogin.", rank);
        } else {
            SendClientMessage(playerid, COLOR_ADMIN, "[Админ] Ти си %s. Влез в админ системата с /alogin.", rank);
        }
    }
}

stock Auth_OnPlayerSpawn(playerid)
{
    if (PlayerData[playerid][pInSkinSelect]) {
        Skin_OnPlayerSpawn(playerid);
        return 1;
    }

    if (!PlayerData[playerid][pLogged]) {
        SendError(playerid, "Трябва да влезеш в акаунта си.");
        KickEx(playerid);
        return 1;
    }

    SetPlayerSkin(playerid, PlayerData[playerid][pSkin]);
    SetPlayerScore(playerid, PlayerData[playerid][pLevel]);
    Money_Sync(playerid);

    if (Hospital_OnPlayerSpawn(playerid)) return 1;

    if (!PlayerData[playerid][pFirstSpawnDone]) {
        PlayerData[playerid][pFirstSpawnDone] = true;
        SetPlayerInterior(playerid, PlayerData[playerid][pInterior]);
        SetPlayerVirtualWorld(playerid, PlayerData[playerid][pVirtualWorld]);
        SetCameraBehindPlayer(playerid);
        Auth_ShowLoginTip(playerid);
    }
    return 1;
}
