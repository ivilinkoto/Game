// Автокъща: по една във всеки град. ALT пеша (или /dealer) до входа -> витрина с 3D модели.
// Докосваш модел -> детайли -> купуваш. Купеното отива на свободно място в гаража.
// Продажба: Меню > Моят гараж > превозното средство > Продай (60% от цената).
//
// Входовете се местят от админ на място: /dealerpos [0 LS | 1 SF | 2 LV]

#define DEALER_RANGE        3.0
#define DEALER_SELL_PERCENT 60
#define DEALER_COLS         5
#define DEALER_CARD_W       82.0
#define DEALER_CARD_H       62.0
#define DEALER_GAP          8.0
#define DEALER_TOP          118.0
#define DEALER_ROW_H        (DEALER_CARD_H + 30.0)

// По подразбиране - до входа на гаражите (сменя се с /dealerpos)
static const Float:DealerDefault[MAX_CITIES][3] = {
    {1760.0, -1862.0, 13.58},
    {-1971.0, 155.0, 27.69},
    {2847.9, 1310.0, 11.39}
};

static Float:DealerPos[MAX_CITIES][3];
static DealerPickup[MAX_CITIES] = {-1, ...};
static Text3D:DealerLabel[MAX_CITIES] = {Text3D:-1, ...};
static DealerIcon[MAX_CITIES] = {-1, ...};

#define DEALER_MAX_CARDS    12

static PlayerText:DlPanel[MAX_PLAYERS] = {INVALID_PLAYER_TEXT_DRAW, ...};
static PlayerText:DlTitle[MAX_PLAYERS] = {INVALID_PLAYER_TEXT_DRAW, ...};
static PlayerText:DlHint[MAX_PLAYERS] = {INVALID_PLAYER_TEXT_DRAW, ...};
static PlayerText:DlCard[MAX_PLAYERS][DEALER_MAX_CARDS];
static PlayerText:DlName[MAX_PLAYERS][DEALER_MAX_CARDS];
static PlayerText:DlPrice[MAX_PLAYERS][DEALER_MAX_CARDS];
static DlCount[MAX_PLAYERS];
static bool:DlActive[MAX_PLAYERS];
static DlPick[MAX_PLAYERS];             // индекс в каталога
static DlSellSlot[MAX_PLAYERS];

// ---------- Входове ----------

stock Dealer_Init()
{
    Database_Exec("CREATE TABLE IF NOT EXISTS dealer_entrances (city INTEGER PRIMARY KEY, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL)");
    for (new c = 0; c < MAX_CITIES; c++) {
        for (new k = 0; k < 3; k++) DealerPos[c][k] = DealerDefault[c][k];
    }
    new DBResult:r = Database_Query("SELECT city, x, y, z FROM dealer_entrances");
    if (r != DBResult:0) {
        new rows = DB_GetRowCount(r);
        for (new i = 0; i < rows; i++) {
            new c = DB_GetFieldIntByName(r, "city");
            if (c >= 0 && c < MAX_CITIES) {
                DealerPos[c][0] = DB_GetFieldFloatByName(r, "x");
                DealerPos[c][1] = DB_GetFieldFloatByName(r, "y");
                DealerPos[c][2] = DB_GetFieldFloatByName(r, "z");
            }
            DB_SelectNextRow(r);
        }
        DB_FreeResultSet(r);
    }
    for (new c = 0; c < MAX_CITIES; c++) Dealer_CreateEntrance(c);
}

static stock Dealer_CreateEntrance(c)
{
    if (DealerPickup[c] != -1) DestroyDynamicPickup(DealerPickup[c]);
    if (DealerLabel[c] != Text3D:-1) DestroyDynamic3DTextLabel(DealerLabel[c]);
    if (DealerIcon[c] != -1) DestroyDynamicMapIcon(DealerIcon[c]);
    DealerIcon[c] = CreateDynamicMapIcon(DealerPos[c][0], DealerPos[c][1], DealerPos[c][2], 55, 0, 0, 0, -1, MAP_ICON_RANGE, MAPICON_LOCAL);
    DealerPickup[c] = CreateDynamicPickup(1239, 1, DealerPos[c][0], DealerPos[c][1], DealerPos[c][2], 0, 0);
    new text[160];
    format(text, sizeof (text), C_BRAND "АВТОКЪЩА %s\n" C_WHITE "Бусове и камиони\n" C_GREY "ALT или /dealer", CityShort[c]);
    DealerLabel[c] = CreateDynamic3DTextLabel(text, COLOR_WHITE, DealerPos[c][0], DealerPos[c][1], DealerPos[c][2] + 0.8, 25.0,
        INVALID_PLAYER_ID, INVALID_VEHICLE_ID, 1, 0, 0);
}

static stock Dealer_GetNear(playerid)
{
    if (GetPlayerVirtualWorld(playerid) != 0) return -1;
    for (new c = 0; c < MAX_CITIES; c++) {
        if (IsPlayerInRangeOfPoint(playerid, DEALER_RANGE, DealerPos[c][0], DealerPos[c][1], DealerPos[c][2])) return c;
    }
    return -1;
}

// ---------- Витрина (TextDraw) ----------

static stock PlayerText:Dl_Label(playerid, Float:x, Float:y, const text[], Float:size, TEXT_DRAW_FONT:font)
{
    new PlayerText:t = CreatePlayerTextDraw(playerid, x, y, text);
    PlayerTextDrawFont(playerid, t, font);
    PlayerTextDrawLetterSize(playerid, t, size, size * 4.0);
    PlayerTextDrawAlignment(playerid, t, TEXT_DRAW_ALIGN_CENTRE);
    PlayerTextDrawColour(playerid, t, COLOR_WHITE);
    PlayerTextDrawSetOutline(playerid, t, 1);
    PlayerTextDrawSetShadow(playerid, t, 0);
    PlayerTextDrawBackgroundColour(playerid, t, 0x000000FF);
    PlayerTextDrawSetProportional(playerid, t, true);
    PlayerTextDrawShow(playerid, t);
    return t;
}

stock Dealer_ShowShowroom(playerid)
{
    Dealer_Destroy(playerid);

    new count = sizeof (VehCatalog);
    if (count > DEALER_MAX_CARDS) count = DEALER_MAX_CARDS;
    new rows = (count + DEALER_COLS - 1) / DEALER_COLS;
    new Float:bottom = DEALER_TOP + rows * DEALER_ROW_H + 18.0;

    DlPanel[playerid] = CreatePlayerTextDraw(playerid, 320.0, DEALER_TOP - 40.0, "_");
    PlayerTextDrawUseBox(playerid, DlPanel[playerid], true);
    PlayerTextDrawBoxColour(playerid, DlPanel[playerid], 0x101418D8);
    PlayerTextDrawAlignment(playerid, DlPanel[playerid], TEXT_DRAW_ALIGN_CENTRE);
    PlayerTextDrawLetterSize(playerid, DlPanel[playerid], 0.0, (bottom - (DEALER_TOP - 40.0)) / 9.0);
    PlayerTextDrawTextSize(playerid, DlPanel[playerid], 0.0, DEALER_COLS * (DEALER_CARD_W + DEALER_GAP) + 16.0);
    PlayerTextDrawShow(playerid, DlPanel[playerid]);

    DlTitle[playerid] = Dl_Label(playerid, 320.0, DEALER_TOP - 34.0, "~w~CARGO STATE ~y~TRUCK DEALER", 0.38, TEXT_DRAW_FONT_2);
    DlHint[playerid] = Dl_Label(playerid, 320.0, bottom - 14.0, "TAP A VEHICLE FOR DETAILS  -  ESC: CLOSE", 0.2, TEXT_DRAW_FONT_2);

    new level = PlayerData[playerid][pLevel], text[64], price[16], payload[16];
    for (new i = 0; i < count; i++) {
        new row = i / DEALER_COLS, col = i % DEALER_COLS;
        new inRow = (row == rows - 1) ? (count - row * DEALER_COLS) : DEALER_COLS;
        new Float:rowWidth = inRow * DEALER_CARD_W + (inRow - 1) * DEALER_GAP;
        new Float:x = 320.0 - rowWidth / 2.0 + col * (DEALER_CARD_W + DEALER_GAP);
        new Float:y = DEALER_TOP + row * DEALER_ROW_H;
        new bool:noLicense = !License_CanDrive(playerid, VehCatalog[i][vcModel]);
        new bool:locked = (VehCatalog[i][vcLevel] > level) || noLicense;

        new PlayerText:c = CreatePlayerTextDraw(playerid, x, y, "_");
        PlayerTextDrawFont(playerid, c, TEXT_DRAW_FONT_MODEL_PREVIEW);
        PlayerTextDrawUseBox(playerid, c, true);
        PlayerTextDrawBoxColour(playerid, c, 0x00000000);
        PlayerTextDrawBackgroundColour(playerid, c, locked ? 0x3A2020FF : 0x2B3038FF);
        PlayerTextDrawTextSize(playerid, c, DEALER_CARD_W, DEALER_CARD_H);
        PlayerTextDrawSetPreviewModel(playerid, c, VehCatalog[i][vcModel]);
        PlayerTextDrawSetPreviewRot(playerid, c, -16.0, 0.0, -45.0, 1.0);
        PlayerTextDrawSetPreviewVehicleColours(playerid, c, 1, 1);
        PlayerTextDrawSetSelectable(playerid, c, true);
        PlayerTextDrawShow(playerid, c);
        DlCard[playerid][i] = c;

        format(text, sizeof (text), "%s%s", locked ? ("~r~") : ("~w~"), VehCatalog[i][vcName]);
        DlName[playerid][i] = Dl_Label(playerid, x + DEALER_CARD_W / 2.0, y + DEALER_CARD_H + 2.0, text, 0.22, TEXT_DRAW_FONT_2);
        if (VehCatalog[i][vcLevel] > level) {
            format(text, sizeof (text), "~r~LEVEL %d", VehCatalog[i][vcLevel]);
        } else if (noLicense) {
            format(text, sizeof (text), "~r~LICENSE %s", LicenseNames[License_ForModel(VehCatalog[i][vcModel])]);
        } else {
            FormatMoney(VehCatalog[i][vcPrice], price);
            Catalog_FormatPayloadLatin(VehCatalog[i][vcModel], payload);
            if (VehCatalog[i][vcPrice] == 0) format(text, sizeof (text), "~g~FREE ~w~%s", payload);
            else format(text, sizeof (text), "~g~$%s ~w~%s", price, payload);
        }
        DlPrice[playerid][i] = Dl_Label(playerid, x + DEALER_CARD_W / 2.0, y + DEALER_CARD_H + 13.0, text, 0.17, TEXT_DRAW_FONT_1);
    }
    DlCount[playerid] = count;
    DlActive[playerid] = true;
    SelectTextDraw(playerid, COLOR_BRAND);
}

stock Dealer_Destroy(playerid)
{
    new bool:wasActive = DlActive[playerid];
    DlActive[playerid] = false;     // преди CancelSelectTextDraw - той вика OnPlayerClickTextDraw

    if (DlPanel[playerid] != INVALID_PLAYER_TEXT_DRAW) PlayerTextDrawDestroy(playerid, DlPanel[playerid]);
    if (DlTitle[playerid] != INVALID_PLAYER_TEXT_DRAW) PlayerTextDrawDestroy(playerid, DlTitle[playerid]);
    if (DlHint[playerid] != INVALID_PLAYER_TEXT_DRAW) PlayerTextDrawDestroy(playerid, DlHint[playerid]);
    DlPanel[playerid] = INVALID_PLAYER_TEXT_DRAW;
    DlTitle[playerid] = INVALID_PLAYER_TEXT_DRAW;
    DlHint[playerid] = INVALID_PLAYER_TEXT_DRAW;
    for (new i = 0; i < DlCount[playerid]; i++) {
        PlayerTextDrawDestroy(playerid, DlCard[playerid][i]);
        PlayerTextDrawDestroy(playerid, DlName[playerid][i]);
        PlayerTextDrawDestroy(playerid, DlPrice[playerid][i]);
    }
    DlCount[playerid] = 0;
    if (wasActive && IsPlayerConnected(playerid)) CancelSelectTextDraw(playerid);
}

stock Dealer_OnPlayerConnect(playerid)
{
    DlActive[playerid] = false;
    DlCount[playerid] = 0;
    DlPanel[playerid] = INVALID_PLAYER_TEXT_DRAW;
    DlTitle[playerid] = INVALID_PLAYER_TEXT_DRAW;
    DlHint[playerid] = INVALID_PLAYER_TEXT_DRAW;
}

stock Dealer_OnClickPlayerTD(playerid, PlayerText:td)
{
    if (!DlActive[playerid]) return 0;
    for (new i = 0; i < DlCount[playerid]; i++) {
        if (DlCard[playerid][i] == td) {
            DlPick[playerid] = i;
            Dealer_Destroy(playerid);
            Dealer_ShowDetails(playerid, i);
            return 1;
        }
    }
    return 0;
}

stock Dealer_OnClickTD(playerid, Text:clickedid)
{
    if (!DlActive[playerid] || clickedid != INVALID_TEXT_DRAW) return 0;
    Dealer_Destroy(playerid);
    return 1;
}

// ---------- Покупка ----------

static stock Dealer_ShowDetails(playerid, i)
{
    new model = VehCatalog[i][vcModel], payload[24], cargo[64], price[16], status[160];
    if (VehCatalog[i][vcTrailer]) {
        new p[16];
        Catalog_FormatPayload(model, p);
        format(payload, sizeof (payload), "до %s", p);
    } else {
        Catalog_FormatPayload(model, payload);
    }
    Catalog_FormatCargo(model, cargo);
    FormatMoney(VehCatalog[i][vcPrice], price);

    if (VehCatalog[i][vcLevel] > PlayerData[playerid][pLevel]) {
        format(status, sizeof (status), C_RED "Нужно ниво %d (ти си ниво %d).", VehCatalog[i][vcLevel], PlayerData[playerid][pLevel]);
    } else if (!License_CanDrive(playerid, model)) {
        format(status, sizeof (status), C_RED "Нужна е категория %s. Вземи я в автошкола (GPS > Автошколи).", LicenseNames[License_ForModel(model)]);
    } else if (Veh_FindFreeSlot(playerid) == -1) {
        format(status, sizeof (status), C_RED "Гаражът ти е пълен. Продай превозно средство от Меню > Моят гараж.");
    } else if (Money_Get(playerid) < VehCatalog[i][vcPrice]) {
        format(status, sizeof (status), C_RED "Нямаш достатъчно пари.");
    } else {
        format(status, sizeof (status), C_GREEN "Можеш да го купиш.");
    }

    ShowPlayerDialog(playerid, DIALOG_DEALER_BUY, DIALOG_STYLE_MSGBOX, C_BRAND "Автокъща",
        C_BRAND "%s\n\n" \
        C_WHITE "Товароносимост: " C_BRAND "%s\n" \
        C_WHITE "Товари: " C_BRAND "%s\n" \
        C_WHITE "Нужно ниво: " C_BRAND "%d\n" \
        C_WHITE "Цена: " C_GREEN "$%s\n\n%s",
        "Купи", "Назад",
        VehCatalog[i][vcName], payload, cargo, VehCatalog[i][vcLevel], price, status);
}

static stock Dealer_Buy(playerid, i)
{
    if (VehCatalog[i][vcLevel] > PlayerData[playerid][pLevel]) return SendError(playerid, "Нивото ти не стига за това превозно средство.");
    if (!License_CanDrive(playerid, VehCatalog[i][vcModel])) return SendError(playerid, "Нямаш нужната шофьорска категория. Вземи я в автошкола.");
    if (Veh_FindFreeSlot(playerid) == -1) return SendError(playerid, "Гаражът ти е пълен.");
    if (!Money_Take(playerid, VehCatalog[i][vcPrice])) return SendError(playerid, "Нямаш достатъчно пари.");

    new slot = Veh_Add(playerid, VehCatalog[i][vcModel], random(126), random(126));
    if (slot == -1) {
        Money_Give(playerid, VehCatalog[i][vcPrice]);
        return SendError(playerid, "Грешка при покупката. Парите са върнати.");
    }
    PlayerPlaySound(playerid, 1058, 0.0, 0.0, 0.0);
    GameTextForPlayer(playerid, "~g~NEW VEHICLE!~n~~w~%s", 4000, 1, VehCatalog[i][vcName]);
    SendClientMessage(playerid, COLOR_BRAND, "[АВТОКЪЩА] " C_WHITE "Купи " C_BRAND "%s" C_WHITE "! Чака те в гаража, място %d.", VehCatalog[i][vcName], slot + 1);
    return 1;
}

// ---------- Продажба (от менюто на гаража) ----------

stock Dealer_AskSell(playerid, slot)
{
    if (!Veh_HasInSlot(playerid, slot)) return;
    if (Veh_Count(playerid) <= 1) {
        SendError(playerid, "Не можеш да продадеш единственото си превозно средство.");
        return;
    }
    new model = Veh_GetModel(playerid, slot), name[24], money[16];
    Veh_GetModelName(model, name);
    new idx = Catalog_Find(model);
    new value = (idx == -1) ? 0 : VehCatalog[idx][vcPrice] * DEALER_SELL_PERCENT / 100;
    FormatMoney(value, money);
    DlSellSlot[playerid] = slot;
    ShowPlayerDialog(playerid, DIALOG_DEALER_SELL, DIALOG_STYLE_MSGBOX, C_BRAND "Продажба",
        C_WHITE "Да продам ли " C_BRAND "%s" C_WHITE " за " C_GREEN "$%s" C_WHITE " (%d%% от цената)?",
        "Продай", "Отказ", name, money, DEALER_SELL_PERCENT);
}

static stock Dealer_Sell(playerid, slot)
{
    if (!Veh_HasInSlot(playerid, slot) || Veh_Count(playerid) <= 1) return;
    new vehicleid = Veh_GetVehicle(playerid, slot);
    if (vehicleid != INVALID_VEHICLE_ID && Delivery_HasJob(playerid) && Delivery_GetJobVehicle(playerid) == vehicleid) {
        SendError(playerid, "Не можеш да продадеш превозното средство по време на курс с него.");
        return;
    }
    new model = Veh_GetModel(playerid, slot), name[24], money[16];
    Veh_GetModelName(model, name);
    new idx = Catalog_Find(model);
    new value = (idx == -1) ? 0 : VehCatalog[idx][vcPrice] * DEALER_SELL_PERCENT / 100;
    if (!Veh_Remove(playerid, slot)) {
        SendError(playerid, "Не може сега - слез от превозното средство (или някой е в него).");
        return;
    }
    Money_Give(playerid, value);
    FormatMoney(value, money);
    SendClientMessage(playerid, COLOR_BRAND, "[АВТОКЪЩА] " C_WHITE "Продаде " C_BRAND "%s" C_WHITE " за " C_GREEN "$%s" C_WHITE ".", name, money);
}

// ---------- Събития ----------

// Достъп за други модули (GPS)
stock Dealer_Total()
{
    return MAX_CITIES;
}

stock Dealer_GetPos(c, &Float:x, &Float:y, &Float:z)
{
    x = DealerPos[c][0];
    y = DealerPos[c][1];
    z = DealerPos[c][2];
}

stock Dealer_GetName(c, dest[], size = sizeof (dest))
{
    format(dest, size, "Автокъща %s", CityNames[c]);
}

stock Dealer_Open(playerid)
{
    if (Dealer_GetNear(playerid) == -1) return 0;
    if (IsPlayerInAnyVehicle(playerid)) {
        SendError(playerid, "Слез от превозното средство.");
        return 1;
    }
    Dealer_ShowShowroom(playerid);
    return 1;
}

stock Dealer_OnKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
{
    if (!PlayerData[playerid][pLogged] || IsPlayerInAnyVehicle(playerid)) return 0;
    if (!((newkeys & KEY_WALK) && !(oldkeys & KEY_WALK))) return 0;
    return Dealer_Open(playerid);
}

stock Dealer_OnDialogResponse(playerid, dialogid, response)
{
    switch (dialogid) {
        case DIALOG_DEALER_BUY: {
            if (!response) {
                Dealer_ShowShowroom(playerid);
                return 1;
            }
            Dealer_Buy(playerid, DlPick[playerid]);
            return 1;
        }
        case DIALOG_DEALER_SELL: {
            if (response) Dealer_Sell(playerid, DlSellSlot[playerid]);
            Garage_ShowMenu(playerid);
            return 1;
        }
    }
    return 0;
}

CMD:dealer(playerid, params[])
{
    #pragma unused params
    if (!Dealer_Open(playerid)) SendError(playerid, "Трябва да си до автокъща (иконата с кола на картата).");
    return 1;
}

CMD:dealerpos(playerid, params[])
{
    if (!Admin_Check(playerid, 5)) return 1;
    if (IsNull(params) || !IsNumeric(params)) return SendUsage(playerid, "/dealerpos [0 LS | 1 SF | 2 LV]");
    new c = strval(params);
    if (c < 0 || c >= MAX_CITIES) return SendError(playerid, "Градът е 0, 1 или 2.");
    if (GetPlayerVirtualWorld(playerid) != 0) return SendError(playerid, "Трябва да си навън (свят 0).");
    new Float:x, Float:y, Float:z, q[160];
    GetPlayerPos(playerid, x, y, z);
    DealerPos[c][0] = x;
    DealerPos[c][1] = y;
    DealerPos[c][2] = z;
    format(q, sizeof (q), "INSERT OR REPLACE INTO dealer_entrances (city, x, y, z) VALUES (%d, %f, %f, %f)", c, x, y, z);
    Database_Exec(q);
    Dealer_CreateEntrance(c);
    SendClientMessage(playerid, COLOR_ADMIN, "[A] Автокъщата в %s е преместена тук.", CityNames[c]);
    return 1;
}
