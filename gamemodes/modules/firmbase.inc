// Услуги на базата на компанията за нейните хора (собственик и служители), според подобренията:
// ALT в превозното средство на базата -> зареждане (с отстъпка от "Фирмена колонка")
// и ремонт (с отстъпка от "Сервиз"). Без тези подобрения базата не предлага услуги.

static FirmBaseVehicle[MAX_PLAYERS];

stock FirmBase_OnKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
{
    if (GetPlayerState(playerid) != PLAYER_STATE_DRIVER) return 0;
    if (!(((newkeys & KEY_WALK) && !(oldkeys & KEY_WALK)) || ((newkeys & KEY_FIRE) && !(oldkeys & KEY_FIRE)))) return 0;
    new c = Company_GetNear(playerid);
    if (c == -1 || Firm_GetCompany(playerid) != c) return 0;
    new pump = Firm_UpEffect(c, 3), service = Firm_UpEffect(c, 4);
    if (pump == 0 && service == 0) return 0;

    new vehicleid = GetPlayerVehicleID(playerid);
    FirmBaseVehicle[playerid] = vehicleid;
    new body[256], line[128];
    body[0] = EOS;
    if (pump > 0) {
        new Float:fuel = Veh_GetFuel(vehicleid);
        new liters = (fuel < 0.0) ? 0 : floatround(float(Veh_GetTank(vehicleid)) - fuel, floatround_floor);
        format(line, sizeof (line), "Зареди догоре\t%d л\t" C_GREEN "$%d " C_GREY "(-%d%%)\n", liters, liters * FUEL_PRICE * (100 - pump) / 100, pump);
    } else {
        format(line, sizeof (line), C_GREY "Колонка\tняма\t-\n");
    }
    strcat(body, line);
    if (service > 0) {
        new cost = Repair_GetCost(vehicleid);
        format(line, sizeof (line), "Ремонт\t\t" C_GREEN "$%d " C_GREY "(-%d%%)\n", cost * (100 - service) / 100, service);
    } else {
        format(line, sizeof (line), C_GREY "Сервиз\tняма\t-\n");
    }
    strcat(body, line);
    new title[64];
    format(title, sizeof (title), C_BRAND "База на %s", CompanyDefs[c][cdName]);
    ShowPlayerDialog(playerid, DIALOG_FIRM_BASE, DIALOG_STYLE_TABLIST, title, "%s", "Избери", "Затвори", body);
    return 1;
}

stock FirmBase_OnDialogResponse(playerid, dialogid, response, listitem)
{
    if (dialogid != DIALOG_FIRM_BASE) return 0;
    if (!response) return 1;
    new vehicleid = FirmBaseVehicle[playerid];
    new c = Company_GetNear(playerid);
    if (c == -1 || Firm_GetCompany(playerid) != c || GetPlayerVehicleID(playerid) != vehicleid) return 1;

    if (listitem == 0) {
        new pump = Firm_UpEffect(c, 3);
        new Float:fuel = Veh_GetFuel(vehicleid);
        if (pump == 0 || fuel < 0.0) return 1;
        new liters = floatround(float(Veh_GetTank(vehicleid)) - fuel, floatround_floor);
        if (liters <= 0) return SendError(playerid, "Резервоарът е пълен.");
        new cost = liters * FUEL_PRICE * (100 - pump) / 100;
        if (!Firm_PayFuel(playerid, cost) && !Money_Take(playerid, cost)) return SendError(playerid, "Нямаш достатъчно пари.");
        Veh_SetFuel(vehicleid, fuel + float(liters));
        PlayerPlaySound(playerid, 1133, 0.0, 0.0, 0.0);
        SendClientMessage(playerid, COLOR_BRAND, "[БАЗА] " C_WHITE "Заредени " C_BRAND "%d л" C_WHITE " за " C_GREEN "$%d" C_WHITE " (фирмена колонка, -%d%%).", liters, cost, pump);
        Tutorial_OnRefuel(playerid);
    } else {
        new service = Firm_UpEffect(c, 4);
        if (service == 0) return 1;
        new cost = Repair_GetCost(vehicleid) * (100 - service) / 100;
        if (Repair_GetCost(vehicleid) == 0) return SendInfo(playerid, "Превозното средство е здраво.");
        if (!Money_Take(playerid, cost)) return SendError(playerid, "Нямаш достатъчно пари за ремонт.");
        RepairVehicle(vehicleid);
        SetVehicleHealth(vehicleid, 1000.0);
        Tune_ResetArmor(vehicleid);
        PlayerPlaySound(playerid, 1133, 0.0, 0.0, 0.0);
        SendClientMessage(playerid, COLOR_BRAND, "[БАЗА] " C_WHITE "Ремонт за " C_GREEN "$%d" C_WHITE " (фирмен сервиз, -%d%%).", cost, service);
    }
    return 1;
}
