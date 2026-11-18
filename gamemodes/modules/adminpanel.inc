// Админ панел (/apanel или Меню > Админ панел) – всичко през прозорци, команди почти не трябват.
//
//  Статистика на играч   – ID (онлайн) или ник (и офлайн)
//  Промяна на играч      – ниво, пари, админ ниво, телепорт, кик, бан, нахрани...
//  Места на картата      – добавяне/триене/местене: магазини, тоалетни, бензиностанции, GPS точки,
//                          бази на компании, входове на гаражи и автокъщи
//  Телепорти             – списък с места
//  Съобщение до всички
//  Име на сървъра
//  Рестарт на сървъра

#define AP_RESTART_SECONDS  10

enum {
    AP_STATS,
    AP_EDIT,
    AP_PLACES,
    AP_TELEPORT,
    AP_ANNOUNCE,
    AP_SERVERNAME,
    AP_RESTART
}

// Избраният играч: онлайн (playerid) или офлайн (ID на акаунт + ник)
static ApTarget[MAX_PLAYERS];                       // playerid или INVALID_PLAYER_ID
static ApTargetAccount[MAX_PLAYERS];
static ApTargetName[MAX_PLAYERS][MAX_PLAYER_NAME + 1];
static ApLookupFor[MAX_PLAYERS];                    // AP_STATS или AP_EDIT
static ApEditAction[MAX_PLAYERS];
static ApPlaceAction[MAX_PLAYERS];
static Float:ApHousePos[MAX_PLAYERS][4];
static ApHousePrice[MAX_PLAYERS];
static ApPickMap[MAX_PLAYERS][MAX_COMPANIES];
static ApNewName[MAX_PLAYERS][32];
static ApNewPlace[MAX_PLAYERS][24];
static ApNewSize[MAX_PLAYERS];
static ApNewProduces[MAX_PLAYERS];
static ApNewAccepts[MAX_PLAYERS];
static bool:ApGoodsAccepts[MAX_PLAYERS];     // false = избираме "дава", true = "приема"
static bool:ApRestarting;
static ApRestartLeft;

// ---------- Настройки на сървъра (текст) ----------

stock AdminPanel_Init()
{
    Database_Exec("CREATE TABLE IF NOT EXISTS server_settings (key TEXT PRIMARY KEY, value TEXT NOT NULL)");
    new name[64];
    if (AdminPanel_GetSetting("server_name", name, sizeof (name)) && name[0] != EOS) {
        SendRconCommand("hostname %s", name);
    }
}

static stock AdminPanel_GetSetting(const key[], dest[], size)
{
    new q[128], bool:found = false;
    dest[0] = EOS;
    format(q, sizeof (q), "SELECT value FROM server_settings WHERE key = '%s'", key);
    new DBResult:r = Database_Query(q);
    if (r != DBResult:0) {
        if (DB_GetRowCount(r) > 0) {
            DB_GetFieldStringByName(r, "value", dest, size);
            found = true;
        }
        DB_FreeResultSet(r);
    }
    return found;
}

static stock AdminPanel_SetSetting(const key[], const value[])
{
    new safe[160], q[256];
    SQL_Escape(value, safe);
    format(q, sizeof (q), "INSERT OR REPLACE INTO server_settings (key, value) VALUES ('%s', '%s')", key, safe);
    Database_Exec(q);
}

// ---------- Главно меню ----------

stock AdminPanel_Show(playerid)
{
    new lvl[24], title[64];
    Admin_GetLevelName(PlayerData[playerid][pAdmin], lvl);
    format(title, sizeof (title), C_ADMIN "Админ панел" C_WHITE " | %s", lvl);
    ShowPlayerDialog(playerid, DIALOG_AP_MAIN, DIALOG_STYLE_LIST, title,
        "Статистика на играч\nПромяна на играч\nМеста на картата\nТелепорти\nСъобщение до всички\nИме на сървъра\n" C_RED "Рестарт на сървъра",
        "Избери", "Затвори");
}

static stock bool:AdminPanel_Need(playerid, level)
{
    if (PlayerData[playerid][pAdmin] >= level) return true;
    SendError(playerid, "Нямаш достатъчно високо админ ниво за това.");
    AdminPanel_Show(playerid);
    return false;
}

// ---------- Търсене на играч ----------

static stock AdminPanel_AskPlayer(playerid, purpose, const error[] = "")
{
    ApLookupFor[playerid] = purpose;
    new err[96];
    if (error[0] != EOS) format(err, sizeof (err), "\n\n" C_RED "%s", error);
    ShowPlayerDialog(playerid, DIALOG_AP_PLAYER, DIALOG_STYLE_INPUT, C_ADMIN "Избор на играч",
        C_WHITE "Въведи " C_BRAND "ID" C_WHITE " на играч онлайн или " C_BRAND "ник" C_WHITE " (и за офлайн играчи):%s",
        "Напред", "Назад", err);
}

// Намира играча: онлайн по ID/ник или офлайн по ник. true = намерен.
static stock bool:AdminPanel_Resolve(playerid, const input[])
{
    new targetid = FindPlayer(input);
    if (targetid != INVALID_PLAYER_ID) {
        ApTarget[playerid] = targetid;
        ApTargetAccount[playerid] = PlayerData[targetid][pID];
        format(ApTargetName[playerid], MAX_PLAYER_NAME + 1, "%s", PlayerData[targetid][pName]);
        return true;
    }
    if (!IsValidNickname(input)) return false;
    new q[128];
    format(q, sizeof (q), "SELECT id, name FROM accounts WHERE name = '%s' LIMIT 1", input);
    new DBResult:r = Database_Query(q), bool:found = false;
    if (r != DBResult:0) {
        if (DB_GetRowCount(r) > 0) {
            ApTarget[playerid] = INVALID_PLAYER_ID;
            ApTargetAccount[playerid] = DB_GetFieldIntByName(r, "id");
            DB_GetFieldStringByName(r, "name", ApTargetName[playerid], MAX_PLAYER_NAME + 1);
            found = true;
        }
        DB_FreeResultSet(r);
    }
    // Ако е онлайн под този ник (въведен изцяло) – ползваме онлайн данните
    if (found) {
        for (new i = 0; i < MAX_PLAYERS; i++) {
            if (IsPlayerLogged(i) && PlayerData[i][pID] == ApTargetAccount[playerid]) ApTarget[playerid] = i;
        }
    }
    return found;
}

static stock bool:AdminPanel_TargetOnline(playerid)
{
    new t = ApTarget[playerid];
    return (t != INVALID_PLAYER_ID && IsPlayerLogged(t) && PlayerData[t][pID] == ApTargetAccount[playerid]);
}

// ---------- Статистика ----------

static stock AdminPanel_ShowStats(playerid)
{
    new body[1400], line[160], money[16];
    new level, exp, cash, admin, playtime, city, lastLogin, registered;
    new bool:online = AdminPanel_TargetOnline(playerid);

    if (online) {
        new t = ApTarget[playerid];
        level = PlayerData[t][pLevel];
        exp = PlayerData[t][pExp];
        cash = PlayerData[t][pMoney];
        admin = PlayerData[t][pAdmin];
        playtime = PlayerData[t][pPlayTime];
        city = PlayerData[t][pGarageCity];
    }
    new q[160];
    format(q, sizeof (q), "SELECT * FROM accounts WHERE id = %d", ApTargetAccount[playerid]);
    new DBResult:r = Database_Query(q);
    if (r != DBResult:0) {
        if (DB_GetRowCount(r) > 0) {
            if (!online) {
                level = DB_GetFieldIntByName(r, "level");
                exp = DB_GetFieldIntByName(r, "exp");
                cash = DB_GetFieldIntByName(r, "money");
                admin = DB_GetFieldIntByName(r, "admin");
                playtime = DB_GetFieldIntByName(r, "playtime");
                city = DB_GetFieldIntByName(r, "garage_city");
            }
            lastLogin = DB_GetFieldIntByName(r, "last_login");
            registered = DB_GetFieldIntByName(r, "registered_at");
        }
        DB_FreeResultSet(r);
    }
    if (city < 0 || city >= MAX_CITIES) city = CITY_LS;
    FormatMoney(cash, money);
    new lvl[24];
    Admin_GetLevelName(admin, lvl);

    format(body, sizeof (body), C_WHITE "Ник: " C_BRAND "%s" C_WHITE " (акаунт #%d)\n", ApTargetName[playerid], ApTargetAccount[playerid]);
    if (online) format(line, sizeof (line), C_WHITE "Състояние: " C_GREEN "онлайн (ID %d)\n", ApTarget[playerid]);
    else format(line, sizeof (line), C_WHITE "Състояние: " C_GREY "офлайн, последно преди %d дни\n", (gettime() - lastLogin) / 86400);
    strcat(body, line);
    format(line, sizeof (line), C_WHITE "Регистриран преди: %d дни\n\n", (gettime() - registered) / 86400);
    strcat(body, line);
    format(line, sizeof (line), C_WHITE "Ниво: " C_BRAND "%d" C_WHITE " (опит %d)\nПари: " C_GREEN "$%s\n", level, exp, money);
    strcat(body, line);
    format(line, sizeof (line), C_WHITE "Админ: %s\nИзиграно: %d ч. %d мин.\nГараж: %s\n", lvl, playtime / 3600, (playtime / 60) % 60, CityNames[city]);
    strcat(body, line);

    // Превозни средства
    format(q, sizeof (q), "SELECT model FROM player_vehicles WHERE owner_id = %d ORDER BY slot", ApTargetAccount[playerid]);
    r = Database_Query(q);
    strcat(body, C_WHITE "Превозни средства: " C_BRAND);
    if (r != DBResult:0) {
        new rows = DB_GetRowCount(r), name[24];
        if (rows == 0) strcat(body, "няма");
        for (new i = 0; i < rows; i++) {
            Veh_GetModelName(DB_GetFieldIntByName(r, "model"), name);
            if (i > 0) strcat(body, ", ");
            strcat(body, name);
            DB_SelectNextRow(r);
        }
        DB_FreeResultSet(r);
    }
    strcat(body, "\n");

    // Компания
    for (new c = 0; c < MAX_COMPANIES; c++) {
        if (Company[c][cpOwner] == ApTargetAccount[playerid]) {
            format(line, sizeof (line), C_WHITE "Компания: " C_BRAND "%s\n", CompanyDefs[c][cdName]);
            strcat(body, line);
        }
    }
    if (online) {
        new h, th, b;
        Needs_Get(ApTarget[playerid], h, th, b);
        format(line, sizeof (line), C_WHITE "Нужди: храна %d%%, жажда %d%%, тоалетна %d%%\n", h, th, b);
        strcat(body, line);
    }
    ShowPlayerDialog(playerid, DIALOG_AP_STATS, DIALOG_STYLE_MSGBOX, C_ADMIN "Статистика", "%s", "Промени", "Назад", body);
}

// ---------- Промяна на играч ----------

enum {
    APE_LEVEL,
    APE_SETMONEY,
    APE_GIVEMONEY,
    APE_ADMIN,
    APE_GOTO,
    APE_GETHERE,
    APE_FEED,
    APE_FLIP,
    APE_SLAP,
    APE_KICK,
    APE_BAN,
    APE_UNBAN,
    APE_LICENSES
}

static stock AdminPanel_ShowEdit(playerid)
{
    new title[80], bool:online = AdminPanel_TargetOnline(playerid);
    format(title, sizeof (title), C_ADMIN "Промяна" C_WHITE " | %s%s", ApTargetName[playerid], online ? (" (онлайн)") : (" (офлайн)"));
    ShowPlayerDialog(playerid, DIALOG_AP_EDIT, DIALOG_STYLE_LIST, title,
        "Задай ниво\nЗадай пари\nДай пари\nАдмин ниво\nОтиди при него\nДоведи го при мен\nНахрани и напои\nИзправи превозното му средство\nШамар\nИзритай\nБан\nМахни бан\nШофьорски категории",
        "Избери", "Назад");
}

static stock AdminPanel_AskValue(playerid, const prompt[])
{
    ShowPlayerDialog(playerid, DIALOG_AP_VALUE, DIALOG_STYLE_INPUT, C_ADMIN "Промяна на играч",
        C_WHITE "Играч: " C_BRAND "%s\n\n" C_WHITE "%s", "Готово", "Назад", ApTargetName[playerid], prompt);
}

static stock AdminPanel_RunCmd(playerid, const cmd[], const params[])
{
    new func[32];
    format(func, sizeof (func), "cmd_%s", cmd);
    CallLocalFunction(func, "is", playerid, params);
}

static stock AdminPanel_DoEdit(playerid, action, const value[])
{
    new bool:online = AdminPanel_TargetOnline(playerid), t = ApTarget[playerid], params[160], q[192];
    switch (action) {
        case APE_LEVEL: {
            if (!AdminPanel_Need(playerid, 4)) return;
            new level = strval(value);
            if (level < 1 || level > LEVEL_MAX_ADMIN) { SendError(playerid, "Нивото е от 1 до 1000."); AdminPanel_ShowEdit(playerid); return; }
            if (online) {
                format(params, sizeof (params), "%d %d", t, level);
                AdminPanel_RunCmd(playerid, "setlevel", params);
            } else {
                format(q, sizeof (q), "UPDATE accounts SET level = %d, exp = %d WHERE id = %d", level, Level_ExpFor(level), ApTargetAccount[playerid]);
                Database_Exec(q);
                SendClientMessage(playerid, COLOR_ADMIN, "[A] Нивото на %s (офлайн) е %d.", ApTargetName[playerid], level);
            }
        }
        case APE_SETMONEY, APE_GIVEMONEY: {
            if (!AdminPanel_Need(playerid, 4)) return;
            if (!IsNumeric(value)) { SendError(playerid, "Въведи число."); AdminPanel_ShowEdit(playerid); return; }
            if (online) {
                format(params, sizeof (params), "%d %s", t, value);
                if (action == APE_SETMONEY) AdminPanel_RunCmd(playerid, "setmoney", params);
                else AdminPanel_RunCmd(playerid, "givemoney", params);
            } else {
                if (action == APE_SETMONEY) format(q, sizeof (q), "UPDATE accounts SET money = %d WHERE id = %d", strval(value), ApTargetAccount[playerid]);
                else format(q, sizeof (q), "UPDATE accounts SET money = money + %d WHERE id = %d", strval(value), ApTargetAccount[playerid]);
                Database_Exec(q);
                SendClientMessage(playerid, COLOR_ADMIN, "[A] Парите на %s (офлайн) са променени.", ApTargetName[playerid]);
            }
        }
        case APE_ADMIN: {
            if (!AdminPanel_Need(playerid, 5)) return;
            new level = strval(value);
            if (!IsNumeric(value) || level < 0 || level > 5) { SendError(playerid, "Админ нивото е от 0 до 5."); AdminPanel_ShowEdit(playerid); return; }
            if (online) {
                format(params, sizeof (params), "%d %d", t, level);
                AdminPanel_RunCmd(playerid, "makeadmin", params);
            } else {
                format(q, sizeof (q), "UPDATE accounts SET admin = %d WHERE id = %d", level, ApTargetAccount[playerid]);
                Database_Exec(q);
                SendClientMessage(playerid, COLOR_ADMIN, "[A] Админ нивото на %s (офлайн) е %d.", ApTargetName[playerid], level);
            }
        }
        case APE_KICK: {
            format(params, sizeof (params), "%d %s", t, value);
            AdminPanel_RunCmd(playerid, "kick", params);
        }
        case APE_BAN: {
            if (!AdminPanel_Need(playerid, 2)) return;
            new days[8], reason[96];
            if (!GetParam(value, 0, days) || !IsNumeric(days) || !GetParamRest(value, 1, reason)) {
                SendError(playerid, "Въведи: дни (0 = завинаги) и причина. Пример: 7 спам");
                AdminPanel_ShowEdit(playerid);
                return;
            }
            if (online) {
                format(params, sizeof (params), "%d %s %s", t, days, reason);
                AdminPanel_RunCmd(playerid, "ban", params);
            } else {
                new safe[192], d = strval(days);
                SQL_Escape(reason, safe);
                format(q, sizeof (q), "INSERT INTO bans (name, reason, admin_name, created_at, expires_at) VALUES ('%s', '%s', '%s', %d, %d)",
                    ApTargetName[playerid], safe, PlayerData[playerid][pName], gettime(), (d == 0) ? 0 : gettime() + d * 86400);
                Database_Exec(q);
                SendClientMessage(playerid, COLOR_ADMIN, "[A] %s (офлайн) е баннат.", ApTargetName[playerid]);
            }
        }
    }
}

// ---------- Шофьорски категории ----------

static stock AdminPanel_GetLicenses(playerid)
{
    if (AdminPanel_TargetOnline(playerid)) return License_GetBits(ApTarget[playerid]);
    new q[96], bits = LIC_DEFAULT;
    format(q, sizeof (q), "SELECT licenses FROM accounts WHERE id = %d", ApTargetAccount[playerid]);
    new DBResult:r = Database_Query(q);
    if (r != DBResult:0) {
        if (DB_GetRowCount(r) > 0) bits = DB_GetFieldIntByName(r, "licenses");
        DB_FreeResultSet(r);
    }
    return bits;
}

static stock AdminPanel_SetLicenses(playerid, bits)
{
    if (AdminPanel_TargetOnline(playerid)) {
        License_SetBits(ApTarget[playerid], bits);
        return;
    }
    new q[96];
    format(q, sizeof (q), "UPDATE accounts SET licenses = %d WHERE id = %d", bits | LIC_DEFAULT, ApTargetAccount[playerid]);
    Database_Exec(q);
}

static stock AdminPanel_ShowLicenses(playerid)
{
    new bits = AdminPanel_GetLicenses(playerid), body[400], line[96], title[80];
    body[0] = EOS;
    for (new c = 0; c < MAX_LICENSES; c++) {
        if (bits & LIC_BIT(c)) format(line, sizeof (line), C_GREEN "[X] " C_BRAND "%s" C_WHITE " - %s\n", LicenseNames[c], LicenseDesc[c]);
        else format(line, sizeof (line), C_GREY "[  ] " C_BRAND "%s" C_WHITE " - %s\n", LicenseNames[c], LicenseDesc[c]);
        strcat(body, line);
    }
    format(title, sizeof (title), C_ADMIN "Категории" C_WHITE " | %s", ApTargetName[playerid]);
    ShowPlayerDialog(playerid, DIALOG_AP_LIC, DIALOG_STYLE_LIST, title, "%s", "Смени", "Назад", body);
}

// ---------- Места на картата ----------

enum {
    APP_ADD_SHOP,
    APP_ADD_DINER,
    APP_ADD_CAFE,
    APP_ADD_TOILET,
    APP_ADD_FUEL,
    APP_ADD_GPS,
    APP_DEL_PLACE,
    APP_DEL_FUEL,
    APP_MOVE_COMPANY,
    APP_MOVE_GARAGE,
    APP_MOVE_DEALER,
    APP_NEW_COMPANY,
    APP_DEL_COMPANY,
    APP_ADD_REST,
    APP_ADD_WEIGH,
    APP_SCHOOL_DOOR,
    APP_SCHOOL_VEH,
    APP_REMOVE_OBJECT,
    APP_RESTORE_OBJECTS,
    APP_HOUSE_ADD,
    APP_HOUSE_DEL
}

static stock AdminPanel_ShowPlaces(playerid)
{
    ShowPlayerDialog(playerid, DIALOG_AP_PLACES, DIALOG_STYLE_LIST, C_ADMIN "Места на картата",
        C_GREEN "+ " C_WHITE "Магазин тук\n" \
        C_GREEN "+ " C_WHITE "Закусвалня тук\n" \
        C_GREEN "+ " C_WHITE "Кафене тук\n" \
        C_GREEN "+ " C_WHITE "Обществена тоалетна тук\n" \
        C_GREEN "+ " C_WHITE "Бензиностанция тук\n" \
        C_GREEN "+ " C_WHITE "GPS точка тук\n" \
        C_RED "- " C_WHITE "Изтрий най-близкото място (магазин, тоалетна, паркинг, кантар)\n" \
        C_RED "- " C_WHITE "Изтрий най-близката бензиностанция\n" \
        C_BRAND "> " C_WHITE "Премести база на компания тук\n" \
        C_BRAND "> " C_WHITE "Премести входа на гаражите тук\n" \
        C_BRAND "> " C_WHITE "Премести автокъща тук\n" \
        C_GREEN "+ " C_WHITE "Нова компания тук\n" \
        C_RED "- " C_WHITE "Закрий компания\n" \
        C_GREEN "+ " C_WHITE "Паркинг за почивка тук\n" \
        C_GREEN "+ " C_WHITE "Кантар тук (застани в центъра му)\n" \
        C_BRAND "> " C_WHITE "Премести входа на автошколата тук (гледай навън)\n" \
        C_BRAND "> " C_WHITE "Изпитният камион на автошколата тръгва оттук\n" \
        C_RED "- " C_WHITE "Махни обекта от картата пред мен (порта, ограда...)\n" \
        C_GREY "  Върни последния махнат обект\n" \
        C_GREEN "+ " C_WHITE "Нова къща тук (застани на вратата, гледай навън)\n" \
        C_RED "- " C_WHITE "Изтрий най-близката къща",
        "Избери", "Назад");
}

static stock AdminPanel_ShowCityPick(playerid)
{
    ShowPlayerDialog(playerid, DIALOG_AP_PICK, DIALOG_STYLE_LIST, C_ADMIN "Кой град?", "Los Santos\nSan Fierro\nLas Venturas", "Избери", "Назад");
}

static stock AdminPanel_ShowCompanyPick(playerid)
{
    new body[2400], line[96], n = 0;
    body[0] = EOS;
    for (new c = 0; c < MAX_COMPANIES; c++) {
        if (!Company_IsValid(c)) continue;
        format(line, sizeof (line), "%s" C_GREY " (%s)\n", CompanyDefs[c][cdName], CompanyDefs[c][cdPlace]);
        strcat(body, line);
        ApPickMap[playerid][n++] = c;
    }
    ShowPlayerDialog(playerid, DIALOG_AP_PICK, DIALOG_STYLE_LIST, C_ADMIN "Коя компания?", "%s", "Избери", "Назад", body);
}

static stock AdminPanel_DoPlace(playerid, action)
{
    if (!AdminPanel_Need(playerid, 3)) return;
    if (GetPlayerVirtualWorld(playerid) != 0) {
        SendError(playerid, "Трябва да си навън (не в гараж).");
        return;
    }
    ApPlaceAction[playerid] = action;
    switch (action) {
        case APP_ADD_SHOP:   Places_AddHere(playerid, PLACE_SHOP);
        case APP_ADD_DINER:  Places_AddHere(playerid, PLACE_DINER);
        case APP_ADD_CAFE:   Places_AddHere(playerid, PLACE_CAFE);
        case APP_ADD_TOILET: Places_AddHere(playerid, PLACE_TOILET);
        case APP_ADD_REST:   Places_AddHere(playerid, PLACE_REST);
        case APP_ADD_WEIGH:  Places_AddHere(playerid, PLACE_WEIGH);
        case APP_SCHOOL_DOOR: {
            if (!AdminPanel_Need(playerid, 5)) return;
            School_SetEntrance(playerid);
        }
        case APP_SCHOOL_VEH: {
            if (!AdminPanel_Need(playerid, 5)) return;
            School_SetVehicleSpawn(playerid);
        }
        case APP_REMOVE_OBJECT: {
            if (!AdminPanel_Need(playerid, 5)) return;
            ShowPlayerDialog(playerid, DIALOG_AP_REMOVE, DIALOG_STYLE_INPUT, C_ADMIN "Махни обект от картата",
                C_WHITE "Застани плътно пред обекта, с лице към него.\n" \
                C_WHITE "Маха се всичко, чийто център е в радиуса около точка на 3 м пред теб.\n\n" \
                C_WHITE "Радиус в метри (1-30). За голяма порта пробвай 8:", "Махни", "Назад");
            return;
        }
        case APP_RESTORE_OBJECTS: {
            if (!AdminPanel_Need(playerid, 5)) return;
            Removed_RestoreLast(playerid);
        }
        case APP_HOUSE_ADD: {
            if (!AdminPanel_Need(playerid, 4)) return;
            GetPlayerPos(playerid, ApHousePos[playerid][0], ApHousePos[playerid][1], ApHousePos[playerid][2]);
            GetPlayerFacingAngle(playerid, ApHousePos[playerid][3]);
            ShowPlayerDialog(playerid, DIALOG_AP_HPRICE, DIALOG_STYLE_INPUT, C_ADMIN "Нова къща (1/2)",
                C_WHITE "Цена на къщата в $ (напр. 25000 за малка, 150000 за имение):", "Напред", "Назад");
            return;
        }
        case APP_HOUSE_DEL: {
            if (!AdminPanel_Need(playerid, 4)) return;
            House_DeleteNearest(playerid);
        }
        case APP_ADD_FUEL:   AdminPanel_RunCmd(playerid, "addfuel", "\1");
        case APP_ADD_GPS: {
            ShowPlayerDialog(playerid, DIALOG_AP_GPSNAME, DIALOG_STYLE_INPUT, C_ADMIN "GPS точка", C_WHITE "Име на точката (категория \"Други\"):", "Добави", "Назад");
            return;
        }
        case APP_DEL_PLACE:  Places_DeleteNearest(playerid);
        case APP_DEL_FUEL:   AdminPanel_RunCmd(playerid, "delfuel", "\1");
        case APP_MOVE_COMPANY: {
            if (!AdminPanel_Need(playerid, 5)) return;
            AdminPanel_ShowCompanyPick(playerid);
            return;
        }
        case APP_NEW_COMPANY: {
            if (!AdminPanel_Need(playerid, 5)) return;
            ApNewProduces[playerid] = 0;
            ApNewAccepts[playerid] = 0;
            ShowPlayerDialog(playerid, DIALOG_AP_CNAME, DIALOG_STYLE_INPUT, C_ADMIN "Нова компания (1/5)",
                C_WHITE "Име на компанията (до 30 символа).\n" C_GREY "Препоръчително на латиница - вижда се и в панела на курса.", "Напред", "Назад");
            return;
        }
        case APP_DEL_COMPANY: {
            if (!AdminPanel_Need(playerid, 5)) return;
            AdminPanel_ShowCompanyPick(playerid);
            return;
        }
        case APP_MOVE_GARAGE, APP_MOVE_DEALER: {
            if (!AdminPanel_Need(playerid, 5)) return;
            AdminPanel_ShowCityPick(playerid);
            return;
        }
    }
    AdminPanel_ShowPlaces(playerid);
}

// Избор на товари с отметки (за нова компания)
static stock AdminPanel_ShowGoodsPick(playerid)
{
    new body[1400], line[96], mask = ApGoodsAccepts[playerid] ? ApNewAccepts[playerid] : ApNewProduces[playerid];
    body[0] = EOS;
    for (new g = 0; g < MAX_GOODS; g++) {
        if (mask & GOOD_BIT(g)) format(line, sizeof (line), C_GREEN "[X] " C_WHITE "%s\n", Goods[g][gdName]);
        else format(line, sizeof (line), C_GREY "[  ] " C_WHITE "%s\n", Goods[g][gdName]);
        strcat(body, line);
    }
    strcat(body, C_BRAND ">> Готово");
    ShowPlayerDialog(playerid, DIALOG_AP_CGOODS, DIALOG_STYLE_LIST,
        ApGoodsAccepts[playerid] ? (C_ADMIN "Нова компания (5/5): ПРИЕМА") : (C_ADMIN "Нова компания (4/5): ДАВА  "),
        "%s", "Избери", "Отказ", body);
}

// ---------- Рестарт ----------

forward AdminPanel_RestartTick();
public AdminPanel_RestartTick()
{
    if (!ApRestarting) return 0;
    if (ApRestartLeft <= 0) {
        for (new i = 0; i < MAX_PLAYERS; i++) {
            if (!IsPlayerLogged(i)) continue;
            Auth_SaveAccount(i);
            Veh_SaveAll(i);
            Needs_Save(i);
        }
        Economy_Save();
        SendRconCommand("gmx");
        return 1;
    }
    GameTextForAll("~r~SERVER RESTART~n~~w~%d", 1100, 3, ApRestartLeft);
    ApRestartLeft--;
    SetTimer("AdminPanel_RestartTick", 1000, false);
    return 1;
}

// ---------- Диалози ----------

stock AdminPanel_OnDialogResponse(playerid, dialogid, response, listitem, const inputtext[])
{
    switch (dialogid) {
        case DIALOG_AP_MAIN: {
            if (!response) return 1;
            switch (listitem) {
                case AP_STATS: AdminPanel_AskPlayer(playerid, AP_STATS);
                case AP_EDIT:  AdminPanel_AskPlayer(playerid, AP_EDIT);
                case AP_PLACES: AdminPanel_ShowPlaces(playerid);
                case AP_TELEPORT: Gps_ShowTeleport(playerid);
                case AP_ANNOUNCE: {
                    if (!AdminPanel_Need(playerid, 2)) return 1;
                    ShowPlayerDialog(playerid, DIALOG_AP_ANNOUNCE, DIALOG_STYLE_INPUT, C_ADMIN "Съобщение до всички", C_WHITE "Текст на съобщението:", "Изпрати", "Назад");
                }
                case AP_SERVERNAME: {
                    if (!AdminPanel_Need(playerid, 5)) return 1;
                    ShowPlayerDialog(playerid, DIALOG_AP_SERVERNAME, DIALOG_STYLE_INPUT, C_ADMIN "Име на сървъра",
                        C_WHITE "Ново име (вижда се в списъка със сървъри):", "Смени", "Назад");
                }
                case AP_RESTART: {
                    if (!AdminPanel_Need(playerid, 5)) return 1;
                    ShowPlayerDialog(playerid, DIALOG_AP_RESTART, DIALOG_STYLE_MSGBOX, C_RED "Рестарт",
                        C_WHITE "Да рестартирам ли сървъра след %d секунди?\nВсички играчи и икономиката се записват преди това.",
                        "Рестарт", "Отказ", AP_RESTART_SECONDS);
                }
            }
            return 1;
        }
        case DIALOG_AP_PLAYER: {
            if (!response) { AdminPanel_Show(playerid); return 1; }
            if (inputtext[0] == EOS || !AdminPanel_Resolve(playerid, inputtext)) {
                AdminPanel_AskPlayer(playerid, ApLookupFor[playerid], "Няма такъв играч (провери ID-то или точния ник).");
                return 1;
            }
            if (ApLookupFor[playerid] == AP_STATS) AdminPanel_ShowStats(playerid);
            else AdminPanel_ShowEdit(playerid);
            return 1;
        }
        case DIALOG_AP_STATS: {
            if (response) AdminPanel_ShowEdit(playerid);
            else AdminPanel_Show(playerid);
            return 1;
        }
        case DIALOG_AP_EDIT: {
            if (!response) { AdminPanel_Show(playerid); return 1; }
            new bool:online = AdminPanel_TargetOnline(playerid), t = ApTarget[playerid], params[16];
            ApEditAction[playerid] = listitem;
            switch (listitem) {
                case APE_LEVEL:     AdminPanel_AskValue(playerid, "Ново ниво (1 и нагоре):");
                case APE_SETMONEY:  AdminPanel_AskValue(playerid, "Нова сума пари:");
                case APE_GIVEMONEY: AdminPanel_AskValue(playerid, "Колко пари да дам (може и минус):");
                case APE_ADMIN:     AdminPanel_AskValue(playerid, "Ново админ ниво (0-5):");
                case APE_GOTO, APE_GETHERE, APE_FEED, APE_FLIP, APE_SLAP, APE_KICK: {
                    if (!online) {
                        SendError(playerid, "Играчът не е онлайн.");
                        AdminPanel_ShowEdit(playerid);
                        return 1;
                    }
                    format(params, sizeof (params), "%d", t);
                    if (listitem == APE_GOTO) AdminPanel_RunCmd(playerid, "goto", params);
                    else if (listitem == APE_GETHERE) AdminPanel_RunCmd(playerid, "gethere", params);
                    else if (listitem == APE_FLIP || listitem == APE_SLAP) {
                        AdminPanel_RunCmd(playerid, (listitem == APE_FLIP) ? ("flip") : ("slap"), params);
                        AdminPanel_ShowEdit(playerid);
                    }
                    else if (listitem == APE_FEED) {
                        Needs_Set(t, 100, 100, 0);
                        SendClientMessage(playerid, COLOR_ADMIN, "[A] %s е нахранен и напоен.", ApTargetName[playerid]);
                        AdminPanel_ShowEdit(playerid);
                    }
                    else AdminPanel_AskValue(playerid, "Причина за изритването:");
                }
                case APE_BAN:   AdminPanel_AskValue(playerid, "Дни (0 = завинаги) и причина, напр.: 7 спам");
                case APE_UNBAN: {
                    if (!AdminPanel_Need(playerid, 2)) return 1;
                    AdminPanel_RunCmd(playerid, "unban", ApTargetName[playerid]);
                    AdminPanel_ShowEdit(playerid);
                }
                case APE_LICENSES: {
                    if (!AdminPanel_Need(playerid, 3)) return 1;
                    AdminPanel_ShowLicenses(playerid);
                }
            }
            return 1;
        }
        case DIALOG_AP_HPRICE: {
            if (!response) { AdminPanel_ShowPlaces(playerid); return 1; }
            new price = strval(inputtext);
            if (!IsNumeric(inputtext) || price < 1000 || price > 10000000) {
                SendError(playerid, "Цената е от $1000 до $10 000 000.");
                AdminPanel_ShowPlaces(playerid);
                return 1;
            }
            ApHousePrice[playerid] = price;
            new body[256], line[48];
            body[0] = EOS;
            for (new i = 0; i < sizeof (HouseInteriors); i++) {
                format(line, sizeof (line), "%s\n", HouseInteriors[i][hiName]);
                strcat(body, line);
            }
            ShowPlayerDialog(playerid, DIALOG_AP_HINT, DIALOG_STYLE_LIST, C_ADMIN "Нова къща (2/2): интериор", "%s", "Създай", "Назад", body);
            return 1;
        }
        case DIALOG_AP_HINT: {
            if (!response) { AdminPanel_ShowPlaces(playerid); return 1; }
            // Админът може да е мръднал - връщаме го на вратата, за да се запише точното място
            SetPlayerPos(playerid, ApHousePos[playerid][0], ApHousePos[playerid][1], ApHousePos[playerid][2]);
            SetPlayerFacingAngle(playerid, ApHousePos[playerid][3]);
            House_Create(playerid, ApHousePrice[playerid], listitem);
            AdminPanel_ShowPlaces(playerid);
            return 1;
        }
        case DIALOG_AP_REMOVE: {
            if (!response) { AdminPanel_ShowPlaces(playerid); return 1; }
            new Float:radius = floatstr(inputtext);
            if (radius < 1.0 || radius > 30.0) {
                SendError(playerid, "Радиусът е от 1 до 30 метра.");
                AdminPanel_ShowPlaces(playerid);
                return 1;
            }
            Removed_AddAhead(playerid, radius);
            return 1;
        }
        case DIALOG_AP_LIC: {
            if (!response || listitem < 0 || listitem >= MAX_LICENSES) { AdminPanel_ShowEdit(playerid); return 1; }
            if (listitem == LIC_B) { SendError(playerid, "Категория B е задължителна."); AdminPanel_ShowLicenses(playerid); return 1; }
            new bits = AdminPanel_GetLicenses(playerid) ^ LIC_BIT(listitem);
            AdminPanel_SetLicenses(playerid, bits);
            new list[24];
            License_Format(bits, list);
            SendClientMessage(playerid, COLOR_ADMIN, "[A] Категориите на %s: %s.", ApTargetName[playerid], list);
            if (AdminPanel_TargetOnline(playerid)) {
                SendClientMessage(ApTarget[playerid], COLOR_ADMIN, "[A] " C_WHITE "Админ промени шофьорските ти категории: " C_BRAND "%s", list);
            }
            AdminPanel_ShowLicenses(playerid);
            return 1;
        }
        case DIALOG_AP_VALUE: {
            if (!response || inputtext[0] == EOS) { AdminPanel_ShowEdit(playerid); return 1; }
            AdminPanel_DoEdit(playerid, ApEditAction[playerid], inputtext);
            return 1;
        }
        case DIALOG_AP_PLACES: {
            if (!response) { AdminPanel_Show(playerid); return 1; }
            AdminPanel_DoPlace(playerid, listitem);
            return 1;
        }
        case DIALOG_AP_PICK: {
            if (!response) { AdminPanel_ShowPlaces(playerid); return 1; }
            new params[8];
            format(params, sizeof (params), "%d", listitem);
            if (ApPlaceAction[playerid] == APP_MOVE_COMPANY || ApPlaceAction[playerid] == APP_DEL_COMPANY) {
                if (listitem < 0 || listitem >= MAX_COMPANIES) return 1;
                format(params, sizeof (params), "%d", ApPickMap[playerid][listitem]);
            }
            switch (ApPlaceAction[playerid]) {
                case APP_DEL_COMPANY: {
                    new c = ApPickMap[playerid][listitem];
                    ApNewSize[playerid] = c;        // запомняме коя (за потвърждението)
                    ShowPlayerDialog(playerid, DIALOG_AP_CDELETE, DIALOG_STYLE_MSGBOX, C_RED "Закриване на компания",
                        C_WHITE "Да закрия ли " C_BRAND "%s" C_WHITE "?\n\nПоръчките и курсовете с нея изчезват, складът се нулира.\nАко има собственик, получава 60%% от цената + касата.",
                        "Закрий", "Отказ", CompanyDefs[c][cdName]);
                    return 1;
                }
                case APP_MOVE_COMPANY: AdminPanel_RunCmd(playerid, "compos", params);
                case APP_MOVE_GARAGE:  AdminPanel_RunCmd(playerid, "garagepos", params);
                case APP_MOVE_DEALER:  AdminPanel_RunCmd(playerid, "dealerpos", params);
            }
            AdminPanel_ShowPlaces(playerid);
            return 1;
        }
        // ----- Нова компания -----
        case DIALOG_AP_CNAME: {
            if (!response) { AdminPanel_ShowPlaces(playerid); return 1; }
            if (inputtext[0] == EOS || strlen(inputtext) > 30) {
                SendError(playerid, "Името трябва да е от 1 до 30 символа.");
                AdminPanel_DoPlace(playerid, APP_NEW_COMPANY);
                return 1;
            }
            format(ApNewName[playerid], 32, "%s", inputtext);
            ShowPlayerDialog(playerid, DIALOG_AP_CPLACE, DIALOG_STYLE_INPUT, C_ADMIN "Нова компания (2/5)",
                C_WHITE "Къде е (град/район), напр. Los Santos:", "Напред", "Назад");
            return 1;
        }
        case DIALOG_AP_CPLACE: {
            if (!response) { AdminPanel_DoPlace(playerid, APP_NEW_COMPANY); return 1; }
            if (inputtext[0] == EOS) format(ApNewPlace[playerid], 24, "San Andreas");
            else format(ApNewPlace[playerid], 24, "%s", inputtext);
            ShowPlayerDialog(playerid, DIALOG_AP_CSIZE, DIALOG_STYLE_LIST, C_ADMIN "Нова компания (3/5): размер",
                "Малка ($50 000, склад 60 т)\nСредна ($120 000, склад 100 т)\nГоляма ($250 000, склад 200 т)", "Напред", "Назад");
            return 1;
        }
        case DIALOG_AP_CSIZE: {
            if (!response) { AdminPanel_DoPlace(playerid, APP_NEW_COMPANY); return 1; }
            ApNewSize[playerid] = (listitem < 0 || listitem > 2) ? 0 : listitem;
            ApGoodsAccepts[playerid] = false;
            AdminPanel_ShowGoodsPick(playerid);
            return 1;
        }
        case DIALOG_AP_CGOODS: {
            if (!response) { AdminPanel_DoPlace(playerid, APP_NEW_COMPANY); return 1; }
            if (listitem >= 0 && listitem < MAX_GOODS) {
                if (ApGoodsAccepts[playerid]) ApNewAccepts[playerid] ^= GOOD_BIT(listitem);
                else ApNewProduces[playerid] ^= GOOD_BIT(listitem);
                AdminPanel_ShowGoodsPick(playerid);
                return 1;
            }
            // "Готово"
            if (!ApGoodsAccepts[playerid]) {
                ApGoodsAccepts[playerid] = true;
                AdminPanel_ShowGoodsPick(playerid);
                return 1;
            }
            if (ApNewProduces[playerid] == 0 && ApNewAccepts[playerid] == 0) {
                SendError(playerid, "Компанията трябва да дава или приема поне един товар.");
                ApGoodsAccepts[playerid] = false;
                AdminPanel_ShowGoodsPick(playerid);
                return 1;
            }
            new produces[320], accepts[320];
            Company_GoodsList(ApNewProduces[playerid], produces);
            Company_GoodsList(ApNewAccepts[playerid], accepts);
            ShowPlayerDialog(playerid, DIALOG_AP_CCONFIRM, DIALOG_STYLE_MSGBOX, C_ADMIN "Нова компания: потвърди",
                C_BRAND "%s" C_GREY " (%s)\n\n" C_WHITE "Дава: " C_GREEN "%s\n" C_WHITE "Приема: " C_BRAND "%s\n\n" C_WHITE "Базата ще е там, където стоиш.",
                "Създай", "Отказ", ApNewName[playerid], ApNewPlace[playerid], produces, accepts);
            return 1;
        }
        case DIALOG_AP_CCONFIRM: {
            if (response) {
                new Float:x, Float:y, Float:z;
                GetPlayerPos(playerid, x, y, z);
                new c = Company_Create(ApNewName[playerid], ApNewPlace[playerid], ApNewSize[playerid], ApNewProduces[playerid], ApNewAccepts[playerid], x, y, z);
                if (c == -1) SendError(playerid, "Няма свободно място за нова компания (максимум 30).");
                else SendClientMessage(playerid, COLOR_ADMIN, "[A] Създадена е компания #%d: %s.", c, ApNewName[playerid]);
            }
            AdminPanel_ShowPlaces(playerid);
            return 1;
        }
        case DIALOG_AP_CDELETE: {
            if (response) {
                new c = ApNewSize[playerid], name[32];
                format(name, sizeof (name), "%s", CompanyDefs[c][cdName]);
                if (Company_Delete(c)) SendClientMessage(playerid, COLOR_ADMIN, "[A] Компанията %s е закрита.", name);
            }
            AdminPanel_ShowPlaces(playerid);
            return 1;
        }
        case DIALOG_AP_GPSNAME: {
            if (response && inputtext[0] != EOS) AdminPanel_RunCmd(playerid, "gpsadd", inputtext);
            AdminPanel_ShowPlaces(playerid);
            return 1;
        }
        case DIALOG_AP_ANNOUNCE: {
            if (response && inputtext[0] != EOS) {
                SendClientMessageToAll(COLOR_ADMIN, "[ОБЯВА] " C_WHITE "%s", inputtext);
                GameTextForAll("~y~ANNOUNCEMENT~n~~w~SEE CHAT", 3000, 3);
            }
            AdminPanel_Show(playerid);
            return 1;
        }
        case DIALOG_AP_SERVERNAME: {
            if (response && inputtext[0] != EOS) {
                new name[64];
                format(name, sizeof (name), "%s", inputtext);
                AdminPanel_SetSetting("server_name", name);
                SendRconCommand("hostname %s", name);
                SendClientMessage(playerid, COLOR_ADMIN, "[A] Името на сървъра е: %s", name);
            }
            AdminPanel_Show(playerid);
            return 1;
        }
        case DIALOG_AP_RESTART: {
            if (!response || ApRestarting) { AdminPanel_Show(playerid); return 1; }
            ApRestarting = true;
            ApRestartLeft = AP_RESTART_SECONDS;
            SendClientMessageToAll(COLOR_RED, "[СЪРВЪР] " C_WHITE "Рестарт след %d секунди (%s). Всичко се записва.", AP_RESTART_SECONDS, PlayerData[playerid][pName]);
            AdminPanel_RestartTick();
            return 1;
        }
    }
    return 0;
}
