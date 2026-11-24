// Мапинг: офис на компания (общ интериор, отделен виртуален свят за всяка компания).
//
// 16 м (X) x 12 м (Y), височина 3.5 м, високо над картата. Вход от юг (-Y).
//
//   север (+Y)
//   +-----------------+------+------------------+
//   |  ЗАЛА ЗА СРЕЩИ  |      |  КАБИНЕТ НА       |
//   |  (стъклена)     |      |  УПРАВИТЕЛЯ       |
//   +--вход--[ стена с името на компанията ]--вход+   y = 0.6
//   |  чакалня      РЕЦЕПЦИЯ (секретарка)  кът    |
//   +------------------[ вход ]------------------+   y = -6
//
// Текстурите са от играта (проверени в Prineside): паркет и тъмно дърво от CJ_OFFICE,
// тапети, таван и мокет от Carter_block_2, стъкло от lan2office. Подът, стените и таванът
// са от плочи 19447 (9.6 x 3.5 м), за да легнат текстурите правилно.

#define OFFICE_X            -1500.0
#define OFFICE_Y            1500.0
#define OFFICE_Z            1300.0
#define OFFICE_INTERIOR     0

#define OFFICE_HALF_W       8.0
#define OFFICE_HALF_L       6.0
#define OFFICE_HEIGHT       3.5

#define OFFICE_ENTRY_X      0.0
#define OFFICE_ENTRY_Y      -4.8         // тук се появяваш и тук е изходът
#define OFFICE_DESK_Y       -3.3         // пред рецепцията (ALT)
#define OFFICE_SECRETARY_Y  -1.0

// Текстури
enum {
    OTX_PARQUET,
    OTX_DARKWOOD,
    OTX_WALLPAPER,
    OTX_SMOOTHWALL,
    OTX_CEILING,
    OTX_CARPET,
    OTX_GLASS,
    OTX_CHROME
}

stock Office_Tex(obj, tex)
{
    switch (tex) {
        case OTX_PARQUET:    SetDynamicObjectMaterial(obj, 0, 2165, "CJ_OFFICE", "CJ_WOOD1", 0);
        case OTX_DARKWOOD:   SetDynamicObjectMaterial(obj, 0, 2165, "CJ_OFFICE", "CJ_WOOD_DARK", 0);
        case OTX_WALLPAPER:  SetDynamicObjectMaterial(obj, 0, 14415, "Carter_block_2", "mp_carter_wallpaper", 0);
        case OTX_SMOOTHWALL: SetDynamicObjectMaterial(obj, 0, 14415, "Carter_block_2", "mp_carter_smoothwall", 0);
        case OTX_CEILING:    SetDynamicObjectMaterial(obj, 0, 14415, "Carter_block_2", "mp_carter_ceiling", 0);
        case OTX_CARPET:     SetDynamicObjectMaterial(obj, 0, 14415, "Carter_block_2", "mp_carter_pcarpet", 0);
        case OTX_GLASS:      SetDynamicObjectMaterial(obj, 0, 3781, "lan2office", "glass_office1", 0);
        case OTX_CHROME:     SetDynamicObjectMaterial(obj, 0, 2165, "CJ_OFFICE", "CJ_CHROME2", 0);
    }
    return obj;
}

stock Office_Obj(model, Float:x, Float:y, Float:z, Float:rx, Float:ry, Float:rz)
{
    return CreateDynamicObject(model, OFFICE_X + x, OFFICE_Y + y, OFFICE_Z + z, rx, ry, rz, -1, -1, -1, 200.0, 200.0);
}

// Плоча 9.6 x 3.5, легнала (под, таван); alongX = дългата страна по X
static stock Office_Slab(Float:x, Float:y, Float:z, bool:alongX, tex)
{
    Office_Tex(Office_Obj(19447, x, y, z, 0.0, 90.0, alongX ? 90.0 : 0.0), tex);
}

// Стена 9.6 x 3.5 от пода до тавана; alongX = по оста X
static stock Office_Wall(Float:x, Float:y, bool:alongX, tex)
{
    Office_Tex(Office_Obj(19447, x, y, OFFICE_HEIGHT / 2.0, 0.0, 0.0, alongX ? 90.0 : 0.0), tex);
}

stock Office_CreateMap()
{
    // ---------- Под: паркет (2 х 4 плочи), горната страна на нивото на пода ----------
    for (new i = 0; i < 2; i++) {
        for (new j = 0; j < 4; j++) {
            Office_Slab(-3.2 + i * 9.6, -4.25 + j * 3.5, -0.15, true, OTX_PARQUET);
        }
    }
    // Мокет в залата за срещи
    Office_Slab(-5.6, 3.4, -0.13, true, OTX_CARPET);
    // ---------- Таван ----------
    for (new i = 0; i < 2; i++) {
        for (new j = 0; j < 4; j++) {
            Office_Slab(-3.2 + i * 9.6, -4.25 + j * 3.5, OFFICE_HEIGHT + 0.15, true, OTX_CEILING);
        }
    }
    // ---------- Външни стени (тапет) ----------
    Office_Wall(-3.2, -OFFICE_HALF_L - 0.1, true, OTX_WALLPAPER);
    Office_Wall(6.4, -OFFICE_HALF_L - 0.1, true, OTX_WALLPAPER);
    Office_Wall(-3.2, OFFICE_HALF_L + 0.1, true, OTX_WALLPAPER);
    Office_Wall(6.4, OFFICE_HALF_L + 0.1, true, OTX_WALLPAPER);
    Office_Wall(-OFFICE_HALF_W - 0.1, -1.2, false, OTX_WALLPAPER);
    Office_Wall(-OFFICE_HALF_W - 0.1, 8.4, false, OTX_WALLPAPER);
    Office_Wall(OFFICE_HALF_W + 0.1, -1.2, false, OTX_WALLPAPER);
    Office_Wall(OFFICE_HALF_W + 0.1, 8.4, false, OTX_WALLPAPER);

    // ---------- Стена зад рецепцията (тъмно дърво), отворите са отстрани ----------
    Office_Wall(0.0, 0.6, true, OTX_DARKWOOD);
    // Стъклени стени: зала за срещи (вляво) и кабинет (вдясно)
    Office_Wall(-1.0, 5.4, false, OTX_GLASS);
    Office_Wall(1.8, 5.4, false, OTX_GLASS);

    // ---------- Входна врата ----------
    Office_Obj(1491, -1.0, -OFFICE_HALF_L + 0.06, 0.0, 0.0, 0.0, 0.0);
    Office_Obj(1491,  1.0, -OFFICE_HALF_L + 0.06, 0.0, 0.0, 0.0, 180.0);

    // ---------- Рецепция: плот от тъмно дърво с хромирана лента ----------
    for (new i = 0; i < 5; i++) {
        for (new side = -1; side <= 1; side += 2) {
            Office_Tex(Office_Obj(19353, side * 1.6, -2.6 + i * 0.2, 1.1 - 1.75, 0.0, 0.0, 90.0), (i == 0) ? OTX_CHROME : OTX_DARKWOOD);
        }
    }
    Office_Obj(2190, 1.2, -2.2, 1.1, 0.0, 0.0, 0.0);              // компютър
    Office_Obj(1714, -0.9, -1.3, 0.0, 0.0, 0.0, 180.0);           // стол
    Office_Obj(2001, -3.4, -2.2, 0.0, 0.0, 0.0, 0.0);             // саксии до рецепцията
    Office_Obj(2001,  3.4, -2.2, 0.0, 0.0, 0.0, 0.0);

    // ---------- Чакалня (запад) ----------
    Office_Obj(1702, -7.3, -3.6, 0.0, 0.0, 0.0, 90.0);
    Office_Obj(1704, -4.9, -2.7, 0.0, 0.0, 0.0, 270.0);
    Office_Obj(1704, -4.9, -4.5, 0.0, 0.0, 0.0, 270.0);
    Office_Obj(1433, -6.1, -3.6, 0.0, 0.0, 0.0, 0.0);
    Office_Obj(948, -7.5, -5.5, 0.0, 0.0, 0.0, 0.0);
    Office_Obj(948, -7.5, -0.3, 0.0, 0.0, 0.0, 0.0);
    // ---------- Кът (изток) ----------
    Office_Obj(2002, 7.6, -3.0, 0.0, 0.0, 0.0, 270.0);
    Office_Obj(1704, 5.6, -5.3, 0.0, 0.0, 0.0, 0.0);
    Office_Obj(1704, 4.2, -5.3, 0.0, 0.0, 0.0, 0.0);
    Office_Obj(948, 7.5, -5.5, 0.0, 0.0, 0.0, 0.0);
    Office_Obj(948, 7.5, -0.3, 0.0, 0.0, 0.0, 0.0);

    // ---------- Зала за срещи (северозапад) ----------
    Office_Obj(2165, -4.5, 2.9, 0.0, 0.0, 0.0, 0.0);
    Office_Obj(2165, -4.5, 4.1, 0.0, 0.0, 0.0, 180.0);
    Office_Obj(1714, -4.5, 2.15, 0.0, 0.0, 0.0, 180.0);
    Office_Obj(1714, -4.5, 4.85, 0.0, 0.0, 0.0, 0.0);
    Office_Obj(19786, -4.5, OFFICE_HALF_L - 0.08, 1.8, 0.0, 0.0, 0.0);   // екран за презентации
    Office_Obj(948, -7.5, 5.5, 0.0, 0.0, 0.0, 0.0);

    // ---------- Кабинет на управителя (североизток) ----------
    Office_Obj(2165, 5.0, 4.4, 0.0, 0.0, 0.0, 180.0);
    Office_Obj(1714, 5.0, 5.15, 0.0, 0.0, 0.0, 0.0);
    Office_Obj(1704, 4.3, 3.0, 0.0, 0.0, 0.0, 180.0);
    Office_Obj(1704, 5.7, 3.0, 0.0, 0.0, 0.0, 180.0);
    Office_Obj(1742, 7.7, 4.6, 0.0, 0.0, 0.0, 270.0);
    Office_Obj(1742, 7.7, 3.2, 0.0, 0.0, 0.0, 270.0);
    Office_Obj(948, 7.5, 1.2, 0.0, 0.0, 0.0, 0.0);
    // Кът между стъклата: автомат и растение
    Office_Obj(1776, 0.4, 5.6, 1.1, 0.0, 0.0, 180.0);
    Office_Obj(948, 1.4, 1.2, 0.0, 0.0, 0.0, 0.0);

    // ---------- Осветление ----------
    for (new i = -1; i <= 1; i++) {
        Office_Obj(1893, i * 5.0, -3.0, OFFICE_HEIGHT - 0.1, 0.0, 0.0, 0.0);
        Office_Obj(1893, i * 5.0, 3.5, OFFICE_HEIGHT - 0.1, 0.0, 0.0, 0.0);
    }
}

// Табелата с името на компанията на стената зад рецепцията (за всяка компания - в нейния свят)
stock Office_CreateSign(world, const name[], rgb)
{
    new obj = CreateDynamicObject(19353, OFFICE_X, OFFICE_Y + 0.43, OFFICE_Z + 1.75, 0.0, 0.0, 90.0, world, -1, -1, 200.0, 200.0);
    // 140 = 512x512 (панелът е квадратен); 1 = в центъра. Цветът е ARGB.
    SetDynamicObjectMaterialText(obj, 0, name, 140, "Arial", 52, 1, 0xFF000000 | rgb, 0xFF2A1E16, 1);
    return obj;
}
