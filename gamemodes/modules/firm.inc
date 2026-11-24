// Компаниите като фирми: служители, облик (име, таг, цвят, слоган) и каса с история.
//
// Служители:
// - Влизане: молба (ALT на базата на чужда компания -> "Кандидатствай") или покана от собственика/мениджъра.
// - Рангове: Шофьор, Старши шофьор, Мениджър (+ собственикът). Местата: малка 5, средна 10, голяма 20.
// - Фирмени поръчки = поръчки от базата на твоята компания: +20% (+25% за старши и мениджър).
// - Комисионна: компанията взима 0-30% от курсовете на служителите си (в касата).
// - Заплата на PayDay (от касата) и карта за гориво (компанията плаща горивото).
// - Фирмен чат: /f [текст]; тагът на компанията стои пред името в чата.
//
// Каса: внасяне (собственик и мениджър), теглене (само собственик), история на движенията.

#define FIRM_RANK_DRIVER        0
#define FIRM_RANK_SENIOR        1
#define FIRM_RANK_MANAGER       2
#define FIRM_RANKS              3

#define FIRM_BONUS_DRIVER       20          // % бонус за фирмените поръчки
#define FIRM_BONUS_SENIOR       25
#define FIRM_COMMISSION_MAX     30
#define FIRM_SALARY_MAX         2000
#define FIRM_RENAME_PRICE       10000
#define FIRM_RENAME_DAYS        7
#define FIRM_LOG_KEEP           100

stock const FirmRankNames[FIRM_RANKS][] = {"Шофьор", "Старши шофьор", "Мениджър"};
static const FirmSlots[] = {5, 10, 20};                    // по размера на компанията

// Цветове на компаниите: име и RGB
enum E_FIRM_COLOR {
    fcName[16],
    fcRGB
}
static const FirmColors[][E_FIRM_COLOR] = {
    {"Оранжев",   0xF5A623},
    {"Червен",    0xE04848},
    {"Син",       0x3D8BFF},
    {"Небесен",   0x5BC8F5},
    {"Зелен",     0x4CC24C},
    {"Тревист",   0x9BD84A},
    {"Жълт",      0xF2D02E},
    {"Лилав",     0xA66BE0},
    {"Розов",     0xF06FB0},
    {"Бял",       0xF0F0F0}
};

static FirmCompany[MAX_PLAYERS] = {-1, ...};       // компанията, в която е нает (-1 = никоя)
static FirmRank[MAX_PLAYERS];
static FirmInviteCompany[MAX_PLAYERS] = {-1, ...};
static FirmInviteFrom[MAX_PLAYERS];
static FirmPanelCompany[MAX_PLAYERS];
#define FIRM_LIST_MAX           20
static FirmPick[MAX_PLAYERS][FIRM_LIST_MAX];       // ID-та на акаунти от списъци
static FirmPickRank[MAX_PLAYERS][FIRM_LIST_MAX];
static FirmPickName[MAX_PLAYERS][FIRM_LIST_MAX][MAX_PLAYER_NAME + 1];
static FirmPickIndex[MAX_PLAYERS];

// ---------- База данни ----------

stock Firm_Init()
{
    Database_Exec("CREATE TABLE IF NOT EXISTS company_members (account_id INTEGER PRIMARY KEY, company INTEGER NOT NULL, rank INTEGER NOT NULL DEFAULT 0, name TEXT NOT NULL, joined INTEGER NOT NULL DEFAULT 0, trips INTEGER NOT NULL DEFAULT 0, brought INTEGER NOT NULL DEFAULT 0)");
    Database_Exec("CREATE TABLE IF NOT EXISTS company_applications (account_id INTEGER PRIMARY KEY, company INTEGER NOT NULL, name TEXT NOT NULL, created INTEGER NOT NULL DEFAULT 0)");
    Database_Exec("CREATE TABLE IF NOT EXISTS company_log (id INTEGER PRIMARY KEY AUTOINCREMENT, company INTEGER NOT NULL, time INTEGER NOT NULL, amount INTEGER NOT NULL, text TEXT NOT NULL)");
}

stock Firm_OnPlayerConnect(playerid)
{
    FirmCompany[playerid] = -1;
    FirmRank[playerid] = 0;
    FirmInviteCompany[playerid] = -1;
}

stock Firm_OnLogin(playerid)
{
    new q[96];
    format(q, sizeof (q), "SELECT company, rank FROM company_members WHERE account_id = %d", PlayerData[playerid][pID]);
    new DBResult:r = Database_Query(q);
    if (r == DBResult:0) return;
    if (DB_GetRowCount(r) > 0) {
        new c = DB_GetFieldIntByName(r, "company");
        if (Company_IsValid(c) && Company[c][cpOwner] != 0) {
            FirmCompany[playerid] = c;
            FirmRank[playerid] = DB_GetFieldIntByName(r, "rank");
        }
    }
    DB_FreeResultSet(r);
}

// Запис в историята на касата
stock Firm_Log(c, amount, const text[])
{
    new safe[128], q[320];
    SQL_Escape(text, safe);
    format(q, sizeof (q), "INSERT INTO company_log (company, time, amount, text) VALUES (%d, %d, %d, '%s')", c, gettime(), amount, safe);
    Database_Exec(q);
    format(q, sizeof (q), "DELETE FROM company_log WHERE company = %d AND id NOT IN (SELECT id FROM company_log WHERE company = %d ORDER BY id DESC LIMIT %d)", c, c, FIRM_LOG_KEEP);
    Database_Exec(q);
}

// ---------- Кой къде е ----------

// Компанията на играча - собствена или тази, в която работи (-1 = никоя)
stock Firm_GetCompany(playerid)
{
    new c = Business_GetOwned(playerid);
    if (c != -1) return c;
    return FirmCompany[playerid];
}

stock Firm_GetMemberCompany(playerid)
{
    return FirmCompany[playerid];
}

static stock bool:Firm_IsOwner(playerid, c)
{
    return (c >= 0 && Company[c][cpOwner] != 0 && Company[c][cpOwner] == PlayerData[playerid][pID]);
}

// Може ли да управлява служителите (собственик или мениджър)
static stock bool:Firm_CanManage(playerid, c)
{
    return Firm_IsOwner(playerid, c) || (FirmCompany[playerid] == c && FirmRank[playerid] == FIRM_RANK_MANAGER);
}

stock Firm_Slots(c)
{
    return FirmSlots[CompanyDefs[c][cdSize]] + Firm_ExtraSlots(c);
}

stock Firm_CountMembers(c)
{
    new q[96], n = 0;
    format(q, sizeof (q), "SELECT COUNT(*) AS n FROM company_members WHERE company = %d", c);
    new DBResult:r = Database_Query(q);
    if (r != DBResult:0) {
        if (DB_GetRowCount(r) > 0) n = DB_GetFieldIntByName(r, "n");
        DB_FreeResultSet(r);
    }
    return n;
}

static stock Firm_CountApplications(c)
{
    new q[96], n = 0;
    format(q, sizeof (q), "SELECT COUNT(*) AS n FROM company_applications WHERE company = %d", c);
    new DBResult:r = Database_Query(q);
    if (r != DBResult:0) {
        if (DB_GetRowCount(r) > 0) n = DB_GetFieldIntByName(r, "n");
        DB_FreeResultSet(r);
    }
    return n;
}

static stock Firm_FindOnline(accountId)
{
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (IsPlayerLogged(i) && PlayerData[i][pID] == accountId) return i;
    }
    return INVALID_PLAYER_ID;
}

// Цветът на компанията като {RRGGBB} за текст
stock Firm_ColorTag(c, dest[], size = sizeof (dest))
{
    new rgb = (Company[c][cpColor] != 0) ? Company[c][cpColor] : 0xF5A623;
    format(dest, size, "{%06x}", rgb);
}

// Съобщение до всички онлайн от компанията (собственик + служители)
stock Firm_Broadcast(c, const text[])
{
    new col[12];
    Firm_ColorTag(c, col);
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (!IsPlayerLogged(i)) continue;
        if (Firm_GetCompany(i) != c) continue;
        SendClientMessage(i, COLOR_WHITE, "%s[%s]" C_WHITE " %s", col, CompanyDefs[c][cdName], text);
    }
}

// ---------- Членство ----------

static stock Firm_AddMember(c, accountId, const name[], rank)
{
    new safe[64], q[256];
    SQL_Escape(name, safe);
    format(q, sizeof (q), "INSERT OR REPLACE INTO company_members (account_id, company, rank, name, joined) VALUES (%d, %d, %d, '%s', %d)",
        accountId, c, rank, safe, gettime());
    Database_Exec(q);
    format(q, sizeof (q), "DELETE FROM company_applications WHERE account_id = %d", accountId);
    Database_Exec(q);
    new p = Firm_FindOnline(accountId);
    if (p != INVALID_PLAYER_ID) {
        FirmCompany[p] = c;
        FirmRank[p] = rank;
    }
}

static stock Firm_RemoveMember(accountId)
{
    new q[96];
    format(q, sizeof (q), "DELETE FROM company_members WHERE account_id = %d", accountId);
    Database_Exec(q);
    new p = Firm_FindOnline(accountId);
    if (p != INVALID_PLAYER_ID) FirmCompany[p] = -1;
}

static stock Firm_SetRank(accountId, rank)
{
    new q[96];
    format(q, sizeof (q), "UPDATE company_members SET rank = %d WHERE account_id = %d", rank, accountId);
    Database_Exec(q);
    new p = Firm_FindOnline(accountId);
    if (p != INVALID_PLAYER_ID) FirmRank[p] = rank;
}

// Компанията остава без собственик (продадена, отнета, закрита): всички служители са освободени
stock Firm_OnCompanyReleased(c)
{
    new q[96];
    format(q, sizeof (q), "DELETE FROM company_members WHERE company = %d", c);
    Database_Exec(q);
    format(q, sizeof (q), "DELETE FROM company_applications WHERE company = %d", c);
    Database_Exec(q);
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (IsPlayerLogged(i) && FirmCompany[i] == c) {
            FirmCompany[i] = -1;
            SendClientMessage(i, COLOR_RED, "[ФИРМА] " C_WHITE "Компанията смени собственика си - вече не работиш там.");
        }
    }
}

// Собственикът купува компания: ако е бил служител някъде, напуска
stock Firm_OnBecameOwner(playerid)
{
    if (FirmCompany[playerid] != -1) Firm_RemoveMember(PlayerData[playerid][pID]);
    new q[96];
    format(q, sizeof (q), "DELETE FROM company_applications WHERE account_id = %d", PlayerData[playerid][pID]);
    Database_Exec(q);
}

// ---------- Молби ----------

stock Firm_Apply(playerid, c)
{
    if (Firm_GetCompany(playerid) != -1) return SendError(playerid, "Вече работиш в компания (или имаш своя).");
    if (Company[c][cpOwner] == 0) return SendError(playerid, "Държавните компании не наемат.");
    if (Firm_CountMembers(c) >= Firm_Slots(c)) return SendError(playerid, "Компанията няма свободни места.");
    new safe[64], q[256];
    SQL_Escape(PlayerData[playerid][pName], safe);
    format(q, sizeof (q), "INSERT OR REPLACE INTO company_applications (account_id, company, name, created) VALUES (%d, %d, '%s', %d)",
        PlayerData[playerid][pID], c, safe, gettime());
    Database_Exec(q);
    SendClientMessage(playerid, COLOR_BRAND, "[ФИРМА] " C_WHITE "Кандидатства в " C_BRAND "%s" C_WHITE ". Чакай одобрение (имаш една молба наведнъж).", CompanyDefs[c][cdName]);
    Tutorial_OnCareer(playerid);
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (IsPlayerLogged(i) && Firm_CanManage(i, c)) {
            SendClientMessage(i, COLOR_BRAND, "[ФИРМА] " C_WHITE "%s кандидатства за работа. Виж молбите в панела на компанията.", PlayerData[playerid][pName]);
        }
    }
    return 1;
}

static stock Firm_ShowApplications(playerid, c)
{
    new q[128];
    format(q, sizeof (q), "SELECT account_id, name, created FROM company_applications WHERE company = %d ORDER BY created LIMIT 20", c);
    new DBResult:r = Database_Query(q);
    new body[1600], line[96], n = 0;
    format(body, sizeof (body), "Кандидат\tПреди\n");
    if (r != DBResult:0) {
        new rows = DB_GetRowCount(r);
        for (new i = 0; i < rows && n < FIRM_LIST_MAX; i++) {
            FirmPick[playerid][n] = DB_GetFieldIntByName(r, "account_id");
            DB_GetFieldStringByName(r, "name", FirmPickName[playerid][n], MAX_PLAYER_NAME + 1);
            new hours = (gettime() - DB_GetFieldIntByName(r, "created")) / 3600;
            format(line, sizeof (line), "%s\t" C_GREY "%d ч\n", FirmPickName[playerid][n], hours);
            strcat(body, line);
            n++;
            DB_SelectNextRow(r);
        }
        DB_FreeResultSet(r);
    }
    FirmPanelCompany[playerid] = c;
    if (n == 0) {
        ShowPlayerDialog(playerid, DIALOG_FIRM_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "Молби за работа", C_WHITE "Няма нови молби.", "Назад", "");
        return;
    }
    ShowPlayerDialog(playerid, DIALOG_FIRM_APPS, DIALOG_STYLE_TABLIST_HEADERS, C_BRAND "Молби за работа", "%s", "Избери", "Назад", body);
}

// ---------- Покани ----------

static stock Firm_AskInvite(playerid, c)
{
    FirmPanelCompany[playerid] = c;
    ShowPlayerDialog(playerid, DIALOG_FIRM_INVITE_ASK, DIALOG_STYLE_INPUT, C_BRAND "Покани шофьор",
        C_WHITE "ID или ник на играча, когото каниш (трябва да е онлайн):", "Покани", "Назад");
}

static stock Firm_SendInvite(playerid, c, const who[])
{
    new target = FindPlayer(who);
    if (target == INVALID_PLAYER_ID || !IsPlayerLogged(target)) return SendError(playerid, "Няма такъв играч онлайн.");
    if (target == playerid) return SendError(playerid, "Не можеш да поканиш себе си.");
    if (Firm_GetCompany(target) != -1) return SendError(playerid, "Този играч вече работи в компания или има своя.");
    if (Firm_CountMembers(c) >= Firm_Slots(c)) return SendError(playerid, "Нямаш свободни места.");
    FirmInviteCompany[target] = c;
    FirmInviteFrom[target] = playerid;
    new salary[16];
    FormatMoney(Company[c][cpSalary], salary);
    ShowPlayerDialog(target, DIALOG_FIRM_INVITE, DIALOG_STYLE_MSGBOX, C_BRAND "Покана за работа",
        C_WHITE "%s те кани да работиш в " C_BRAND "%s" C_WHITE " като шофьор.\n\n" \
        C_WHITE "Фирмени поръчки: " C_GREEN "+%d%%\n" \
        C_WHITE "Комисионна на компанията: " C_RED "%d%%\n" \
        C_WHITE "Заплата на PayDay: " C_GREEN "$%s\n" \
        C_WHITE "Карта за гориво: %s",
        "Приеми", "Откажи", PlayerData[playerid][pName], CompanyDefs[c][cdName], FIRM_BONUS_DRIVER, Company[c][cpCommission], salary,
        Company[c][cpFuelCard] ? (C_GREEN "да") : (C_GREY "не"));
    SendClientMessage(playerid, COLOR_BRAND, "[ФИРМА] " C_WHITE "Покана изпратена до %s.", PlayerData[target][pName]);
    return 1;
}

// ---------- Служители ----------

static stock Firm_ShowMembers(playerid, c)
{
    new q[160];
    format(q, sizeof (q), "SELECT account_id, name, rank, trips, brought FROM company_members WHERE company = %d ORDER BY rank DESC, brought DESC LIMIT 20", c);
    new DBResult:r = Database_Query(q);
    new body[2400], line[160], n = 0, money[16];
    format(body, sizeof (body), "Име\tРанг\tКурсове\tДонесено\n");
    if (r != DBResult:0) {
        new rows = DB_GetRowCount(r);
        for (new i = 0; i < rows && n < FIRM_LIST_MAX; i++) {
            FirmPick[playerid][n] = DB_GetFieldIntByName(r, "account_id");
            FirmPickRank[playerid][n] = DB_GetFieldIntByName(r, "rank");
            DB_GetFieldStringByName(r, "name", FirmPickName[playerid][n], MAX_PLAYER_NAME + 1);
            if (FirmPickRank[playerid][n] < 0 || FirmPickRank[playerid][n] >= FIRM_RANKS) FirmPickRank[playerid][n] = 0;
            FormatMoney(DB_GetFieldIntByName(r, "brought"), money);
            new online = (Firm_FindOnline(FirmPick[playerid][n]) != INVALID_PLAYER_ID);
            format(line, sizeof (line), "%s%s\t%s\t%d\t" C_GREEN "$%s\n",
                online ? (C_GREEN) : (C_WHITE), FirmPickName[playerid][n], FirmRankNames[FirmPickRank[playerid][n]],
                DB_GetFieldIntByName(r, "trips"), money);
            strcat(body, line);
            n++;
            DB_SelectNextRow(r);
        }
        DB_FreeResultSet(r);
    }
    FirmPanelCompany[playerid] = c;
    new title[96];
    format(title, sizeof (title), C_BRAND "Служители" C_WHITE " | %d / %d " C_GREY "(зелено = онлайн)", n, Firm_Slots(c));
    if (n == 0) {
        ShowPlayerDialog(playerid, DIALOG_FIRM_INFO, DIALOG_STYLE_MSGBOX, title,
            C_WHITE "Още нямаш служители.\n\nПокани шофьор от панела или чакай молби (ALT на базата).", "Назад", "");
        return;
    }
    ShowPlayerDialog(playerid, DIALOG_FIRM_MEMBERS, DIALOG_STYLE_TABLIST_HEADERS, title, "%s", "Избери", "Назад", body);
}

static stock Firm_ShowMemberActions(playerid)
{
    new i = FirmPickIndex[playerid], title[64];
    format(title, sizeof (title), C_BRAND "%s" C_WHITE " | %s", FirmPickName[playerid][i], FirmRankNames[FirmPickRank[playerid][i]]);
    ShowPlayerDialog(playerid, DIALOG_FIRM_MEMBER_ACT, DIALOG_STYLE_LIST, title,
        C_GREEN "Повиши\n" C_BRAND "Понижи\n" C_RED "Уволни", "Избери", "Назад");
}

static stock Firm_DoMemberAction(playerid, action)
{
    new c = FirmPanelCompany[playerid], i = FirmPickIndex[playerid];
    new acc = FirmPick[playerid][i], rank = FirmPickRank[playerid][i];
    new bool:owner = Firm_IsOwner(playerid, c);
    if (!Firm_CanManage(playerid, c)) return 1;
    // Мениджърът не пипа други мениджъри и не прави мениджъри
    if (!owner && rank >= FIRM_RANK_MANAGER) return SendError(playerid, "Само собственикът управлява мениджърите.");
    new msg[128];
    switch (action) {
        case 0: {
            new maxRank = owner ? FIRM_RANK_MANAGER : FIRM_RANK_SENIOR;
            if (rank >= maxRank) return SendError(playerid, "Не може повече.");
            Firm_SetRank(acc, rank + 1);
            format(msg, sizeof (msg), "%s е повишен в %s.", FirmPickName[playerid][i], FirmRankNames[rank + 1]);
        }
        case 1: {
            if (rank <= FIRM_RANK_DRIVER) return SendError(playerid, "Вече е шофьор.");
            Firm_SetRank(acc, rank - 1);
            format(msg, sizeof (msg), "%s е понижен в %s.", FirmPickName[playerid][i], FirmRankNames[rank - 1]);
        }
        default: {
            new p = Firm_FindOnline(acc);
            Firm_RemoveMember(acc);
            if (p != INVALID_PLAYER_ID) SendClientMessage(p, COLOR_RED, "[ФИРМА] " C_WHITE "Уволнен си от %s.", CompanyDefs[c][cdName]);
            format(msg, sizeof (msg), "%s е уволнен от %s.", FirmPickName[playerid][i], PlayerData[playerid][pName]);
            Firm_Broadcast(c, msg);
            Firm_ShowMembers(playerid, c);
            return 1;
        }
    }
    Firm_Broadcast(c, msg);
    Firm_ShowMembers(playerid, c);
    return 1;
}

// ---------- Каса ----------

stock Firm_ShowCash(playerid, c)
{
    new cash[16], title[64];
    FormatMoney(Company[c][cpCash], cash);
    FirmPanelCompany[playerid] = c;
    format(title, sizeof (title), C_BRAND "Каса" C_WHITE " | $%s", cash);
    if (Firm_IsOwner(playerid, c)) {
        ShowPlayerDialog(playerid, DIALOG_FIRM_CASH, DIALOG_STYLE_LIST, title, "Внеси пари\nИзтегли пари\nИстория на касата", "Избери", "Назад");
    } else {
        ShowPlayerDialog(playerid, DIALOG_FIRM_CASH, DIALOG_STYLE_LIST, title, "Внеси пари\n" C_GREY "Изтегли пари (само собственикът)\n" C_WHITE "История на касата", "Избери", "Назад");
    }
}

static stock Firm_ShowHistory(playerid, c)
{
    new q[128];
    format(q, sizeof (q), "SELECT time, amount, text FROM company_log WHERE company = %d ORDER BY id DESC LIMIT 20", c);
    new DBResult:r = Database_Query(q);
    new body[2400], line[160], text[96], money[16];
    format(body, sizeof (body), "Преди\tСума\tКакво\n");
    new n = 0;
    if (r != DBResult:0) {
        new rows = DB_GetRowCount(r);
        for (new i = 0; i < rows; i++) {
            new ago = gettime() - DB_GetFieldIntByName(r, "time"), amount = DB_GetFieldIntByName(r, "amount");
            DB_GetFieldStringByName(r, "text", text, sizeof (text));
            FormatMoney((amount < 0) ? -amount : amount, money);
            new when[16];
            if (ago < 3600) format(when, sizeof (when), "%d мин", ago / 60);
            else if (ago < 86400) format(when, sizeof (when), "%d ч", ago / 3600);
            else format(when, sizeof (when), "%d дни", ago / 86400);
            if (amount >= 0) format(line, sizeof (line), C_GREY "%s\t" C_GREEN "+$%s\t" C_WHITE "%s\n", when, money, text);
            else format(line, sizeof (line), C_GREY "%s\t" C_RED "-$%s\t" C_WHITE "%s\n", when, money, text);
            strcat(body, line);
            n++;
            DB_SelectNextRow(r);
        }
        DB_FreeResultSet(r);
    }
    if (n == 0) strcat(body, C_GREY "-\t-\tНяма движения\n");
    ShowPlayerDialog(playerid, DIALOG_FIRM_INFO, DIALOG_STYLE_TABLIST_HEADERS, C_BRAND "История на касата", "%s", "Назад", "", body);
}

// ---------- Облик ----------

static stock Firm_ColorName(c)
{
    new name[16];
    format(name, sizeof (name), "Оранжев");
    for (new i = 0; i < sizeof (FirmColors); i++) {
        if (FirmColors[i][fcRGB] == Company[c][cpColor]) format(name, sizeof (name), "%s", FirmColors[i][fcName]);
    }
    return name;
}


static stock Firm_ShowLook(playerid, c)
{
    new body[512], col[12], tag[16], slogan[64];
    Firm_ColorTag(c, col);
    if (Company[c][cpTag][0] != EOS) format(tag, sizeof (tag), "[%s]", Company[c][cpTag]);
    else format(tag, sizeof (tag), C_GREY "няма");
    if (Company[c][cpSlogan][0] != EOS) format(slogan, sizeof (slogan), "%s", Company[c][cpSlogan]);
    else format(slogan, sizeof (slogan), C_GREY "няма");
    format(body, sizeof (body),
        "Име\t%s%s\t" C_GREY "$%d, веднъж на %d дни\n" \
        "Таг в чата\t%s\t" C_GREY "безплатно\n" \
        "Цвят\t%s%s\t" C_GREY "безплатно\n" \
        "Слоган\t%s\t" C_GREY "безплатно",
        col, CompanyDefs[c][cdName], FIRM_RENAME_PRICE, FIRM_RENAME_DAYS, tag, col, Firm_ColorName(c), slogan);
    FirmPanelCompany[playerid] = c;
    ShowPlayerDialog(playerid, DIALOG_FIRM_LOOK, DIALOG_STYLE_TABLIST, C_BRAND "Облик на компанията", "%s", "Смени", "Назад", body);
}

static stock bool:Firm_ValidName(const name[])
{
    new len = strlen(name);
    if (len < 3 || len > 30) return false;
    for (new i = 0; i < len; i++) {
        new ch = name[i];
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) continue;
        if (ch == ' ' || ch == '.' || ch == '-' || ch == '&' || ch == '\'') continue;
        return false;
    }
    return true;
}

static stock Firm_Rename(playerid, c, const name[])
{
    if (!Firm_IsOwner(playerid, c)) return 1;
    if (!Firm_ValidName(name)) return SendError(playerid, "Името е 3-30 знака: латински букви, цифри, интервал и . - & '");
    new wait = Company[c][cpRenamedAt] + FIRM_RENAME_DAYS * 86400 - gettime();
    if (wait > 0) {
        SendClientMessage(playerid, COLOR_RED, "[!] " C_WHITE "Името може да се смени пак след %d ч.", (wait + 3599) / 3600);
        return 1;
    }
    if (Company[c][cpCash] < FIRM_RENAME_PRICE) return SendError(playerid, "В касата няма достатъчно пари за смяната ($10 000).");
    for (new i = 0; i < MAX_COMPANIES; i++) {
        if (i != c && Company_IsValid(i) && !strcmp(CompanyDefs[i][cdName], name, true)) return SendError(playerid, "Има компания с такова име.");
    }
    new old[32], log[96], safe[64], q[160];
    format(old, sizeof (old), "%s", CompanyDefs[c][cdName]);
    Company[c][cpCash] -= FIRM_RENAME_PRICE;
    Company[c][cpRenamedAt] = gettime();
    format(CompanyDefs[c][cdName], 32, "%s", name);
    SQL_Escape(name, safe);
    format(q, sizeof (q), "UPDATE company_defs SET name = '%s' WHERE id = %d", safe, c);
    Database_Exec(q);
    Company_Save(c);
    Company_CreateMarkers(c);
    format(log, sizeof (log), "Смяна на името (беше %s)", old);
    Firm_Log(c, -FIRM_RENAME_PRICE, log);
    SendClientMessageToAll(COLOR_BRAND, "[CARGO] " C_WHITE "Компанията " C_GREY "%s" C_WHITE " вече се казва " C_BRAND "%s" C_WHITE ".", old, name);
    return 1;
}

static stock Firm_SetTag(playerid, c, const tag[])
{
    new len = strlen(tag), up[8];
    if (len == 0) {
        Company[c][cpTag][0] = EOS;
        Company_Save(c);
        SendClientMessage(playerid, COLOR_BRAND, "[ФИРМА] " C_WHITE "Тагът е махнат.");
        return 1;
    }
    if (len < 2 || len > 4) return SendError(playerid, "Тагът е 2-4 знака: латински букви и цифри.");
    for (new i = 0; i < len; i++) {
        new ch = tag[i];
        if (ch >= 'a' && ch <= 'z') ch -= 32;
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9'))) return SendError(playerid, "Тагът е 2-4 знака: латински букви и цифри.");
        up[i] = ch;
    }
    up[len] = EOS;
    format(Company[c][cpTag], 5, "%s", up);
    Company_Save(c);
    SendClientMessage(playerid, COLOR_BRAND, "[ФИРМА] " C_WHITE "Тагът е " C_BRAND "[%s]" C_WHITE ".", up);
    return 1;
}

static stock Firm_SetSlogan(playerid, c, const text[])
{
    if (strlen(text) > 40) return SendError(playerid, "Слоганът е до 40 знака.");
    for (new i = 0; text[i] != EOS; i++) {
        if (text[i] == '{' || text[i] == '}' || text[i] == '~') return SendError(playerid, "Без знаците { } ~.");
    }
    format(Company[c][cpSlogan], 48, "%s", text);
    Company_Save(c);
    Company_CreateMarkers(c);
    SendClientMessage(playerid, COLOR_BRAND, "[ФИРМА] " C_WHITE "Слоганът е сменен.");
    return 1;
}

// ---------- Условия за служителите ----------

static stock Firm_ShowTerms(playerid, c)
{
    new body[400], salary[16];
    FormatMoney(Company[c][cpSalary], salary);
    format(body, sizeof (body),
        "Комисионна от курсовете\t" C_BRAND "%d%%\t" C_GREY "0-%d%%, отива в касата\n" \
        "Заплата на PayDay\t" C_GREEN "$%s\t" C_GREY "плаща се от касата\n" \
        "Карта за гориво\t%s\t" C_GREY "касата плаща горивото",
        Company[c][cpCommission], FIRM_COMMISSION_MAX, salary,
        Company[c][cpFuelCard] ? (C_GREEN "да") : (C_GREY "не"));
    FirmPanelCompany[playerid] = c;
    ShowPlayerDialog(playerid, DIALOG_FIRM_TERMS, DIALOG_STYLE_TABLIST, C_BRAND "Условия за служителите", "%s", "Смени", "Назад", body);
}

// ---------- Панел на служителя ----------

stock Firm_ShowMemberPanel(playerid)
{
    new c = FirmCompany[playerid];
    if (c == -1) return 0;
    FirmPanelCompany[playerid] = c;
    new body[700], salary[16], col[12], title[96];
    FormatMoney(Company[c][cpSalary], salary);
    Firm_ColorTag(c, col);
    new bonus = (FirmRank[playerid] >= FIRM_RANK_SENIOR) ? FIRM_BONUS_SENIOR : FIRM_BONUS_DRIVER;
    format(body, sizeof (body),
        "Ранг\t" C_BRAND "%s\n" \
        "Фирмени поръчки\t" C_GREEN "+%d%%\n" \
        "Комисионна\t" C_RED "%d%%\n" \
        "Заплата на PayDay\t" C_GREEN "$%s\n" \
        "Карта за гориво\t%s\n" \
        "Каса и история\t\n",
        FirmRankNames[FirmRank[playerid]], bonus, Company[c][cpCommission], salary,
        Company[c][cpFuelCard] ? (C_GREEN "да") : (C_GREY "не"));
    if (FirmRank[playerid] == FIRM_RANK_MANAGER) {
        strcat(body, "Служители\t\nМолби за работа\t\nПокани шофьор\t\n");
    }
    strcat(body, C_RED "Напусни компанията\t");
    format(title, sizeof (title), "%s%s" C_WHITE " | служител", col, CompanyDefs[c][cdName]);
    ShowPlayerDialog(playerid, DIALOG_FIRM_MEMBER_PANEL, DIALOG_STYLE_TABLIST, title, "%s", "Избери", "Затвори", body);
    return 1;
}

// ---------- Панел на собственика (вика се от business.inc) ----------

stock Firm_ShowOwnerExtras(playerid, c, body[], size)
{
    #pragma unused playerid
    new line[160], cash[16];
    FormatMoney(Company[c][cpCash], cash);
    format(line, sizeof (line), "Каса\t" C_GREEN "$%s\t" C_GREY "внеси, изтегли, история\n", cash);
    strcat(body, line, size);
    format(line, sizeof (line), "Служители\t%d / %d\t\n", Firm_CountMembers(c), Firm_Slots(c));
    strcat(body, line, size);
    new apps = Firm_CountApplications(c);
    if (apps > 0) format(line, sizeof (line), "Молби за работа\t" C_GREEN "%d нови\t\n", apps);
    else format(line, sizeof (line), "Молби за работа\t" C_GREY "няма\t\n");
    strcat(body, line, size);
    strcat(body, "Покани шофьор\t\t\n", size);
    strcat(body, "Облик: име, таг, цвят, слоган\t\t\n", size);
    format(line, sizeof (line), "Условия за служителите\t" C_GREY "%d%%, заплата, гориво\t\n", Company[c][cpCommission]);
    strcat(body, line, size);
    format(line, sizeof (line), "Подобрения и ниво\t" C_BRAND "ниво %d" C_GREY ", репутация %d%%\t\n", Firm_Level(c), Company[c][cpRep]);
    strcat(body, line, size);
}

// Изборът в панела на собственика (редове 0-6 от Firm_ShowOwnerExtras)
stock Firm_OwnerPick(playerid, c, row)
{
    FirmPanelCompany[playerid] = c;
    switch (row) {
        case 0: Firm_ShowCash(playerid, c);
        case 1: Firm_ShowMembers(playerid, c);
        case 2: Firm_ShowApplications(playerid, c);
        case 3: Firm_AskInvite(playerid, c);
        case 4: Firm_ShowLook(playerid, c);
        case 5: Firm_ShowTerms(playerid, c);
        case 6: Firm_OpenUpgrades(playerid, c);
    }
}

// ---------- Курсове, PayDay, гориво, чат ----------

// Процент бонус за фирмена поръчка (0 = не е фирмена)
stock Firm_OrderBonus(playerid, from)
{
    if (FirmCompany[playerid] == -1 || FirmCompany[playerid] != from) return 0;
    // + ниво на компанията (по 1% за всяко ниво над 1)
    return ((FirmRank[playerid] >= FIRM_RANK_SENIOR) ? FIRM_BONUS_SENIOR : FIRM_BONUS_DRIVER) + Firm_Level(from) - 1;
}

// След доставка: бонус за фирмена поръчка и комисионна. Връща промяната в плащането на шофьора.
stock Firm_OnDelivery(playerid, from, base, total, line[], size)
{
    line[0] = EOS;
    new c = FirmCompany[playerid];
    if (c == -1) return 0;
    new bonus = 0, pct = Firm_OrderBonus(playerid, from);
    if (pct > 0) bonus = base * pct / 100;
    new commission = (total + bonus) * Company[c][cpCommission] / 100;
    if (commission > 0) {
        Company[c][cpCash] += commission;
        Company[c][cpEarned] += commission;
        Company_Save(c);
        new log[96];
        format(log, sizeof (log), "Комисионна от %s", PlayerData[playerid][pName]);
        Firm_Log(c, commission, log);
    }
    new q[160];
    format(q, sizeof (q), "UPDATE company_members SET trips = trips + 1, brought = brought + %d WHERE account_id = %d", commission, PlayerData[playerid][pID]);
    Database_Exec(q);
    new sBonus[16], sComm[16];
    FormatMoney(bonus, sBonus);
    FormatMoney(commission, sComm);
    format(line, size, C_WHITE "Фирмена поръчка (+%d%%):\t" C_GREEN "+$%s\n" C_WHITE "Комисионна на компанията:\t" C_RED "-$%s\n", pct, sBonus, sComm);
    return bonus - commission;
}

// PayDay: заплата от касата
stock Firm_PaySalary(playerid)
{
    new c = FirmCompany[playerid];
    if (c == -1 || Company[c][cpSalary] <= 0) return;
    new salary = Company[c][cpSalary];
    if (Company[c][cpCash] < salary) {
        SendClientMessage(playerid, COLOR_RED, "[PAYDAY] " C_WHITE "Касата на компанията е празна - няма заплата този път.");
        return;
    }
    Company[c][cpCash] -= salary;
    Company_Save(c);
    Money_Give(playerid, salary);
    new log[64];
    format(log, sizeof (log), "Заплата на %s", PlayerData[playerid][pName]);
    Firm_Log(c, -salary, log);
    SendClientMessage(playerid, COLOR_BRAND, "[PAYDAY] " C_WHITE "Заплата от %s: " C_GREEN "$%d", CompanyDefs[c][cdName], salary);
}

// Карта за гориво: 1 = компанията плати
stock Firm_PayFuel(playerid, cost)
{
    new c = FirmCompany[playerid];
    if (c == -1 || !Company[c][cpFuelCard] || cost <= 0) return 0;
    if (Company[c][cpCash] < cost) {
        SendClientMessage(playerid, COLOR_RED, "[ГОРИВО] " C_WHITE "Касата на компанията няма пари за гориво - плащаш ти.");
        return 0;
    }
    Company[c][cpCash] -= cost;
    Company_Save(c);
    new log[64];
    format(log, sizeof (log), "Гориво за %s", PlayerData[playerid][pName]);
    Firm_Log(c, -cost, log);
    return 1;
}

// Таг пред името в чата: "{цвят}[TAG] " или празно
stock Firm_ChatPrefix(playerid, dest[], size = sizeof (dest))
{
    dest[0] = EOS;
    new c = Firm_GetCompany(playerid);
    if (c == -1 || Company[c][cpTag][0] == EOS) return;
    new col[12];
    Firm_ColorTag(c, col);
    format(dest, size, "%s[%s] ", col, Company[c][cpTag]);
}

CMD:f(playerid, params[])
{
    new c = Firm_GetCompany(playerid);
    if (c == -1) return SendError(playerid, "Не работиш в компания.");
    if (IsNull(params)) return SendUsage(playerid, "/f [текст] - фирмен чат");
    new col[12], rank[24];
    Firm_ColorTag(c, col);
    if (Firm_IsOwner(playerid, c)) format(rank, sizeof (rank), "Собственик");
    else format(rank, sizeof (rank), "%s", FirmRankNames[FirmRank[playerid]]);
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (!IsPlayerLogged(i) || Firm_GetCompany(i) != c) continue;
        SendClientMessage(i, COLOR_WHITE, "%s[ФИРМА] " C_GREY "%s " C_WHITE "%s: %s", col, rank, PlayerData[playerid][pName], params);
    }
    return 1;
}

// ---------- Диалози ----------

stock Firm_OnDialogResponse(playerid, dialogid, response, listitem, const inputtext[])
{
    if (Firm_UpgradesDialog(playerid, dialogid, response, listitem)) return 1;
    new c = FirmPanelCompany[playerid];
    switch (dialogid) {
        case DIALOG_FIRM_INFO: {
            if (Firm_GetCompany(playerid) != -1) Business_OpenOwn(playerid);
            return 1;
        }
        case DIALOG_FIRM_INVITE: {
            new ic = FirmInviteCompany[playerid], from = FirmInviteFrom[playerid];
            FirmInviteCompany[playerid] = -1;
            if (ic == -1 || !Company_IsValid(ic) || Company[ic][cpOwner] == 0) return 1;
            if (!response) {
                if (IsPlayerLogged(from)) SendClientMessage(from, COLOR_GREY, "[ФИРМА] " C_WHITE "%s отказа поканата.", PlayerData[playerid][pName]);
                return 1;
            }
            if (Firm_GetCompany(playerid) != -1) return SendError(playerid, "Вече работиш в компания.");
            if (Firm_CountMembers(ic) >= Firm_Slots(ic)) return SendError(playerid, "Компанията вече няма свободни места.");
            Firm_AddMember(ic, PlayerData[playerid][pID], PlayerData[playerid][pName], FIRM_RANK_DRIVER);
            new msg[96];
            format(msg, sizeof (msg), "%s е новият ни шофьор! Добре дошъл.", PlayerData[playerid][pName]);
            Firm_Broadcast(ic, msg);
            SendClientMessage(playerid, COLOR_BRAND, "[ФИРМА] " C_WHITE "Панелът е в " C_BRAND "GPS > Моето имущество > Моята компания" C_WHITE ", чатът - " C_BRAND "/f" C_WHITE ".");
            return 1;
        }
    }

    if (c < 0 || c >= MAX_COMPANIES || !Company_IsValid(c)) return 0;

    switch (dialogid) {
        case DIALOG_FIRM_CASH: {
            if (!response) { Business_OpenOwn(playerid); return 1; }
            switch (listitem) {
                case 0: ShowPlayerDialog(playerid, DIALOG_FIRM_DEPOSIT, DIALOG_STYLE_INPUT, C_BRAND "Внеси в касата", C_WHITE "Колко пари внасяш?", "Внеси", "Назад");
                case 1: {
                    if (!Firm_IsOwner(playerid, c)) { SendError(playerid, "Само собственикът тегли от касата."); Firm_ShowCash(playerid, c); return 1; }
                    ShowPlayerDialog(playerid, DIALOG_FIRM_WITHDRAW, DIALOG_STYLE_INPUT, C_BRAND "Изтегли от касата", C_WHITE "Колко пари теглиш? (" C_BRAND "всичко" C_WHITE " = цялата каса)", "Изтегли", "Назад");
                }
                case 2: Firm_ShowHistory(playerid, c);
            }
            return 1;
        }
        case DIALOG_FIRM_DEPOSIT: {
            if (!response) { Firm_ShowCash(playerid, c); return 1; }
            new amount = strval(inputtext);
            if (!IsNumeric(inputtext) || amount <= 0) { SendError(playerid, "Въведи сума."); Firm_ShowCash(playerid, c); return 1; }
            if (Firm_GetCompany(playerid) != c) return 1;
            if (!Money_Take(playerid, amount)) { SendError(playerid, "Нямаш толкова пари."); Firm_ShowCash(playerid, c); return 1; }
            Company[c][cpCash] += amount;
            Company_Save(c);
            new log[64];
            format(log, sizeof (log), "Внесени от %s", PlayerData[playerid][pName]);
            Firm_Log(c, amount, log);
            SendClientMessage(playerid, COLOR_BRAND, "[ФИРМА] " C_WHITE "Внесе " C_GREEN "$%d" C_WHITE " в касата.", amount);
            Firm_ShowCash(playerid, c);
            return 1;
        }
        case DIALOG_FIRM_WITHDRAW: {
            if (!response || !Firm_IsOwner(playerid, c)) { Firm_ShowCash(playerid, c); return 1; }
            new amount = (inputtext[0] != EOS && !strcmp(inputtext, "всичко", true)) ? Company[c][cpCash] : strval(inputtext);
            if (amount <= 0 || amount > Company[c][cpCash]) { SendError(playerid, "Няма толкова пари в касата."); Firm_ShowCash(playerid, c); return 1; }
            Company[c][cpCash] -= amount;
            Company_Save(c);
            Money_Give(playerid, amount);
            Firm_Log(c, -amount, "Изтеглени от собственика");
            SendClientMessage(playerid, COLOR_BRAND, "[ФИРМА] " C_WHITE "Изтегли " C_GREEN "$%d" C_WHITE " от касата.", amount);
            Firm_ShowCash(playerid, c);
            return 1;
        }
        case DIALOG_FIRM_MEMBERS: {
            if (!response || listitem < 0 || listitem >= FIRM_LIST_MAX) { Business_OpenOwn(playerid); return 1; }
            if (!Firm_CanManage(playerid, c)) return 1;
            FirmPickIndex[playerid] = listitem;
            Firm_ShowMemberActions(playerid);
            return 1;
        }
        case DIALOG_FIRM_MEMBER_ACT: {
            if (!response) { Firm_ShowMembers(playerid, c); return 1; }
            Firm_DoMemberAction(playerid, listitem);
            return 1;
        }
        case DIALOG_FIRM_APPS: {
            if (!response || listitem < 0 || listitem >= FIRM_LIST_MAX) { Business_OpenOwn(playerid); return 1; }
            FirmPickIndex[playerid] = listitem;
            new title[64];
            format(title, sizeof (title), C_BRAND "Молба от %s", FirmPickName[playerid][listitem]);
            ShowPlayerDialog(playerid, DIALOG_FIRM_APP_ACT, DIALOG_STYLE_MSGBOX, title,
                C_WHITE "Да наема ли " C_BRAND "%s" C_WHITE " като шофьор?", "Наеми", "Откажи", FirmPickName[playerid][listitem]);
            return 1;
        }
        case DIALOG_FIRM_APP_ACT: {
            if (!Firm_CanManage(playerid, c)) return 1;
            new i = FirmPickIndex[playerid], acc = FirmPick[playerid][i], p = Firm_FindOnline(acc), q[96];
            if (response) {
                if (Firm_CountMembers(c) >= Firm_Slots(c)) { SendError(playerid, "Нямаш свободни места."); Firm_ShowApplications(playerid, c); return 1; }
                // Може междувременно да е влязъл другаде
                format(q, sizeof (q), "SELECT company FROM company_members WHERE account_id = %d", acc);
                new DBResult:r = Database_Query(q), bool:taken = false;
                if (r != DBResult:0) {
                    taken = (DB_GetRowCount(r) > 0);
                    DB_FreeResultSet(r);
                }
                if (taken) {
                    format(q, sizeof (q), "DELETE FROM company_applications WHERE account_id = %d", acc);
                    Database_Exec(q);
                    SendError(playerid, "Този играч вече работи другаде.");
                    Firm_ShowApplications(playerid, c);
                    return 1;
                }
                Firm_AddMember(c, acc, FirmPickName[playerid][i], FIRM_RANK_DRIVER);
                new msg[96];
                format(msg, sizeof (msg), "%s е новият ни шофьор! Добре дошъл.", FirmPickName[playerid][i]);
                Firm_Broadcast(c, msg);
            } else {
                format(q, sizeof (q), "DELETE FROM company_applications WHERE account_id = %d", acc);
                Database_Exec(q);
                if (p != INVALID_PLAYER_ID) SendClientMessage(p, COLOR_GREY, "[ФИРМА] " C_WHITE "Молбата ти в %s е отказана.", CompanyDefs[c][cdName]);
            }
            Firm_ShowApplications(playerid, c);
            return 1;
        }
        case DIALOG_FIRM_INVITE_ASK: {
            if (!response || inputtext[0] == EOS) { Business_OpenOwn(playerid); return 1; }
            if (Firm_CanManage(playerid, c)) Firm_SendInvite(playerid, c, inputtext);
            return 1;
        }
        case DIALOG_FIRM_LOOK: {
            if (!response || !Firm_IsOwner(playerid, c)) { Business_OpenOwn(playerid); return 1; }
            switch (listitem) {
                case 0: ShowPlayerDialog(playerid, DIALOG_FIRM_RENAME, DIALOG_STYLE_INPUT, C_BRAND "Ново име",
                    C_WHITE "Ново име на компанията (3-30 знака, на латиница - вижда се и в панела на курса).\n" \
                    C_GREY "Цена: $10 000 от касата. Може веднъж на 7 дни.", "Смени", "Назад");
                case 1: ShowPlayerDialog(playerid, DIALOG_FIRM_TAG, DIALOG_STYLE_INPUT, C_BRAND "Таг",
                    C_WHITE "Таг от 2-4 латински букви или цифри (напр. RTL).\nСтои пред името на хората ти в чата.\n" C_GREY "Празно = без таг.", "Готово", "Назад");
                case 2: {
                    new body[400], line[64];
                    body[0] = EOS;
                    for (new i = 0; i < sizeof (FirmColors); i++) {
                        format(line, sizeof (line), "{%06x}%s\n", FirmColors[i][fcRGB], FirmColors[i][fcName]);
                        strcat(body, line);
                    }
                    ShowPlayerDialog(playerid, DIALOG_FIRM_COLOR, DIALOG_STYLE_LIST, C_BRAND "Цвят на компанията", "%s", "Избери", "Назад", body);
                }
                case 3: ShowPlayerDialog(playerid, DIALOG_FIRM_SLOGAN, DIALOG_STYLE_INPUT, C_BRAND "Слоган",
                    C_WHITE "Слоган до 40 знака - стои под името на базата.\n" C_GREY "Празно = без слоган.", "Готово", "Назад");
            }
            return 1;
        }
        case DIALOG_FIRM_RENAME: {
            if (response && Firm_IsOwner(playerid, c)) Firm_Rename(playerid, c, inputtext);
            Firm_ShowLook(playerid, c);
            return 1;
        }
        case DIALOG_FIRM_TAG: {
            if (response && Firm_IsOwner(playerid, c)) Firm_SetTag(playerid, c, inputtext);
            Firm_ShowLook(playerid, c);
            return 1;
        }
        case DIALOG_FIRM_COLOR: {
            if (response && Firm_IsOwner(playerid, c) && listitem >= 0 && listitem < sizeof (FirmColors)) {
                Company[c][cpColor] = FirmColors[listitem][fcRGB];
                Company_Save(c);
                Company_CreateMarkers(c);
            }
            Firm_ShowLook(playerid, c);
            return 1;
        }
        case DIALOG_FIRM_SLOGAN: {
            if (response && Firm_IsOwner(playerid, c)) Firm_SetSlogan(playerid, c, inputtext);
            Firm_ShowLook(playerid, c);
            return 1;
        }
        case DIALOG_FIRM_TERMS: {
            if (!response || !Firm_IsOwner(playerid, c)) { Business_OpenOwn(playerid); return 1; }
            switch (listitem) {
                case 0: ShowPlayerDialog(playerid, DIALOG_FIRM_COMMISSION, DIALOG_STYLE_INPUT, C_BRAND "Комисионна",
                    C_WHITE "Колко %% от курсовете на служителите взима компанията? (0-30)", "Готово", "Назад");
                case 1: ShowPlayerDialog(playerid, DIALOG_FIRM_SALARY, DIALOG_STYLE_INPUT, C_BRAND "Заплата на PayDay",
                    C_WHITE "Колко плаща касата на всеки служител на PayDay? (0-2000)\n" C_GREY "Само ако са изиграли 20-те минути и в касата има пари.", "Готово", "Назад");
                case 2: {
                    Company[c][cpFuelCard] = !Company[c][cpFuelCard];
                    Company_Save(c);
                    Firm_ShowTerms(playerid, c);
                }
            }
            return 1;
        }
        case DIALOG_FIRM_COMMISSION: {
            if (response && Firm_IsOwner(playerid, c)) {
                new v = strval(inputtext);
                if (!IsNumeric(inputtext) || v < 0 || v > FIRM_COMMISSION_MAX) SendError(playerid, "От 0 до 30.");
                else {
                    Company[c][cpCommission] = v;
                    Company_Save(c);
                    new msg[96];
                    format(msg, sizeof (msg), "Комисионната от курсовете вече е %d%%.", v);
                    Firm_Broadcast(c, msg);
                }
            }
            Firm_ShowTerms(playerid, c);
            return 1;
        }
        case DIALOG_FIRM_SALARY: {
            if (response && Firm_IsOwner(playerid, c)) {
                new v = strval(inputtext);
                if (!IsNumeric(inputtext) || v < 0 || v > FIRM_SALARY_MAX) SendError(playerid, "От 0 до 2000.");
                else {
                    Company[c][cpSalary] = v;
                    Company_Save(c);
                    new msg[96];
                    format(msg, sizeof (msg), "Заплатата на PayDay вече е $%d.", v);
                    Firm_Broadcast(c, msg);
                }
            }
            Firm_ShowTerms(playerid, c);
            return 1;
        }
        case DIALOG_FIRM_MEMBER_PANEL: {
            if (!response || FirmCompany[playerid] != c) return 1;
            // Редове: 0-4 информация, 5 каса, (мениджър: 6 служители, 7 молби, 8 покани), последният - напусни
            new leaveRow = (FirmRank[playerid] == FIRM_RANK_MANAGER) ? 9 : 6;
            if (listitem == 5) Firm_ShowCash(playerid, c);
            else if (listitem == leaveRow) {
                ShowPlayerDialog(playerid, DIALOG_FIRM_LEAVE, DIALOG_STYLE_MSGBOX, C_RED "Напускане",
                    C_WHITE "Сигурен ли си, че напускаш " C_BRAND "%s" C_WHITE "?", "Напусни", "Назад", CompanyDefs[c][cdName]);
            }
            else if (FirmRank[playerid] == FIRM_RANK_MANAGER && listitem == 6) Firm_ShowMembers(playerid, c);
            else if (FirmRank[playerid] == FIRM_RANK_MANAGER && listitem == 7) Firm_ShowApplications(playerid, c);
            else if (FirmRank[playerid] == FIRM_RANK_MANAGER && listitem == 8) Firm_AskInvite(playerid, c);
            else Firm_ShowMemberPanel(playerid);
            return 1;
        }
        case DIALOG_FIRM_LEAVE: {
            if (!response) { Firm_ShowMemberPanel(playerid); return 1; }
            if (FirmCompany[playerid] != c) return 1;
            Firm_RemoveMember(PlayerData[playerid][pID]);
            new msg[96];
            format(msg, sizeof (msg), "%s напусна компанията.", PlayerData[playerid][pName]);
            Firm_Broadcast(c, msg);
            SendClientMessage(playerid, COLOR_BRAND, "[ФИРМА] " C_WHITE "Напусна %s.", CompanyDefs[c][cdName]);
            return 1;
        }
    }
    return 0;
}

// =====================================================================
// Етап 2: подобрения на базата, ниво и репутация на компанията
// =====================================================================
//
// Подобрения (по 3 нива, плащат се от касата; по-високите искат по-високо ниво на компанията):
//   0 Склад        - по-голям капацитет (+25/50/100%)
//   1 Производство - произвежда повече (+20/40/70%)
//   2 Товарна рампа - по-бързо товарене/разтоварване в базата (-20/35/50% време)
//   3 Колонка      - служителите зареждат на базата с отстъпка (25/40/50%)
//   4 Сервиз       - служителите ремонтират на базата с отстъпка (25/50/75%)
//   5 Офис         - още места за служители (+3/6/10)
// Ниво: опит от всеки курс от/до компанията. Всяко ниво над 1 дава +1% на фирмените поръчки.
// Репутация 0-100: +1 на курс, -2 на час, ако 24 ч няма курс. Влияе на производството (75-125%)
// и на плащането на поръчките от компанията (90-110%).

#define FIRM_UPGRADES       6
#define FIRM_MAX_LEVEL      10

static const FirmUpNames[FIRM_UPGRADES][] = {"Склад", "Производство", "Товарна рампа", "Фирмена колонка", "Сервиз", "Офис"};
static const FirmUpEffect[FIRM_UPGRADES][4] = {
    {0, 25, 50, 100},       // % капацитет
    {0, 20, 40, 70},        // % производство
    {0, 20, 35, 50},        // % по-малко време
    {0, 25, 40, 50},        // % отстъпка гориво
    {0, 25, 50, 75},        // % отстъпка ремонт
    {0, 3, 6, 10}           // места
};
static const FirmUpPrice[4] = {0, 15000, 40000, 90000};
static const FirmUpNeedLevel[4] = {0, 1, 3, 5};
static const FirmLevelXP[FIRM_MAX_LEVEL] = {0, 500, 1500, 3000, 5000, 7500, 10500, 14000, 18000, 22500};
static FirmUpPick[MAX_PLAYERS];

stock Firm_Level(c)
{
    new level = 1;
    for (new i = 0; i < FIRM_MAX_LEVEL; i++) {
        if (Company[c][cpXP] >= FirmLevelXP[i]) level = i + 1;
    }
    return level;
}

stock Firm_UpEffect(c, up)
{
    return FirmUpEffect[up][Company[c][cpUp][up]];
}

// Опит и репутация от курс (вика се от Company_OnDelivery)
stock Firm_AddXP(c, xp)
{
    if (!Company_IsValid(c)) return;
    new before = Firm_Level(c);
    Company[c][cpXP] += xp;
    if (Company[c][cpRep] < 100) Company[c][cpRep]++;
    Company[c][cpLastDelivery] = gettime();
    new after = Firm_Level(c);
    if (after > before && Company[c][cpOwner] != 0) {
        new msg[96];
        format(msg, sizeof (msg), "Компанията стана ниво %d! Фирмените поръчки вече дават още +%d%%.", after, after - 1);
        Firm_Broadcast(c, msg);
    }
}

// Всеки час: репутацията пада, ако 24 ч няма курс
stock Firm_HourlyTick()
{
    new limit = gettime() - 86400;
    for (new c = 0; c < MAX_COMPANIES; c++) {
        if (!Company_IsValid(c)) continue;
        if (Company[c][cpLastDelivery] < limit && Company[c][cpRep] > 0) {
            Company[c][cpRep] -= 2;
            if (Company[c][cpRep] < 0) Company[c][cpRep] = 0;
            Company_Save(c);
        }
    }
}

// % множител на производството от репутацията (75-125)
stock Firm_RepProduction(c)
{
    return 75 + Company[c][cpRep] / 2;
}

// % множител на плащането на поръчка от компанията (90-110)
stock Firm_RepPay(c)
{
    return 90 + Company[c][cpRep] / 5;
}

// Колко % от времето за товарене остава на базата на компанията
stock Firm_DockPercent(c)
{
    if (!Company_IsValid(c)) return 100;
    return 100 - Firm_UpEffect(c, 2);
}

// Допълнителни места (офис)
stock Firm_ExtraSlots(c)
{
    return Firm_UpEffect(c, 5);
}

// ---------- Прозорец "Подобрения и ниво" ----------

static stock Firm_ShowUpgrades(playerid, c)
{
    new body[1400], line[200], level = Firm_Level(c), next[48];
    if (level < FIRM_MAX_LEVEL) format(next, sizeof (next), "още %d опит до ниво %d", FirmLevelXP[level] - Company[c][cpXP], level + 1);
    else format(next, sizeof (next), "максимално ниво");
    format(body, sizeof (body), "Подобрение\tНиво\tЕфект\tСледващо\n");
    for (new u = 0; u < FIRM_UPGRADES; u++) {
        new lv = Company[c][cpUp][u], eff[48], nxt[64];
        switch (u) {
            case 0: format(eff, sizeof (eff), "+%d%% склад", FirmUpEffect[u][lv]);
            case 1: format(eff, sizeof (eff), "+%d%% производство", FirmUpEffect[u][lv]);
            case 2: format(eff, sizeof (eff), "-%d%% време товарене", FirmUpEffect[u][lv]);
            case 3: format(eff, sizeof (eff), "-%d%% гориво", FirmUpEffect[u][lv]);
            case 4: format(eff, sizeof (eff), "-%d%% ремонт", FirmUpEffect[u][lv]);
            default: format(eff, sizeof (eff), "+%d места", FirmUpEffect[u][lv]);
        }
        if (lv >= 3) format(nxt, sizeof (nxt), C_GREY "максимум");
        else if (level < FirmUpNeedLevel[lv + 1]) format(nxt, sizeof (nxt), C_RED "ниво %d на компанията", FirmUpNeedLevel[lv + 1]);
        else format(nxt, sizeof (nxt), C_GREEN "$%d", FirmUpPrice[lv + 1]);
        format(line, sizeof (line), "%s\t%d/3\t%s\t%s\n", FirmUpNames[u], lv, eff, nxt);
        strcat(body, line);
    }
    new title[128];
    format(title, sizeof (title), C_BRAND "Ниво %d" C_WHITE " (%s) | репутация %d%%", level, next, Company[c][cpRep]);
    FirmPanelCompany[playerid] = c;
    ShowPlayerDialog(playerid, DIALOG_FIRM_UPGRADES, DIALOG_STYLE_TABLIST_HEADERS, title, "%s", "Подобри", "Назад", body);
}

static stock Firm_BuyUpgrade(playerid, c, u)
{
    if (!Firm_IsOwner(playerid, c)) return SendError(playerid, "Само собственикът подобрява базата.");
    new lv = Company[c][cpUp][u];
    if (lv >= 3) return SendError(playerid, "Това подобрение е на максимум.");
    if (Firm_Level(c) < FirmUpNeedLevel[lv + 1]) {
        SendClientMessage(playerid, COLOR_RED, "[!] " C_WHITE "Трябва ниво %d на компанията.", FirmUpNeedLevel[lv + 1]);
        return 1;
    }
    new price = FirmUpPrice[lv + 1];
    if (Company[c][cpCash] < price) return SendError(playerid, "В касата няма достатъчно пари. Внеси от Каса > Внеси пари.");
    Company[c][cpCash] -= price;
    Company[c][cpUp][u] = lv + 1;
    Company_Save(c);
    new log[64], msg[96];
    format(log, sizeof (log), "Подобрение: %s ниво %d", FirmUpNames[u], lv + 1);
    Firm_Log(c, -price, log);
    format(msg, sizeof (msg), "Базата е подобрена: %s - ниво %d!", FirmUpNames[u], lv + 1);
    Firm_Broadcast(c, msg);
    PlayerPlaySound(playerid, 1058, 0.0, 0.0, 0.0);
    if (u == 0) Economy_RefreshLabels();
    return 1;
}

stock Firm_UpgradesDialog(playerid, dialogid, response, listitem)
{
    switch (dialogid) {
        case DIALOG_FIRM_UPGRADES: {
            new c = FirmPanelCompany[playerid];
            if (!response) { Business_OpenOwn(playerid); return 1; }
            if (listitem < 0 || listitem >= FIRM_UPGRADES || !Company_IsValid(c)) return 1;
            FirmUpPick[playerid] = listitem;
            new lv = Company[c][cpUp][listitem];
            if (lv >= 3) { SendError(playerid, "Това подобрение е на максимум."); Firm_ShowUpgrades(playerid, c); return 1; }
            ShowPlayerDialog(playerid, DIALOG_FIRM_UPGRADE_OK, DIALOG_STYLE_MSGBOX, C_BRAND "Подобрение",
                C_WHITE "%s: ниво %d -> " C_BRAND "%d\n\n" C_WHITE "Цена: " C_GREEN "$%d" C_WHITE " от касата (в нея има $%d).",
                "Купи", "Назад", FirmUpNames[listitem], lv, lv + 1, FirmUpPrice[lv + 1], Company[c][cpCash]);
            return 1;
        }
        case DIALOG_FIRM_UPGRADE_OK: {
            new c = FirmPanelCompany[playerid];
            if (response && Company_IsValid(c)) Firm_BuyUpgrade(playerid, c, FirmUpPick[playerid]);
            if (Company_IsValid(c)) Firm_ShowUpgrades(playerid, c);
            return 1;
        }
    }
    return 0;
}

// Ред 6 в панела на собственика
stock Firm_OpenUpgrades(playerid, c)
{
    Firm_ShowUpgrades(playerid, c);
}
