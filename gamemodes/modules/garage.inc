// Личен гараж: влизане/излизане (пеша и с камион), спаун, меню "Моят гараж".
//
// Вход/изход: ALT пеша, H (клаксон) с превозно средство.
// На входа в твоя град -> влизаш (с твой камион - той отива на мястото си, ти си в него).
// До вратата отвътре -> излизаш (с камиона, ако караш).

#define GARAGE_FREEZE_MS    1500

stock bool:Garage_IsInside(playerid)
{
    return PlayerData[playerid][pInGarage];
}

// Подготвя обектите около точката, преди да преместим играча (за да не пропадне).
static stock Garage_Preload(playerid, Float:x, Float:y, Float:z, world)
{
    Streamer_UpdateEx(playerid, x, y, z, world, 0);
    TogglePlayerControllable(playerid, false);
    SetTimerEx("Garage_Unfreeze", GARAGE_FREEZE_MS, false, "ii", playerid, PlayerData[playerid][pConnSeq]);
}

forward Garage_Unfreeze(playerid, seq);
public Garage_Unfreeze(playerid, seq)
{
    if (IsPlayerConnected(playerid) && PlayerData[playerid][pConnSeq] == seq) {
        TogglePlayerControllable(playerid, true);
    }
    return 1;
}

// Поставя играча пеша в гаража (при спаун и при влизане пеша).
stock Garage_PutPlayerInside(playerid)
{
    new world = Garage_GetWorld(playerid);
    new Float:x = GARAGE_X + GARAGE_FOOT_X, Float:y = GARAGE_Y + GARAGE_FOOT_Y, Float:z = GARAGE_Z + 1.0;

    PlayerData[playerid][pInGarage] = true;
    Veh_ParkAllInGarage(playerid);

    SetPlayerVirtualWorld(playerid, world);
    SetPlayerInterior(playerid, 0);
    Garage_Preload(playerid, x, y, z, world);
    SetPlayerPos(playerid, x, y, z);
    SetPlayerFacingAngle(playerid, 0.0);
    SetCameraBehindPlayer(playerid);
}

// Влиза в гаража с камиона си (той отива на мястото си, играчът остава в него).
static stock Garage_EnterWithVehicle(playerid, vehicleid, slot)
{
    new world = Garage_GetWorld(playerid);
    PlayerData[playerid][pInGarage] = true;
    Veh_ParkAllInGarage(playerid);

    new Float:x = GARAGE_X + GarageSlotPos[slot][0];
    new Float:y = GARAGE_Y + GarageSlotPos[slot][1];
    new Float:z = GARAGE_Z + 1.5;

    SetPlayerVirtualWorld(playerid, world);
    SetPlayerInterior(playerid, 0);
    Garage_Preload(playerid, x, y, z, world);
    Veh_ParkInGarage(playerid, slot);
    PutPlayerInVehicle(playerid, vehicleid, 0);
    SetCameraBehindPlayer(playerid);
    GameTextForPlayer(playerid, "~y~GARAGE", 2000, 1);
}

static stock Garage_ExitOnFoot(playerid)
{
    new city = PlayerData[playerid][pGarageCity];
    new Float:x, Float:y, Float:z, Float:a;
    City_GetExitPoint(city, 3.5, x, y, z, a);

    PlayerData[playerid][pInGarage] = false;
    SetPlayerVirtualWorld(playerid, 0);
    SetPlayerInterior(playerid, 0);
    Garage_Preload(playerid, x, y, z, 0);
    SetPlayerPos(playerid, x, y, z + 0.3);
    SetPlayerFacingAngle(playerid, a);
    SetCameraBehindPlayer(playerid);
}

static stock Garage_ExitWithVehicle(playerid, vehicleid, slot)
{
    new city = PlayerData[playerid][pGarageCity];
    new Float:x, Float:y, Float:z, Float:a;
    City_GetExitPoint(city, 7.0, x, y, z, a);

    PlayerData[playerid][pInGarage] = false;
    Veh_SetOut(playerid, slot, true);

    SetPlayerVirtualWorld(playerid, 0);
    SetPlayerInterior(playerid, 0);
    Garage_Preload(playerid, x, y, z, 0);
    SetVehicleVirtualWorld(vehicleid, 0);
    LinkVehicleToInterior(vehicleid, 0);
    SetVehiclePos(vehicleid, x, y, z + 1.0);
    SetVehicleZAngle(vehicleid, a);
    PutPlayerInVehicle(playerid, vehicleid, 0);
    SetCameraBehindPlayer(playerid);
    GameTextForPlayer(playerid, "~y~DRIVE SAFE!", 2000, 1);
}

// ---------- ALT / H ----------

stock Garage_OnKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
{
    if (!PlayerData[playerid][pLogged] || !PlayerData[playerid][pSpawned]) return 0;

    new vehicleid = GetPlayerVehicleID(playerid);
    new bool:driver = (vehicleid != 0 && GetPlayerState(playerid) == PLAYER_STATE_DRIVER);
    new KEY:key = driver ? KEY_CROUCH : KEY_WALK;     // в превозно средство KEY_CROUCH е клаксонът (H)
    if (!((newkeys & key) && !(oldkeys & key))) return 0;
    if (vehicleid != 0 && !driver) return 0;

    // --- Отвътре: до вратата ---
    if (PlayerData[playerid][pInGarage]) {
        new Float:range = driver ? GARAGE_DOOR_RANGE_VEH : GARAGE_DOOR_RANGE_FOOT;
        if (!IsPlayerInRangeOfPoint(playerid, range, GARAGE_X, GARAGE_Y + GARAGE_DOOR_Y, GARAGE_Z + 1.0)) return 0;

        if (vehicleid == 0) {
            Garage_ExitOnFoot(playerid);
            return 1;
        }
        if (!driver) return 1;
        new slot = Veh_GetSlot(vehicleid);
        if (Veh_GetOwner(vehicleid) != playerid || slot == -1) return 1;
        Garage_ExitWithVehicle(playerid, vehicleid, slot);
        return 1;
    }

    // --- Отвън: до входа ---
    if (GetPlayerVirtualWorld(playerid) != 0) return 0;
    new city = City_GetEntranceNear(playerid, driver ? GARAGE_ENTER_RANGE_VEH : GARAGE_ENTER_RANGE_FOOT);
    if (city == -1) return 0;

    if (city != PlayerData[playerid][pGarageCity]) {
        new msg[128];
        format(msg, sizeof (msg), "Твоят гараж е в %s. Тук нямаш гараж.", CityNames[PlayerData[playerid][pGarageCity]]);
        SendError(playerid, msg);
        return 1;
    }

    if (vehicleid == 0) {
        Garage_PutPlayerInside(playerid);
        return 1;
    }
    new slot = Veh_GetSlot(vehicleid);
    if (Veh_GetOwner(vehicleid) != playerid || slot == -1) {
        SendError(playerid, "В гаража влизат само твоите камиони.");
        return 1;
    }
    Garage_EnterWithVehicle(playerid, vehicleid, slot);
    return 1;
}

// ---------- Спаун ----------

stock Garage_OnPlayerSpawn(playerid)
{
    City_ShowGarageIcon(playerid);
    Garage_PutPlayerInside(playerid);
}

stock Garage_OnPlayerConnect(playerid)
{
    PlayerData[playerid][pInGarage] = false;
}

// ---------- Меню "Моят гараж" ----------

stock Garage_ShowMenu(playerid)
{
    new body[640], line[160], name[24], status[48], payload[16];
    format(body, sizeof (body), "Място\tПревозно средство\tТовароносимост\tСъстояние\n");
    for (new s = 0; s < GARAGE_SLOTS; s++) {
        if (!Veh_HasInSlot(playerid, s)) {
            format(line, sizeof (line), "%d\t" C_GREY "празно\t-\t-\n", s + 1);
        } else {
            Veh_GetModelName(Veh_GetModel(playerid, s), name);
            new vehicleid = Veh_GetVehicle(playerid, s), Float:hp = 1000.0;
            if (vehicleid != INVALID_VEHICLE_ID) GetVehicleHealth(vehicleid, hp);
            new percent = floatround((hp - 250.0) / 7.5);
            if (percent < 0) percent = 0;
            if (percent > 100) percent = 100;
            if (Veh_IsOut(playerid, s)) format(status, sizeof (status), C_BRAND "навън" C_WHITE ", %d%%", percent);
            else format(status, sizeof (status), C_GREEN "в гаража" C_WHITE ", %d%%", percent);
            Catalog_FormatPayload(Veh_GetModel(playerid, s), payload);
            format(line, sizeof (line), "%d\t%s\t%s\t%s\n", s + 1, name, payload, status);
        }
        strcat(body, line);
    }
    format(line, sizeof (line), C_BRAND "GPS до гаража\t" C_WHITE "%s\t\t\n", CityNames[PlayerData[playerid][pGarageCity]]);
    strcat(body, line);

    ShowPlayerDialog(playerid, DIALOG_GARAGE_MENU, DIALOG_STYLE_TABLIST_HEADERS, C_BRAND "Моят гараж", "%s", "Избери", "Назад", body);
}

static GarageMenuSlot[MAX_PLAYERS];

stock Garage_OnDialogResponse(playerid, dialogid, response, listitem)
{
    switch (dialogid) {
        case DIALOG_GARAGE_MENU: {
            if (!response) {
                Menu_Show(playerid);
                return 1;
            }
            if (listitem == GARAGE_SLOTS) {
                new Float:x, Float:y, Float:z, Float:a;
                City_GetEntrance(PlayerData[playerid][pGarageCity], x, y, z, a);
                if (PlayerData[playerid][pInGarage]) {
                    SendInfo(playerid, "Вече си в гаража.");
                    return 1;
                }
                Gps_SetPoint(playerid, "Моят гараж", x, y, z);
                return 1;
            }
            if (listitem < 0 || listitem >= GARAGE_SLOTS) return 1;
            if (!Veh_HasInSlot(playerid, listitem)) {
                SendInfo(playerid, "Мястото е празно. Нови камиони - скоро в автокъщата.");
                Garage_ShowMenu(playerid);
                return 1;
            }
            GarageMenuSlot[playerid] = listitem;
            new name[24], payload[16], cargo[48], model = Veh_GetModel(playerid, listitem);
            Veh_GetModelName(model, name);
            Catalog_FormatPayload(model, payload);
            Catalog_FormatCargo(model, cargo);
            if (Veh_IsOut(playerid, listitem)) {
                ShowPlayerDialog(playerid, DIALOG_GARAGE_TRUCK, DIALOG_STYLE_MSGBOX, C_BRAND "Моят гараж",
                    C_WHITE "%s е навън.\n\nДа го върна ли на мястото му в гаража?", "Върни", "Назад", name);
            } else {
                ShowPlayerDialog(playerid, DIALOG_GARAGE_TRUCK, DIALOG_STYLE_MSGBOX, C_BRAND "Моят гараж",
                    C_WHITE "%s е в гаража, място %d.\n" \
                    "Товароносимост: " C_BRAND "%s" C_WHITE ", Товари: " C_BRAND "%s" C_WHITE "\n\nВлез в гаража, качи се в него и натисни " C_BRAND "H" C_WHITE " до вратата, за да излезеш.\n" \
                    C_GREY "Тунинг и ремонт: седни в него в гаража и натисни ALT.", "Продай", "Назад", name, listitem + 1, payload, cargo);
            }
            return 1;
        }
        case DIALOG_GARAGE_TRUCK: {
            new slot = GarageMenuSlot[playerid];
            if (response && slot >= 0 && slot < GARAGE_SLOTS && !Veh_IsOut(playerid, slot)) {
                Dealer_AskSell(playerid, slot);
                return 1;
            }
            if (response && slot >= 0 && slot < GARAGE_SLOTS && Veh_IsOut(playerid, slot)) {
                if (Veh_ReturnToGarage(playerid, slot)) {
                    SendClientMessage(playerid, COLOR_BRAND, "[ГАРАЖ] " C_WHITE "Камионът е върнат в гаража.");
                } else {
                    SendError(playerid, "Не може сега - слез от камиона (или някой е в него).");
                }
            }
            Garage_ShowMenu(playerid);
            return 1;
        }
    }
    return 0;
}
