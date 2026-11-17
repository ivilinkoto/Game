// Товари: вид, ставка ($ за тон и км), тегло, ценност.
// Името на латиница е за TextDraw-а (SA шрифтовете не показват кирилица).

enum {
    GOOD_PARCELS,
    GOOD_MEDS,
    GOOD_FOOD,
    GOOD_DRINKS,
    GOOD_ELECTRONICS,
    GOOD_FURNITURE,
    GOOD_METAL,
    GOOD_PARTS,
    GOOD_SAND,
    GOOD_GRAVEL,
    GOOD_ORE,
    GOOD_WOOD,
    GOOD_FUEL,
    GOOD_FROZEN,            // хладилни
    GOOD_DAIRY,
    GOOD_GLASS,             // чупливи
    MAX_GOODS
}

// Особености на товара
#define GOODFLAG_PERISHABLE (1 << 0)    // хладилен: по-кратък срок, закъснението го разваля
#define GOODFLAG_FRAGILE    (1 << 1)    // чуплив: щетите тежат двойно, над 40% се чупи

#define GOOD_BIT(%0)    (1 << (%0))

enum E_GOOD {
    gdName[48],
    gdNameEN[16],
    gdCargo,            // CARGO_* (вид)
    gdRate,             // $ за тон и км
    gdMinKg,
    gdMaxKg,
    gdStars,            // ценност 1-5
    gdFlags             // GOODFLAG_*
}

stock const Goods[MAX_GOODS][E_GOOD] = {
    {"Пратки",          "PARCELS",      CARGO_PARCEL, 150,  200,  1500, 3, 0},
    {"Медикаменти",     "MEDICINE",     CARGO_PARCEL, 190,  100,   800, 5, GOODFLAG_PERISHABLE | GOODFLAG_FRAGILE},
    {"Храни",           "FOOD",         CARGO_BOX,     40, 1000,  8000, 2, 0},
    {"Напитки",         "DRINKS",       CARGO_BOX,     36, 1000,  8000, 2, 0},
    {"Електроника",     "ELECTRONICS",  CARGO_BOX,     62, 1000,  6000, 4, GOODFLAG_FRAGILE},
    {"Мебели",          "FURNITURE",    CARGO_BOX,     45, 1000,  7000, 3, 0},
    {"Метални изделия", "METAL GOODS",  CARGO_BOX,     38, 2000,  8000, 2, 0},
    {"Резервни части",  "SPARE PARTS",  CARGO_BOX,     50, 1000,  6000, 3, 0},
    {"Пясък",           "SAND",         CARGO_BULK,    26, 4000, 12000, 1, 0},
    {"Чакъл",           "GRAVEL",       CARGO_BULK,    26, 4000, 12000, 1, 0},
    {"Руда",            "ORE",          CARGO_BULK,    30, 4000, 12000, 1, 0},
    {"Дърва",           "TIMBER",       CARGO_BULK,    32, 4000, 12000, 2, 0},
    {"Гориво",          "FUEL",         CARGO_LIQUID,  34, 8000, 25000, 3, 0},
    {"Замразени храни", "FROZEN FOOD",  CARGO_BOX,     58, 1000,  8000, 4, GOODFLAG_PERISHABLE},
    {"Млечни продукти", "DAIRY",        CARGO_BOX,     52, 1000,  7000, 3, GOODFLAG_PERISHABLE},
    {"Стъкло",          "GLASS",        CARGO_BOX,     55, 1000,  6000, 4, GOODFLAG_FRAGILE}
};

#define DELIVERY_BASE_PAY   150

// Плащане = база + км x тонове x ставка
stock Goods_CalcPay(good, kg, Float:meters)
{
    new Float:pay = float(DELIVERY_BASE_PAY) + (meters / 1000.0) * (float(kg) / 1000.0) * float(Goods[good][gdRate]);
    return floatround(pay);
}

// Ценност като звездички: за диалозите (с цветове) и за TextDraw-а (с ~кодове~).
stock Goods_Stars(good, dest[], size = sizeof (dest))
{
    new n = Goods[good][gdStars];
    format(dest, size, C_BRAND);
    for (new i = 0; i < n; i++) strcat(dest, "*", size);
    strcat(dest, "{555555}", size);
    for (new i = n; i < 5; i++) strcat(dest, "*", size);
}

// "1.2 т"
stock FormatTons(kg, dest[], size = sizeof (dest))
{
    format(dest, size, "%d.%d т", kg / 1000, (kg % 1000) / 100);
}
