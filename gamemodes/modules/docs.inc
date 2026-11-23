// Документи: лична карта, медицинско свидетелство, шофьорска книжка.
//
// Пазят се в акаунта (номер, дати, кръвна група). Показват се като карта (TextDraw) в средата
// на екрана - на себе си или на играч наблизо: Меню > Документи. Картата изчезва след 15 сек.
// Текстът в картите е на латиница (шрифтовете на SA нямат кирилица).

#define DOC_ID          0
#define DOC_MEDICAL     1
#define DOC_LICENSE     2
#define DOC_COUNT       3
#define DOC_SHOW_MS     15000
#define DOC_MAX_TD      40

static const DocTitles[DOC_COUNT][] = {"IDENTITY CARD", "MEDICAL CERTIFICATE", "DRIVING LICENCE"};
static const DocNamesBG[DOC_COUNT][] = {"Лична карта", "Медицинско свидетелство", "Шофьорска книжка"};
static const DocThemes[DOC_COUNT] = {0x1F4E8CFF, 0x2E7D4FFF, 0xB03A2EFF};
static const DocBloodTypes[][] = {"0+", "0-", "A+", "A-", "B+", "B-", "AB+", "AB-"};

static DocIdNumber[MAX_PLAYERS];
static DocIdIssued[MAX_PLAYERS];
static DocBirth[MAX_PLAYERS];              // ггггммдд
static DocMedIssued[MAX_PLAYERS];
static DocBlood[MAX_PLAYERS];

static PlayerText:DocTD[MAX_PLAYERS][DOC_MAX_TD];
static DocTDCount[MAX_PLAYERS];
static DocHideTimer[MAX_PLAYERS];
static DocPick[MAX_PLAYERS];
static PlayerText:DocCloseTD[MAX_PLAYERS] = {PlayerText:INVALID_TEXT_DRAW, ...};
static bool:DocOpen[MAX_PLAYERS];

// ---------- Данни ----------

stock Docs_Init()
{
    Database_AddColumn("accounts", "id_number", "INTEGER NOT NULL DEFAULT 0");
    Database_AddColumn("accounts", "id_issued", "INTEGER NOT NULL DEFAULT 0");
    Database_AddColumn("accounts", "birth", "INTEGER NOT NULL DEFAULT 0");
    Database_AddColumn("accounts", "med_issued", "INTEGER NOT NULL DEFAULT 0");
    Database_AddColumn("accounts", "blood", "INTEGER NOT NULL DEFAULT 0");
}

stock Docs_OnPlayerConnect(playerid)
{
    DocIdNumber[playerid] = 0;
    DocIdIssued[playerid] = 0;
    DocBirth[playerid] = 0;
    DocMedIssued[playerid] = 0;
    DocBlood[playerid] = 0;
    DocTDCount[playerid] = 0;
    DocHideTimer[playerid] = 0;
    DocOpen[playerid] = false;
}

stock Docs_OnPlayerDisconnect(playerid)
{
    if (DocHideTimer[playerid] != 0) KillTimer(DocHideTimer[playerid]);
    DocHideTimer[playerid] = 0;
}

stock Docs_OnLogin(playerid)
{
    new q[128];
    format(q, sizeof (q), "SELECT id_number, id_issued, birth, med_issued, blood FROM accounts WHERE id = %d", PlayerData[playerid][pID]);
    new DBResult:r = Database_Query(q);
    if (r == DBResult:0) return;
    if (DB_GetRowCount(r) > 0) {
        DocIdNumber[playerid] = DB_GetFieldIntByName(r, "id_number");
        DocIdIssued[playerid] = DB_GetFieldIntByName(r, "id_issued");
        DocBirth[playerid] = DB_GetFieldIntByName(r, "birth");
        DocMedIssued[playerid] = DB_GetFieldIntByName(r, "med_issued");
        DocBlood[playerid] = DB_GetFieldIntByName(r, "blood");
        if (DocBlood[playerid] < 0 || DocBlood[playerid] >= sizeof (DocBloodTypes)) DocBlood[playerid] = 0;
    }
    DB_FreeResultSet(r);
}

stock Docs_HasId(playerid)
{
    return (DocIdNumber[playerid] != 0);
}

stock Docs_HasMedical(playerid)
{
    return (DocMedIssued[playerid] != 0);
}

// Дава лична карта (номер, дата на раждане) - 1 ако е нова
stock Docs_IssueId(playerid)
{
    if (DocIdNumber[playerid] != 0) return 0;
    DocIdNumber[playerid] = 7000000 + PlayerData[playerid][pID] * 13 + random(10);
    DocIdIssued[playerid] = gettime();
    DocBirth[playerid] = (1975 + random(28)) * 10000 + (1 + random(12)) * 100 + (1 + random(28));
    new q[160];
    format(q, sizeof (q), "UPDATE accounts SET id_number = %d, id_issued = %d, birth = %d WHERE id = %d",
        DocIdNumber[playerid], DocIdIssued[playerid], DocBirth[playerid], PlayerData[playerid][pID]);
    Database_Exec(q);
    return 1;
}

stock Docs_IssueMedical(playerid)
{
    if (DocMedIssued[playerid] != 0) return 0;
    DocMedIssued[playerid] = gettime();
    DocBlood[playerid] = random(sizeof (DocBloodTypes));
    new q[128];
    format(q, sizeof (q), "UPDATE accounts SET med_issued = %d, blood = %d WHERE id = %d", DocMedIssued[playerid], DocBlood[playerid], PlayerData[playerid][pID]);
    Database_Exec(q);
    return 1;
}

// Дата от unix време: "дд.мм.гггг"
stock Docs_FormatDate(ts, dest[], size = sizeof (dest))
{
    new z = ts / 86400 + 719468;
    new era = z / 146097;
    new doe = z - era * 146097;
    new yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    new y = yoe + era * 400;
    new doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    new mp = (5 * doy + 2) / 153;
    new d = doy - (153 * mp + 2) / 5 + 1;
    new m = (mp < 10) ? (mp + 3) : (mp - 9);
    if (m <= 2) y++;
    format(dest, size, "%02d.%02d.%d", d, m, y);
}

static stock Docs_FormatBirth(birth, dest[], size = sizeof (dest))
{
    format(dest, size, "%02d.%02d.%d", birth % 100, (birth / 100) % 100, birth / 10000);
}

// ---------- Карта (TextDraw) ----------

static stock Docs_Hide(playerid)
{
    for (new i = 0; i < DocTDCount[playerid]; i++) PlayerTextDrawDestroy(playerid, DocTD[playerid][i]);
    DocTDCount[playerid] = 0;
    DocCloseTD[playerid] = PlayerText:INVALID_TEXT_DRAW;
    if (DocOpen[playerid]) {
        DocOpen[playerid] = false;
        CancelSelectTextDraw(playerid);
    }
    if (DocHideTimer[playerid] != 0) KillTimer(DocHideTimer[playerid]);
    DocHideTimer[playerid] = 0;
}

forward Docs_HideTimer(playerid);
public Docs_HideTimer(playerid)
{
    DocHideTimer[playerid] = 0;
    Docs_Hide(playerid);
    return 1;
}

// Правоъгълник с цвят (спрайт "бяло", оцветен)
static stock Docs_Rect(playerid, Float:x, Float:y, Float:w, Float:h, color)
{
    if (DocTDCount[playerid] >= DOC_MAX_TD) return;
    new PlayerText:t = CreatePlayerTextDraw(playerid, x, y, "LD_SPAC:white");
    PlayerTextDrawFont(playerid, t, TEXT_DRAW_FONT_SPRITE_DRAW);
    PlayerTextDrawTextSize(playerid, t, w, h);
    PlayerTextDrawColour(playerid, t, color);
    PlayerTextDrawShow(playerid, t);
    DocTD[playerid][DocTDCount[playerid]++] = t;
}

static stock Docs_Text(playerid, Float:x, Float:y, const text[], Float:sx, Float:sy, color, TEXT_DRAW_FONT:font = TEXT_DRAW_FONT_1, TEXT_DRAW_ALIGN:align = TEXT_DRAW_ALIGN_LEFT)
{
    if (DocTDCount[playerid] >= DOC_MAX_TD) return;
    new PlayerText:t = CreatePlayerTextDraw(playerid, x, y, text);
    PlayerTextDrawFont(playerid, t, font);
    PlayerTextDrawLetterSize(playerid, t, sx, sy);
    PlayerTextDrawColour(playerid, t, color);
    PlayerTextDrawSetShadow(playerid, t, 0);
    PlayerTextDrawSetOutline(playerid, t, 0);
    PlayerTextDrawAlignment(playerid, t, align);
    PlayerTextDrawSetProportional(playerid, t, true);
    PlayerTextDrawShow(playerid, t);
    DocTD[playerid][DocTDCount[playerid]++] = t;
}

// Етикет и стойност (поле от картата)
static stock Docs_Field(playerid, Float:x, Float:y, const label[], const value[])
{
    Docs_Text(playerid, x, y, label, 0.13, 0.62, 0x857D70FF, TEXT_DRAW_FONT_2);
    Docs_Text(playerid, x, y + 6.0, value, 0.19, 0.92, 0x22201CFF, TEXT_DRAW_FONT_1);
}

// Показва документа doc на owner на viewer
stock Docs_Show(viewerid, ownerid, doc)
{
    Docs_Hide(viewerid);
    new Float:X0 = 200.0, Float:Y0 = 135.0, Float:W = 240.0, Float:H = 150.0;
    new theme = DocThemes[doc], text[48], date[16];

    // Сянка, хартия, лента горе
    Docs_Rect(viewerid, X0 + 3.0, Y0 + 3.0, W, H, 0x00000077);
    Docs_Rect(viewerid, X0, Y0, W, H, 0xF4F0E6FF);
    Docs_Rect(viewerid, X0, Y0, W, 26.0, theme);
    Docs_Rect(viewerid, X0, Y0 + 26.0, W, 2.0, 0xF5A623FF);
    Docs_Text(viewerid, X0 + 8.0, Y0 + 3.0, "STATE OF SAN ANDREAS", 0.15, 0.7, 0xFFFFFFCC, TEXT_DRAW_FONT_2);
    Docs_Text(viewerid, X0 + 8.0, Y0 + 11.0, DocTitles[doc], 0.24, 1.15, 0xFFFFFFFF, TEXT_DRAW_FONT_2);
    // Емблема вдясно: кръг от букви
    Docs_Text(viewerid, X0 + W - 8.0, Y0 + 6.0, "CS", 0.32, 1.5, 0xF5A623FF, TEXT_DRAW_FONT_2, TEXT_DRAW_ALIGN_RIGHT);

    // Снимка (моделът на скина)
    Docs_Rect(viewerid, X0 + 9.0, Y0 + 35.0, 60.0, 76.0, 0xBFB8AAFF);
    if (DocTDCount[viewerid] < DOC_MAX_TD) {
        new PlayerText:p = CreatePlayerTextDraw(viewerid, X0 + 10.0, Y0 + 36.0, "_");
        PlayerTextDrawFont(viewerid, p, TEXT_DRAW_FONT_MODEL_PREVIEW);
        PlayerTextDrawUseBox(viewerid, p, true);
        PlayerTextDrawBoxColour(viewerid, p, 0x00000000);
        PlayerTextDrawBackgroundColour(viewerid, p, 0xD6DEE6FF);
        PlayerTextDrawTextSize(viewerid, p, 58.0, 74.0);
        PlayerTextDrawSetPreviewModel(viewerid, p, GetPlayerSkin(ownerid));
        PlayerTextDrawSetPreviewRot(viewerid, p, -8.0, 0.0, 0.0, 0.75);
        PlayerTextDrawShow(viewerid, p);
        DocTD[viewerid][DocTDCount[viewerid]++] = p;
    }

    new Float:fx = X0 + 78.0, Float:fy = Y0 + 34.0;
    Docs_Field(viewerid, fx, fy, "NAME", PlayerData[ownerid][pName]);

    switch (doc) {
        case DOC_ID: {
            Docs_FormatBirth(DocBirth[ownerid], date);
            Docs_Field(viewerid, fx, fy + 19.0, "SEX", (PlayerData[ownerid][pGender] == 1) ? ("F") : ("M"));
            Docs_Field(viewerid, fx + 60.0, fy + 19.0, "DATE OF BIRTH", date);
            Docs_Field(viewerid, fx, fy + 38.0, "CITY", CityNames[PlayerData[ownerid][pGarageCity]]);
            Docs_FormatDate(DocIdIssued[ownerid], date);
            Docs_Field(viewerid, fx + 90.0, fy + 38.0, "ISSUED", date);
            format(text, sizeof (text), "%d", DocIdNumber[ownerid]);
            Docs_Field(viewerid, fx, fy + 57.0, "ID NUMBER", text);
        }
        case DOC_MEDICAL: {
            Docs_Field(viewerid, fx, fy + 19.0, "BLOOD TYPE", DocBloodTypes[DocBlood[ownerid]]);
            Docs_Field(viewerid, fx + 70.0, fy + 19.0, "STATUS", "FIT TO DRIVE");
            Docs_FormatDate(DocMedIssued[ownerid], date);
            Docs_Field(viewerid, fx, fy + 38.0, "ISSUED", date);
            Docs_FormatDate(DocMedIssued[ownerid] + 365 * 86400, date);
            Docs_Field(viewerid, fx + 70.0, fy + 38.0, "VALID UNTIL", date);
            Docs_Field(viewerid, fx, fy + 57.0, "DOCTOR", "Dr. A. Petrova, All Saints");
        }
        default: {
            format(text, sizeof (text), "%d", DocIdNumber[ownerid]);
            Docs_Field(viewerid, fx, fy + 19.0, "ID NUMBER", text);
            Docs_FormatBirth(DocBirth[ownerid], date);
            Docs_Field(viewerid, fx + 80.0, fy + 19.0, "DATE OF BIRTH", date);
            Docs_Text(viewerid, fx, fy + 38.0, "CATEGORIES", 0.13, 0.62, 0x857D70FF, TEXT_DRAW_FONT_2);
            // Кутийки с категориите: оцветени = има я
            new bits = License_GetBits(ownerid);
            for (new c = 0; c < MAX_LICENSES; c++) {
                new Float:bx = fx + c * 36.0, bool:has = (bits & LIC_BIT(c)) != 0;
                Docs_Rect(viewerid, bx, fy + 46.0, 32.0, 14.0, has ? theme : 0xD9D3C7FF);
                Docs_Text(viewerid, bx + 16.0, fy + 48.5, LicenseNames[c], 0.2, 0.95, has ? 0xFFFFFFFF : 0x9C9488FF, TEXT_DRAW_FONT_2, TEXT_DRAW_ALIGN_CENTRE);
            }
        }
    }

    // Долна лента
    Docs_Rect(viewerid, X0, Y0 + H - 16.0, W, 16.0, 0xE6E0D2FF);
    Docs_Text(viewerid, X0 + 8.0, Y0 + H - 12.5, "CARGO STATE  -  TRUCK SIMULATOR", 0.13, 0.62, 0x857D70FF, TEXT_DRAW_FONT_2);
    Docs_Text(viewerid, X0 + W - 8.0, Y0 + H - 13.5, PlayerData[ownerid][pName], 0.18, 0.85, theme, TEXT_DRAW_FONT_0, TEXT_DRAW_ALIGN_RIGHT);

    // Бутон X (горе вдясно) - затваря; ESC също затваря (виж Docs_OnClickTD)
    if (DocTDCount[viewerid] < DOC_MAX_TD) {
        new PlayerText:x = CreatePlayerTextDraw(viewerid, X0 + W - 7.0, Y0 - 13.0, "X");
        PlayerTextDrawFont(viewerid, x, TEXT_DRAW_FONT_2);
        PlayerTextDrawLetterSize(viewerid, x, 0.3, 1.2);
        PlayerTextDrawAlignment(viewerid, x, TEXT_DRAW_ALIGN_CENTRE);
        PlayerTextDrawColour(viewerid, x, 0xFFFFFFFF);
        PlayerTextDrawUseBox(viewerid, x, true);
        PlayerTextDrawBoxColour(viewerid, x, 0xB03A2EFF);
        PlayerTextDrawTextSize(viewerid, x, 10.0, 12.0);
        PlayerTextDrawSetShadow(viewerid, x, 0);
        PlayerTextDrawSetSelectable(viewerid, x, true);
        PlayerTextDrawShow(viewerid, x);
        DocTD[viewerid][DocTDCount[viewerid]++] = x;
        DocCloseTD[viewerid] = x;
    }
    DocOpen[viewerid] = true;
    SelectTextDraw(viewerid, 0xF5A623FF);
    DocHideTimer[viewerid] = SetTimerEx("Docs_HideTimer", DOC_SHOW_MS, false, "i", viewerid);
}

// ESC (клик никъде) или X - затваря картата
stock Docs_OnClickTD(playerid, Text:clickedid)
{
    if (!DocOpen[playerid] || clickedid != INVALID_TEXT_DRAW) return 0;
    DocOpen[playerid] = false;           // изборът вече е прекратен от ESC
    Docs_Hide(playerid);
    return 1;
}

stock Docs_OnClickPlayerTD(playerid, PlayerText:td)
{
    if (!DocOpen[playerid] || td != DocCloseTD[playerid]) return 0;
    Docs_Hide(playerid);
    return 1;
}

stock Docs_ShowLicenseCard(playerid)
{
    Docs_Show(playerid, playerid, DOC_LICENSE);
}

// ---------- Меню > Документи ----------

stock bool:Docs_Has(playerid, doc)
{
    switch (doc) {
        case DOC_ID: return Docs_HasId(playerid) != 0;
        case DOC_MEDICAL: return Docs_HasMedical(playerid) != 0;
    }
    return (License_GetBits(playerid) != 0);
}

stock Docs_ShowMenu(playerid)
{
    new body[400], line[96];
    format(body, sizeof (body), "Документ\tСтатус\n");
    for (new d = 0; d < DOC_COUNT; d++) {
        if (Docs_Has(playerid, d)) format(line, sizeof (line), "%s\t" C_GREEN "имаш го\n", DocNamesBG[d]);
        else format(line, sizeof (line), "%s\t" C_GREY "нямаш\n", DocNamesBG[d]);
        strcat(body, line);
    }
    ShowPlayerDialog(playerid, DIALOG_DOCS, DIALOG_STYLE_TABLIST_HEADERS, C_BRAND "Документи", "%s", "Избери", "Назад", body);
}

// Най-близкият играч до 3 м
static stock Docs_NearestPlayer(playerid)
{
    new best = INVALID_PLAYER_ID, Float:bestD = 3.0, Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (i == playerid || !IsPlayerLogged(i) || GetPlayerVirtualWorld(i) != GetPlayerVirtualWorld(playerid)) continue;
        new Float:d = GetPlayerDistanceFromPoint(i, x, y, z);
        if (d < bestD) {
            bestD = d;
            best = i;
        }
    }
    return best;
}

stock Docs_OnDialogResponse(playerid, dialogid, response, listitem)
{
    switch (dialogid) {
        case DIALOG_DOCS: {
            if (!response) { Menu_Show(playerid); return 1; }
            if (listitem < 0 || listitem >= DOC_COUNT) return 1;
            if (!Docs_Has(playerid, listitem)) {
                SendError(playerid, "Нямаш този документ. Наставникът на гарата ще ти каже как да го вземеш.");
                Docs_ShowMenu(playerid);
                return 1;
            }
            DocPick[playerid] = listitem;
            ShowPlayerDialog(playerid, DIALOG_DOCS_ACTION, DIALOG_STYLE_LIST, C_BRAND "Документи",
                "Виж го\nПокажи го на най-близкия играч", "Избери", "Назад");
            return 1;
        }
        case DIALOG_DOCS_ACTION: {
            if (!response) { Docs_ShowMenu(playerid); return 1; }
            new doc = DocPick[playerid];
            if (listitem == 0) {
                Docs_Show(playerid, playerid, doc);
            } else {
                new t = Docs_NearestPlayer(playerid);
                if (t == INVALID_PLAYER_ID) return SendError(playerid, "Няма играч до теб (до 3 м).");
                Docs_Show(t, playerid, doc);
                Docs_Show(playerid, playerid, doc);
                SendClientMessage(t, COLOR_BRAND, "[ДОКУМЕНТИ] " C_WHITE "%s ти показва: %s.", PlayerData[playerid][pName], DocNamesBG[doc]);
                SendClientMessage(playerid, COLOR_BRAND, "[ДОКУМЕНТИ] " C_WHITE "Показа %s на %s.", DocNamesBG[doc], PlayerData[t][pName]);
            }
            return 1;
        }
    }
    return 0;
}

CMD:docs(playerid, params[])
{
    #pragma unused params
    Docs_ShowMenu(playerid);
    return 1;
}
