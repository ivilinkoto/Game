// HUD долу в средата: скорост и състояние на машината - само в превозно средство.
// Парите не се дублират - играта сама ги показва горе вдясно.
// В средата, за да не застъпва бутоните на мобилния клиент (долу вляво и вдясно).
// Текстът е на латиница: шрифтовете на SA не показват кирилица.
//
// В етап 2 тук се добавят гориво и товар.

#define HUD_X           320.0
#define HUD_Y_MONEY     392.0
#define HUD_Y_SPEED     398.0
#define HUD_Y_HEALTH    414.0

static PlayerText:HudMoney[MAX_PLAYERS] = {INVALID_PLAYER_TEXT_DRAW, ...};
static PlayerText:HudSpeed[MAX_PLAYERS] = {INVALID_PLAYER_TEXT_DRAW, ...};
static PlayerText:HudHealth[MAX_PLAYERS] = {INVALID_PLAYER_TEXT_DRAW, ...};
static bool:HudInVehicle[MAX_PLAYERS];

static stock PlayerText:Hud_MakeText(playerid, Float:y, Float:size)
{
    new PlayerText:t = CreatePlayerTextDraw(playerid, HUD_X, y, "_");
    PlayerTextDrawFont(playerid, t, TEXT_DRAW_FONT_3);
    PlayerTextDrawLetterSize(playerid, t, size, size * 4.0);
    PlayerTextDrawAlignment(playerid, t, TEXT_DRAW_ALIGN_CENTRE);
    PlayerTextDrawColour(playerid, t, COLOR_WHITE);
    PlayerTextDrawSetOutline(playerid, t, 1);
    PlayerTextDrawSetShadow(playerid, t, 0);
    PlayerTextDrawBackgroundColour(playerid, t, 0x000000FF);
    PlayerTextDrawSetProportional(playerid, t, true);
    return t;
}

stock Hud_Create(playerid)
{
    Hud_Destroy(playerid);
    HudMoney[playerid]  = Hud_MakeText(playerid, HUD_Y_MONEY, 0.36);
    HudSpeed[playerid]  = Hud_MakeText(playerid, HUD_Y_SPEED, 0.36);
    HudHealth[playerid] = Hud_MakeText(playerid, HUD_Y_HEALTH, 0.28);
    HudInVehicle[playerid] = false;
}

stock Hud_Destroy(playerid)
{
    if (HudMoney[playerid] != INVALID_PLAYER_TEXT_DRAW) PlayerTextDrawDestroy(playerid, HudMoney[playerid]);
    if (HudSpeed[playerid] != INVALID_PLAYER_TEXT_DRAW) PlayerTextDrawDestroy(playerid, HudSpeed[playerid]);
    if (HudHealth[playerid] != INVALID_PLAYER_TEXT_DRAW) PlayerTextDrawDestroy(playerid, HudHealth[playerid]);
    HudMoney[playerid] = INVALID_PLAYER_TEXT_DRAW;
    HudSpeed[playerid] = INVALID_PLAYER_TEXT_DRAW;
    HudHealth[playerid] = INVALID_PLAYER_TEXT_DRAW;
    HudInVehicle[playerid] = false;
}

stock Hud_Show(playerid)
{
    if (!PlayerData[playerid][pHudOn] || HudMoney[playerid] == INVALID_PLAYER_TEXT_DRAW) return;
    HudInVehicle[playerid] = false;
    Hud_Update(playerid);
}

stock Hud_Hide(playerid)
{
    if (HudMoney[playerid] == INVALID_PLAYER_TEXT_DRAW) return;
    PlayerTextDrawHide(playerid, HudMoney[playerid]);
    PlayerTextDrawHide(playerid, HudSpeed[playerid]);
    PlayerTextDrawHide(playerid, HudHealth[playerid]);
    HudInVehicle[playerid] = false;
}

stock Hud_UpdateMoney(playerid)
{
    if (HudMoney[playerid] == INVALID_PLAYER_TEXT_DRAW) return;
    new money[16], text[32];
    FormatMoney(PlayerData[playerid][pMoney], money);
    format(text, sizeof (text), "~g~$~w~%s", money);
    PlayerTextDrawSetString(playerid, HudMoney[playerid], text);
}

// Вика се от таймера на HUD-а (няколко пъти в секунда).
stock Hud_Update(playerid)
{
    if (!PlayerData[playerid][pHudOn] || !PlayerData[playerid][pSpawned] || HudMoney[playerid] == INVALID_PLAYER_TEXT_DRAW) return;

    new vehicleid = GetPlayerVehicleID(playerid);
    if (vehicleid == 0) {
        if (HudInVehicle[playerid]) {
            PlayerTextDrawHide(playerid, HudSpeed[playerid]);
            PlayerTextDrawHide(playerid, HudHealth[playerid]);
            HudInVehicle[playerid] = false;
        }
        return;
    }

    new Float:vx, Float:vy, Float:vz, Float:health, text[48];
    GetVehicleVelocity(vehicleid, vx, vy, vz);
    new speed = floatround(VectorSize(vx, vy, vz) * 180.0);
    format(text, sizeof (text), "%d ~y~KM/H", speed);
    PlayerTextDrawSetString(playerid, HudSpeed[playerid], text);

    GetVehicleHealth(vehicleid, health);
    // 250 = гори, 1000 = здрава -> 0..100%
    new percent = floatround((health - 250.0) / 7.5);
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    format(text, sizeof (text), "%sENGINE %d%%", (percent < 30) ? ("~r~") : ("~w~"), percent);
    PlayerTextDrawSetString(playerid, HudHealth[playerid], text);

    if (!HudInVehicle[playerid]) {
        PlayerTextDrawShow(playerid, HudSpeed[playerid]);
        PlayerTextDrawShow(playerid, HudHealth[playerid]);
        HudInVehicle[playerid] = true;
    }
}

stock Hud_SetEnabled(playerid, bool:on)
{
    PlayerData[playerid][pHudOn] = on;
    if (on) Hud_Show(playerid);
    else Hud_Hide(playerid);
}

forward Hud_Tick();
public Hud_Tick()
{
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (IsPlayerLogged(i)) Hud_Update(i);
    }
    Delivery_Tick();
    return 1;
}
