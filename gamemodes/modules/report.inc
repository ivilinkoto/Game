// Репорт система.
//
// Играч:  /report (или "Изпрати репорт" от менюто с Y) -> категория -> текст.
//         При "Нарушение" автоматично се записват позиция, хора наоколо и
//         последните съобщения в чата наблизо.
//         След затваряне играчът оценява помощта с 1-5 звезди.
// Админ:  /reports – опашка с репорти (с бутони, удобно на телефон)
//         /ar [id] – поемаш репорта       /rr [id] [текст] – отговаряш
//         /cr [id] – затваряш             /fr [id] – фалшив репорт (блокира репортите на играча)
//         /rstats  – статистика на админите (ниво 4+)

#define REPORT_COOLDOWN_SECONDS     60
#define REPORT_MIN_LENGTH           5
#define REPORT_FALSE_BLOCK_MINUTES  30
#define REPORT_REMIND_SECONDS       120
#define REPORT_LIST_MAX             20

#define REPORT_OPEN     0
#define REPORT_CLAIMED  1
#define REPORT_CLOSED   2
#define REPORT_FALSE    3

#define REPORT_CAT_VIOLATION 1

static const ReportCategories[][] = {
    "Въпрос",
    "Нарушение на играч",
    "Бъг / проблем",
    "Помощ (заседнал съм и т.н.)"
};

// Зареден репорт (Report_Load)
static gRepStatus, gRepReporterID, gRepCategory, gRepCreated, gRepRating;
static gRepReporterName[MAX_PLAYER_NAME + 1], gRepAdmin[MAX_PLAYER_NAME + 1], gRepTarget[MAX_PLAYER_NAME + 1];
static gRepText[160], gRepContext[1024];

// ============================================================
//  База
// ============================================================

stock Report_CreateTables()
{
    new q[1024];
    q[0] = EOS;
    strcat(q, "CREATE TABLE IF NOT EXISTS reports (");
    strcat(q, "id INTEGER PRIMARY KEY AUTOINCREMENT,");
    strcat(q, "reporter_id INTEGER NOT NULL,");
    strcat(q, "reporter_name TEXT NOT NULL,");
    strcat(q, "category INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "target TEXT NOT NULL DEFAULT '',");
    strcat(q, "text TEXT NOT NULL,");
    strcat(q, "context TEXT NOT NULL DEFAULT '',");
    strcat(q, "status INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "admin_name TEXT NOT NULL DEFAULT '',");
    strcat(q, "rating INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "created_at INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "claimed_at INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "closed_at INTEGER NOT NULL DEFAULT 0)");
    Database_Exec(q);

    q[0] = EOS;
    strcat(q, "CREATE TABLE IF NOT EXISTS report_messages (");
    strcat(q, "id INTEGER PRIMARY KEY AUTOINCREMENT,");
    strcat(q, "report_id INTEGER NOT NULL,");
    strcat(q, "author TEXT NOT NULL,");
    strcat(q, "is_admin INTEGER NOT NULL DEFAULT 0,");
    strcat(q, "text TEXT NOT NULL,");
    strcat(q, "date INTEGER NOT NULL DEFAULT 0)");
    Database_Exec(q);

    Database_Exec("CREATE INDEX IF NOT EXISTS idx_reports_status ON reports (status)");
    Database_Exec("CREATE INDEX IF NOT EXISTS idx_reports_reporter ON reports (reporter_id)");
    Database_Exec("CREATE INDEX IF NOT EXISTS idx_report_messages ON report_messages (report_id)");
}

// Зарежда репорт в gRep*. Връща false, ако няма такъв.
stock bool:Report_Load(reportid)
{
    new q[128];
    format(q, sizeof (q), "SELECT * FROM reports WHERE id = %d LIMIT 1", reportid);
    new DBResult:result = Database_Query(q);
    if (result == DBResult:0) return false;
    if (DB_GetRowCount(result) == 0) {
        DB_FreeResultSet(result);
        return false;
    }
    gRepStatus     = DB_GetFieldIntByName(result, "status");
    gRepReporterID = DB_GetFieldIntByName(result, "reporter_id");
    gRepCategory   = DB_GetFieldIntByName(result, "category");
    gRepCreated    = DB_GetFieldIntByName(result, "created_at");
    gRepRating     = DB_GetFieldIntByName(result, "rating");
    DB_GetFieldStringByName(result, "reporter_name", gRepReporterName);
    DB_GetFieldStringByName(result, "admin_name", gRepAdmin);
    DB_GetFieldStringByName(result, "target", gRepTarget);
    DB_GetFieldStringByName(result, "text", gRepText);
    DB_GetFieldStringByName(result, "context", gRepContext);
    DB_FreeResultSet(result);
    return true;
}

// ID на отворения репорт на играча (0 ако няма).
stock Report_GetOpenID(playerid)
{
    new q[160], id = 0;
    format(q, sizeof (q), "SELECT id FROM reports WHERE reporter_id = %d AND status IN (%d, %d) ORDER BY id DESC LIMIT 1",
        PlayerData[playerid][pID], REPORT_OPEN, REPORT_CLAIMED);
    new DBResult:result = Database_Query(q);
    if (result != DBResult:0) {
        if (DB_GetRowCount(result) > 0) id = DB_GetFieldIntByName(result, "id");
        DB_FreeResultSet(result);
    }
    return id;
}

stock Report_AddMessage(reportid, const author[], bool:isAdmin, const text[])
{
    new eText[300], q[512];
    SQL_Escape(text, eText);
    format(q, sizeof (q), "INSERT INTO report_messages (report_id, author, is_admin, text, date) VALUES (%d, '%s', %d, '%s', %d)",
        reportid, author, isAdmin ? 1 : 0, eText, gettime());
    Database_Exec(q);
}

// Онлайн играч с даден акаунт (INVALID_PLAYER_ID, ако не е в играта).
stock Report_FindOnline(accountid)
{
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (IsPlayerConnected(i) && PlayerData[i][pLogged] && PlayerData[i][pID] == accountid) return i;
    }
    return INVALID_PLAYER_ID;
}

stock Report_StatusName(status, dest[], size = sizeof (dest))
{
    switch (status) {
        case REPORT_OPEN:    format(dest, size, C_GOLD "чака");
        case REPORT_CLAIMED: format(dest, size, C_BLUE "поет");
        case REPORT_CLOSED:  format(dest, size, C_GREEN "затворен");
        default:             format(dest, size, C_RED "фалшив");
    }
}

stock Report_Ago(timestamp, dest[], size = sizeof (dest))
{
    new diff = gettime() - timestamp;
    if (diff < 60) format(dest, size, "преди %d сек.", diff);
    else if (diff < 3600) format(dest, size, "преди %d мин.", diff / 60);
    else format(dest, size, "преди %d ч.", diff / 3600);
}

// ============================================================
//  Играч – създаване на репорт
// ============================================================

CMD:report(playerid, params[])
{
    #pragma unused params
    Report_Start(playerid);
    return 1;
}

stock Report_Start(playerid)
{
    if (PlayerData[playerid][pReportBlockedUntil] > gettime()) {
        new msg[128];
        format(msg, sizeof (msg), "Не можеш да пращаш репорти още %d мин. (фалшив репорт).",
            (PlayerData[playerid][pReportBlockedUntil] - gettime()) / 60 + 1);
        SendError(playerid, msg);
        return;
    }

    new openid = Report_GetOpenID(playerid);
    if (openid != 0 && Report_Load(openid)) {
        new body[700], status[32], ago[32];
        Report_StatusName(gRepStatus, status);
        Report_Ago(gRepCreated, ago);
        format(body, sizeof (body),
            C_WHITE "Вече имаш отворен репорт " C_BRAND "#%d" C_WHITE " (%s" C_WHITE ", %s).\n\n" \
            C_GREY "Категория: " C_WHITE "%s\n" \
            C_GREY "Текст: " C_WHITE "%s\n\n" \
            C_GREY "Можеш да добавиш съобщение или да го затвориш, ако проблемът е решен.",
            openid, status, ago, ReportCategories[gRepCategory], gRepText);
        PlayerData[playerid][pReportViewID] = openid;
        ShowPlayerDialog(playerid, DIALOG_REPORT_OPEN, DIALOG_STYLE_MSGBOX, C_BRAND "Репорт " C_WHITE "| Отворен", "%s", "Добави", "Затвори го", body);
        return;
    }

    new body[256];
    body[0] = EOS;
    for (new i = 0; i < sizeof (ReportCategories); i++) {
        strcat(body, C_BRAND "» " C_WHITE);
        strcat(body, ReportCategories[i]);
        if (i < sizeof (ReportCategories) - 1) strcat(body, "\n");
    }
    ShowPlayerDialog(playerid, DIALOG_REPORT_CATEGORY, DIALOG_STYLE_LIST, C_BRAND "Репорт " C_WHITE "| За какво се отнася?", "%s", "Избери", "Отказ", body);
}

stock Report_ShowTargetDialog(playerid)
{
    ShowPlayerDialog(playerid, DIALOG_REPORT_TARGET, DIALOG_STYLE_INPUT, C_BRAND "Репорт " C_WHITE "| Нарушител",
        C_WHITE "Въведи " C_GOLD "ID или името" C_WHITE " на играча, който нарушава правилата.\n\n" \
        C_GREY "Ако не го знаеш, остави празно и натисни \"Напред\".", "Напред", "Назад");
}

stock Report_ShowTextDialog(playerid, const error[])
{
    new body[700];
    format(body, sizeof (body),
        C_WHITE "Категория: " C_BRAND "%s\n\n" \
        C_WHITE "Опиши накратко какво се случва.\n" \
        C_GREY "Колкото по-ясно пишеш, толкова по-бързо ще ти помогнем.",
        ReportCategories[PlayerData[playerid][pReportCategory]]);
    Auth_AppendError(body, sizeof (body), error);
    ShowPlayerDialog(playerid, DIALOG_REPORT_TEXT, DIALOG_STYLE_INPUT, C_BRAND "Репорт " C_WHITE "| Описание", "%s", "Изпрати", "Назад", body);
}

// Позиция, хора наоколо и чат наблизо – само при нарушение.
stock Report_BuildContext(playerid, dest[], size)
{
    new Float:x, Float:y, Float:z, line[128];
    GetPlayerPos(playerid, x, y, z);
    format(dest, size, "Позиция: %.1f, %.1f, %.1f (инт. %d, свят %d)\nНаблизо: ",
        x, y, z, GetPlayerInterior(playerid), GetPlayerVirtualWorld(playerid));

    new count = 0;
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (i == playerid || !IsPlayerConnected(i) || !PlayerData[i][pLogged]) continue;
        if (!IsPlayerNearPlayer(playerid, i, 50.0)) continue;
        new Float:px, Float:py, Float:pz;
        GetPlayerPos(i, px, py, pz);
        if (count > 0) strcat(dest, ", ", size);
        format(line, sizeof (line), "%s[%d] %.0fм", PlayerData[i][pName], i, GetPlayerDistanceFromPoint(playerid, px, py, pz));
        strcat(dest, line, size);
        if (++count >= 8) break;
    }
    if (count == 0) strcat(dest, "никой", size);

    strcat(dest, "\nЧат наблизо (последни 2 мин.):\n", size);
    Chat_GetNearbyLog(playerid, dest, size, 50.0, 120, 6);
}

stock Report_Submit(playerid, const text[])
{
    new now = gettime();
    if (now - PlayerData[playerid][pLastReportTime] < REPORT_COOLDOWN_SECONDS) {
        new err[96];
        format(err, sizeof (err), "Изчакай още %d сек. преди следващия репорт.",
            REPORT_COOLDOWN_SECONDS - (now - PlayerData[playerid][pLastReportTime]));
        Report_ShowTextDialog(playerid, err);
        return;
    }

    new context[1024], eText[300], eContext[2048], eTarget[64], q[2600];
    context[0] = EOS;
    if (PlayerData[playerid][pReportCategory] == REPORT_CAT_VIOLATION) {
        Report_BuildContext(playerid, context, sizeof (context));
    }
    SQL_Escape(text, eText);
    SQL_Escape(context, eContext);
    SQL_Escape(PlayerData[playerid][pReportTarget], eTarget);

    format(q, sizeof (q),
        "INSERT INTO reports (reporter_id, reporter_name, category, target, text, context, status, created_at) " \
        "VALUES (%d, '%s', %d, '%s', '%s', '%s', %d, %d)",
        PlayerData[playerid][pID], PlayerData[playerid][pName], PlayerData[playerid][pReportCategory],
        eTarget, eText, eContext, REPORT_OPEN, now);
    Database_Exec(q);

    new reportid = Report_GetOpenID(playerid);
    if (reportid == 0) {
        SendError(playerid, "Репортът не можа да се запише. Опитай пак.");
        return;
    }
    PlayerData[playerid][pLastReportTime] = now;
    Report_AddMessage(reportid, PlayerData[playerid][pName], false, text);

    SendClientMessage(playerid, COLOR_BRAND, "[Репорт #%d] {FFFFFF}Изпратен е. Администратор ще ти отговори възможно най-скоро.", reportid);

    new msg[200];
    if (PlayerData[playerid][pReportTarget][0] != EOS) {
        format(msg, sizeof (msg), "[Репорт #%d] %s от %s [%d] срещу %s: %s  (/ar %d)",
            reportid, ReportCategories[PlayerData[playerid][pReportCategory]], PlayerData[playerid][pName], playerid,
            PlayerData[playerid][pReportTarget], text, reportid);
    } else {
        format(msg, sizeof (msg), "[Репорт #%d] %s от %s [%d]: %s  (/ar %d)",
            reportid, ReportCategories[PlayerData[playerid][pReportCategory]], PlayerData[playerid][pName], playerid, text, reportid);
    }
    Admin_Broadcast(msg);
}

// ============================================================
//  Админ – опашка и действия
// ============================================================

stock Report_ShowList(playerid)
{
    new q[256];
    format(q, sizeof (q),
        "SELECT id, reporter_name, category, status, admin_name, created_at FROM reports WHERE status IN (%d, %d) ORDER BY id LIMIT %d",
        REPORT_OPEN, REPORT_CLAIMED, REPORT_LIST_MAX);
    new DBResult:result = Database_Query(q);

    new body[2048], line[160], count = 0;
    body[0] = EOS;
    if (result != DBResult:0) {
        if (DB_GetRowCount(result) > 0) {
            new id, cat, status, created, name[MAX_PLAYER_NAME + 1], admin[MAX_PLAYER_NAME + 1], ago[32];
            do {
                id = DB_GetFieldIntByName(result, "id");
                cat = DB_GetFieldIntByName(result, "category");
                status = DB_GetFieldIntByName(result, "status");
                created = DB_GetFieldIntByName(result, "created_at");
                DB_GetFieldStringByName(result, "reporter_name", name);
                DB_GetFieldStringByName(result, "admin_name", admin);
                Report_Ago(created, ago);
                if (cat < 0 || cat >= sizeof (ReportCategories)) cat = 0;

                if (status == REPORT_OPEN) {
                    format(line, sizeof (line), C_GOLD "#%d " C_WHITE "%s - %s " C_GREY "(%s)\n", id, ReportCategories[cat], name, ago);
                } else {
                    format(line, sizeof (line), C_BLUE "#%d " C_WHITE "%s - %s " C_GREY "(поет от %s)\n", id, ReportCategories[cat], name, admin);
                }
                strcat(body, line);
                PlayerData[playerid][pReportList][count] = id;
                count++;
            } while (count < REPORT_LIST_MAX && DB_SelectNextRow(result));
        }
        DB_FreeResultSet(result);
    }
    PlayerData[playerid][pReportListCount] = count;

    if (count == 0) {
        ShowPlayerDialog(playerid, DIALOG_INFO, DIALOG_STYLE_MSGBOX, C_RED "Репорти", C_GREEN "Няма отворени репорти. Браво!", "Затвори", "");
        return;
    }
    new title[64];
    format(title, sizeof (title), C_RED "Репорти " C_WHITE "| %d отворени", count);
    ShowPlayerDialog(playerid, DIALOG_REPORTS_LIST, DIALOG_STYLE_LIST, title, "%s", "Отвори", "Затвори", body);
}

stock Report_ShowView(playerid, reportid)
{
    if (!Report_Load(reportid)) {
        SendError(playerid, "Няма такъв репорт.");
        return;
    }
    PlayerData[playerid][pReportViewID] = reportid;

    new body[3000], line[400], status[32], ago[32];
    Report_StatusName(gRepStatus, status);
    Report_Ago(gRepCreated, ago);
    new online = Report_FindOnline(gRepReporterID);
    if (gRepCategory < 0 || gRepCategory >= sizeof (ReportCategories)) gRepCategory = 0;

    new onlineText[16];
    if (online != INVALID_PLAYER_ID) format(onlineText, sizeof (onlineText), "(онлайн)");
    else format(onlineText, sizeof (onlineText), "(офлайн)");
    format(body, sizeof (body),
        C_GREY "Статус: %s" C_GREY "  |  " C_WHITE "%s\n" \
        C_GREY "От: " C_WHITE "%s %s\n" \
        C_GREY "Категория: " C_WHITE "%s\n",
        status, ago, gRepReporterName, onlineText, ReportCategories[gRepCategory]);
    if (gRepTarget[0] != EOS) {
        format(line, sizeof (line), C_GREY "Срещу: " C_RED "%s\n", gRepTarget);
        strcat(body, line);
    }
    if (gRepAdmin[0] != EOS) {
        format(line, sizeof (line), C_GREY "Админ: " C_WHITE "%s\n", gRepAdmin);
        strcat(body, line);
    }

    // Разговор
    strcat(body, "\n" C_BRAND "Съобщения:\n");
    new q[160];
    format(q, sizeof (q), "SELECT author, is_admin, text FROM report_messages WHERE report_id = %d ORDER BY id LIMIT 15", reportid);
    new DBResult:result = Database_Query(q);
    if (result != DBResult:0) {
        if (DB_GetRowCount(result) > 0) {
            new author[MAX_PLAYER_NAME + 1], text[160], isAdmin;
            do {
                DB_GetFieldStringByName(result, "author", author);
                DB_GetFieldStringByName(result, "text", text);
                isAdmin = DB_GetFieldIntByName(result, "is_admin");
                if (isAdmin) format(line, sizeof (line), C_RED "%s: " C_WHITE "%s\n", author, text);
                else format(line, sizeof (line), C_GOLD "%s: " C_WHITE "%s\n", author, text);
                strcat(body, line);
            } while (DB_SelectNextRow(result));
        }
        DB_FreeResultSet(result);
    }

    if (gRepContext[0] != EOS) {
        strcat(body, "\n" C_BRAND "Автоматичен контекст:\n" C_GREY);
        strcat(body, gRepContext);
    }

    new title[64];
    format(title, sizeof (title), C_RED "Репорт #%d", reportid);
    ShowPlayerDialog(playerid, DIALOG_REPORT_VIEW, DIALOG_STYLE_MSGBOX, title, "%s", "Действия", "Назад", body);
}

stock Report_ShowActions(playerid)
{
    ShowPlayerDialog(playerid, DIALOG_REPORT_ACTIONS, DIALOG_STYLE_LIST, C_RED "Репорт " C_WHITE "| Действия",
        C_BLUE "» " C_WHITE "Поеми репорта\n" \
        C_BLUE "» " C_WHITE "Отговори на играча\n" \
        C_BLUE "» " C_WHITE "Телепорт до играча\n" \
        C_GREEN "» " C_WHITE "Затвори (решен)\n" \
        C_RED "» " C_WHITE "Фалшив репорт", "Избери", "Назад");
}

stock bool:Report_Claim(playerid, reportid)
{
    if (!Report_Load(reportid)) {
        SendError(playerid, "Няма такъв репорт.");
        return false;
    }
    if (gRepStatus == REPORT_CLOSED || gRepStatus == REPORT_FALSE) {
        SendError(playerid, "Този репорт вече е затворен.");
        return false;
    }
    if (gRepStatus == REPORT_CLAIMED) {
        if (!strcmp(gRepAdmin, PlayerData[playerid][pName], true)) return true; // вече е мой
        new err[96];
        format(err, sizeof (err), "Репортът вече е поет от %s.", gRepAdmin);
        SendError(playerid, err);
        return false;
    }

    new q[256];
    format(q, sizeof (q), "UPDATE reports SET status = %d, admin_name = '%s', claimed_at = %d WHERE id = %d AND status = %d",
        REPORT_CLAIMED, PlayerData[playerid][pName], gettime(), reportid, REPORT_OPEN);
    Database_Exec(q);

    new msg[128];
    format(msg, sizeof (msg), "[Репорт #%d] %s пое репорта на %s.", reportid, PlayerData[playerid][pName], gRepReporterName);
    Admin_Broadcast(msg);

    new reporter = Report_FindOnline(gRepReporterID);
    if (reporter != INVALID_PLAYER_ID) {
        SendClientMessage(reporter, COLOR_BRAND, "[Репорт #%d] {FFFFFF}Администратор {FF6347}%s{FFFFFF} пое репорта ти.", reportid, PlayerData[playerid][pName]);
    }
    return true;
}

stock Report_Reply(playerid, reportid, const text[])
{
    if (!Report_Claim(playerid, reportid)) return;

    Report_AddMessage(reportid, PlayerData[playerid][pName], true, text);

    new reporter = Report_FindOnline(gRepReporterID);
    if (reporter != INVALID_PLAYER_ID) {
        SendClientMessage(reporter, COLOR_ADMIN, "[Репорт #%d] Админ %s: {FFFFFF}%s", reportid, PlayerData[playerid][pName], text);
        SendClientMessage(reporter, COLOR_GREY, "Отговори с /report.");
    } else {
        SendInfo(playerid, "Играчът е офлайн – ще види отговора в историята на репорта.");
    }
    SendClientMessage(playerid, COLOR_ADMIN, "[Репорт #%d] Ти до %s: {FFFFFF}%s", reportid, gRepReporterName, text);
}

stock Report_Close(playerid, reportid, bool:isFalse)
{
    if (!Report_Load(reportid)) {
        SendError(playerid, "Няма такъв репорт.");
        return;
    }
    if (gRepStatus == REPORT_CLOSED || gRepStatus == REPORT_FALSE) {
        SendError(playerid, "Този репорт вече е затворен.");
        return;
    }
    if (gRepStatus == REPORT_CLAIMED && strcmp(gRepAdmin, PlayerData[playerid][pName], true) != 0
        && PlayerData[playerid][pAdmin] < ADMIN_ADMIN) {
        new err[96];
        format(err, sizeof (err), "Репортът е поет от %s. Само той (или админ 3+) може да го затвори.", gRepAdmin);
        SendError(playerid, err);
        return;
    }

    new q[256], now = gettime();
    format(q, sizeof (q), "UPDATE reports SET status = %d, admin_name = '%s', closed_at = %d WHERE id = %d",
        isFalse ? REPORT_FALSE : REPORT_CLOSED, PlayerData[playerid][pName], now, reportid);
    Database_Exec(q);

    new reporter = Report_FindOnline(gRepReporterID), msg[160];

    if (isFalse) {
        new until = now + REPORT_FALSE_BLOCK_MINUTES * 60;
        format(q, sizeof (q), "UPDATE accounts SET report_block_until = %d WHERE id = %d", until, gRepReporterID);
        Database_Exec(q);
        if (reporter != INVALID_PLAYER_ID) {
            PlayerData[reporter][pReportBlockedUntil] = until;
            SendClientMessage(reporter, COLOR_RED, "[Репорт #%d] Репортът ти е отбелязан като фалшив. Не можеш да пращаш репорти %d мин.",
                reportid, REPORT_FALSE_BLOCK_MINUTES);
        }
        format(msg, sizeof (msg), "[Репорт #%d] %s го отбеляза като ФАЛШИВ (%s е блокиран за %d мин.).",
            reportid, PlayerData[playerid][pName], gRepReporterName, REPORT_FALSE_BLOCK_MINUTES);
        Log_Add("report_false", PlayerData[playerid][pName], gRepReporterName, "");
    } else {
        if (reporter != INVALID_PLAYER_ID) {
            PlayerData[reporter][pRateReportID] = reportid;
            ShowPlayerDialog(reporter, DIALOG_REPORT_RATE, DIALOG_STYLE_LIST, C_BRAND "Оцени помощта",
                C_GOLD "***** " C_WHITE "Отлично\n" \
                C_GOLD "**** " C_WHITE "Много добре\n" \
                C_GOLD "*** " C_WHITE "Добре\n" \
                C_GOLD "** " C_WHITE "Слабо\n" \
                C_GOLD "* " C_WHITE "Не ми помогна", "Оцени", "Пропусни");
            SendClientMessage(reporter, COLOR_BRAND, "[Репорт #%d] {FFFFFF}Затворен от %s.", reportid, PlayerData[playerid][pName]);
        }
        format(msg, sizeof (msg), "[Репорт #%d] %s затвори репорта на %s.", reportid, PlayerData[playerid][pName], gRepReporterName);
    }
    Admin_Broadcast(msg);
}

// ============================================================
//  Отговори на диалозите
// ============================================================

stock Report_OnDialogResponse(playerid, dialogid, response, listitem, const inputtext[])
{
    switch (dialogid) {
        case DIALOG_REPORT_OPEN: {
            new reportid = PlayerData[playerid][pReportViewID];
            if (response) {
                ShowPlayerDialog(playerid, DIALOG_REPORT_ADD, DIALOG_STYLE_INPUT, C_BRAND "Репорт " C_WHITE "| Ново съобщение",
                    C_WHITE "Напиши съобщение към администраторите:", "Изпрати", "Отказ");
            } else {
                // Играчът сам затваря репорта си
                new q[160];
                format(q, sizeof (q), "UPDATE reports SET status = %d, closed_at = %d WHERE id = %d AND reporter_id = %d",
                    REPORT_CLOSED, gettime(), reportid, PlayerData[playerid][pID]);
                Database_Exec(q);
                SendInfo(playerid, "Репортът ти е затворен.");
                new msg[96];
                format(msg, sizeof (msg), "[Репорт #%d] %s затвори сам репорта си.", reportid, PlayerData[playerid][pName]);
                Admin_Broadcast(msg);
            }
            return 1;
        }

        case DIALOG_REPORT_ADD: {
            if (!response || strlen(inputtext) < 2) return 1;
            new reportid = PlayerData[playerid][pReportViewID];
            Report_AddMessage(reportid, PlayerData[playerid][pName], false, inputtext);
            SendClientMessage(playerid, COLOR_BRAND, "[Репорт #%d] {FFFFFF}Съобщението е добавено.", reportid);
            new msg[200];
            format(msg, sizeof (msg), "[Репорт #%d] %s добави: %s", reportid, PlayerData[playerid][pName], inputtext);
            Admin_Broadcast(msg);
            return 1;
        }

        case DIALOG_REPORT_CATEGORY: {
            if (!response) return 1;
            if (listitem < 0 || listitem >= sizeof (ReportCategories)) listitem = 0;
            PlayerData[playerid][pReportCategory] = listitem;
            PlayerData[playerid][pReportTarget][0] = EOS;
            if (listitem == REPORT_CAT_VIOLATION) Report_ShowTargetDialog(playerid);
            else Report_ShowTextDialog(playerid, "");
            return 1;
        }

        case DIALOG_REPORT_TARGET: {
            if (!response) {
                Report_Start(playerid);
                return 1;
            }
            if (inputtext[0] != EOS) {
                new target = FindPlayer(inputtext);
                if (target != INVALID_PLAYER_ID) {
                    format(PlayerData[playerid][pReportTarget], MAX_PLAYER_NAME + 1, "%s", PlayerData[target][pName]);
                } else {
                    // Пазим каквото е написал – може да е излязъл
                    strmid(PlayerData[playerid][pReportTarget], inputtext, 0, MAX_PLAYER_NAME, MAX_PLAYER_NAME + 1);
                }
            }
            Report_ShowTextDialog(playerid, "");
            return 1;
        }

        case DIALOG_REPORT_TEXT: {
            if (!response) {
                Report_Start(playerid);
                return 1;
            }
            if (strlen(inputtext) < REPORT_MIN_LENGTH) {
                Report_ShowTextDialog(playerid, "Описанието е твърде кратко.");
                return 1;
            }
            Report_Submit(playerid, inputtext);
            return 1;
        }

        case DIALOG_REPORT_RATE: {
            new reportid = PlayerData[playerid][pRateReportID];
            PlayerData[playerid][pRateReportID] = 0;
            if (!response || reportid == 0) return 1;
            new stars = 5 - listitem;
            if (stars < 1 || stars > 5) return 1;
            new q[160];
            format(q, sizeof (q), "UPDATE reports SET rating = %d WHERE id = %d AND reporter_id = %d", stars, reportid, PlayerData[playerid][pID]);
            Database_Exec(q);
            SendClientMessage(playerid, COLOR_BRAND, "[HOTFIX] {FFFFFF}Благодарим за оценката (%d/5)!", stars);
            return 1;
        }

        case DIALOG_REPORTS_LIST: {
            if (!response) return 1;
            if (listitem < 0 || listitem >= PlayerData[playerid][pReportListCount]) return 1;
            Report_ShowView(playerid, PlayerData[playerid][pReportList][listitem]);
            return 1;
        }

        case DIALOG_REPORT_VIEW: {
            if (!response) {
                Report_ShowList(playerid);
                return 1;
            }
            Report_ShowActions(playerid);
            return 1;
        }

        case DIALOG_REPORT_ACTIONS: {
            new reportid = PlayerData[playerid][pReportViewID];
            if (!response) {
                Report_ShowView(playerid, reportid);
                return 1;
            }
            switch (listitem) {
                case 0: {
                    if (Report_Claim(playerid, reportid)) SendClientMessage(playerid, COLOR_ADMIN, "[Репорт #%d] Пое репорта.", reportid);
                }
                case 1: {
                    ShowPlayerDialog(playerid, DIALOG_REPORT_REPLY, DIALOG_STYLE_INPUT, C_RED "Репорт " C_WHITE "| Отговор",
                        C_WHITE "Напиши отговор до играча:", "Изпрати", "Назад");
                }
                case 2: {
                    if (!Report_Load(reportid)) return 1;
                    new target = Report_FindOnline(gRepReporterID);
                    if (target == INVALID_PLAYER_ID) {
                        SendError(playerid, "Играчът е офлайн.");
                    } else {
                        new Float:x, Float:y, Float:z;
                        GetPlayerPos(target, x, y, z);
                        SetPlayerInterior(playerid, GetPlayerInterior(target));
                        SetPlayerVirtualWorld(playerid, GetPlayerVirtualWorld(target));
                        SetPlayerPos(playerid, x + 1.0, y, z);
                    }
                }
                case 3: Report_Close(playerid, reportid, false);
                case 4: Report_Close(playerid, reportid, true);
            }
            return 1;
        }

        case DIALOG_REPORT_REPLY: {
            new reportid = PlayerData[playerid][pReportViewID];
            if (!response) {
                Report_ShowActions(playerid);
                return 1;
            }
            if (strlen(inputtext) < 2) return 1;
            Report_Reply(playerid, reportid, inputtext);
            return 1;
        }
    }
    return 0;
}

// ============================================================
//  Админ команди
// ============================================================

CMD:reports(playerid, params[])
{
    #pragma unused params
    if (!Admin_Check(playerid, ADMIN_HELPER)) return 1;
    Report_ShowList(playerid);
    return 1;
}

CMD:ar(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_HELPER)) return 1;
    new reportid;
    if (!GetParamInt(params, 0, reportid)) return SendUsage(playerid, "/ar [номер на репорта]");
    if (Report_Claim(playerid, reportid)) {
        PlayerData[playerid][pReportViewID] = reportid;
        Report_ShowView(playerid, reportid);
    }
    return 1;
}

CMD:rr(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_HELPER)) return 1;
    new reportid, text[128];
    if (!GetParamInt(params, 0, reportid) || !GetParamRest(params, 1, text)) return SendUsage(playerid, "/rr [номер] [отговор]");
    Report_Reply(playerid, reportid, text);
    return 1;
}

CMD:cr(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_HELPER)) return 1;
    new reportid;
    if (!GetParamInt(params, 0, reportid)) return SendUsage(playerid, "/cr [номер на репорта]");
    Report_Close(playerid, reportid, false);
    return 1;
}

CMD:fr(playerid, params[])
{
    if (!Admin_Check(playerid, ADMIN_HELPER)) return 1;
    new reportid;
    if (!GetParamInt(params, 0, reportid)) return SendUsage(playerid, "/fr [номер на репорта] - фалшив репорт");
    Report_Close(playerid, reportid, true);
    return 1;
}

CMD:rstats(playerid, params[])
{
    #pragma unused params
    if (!Admin_Check(playerid, ADMIN_HEAD)) return 1;

    new q[384];
    format(q, sizeof (q),
        "SELECT admin_name, COUNT(*) AS total, AVG(NULLIF(rating, 0)) AS avg_rating, SUM(rating > 0) AS rated " \
        "FROM reports WHERE status = %d AND admin_name != '' AND closed_at > %d GROUP BY admin_name ORDER BY total DESC LIMIT 15",
        REPORT_CLOSED, gettime() - 30 * 86400);
    new DBResult:result = Database_Query(q);

    new body[2048], line[160];
    format(body, sizeof (body), C_GREY "Затворени репорти за последните 30 дни:\n\n");
    if (result != DBResult:0) {
        if (DB_GetRowCount(result) > 0) {
            new name[MAX_PLAYER_NAME + 1], total, rated, Float:avg;
            do {
                DB_GetFieldStringByName(result, "admin_name", name);
                total = DB_GetFieldIntByName(result, "total");
                rated = DB_GetFieldIntByName(result, "rated");
                avg = DB_GetFieldFloatByName(result, "avg_rating");
                if (rated > 0) {
                    format(line, sizeof (line), C_WHITE "%s: " C_GOLD "%d" C_WHITE " репорта, оценка " C_GOLD "%.1f/5" C_GREY " (%d оценки)\n", name, total, avg, rated);
                } else {
                    format(line, sizeof (line), C_WHITE "%s: " C_GOLD "%d" C_WHITE " репорта, " C_GREY "без оценки\n", name, total);
                }
                strcat(body, line);
            } while (DB_SelectNextRow(result));
        } else {
            strcat(body, C_GREY "Още няма данни.");
        }
        DB_FreeResultSet(result);
    }
    ShowPlayerDialog(playerid, DIALOG_INFO, DIALOG_STYLE_MSGBOX, C_RED "Статистика на админите", "%s", "Затвори", "", body);
    return 1;
}

// Напомняне на админите за неотговорени репорти.
stock Report_OnTick(tick)
{
    if (tick % REPORT_REMIND_SECONDS != 0) return;

    new q[128], count = 0;
    format(q, sizeof (q), "SELECT COUNT(*) AS c FROM reports WHERE status = %d", REPORT_OPEN);
    new DBResult:result = Database_Query(q);
    if (result != DBResult:0) {
        count = DB_GetFieldIntByName(result, "c");
        DB_FreeResultSet(result);
    }
    if (count > 0) {
        new msg[96];
        format(msg, sizeof (msg), "[Репорти] Има %d неотговорени репорта. Виж /reports.", count);
        Admin_Broadcast(msg);
    }
}
