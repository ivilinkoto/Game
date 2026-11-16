// Каталог на превозните средства: товароносимост, видове товари, ниво, цена.
//
// Товароносимостта е в кг. За влекачите (с ремарке) това е максималното тегло на ремаркето.
// Цените са за автокъщата (идва по-късно); стартовият бус е безплатен.

// Видове товари (битове)
#define CARGO_PARCEL    (1 << 0)    // пратки и колети - градски доставки
#define CARGO_BOX       (1 << 1)    // кутии и палети
#define CARGO_BULK      (1 << 2)    // насипни: пясък, руда, дърва
#define CARGO_LIQUID    (1 << 3)    // течни: гориво

enum E_VEH_CLASS {
    vcModel,
    vcName[16],
    vcPayload,          // кг
    vcCargo,            // CARGO_* битове
    bool:vcTrailer,     // влекач - товарът е в ремарке
    vcLevel,            // нужно ниво за покупка
    vcPrice
}

stock const VehCatalog[][E_VEH_CLASS] = {
    {440, "Rumpo",       1500, CARGO_PARCEL,                             false, 1,      0},
    {482, "Burrito",     1200, CARGO_PARCEL,                             false, 1,   9000},
    {498, "Boxville",    3000, CARGO_PARCEL | CARGO_BOX,                 false, 2,  18000},
    {414, "Mule",        5000, CARGO_PARCEL | CARGO_BOX,                 false, 2,  28000},
    {499, "Benson",      6000, CARGO_BOX,                                false, 3,  35000},
    {456, "Yankee",      8000, CARGO_BOX,                                false, 3,  45000},
    {455, "Flatbed",    12000, CARGO_BULK,                               false, 4,  60000},
    {403, "Linerunner", 25000, CARGO_BOX | CARGO_BULK | CARGO_LIQUID,    true,  4,  90000},
    {514, "Tanker",     30000, CARGO_BOX | CARGO_BULK | CARGO_LIQUID,    true,  5, 120000},
    {515, "Roadtrain",  40000, CARGO_BOX | CARGO_BULK | CARGO_LIQUID,    true,  5, 160000}
};

stock Catalog_Find(model)
{
    for (new i = 0; i < sizeof (VehCatalog); i++) {
        if (VehCatalog[i][vcModel] == model) return i;
    }
    return -1;
}

stock Catalog_GetPayload(model)
{
    new i = Catalog_Find(model);
    return (i == -1) ? 0 : VehCatalog[i][vcPayload];
}

stock Catalog_GetCargo(model)
{
    new i = Catalog_Find(model);
    return (i == -1) ? 0 : VehCatalog[i][vcCargo];
}

// "1.5 т"
stock Catalog_FormatPayload(model, dest[], size = sizeof (dest))
{
    new kg = Catalog_GetPayload(model);
    if (kg % 1000 == 0) format(dest, size, "%d т", kg / 1000);
    else format(dest, size, "%d.%d т", kg / 1000, (kg % 1000) / 100);
}

// "1.5 T" - за TextDraw
stock Catalog_FormatPayloadLatin(model, dest[], size = sizeof (dest))
{
    new kg = Catalog_GetPayload(model);
    if (kg % 1000 == 0) format(dest, size, "%d T", kg / 1000);
    else format(dest, size, "%d.%d T", kg / 1000, (kg % 1000) / 100);
}

// "пратки, кутии"
stock Catalog_FormatCargo(model, dest[], size = sizeof (dest))
{
    new cargo = Catalog_GetCargo(model);
    dest[0] = EOS;
    if (cargo & CARGO_PARCEL) strcat(dest, "пратки, ", size);
    if (cargo & CARGO_BOX)    strcat(dest, "кутии, ", size);
    if (cargo & CARGO_BULK)   strcat(dest, "насипни, ", size);
    if (cargo & CARGO_LIQUID) strcat(dest, "гориво, ", size);
    new len = strlen(dest);
    if (len >= 2) dest[len - 2] = EOS;
    else format(dest, size, "-");
}
