// Пазар на стоки (суровини). Фермерите продават тук реколтата си.
// Колкото повече има на склад, толкова по-ниска е изкупната цена.
// Градът постепенно „изяжда“ склада, а по-късно пицарии и хот-дог колички
// ще купуват оттук суровините си – така работите са свързани.

enum {
    GOODS_WHEAT,
    GOODS_POTATO,
    GOODS_TOMATO,
    MAX_GOODS
}

enum E_GOODS {
    gdName[24],
    gdKey[16],          // ключ в базата
    gdBasePrice,        // базова цена за кг
    gdCapacity          // при толкова кг на склад цената е минимална
}

static const GoodsInfo[MAX_GOODS][E_GOODS] = {
    {"Пшеница", "wheat",  12, 3000},
    {"Картофи", "potato", 20, 2000},
    {"Домати",  "tomato", 35, 1500}
};

#define MARKET_CONSUME_MINUTES  10      // на всеки толкова минути градът изразходва
#define MARKET_CONSUME_PERCENT  4       // ...толкова процента от склада

static gGoodsStock[MAX_GOODS];

stock Market_Init()
{
    Database_Exec("CREATE TABLE IF NOT EXISTS market_goods (goods_key TEXT PRIMARY KEY, stock INTEGER NOT NULL DEFAULT 0)");
    new q[128];
    for (new i = 0; i < MAX_GOODS; i++) {
        gGoodsStock[i] = 0;
        format(q, sizeof (q), "SELECT stock FROM market_goods WHERE goods_key = '%s'", GoodsInfo[i][gdKey]);
        new DBResult:result = Database_Query(q);
        if (result != DBResult:0) {
            if (DB_GetRowCount(result) > 0) gGoodsStock[i] = DB_GetFieldIntByName(result, "stock");
            DB_FreeResultSet(result);
        }
    }
}

stock Market_Save(goods)
{
    new q[160];
    format(q, sizeof (q), "INSERT OR REPLACE INTO market_goods (goods_key, stock) VALUES ('%s', %d)", GoodsInfo[goods][gdKey], gGoodsStock[goods]);
    Database_Exec(q);
}

stock Market_GetName(goods, dest[], size = sizeof (dest))
{
    format(dest, size, "%s", GoodsInfo[goods][gdName]);
}

stock Market_GetStock(goods)
{
    return gGoodsStock[goods];
}

// Изкупна цена за кг: от 130% (празен склад) до 50% (пълен склад) от базовата.
stock Market_GetPrice(goods)
{
    new base = GoodsInfo[goods][gdBasePrice], cap = GoodsInfo[goods][gdCapacity];
    new stock = gGoodsStock[goods];
    if (stock > cap) stock = cap;
    new percent = 130 - (80 * stock / cap);
    new price = base * percent / 100;
    return (price < 1) ? 1 : price;
}

stock Market_Add(goods, kg)
{
    gGoodsStock[goods] += kg;
    Market_Save(goods);
}

// Взима от склада (за бъдещите бизнеси). Връща колко реално е взето.
stock Market_Take(goods, kg)
{
    if (kg > gGoodsStock[goods]) kg = gGoodsStock[goods];
    gGoodsStock[goods] -= kg;
    Market_Save(goods);
    return kg;
}

stock Market_OnTick(tick)
{
    if (tick % (MARKET_CONSUME_MINUTES * 60) != 0) return;
    for (new i = 0; i < MAX_GOODS; i++) {
        if (gGoodsStock[i] <= 0) continue;
        new used = gGoodsStock[i] * MARKET_CONSUME_PERCENT / 100;
        if (used < 1) used = 1;
        Market_Take(i, used);
    }
}
