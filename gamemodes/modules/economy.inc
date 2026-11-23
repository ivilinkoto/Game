// Жива икономика: складове на компаниите, производство, потребление, търсене и цени.
//
// - Всяка компания има склад (кг) за всеки товар, който дава или приема.
// - Производителите произвеждат всяка минута. Ако приемат и суровини (напр. леярната - руда),
//   с налични суровини произвеждат x1.5 (и ги изразходват), без тях - x0.5.
// - Компаниите, които само приемат (магазини, строеж, бензиностанции), изразходват всяка минута.
// - Поръчка излиза само ако в склада на А има товара; при приемане товарът се взема от склада,
//   при доставка влиза в склада на Б. Пълен склад не приема.
// - Плащането следва търсенето при получателя: празен склад до +60%, пълен до -30%.

#define ECONOMY_TICK_SECONDS    60
#define ECONOMY_FULL_PERCENT    95          // над толкова % пълнота компанията не приема

// Капацитет на склада за един товар (кг) според размера на компанията
static const EconomyCapacity[] = {60000, 100000, 200000};
// Производство на минута за всеки произвеждан товар (кг)
static const EconomyProduction[] = {133, 250, 500};
// Потребление на минута за всеки приеман товар при чистите потребители (кг)
static const EconomyConsumption[] = {100, 200, 400};

new CompanyStock[MAX_COMPANIES][MAX_GOODS];

stock Economy_Capacity(c)
{
    return EconomyCapacity[CompanyDefs[c][cdSize]];
}

// Пълнота 0-100 %
stock Economy_FillPercent(c, good)
{
    new cap = Economy_Capacity(c);
    if (cap <= 0) return 0;
    new p = CompanyStock[c][good] * 100 / cap;
    return (p > 100) ? 100 : p;
}

// Множител на плащането според търсенето при получателя (в %): 160 при празен склад, 70 при пълен
stock Economy_DemandPercent(c, good)
{
    return 160 - 90 * Economy_FillPercent(c, good) / 100;
}

stock bool:Economy_CanAccept(c, good)
{
    return (Company_Accepts(c, good) && Economy_FillPercent(c, good) < ECONOMY_FULL_PERCENT);
}

stock bool:Economy_HasStock(c, good, kg)
{
    return (CompanyStock[c][good] >= kg);
}

stock Economy_Take(c, good, kg)
{
    CompanyStock[c][good] -= kg;
    if (CompanyStock[c][good] < 0) CompanyStock[c][good] = 0;
}

stock Economy_Put(c, good, kg)
{
    CompanyStock[c][good] += kg;
    new cap = Economy_Capacity(c);
    if (CompanyStock[c][good] > cap) CompanyStock[c][good] = cap;
}

// Нова компания: начални наличности (както при старта)
stock Economy_OnCompanyCreated(c)
{
    new cap = Economy_Capacity(c);
    for (new g = 0; g < MAX_GOODS; g++) {
        if (Company_Produces(c, g)) CompanyStock[c][g] = cap / 2;
        else if (Company_Accepts(c, g)) CompanyStock[c][g] = cap * 2 / 5;
        else CompanyStock[c][g] = 0;
    }
}

stock Economy_ClearCompany(c)
{
    for (new g = 0; g < MAX_GOODS; g++) CompanyStock[c][g] = 0;
}

// ---------- Начало и запис ----------

stock Economy_Init()
{
    Database_Exec("CREATE TABLE IF NOT EXISTS economy_stock (company INTEGER NOT NULL, good INTEGER NOT NULL, kg INTEGER NOT NULL DEFAULT 0, PRIMARY KEY (company, good))");

    // Начални наличности: 50% за произвежданото, 40% за приеманото
    for (new c = 0; c < MAX_COMPANIES; c++) {
        new cap = Economy_Capacity(c);
        for (new g = 0; g < MAX_GOODS; g++) {
            if (Company_Produces(c, g)) CompanyStock[c][g] = cap / 2;
            else if (Company_Accepts(c, g)) CompanyStock[c][g] = cap * 2 / 5;
            else CompanyStock[c][g] = 0;
        }
    }

    // Записаните наличности (ако има)
    new DBResult:r = Database_Query("SELECT company, good, kg FROM economy_stock");
    if (r != DBResult:0) {
        new rows = DB_GetRowCount(r);
        for (new i = 0; i < rows; i++) {
            new c = DB_GetFieldIntByName(r, "company"), g = DB_GetFieldIntByName(r, "good");
            if (c >= 0 && c < MAX_COMPANIES && g >= 0 && g < MAX_GOODS && (Company_Produces(c, g) || Company_Accepts(c, g))) {
                CompanyStock[c][g] = DB_GetFieldIntByName(r, "kg");
            }
            DB_SelectNextRow(r);
        }
        DB_FreeResultSet(r);
    }
    Economy_RefreshLabels();
}

stock Economy_Save()
{
    new q[160];
    Database_Exec("BEGIN TRANSACTION");
    for (new c = 0; c < MAX_COMPANIES; c++) {
        for (new g = 0; g < MAX_GOODS; g++) {
            if (!Company_Produces(c, g) && !Company_Accepts(c, g)) continue;
            format(q, sizeof (q), "INSERT OR REPLACE INTO economy_stock (company, good, kg) VALUES (%d, %d, %d)", c, g, CompanyStock[c][g]);
            Database_Exec(q);
        }
    }
    Database_Exec("COMMIT");
}

// ---------- Всяка минута ----------

stock Economy_Tick()
{
    for (new c = 0; c < MAX_COMPANIES; c++) {
        new size = CompanyDefs[c][cdSize];
        new produces = CompanyDefs[c][cdProduces], accepts = CompanyDefs[c][cdAccepts];

        if (produces != 0) {
            // Суровини: всичко, което компанията приема
            new inputKg = 0;
            for (new g = 0; g < MAX_GOODS; g++) {
                if (accepts & GOOD_BIT(g)) inputKg += CompanyStock[c][g];
            }
            new base = EconomyProduction[size], made;
            if (accepts == 0) {
                made = base;                            // внос / добив - без суровини
            } else if (inputKg > 0) {
                made = base * 3 / 2;                    // с суровини x1.5
                // Изразходва суровини: половината от произведеното, по равно
                new need = base / 2, kinds = 0;
                for (new g = 0; g < MAX_GOODS; g++) {
                    if ((accepts & GOOD_BIT(g)) && CompanyStock[c][g] > 0) kinds++;
                }
                for (new g = 0; g < MAX_GOODS && kinds > 0; g++) {
                    if ((accepts & GOOD_BIT(g)) && CompanyStock[c][g] > 0) Economy_Take(c, g, need / kinds);
                }
            } else {
                made = base / 2;                        // без суровини x0.5
            }
            for (new g = 0; g < MAX_GOODS; g++) {
                if (produces & GOOD_BIT(g)) Economy_Put(c, g, made);
            }
        } else {
            // Чист потребител: изразходва всичко, което приема
            for (new g = 0; g < MAX_GOODS; g++) {
                if (accepts & GOOD_BIT(g)) Economy_Take(c, g, EconomyConsumption[size]);
            }
        }
    }
    Economy_RefreshLabels();
}

// ---------- Надписи ----------

// "пратки 60%, медикаменти 40%" (за приеманите: червено при голямо търсене)
stock Economy_GoodsLine(c, mask, bool:accepting, dest[], size = sizeof (dest))
{
    new part[64];
    dest[0] = EOS;
    for (new g = 0; g < MAX_GOODS; g++) {
        if (!(mask & GOOD_BIT(g))) continue;
        new fill = Economy_FillPercent(c, g);
        if (accepting) {
            if (fill < 25) format(part, sizeof (part), "%s " C_RED "%d%%" C_WHITE, Goods[g][gdName], fill);
            else if (fill >= ECONOMY_FULL_PERCENT) format(part, sizeof (part), "%s " C_GREY "пълен" C_WHITE, Goods[g][gdName]);
            else format(part, sizeof (part), "%s %d%%", Goods[g][gdName], fill);
        } else {
            format(part, sizeof (part), "%s %d%%", Goods[g][gdName], fill);
        }
        if (dest[0] != EOS) strcat(dest, ", ", size);
        strcat(dest, part, size);
    }
    if (dest[0] == EOS) format(dest, size, "-");
}

stock Economy_RefreshLabels()
{
    for (new c = 0; c < MAX_COMPANIES; c++) Company_CreateMarkers(c);
}

// ---------- Борса: къде кой товар е най-търсен ----------

stock Economy_ShowMarket(playerid)
{
    Tutorial_OnCareer(playerid);        // увод, задача 7
    new body[2400], line[200];
    format(body, sizeof (body), "Товар\tНай-търсен в\tПлащане\tНаличност\n");
    for (new g = 0; g < MAX_GOODS; g++) {
        // Най-голямо търсене
        new bestC = -1, bestPct = 0;
        for (new c = 0; c < MAX_COMPANIES; c++) {
            if (!Economy_CanAccept(c, g)) continue;
            new pct = Economy_DemandPercent(c, g);
            if (pct > bestPct) {
                bestPct = pct;
                bestC = c;
            }
        }
        // Обща наличност при производителите
        new supply = 0;
        for (new c = 0; c < MAX_COMPANIES; c++) {
            if (Company_Produces(c, g)) supply += CompanyStock[c][g];
        }
        if (bestC == -1) {
            format(line, sizeof (line), "%s\t" C_GREY "никъде не трябва\t-\t%d т\n", Goods[g][gdName], supply / 1000);
        } else {
            new diff = bestPct - 100;
            if (diff >= 0) format(line, sizeof (line), "%s\t%s\t" C_GREEN "+%d%%\t" C_WHITE "%d т\n", Goods[g][gdName], CompanyDefs[bestC][cdName], diff, supply / 1000);
            else format(line, sizeof (line), "%s\t%s\t" C_RED "%d%%\t" C_WHITE "%d т\n", Goods[g][gdName], CompanyDefs[bestC][cdName], diff, supply / 1000);
        }
        strcat(body, line);
    }
    ShowPlayerDialog(playerid, DIALOG_MARKET, DIALOG_STYLE_TABLIST_HEADERS, C_BRAND "Борса на товарите", "%s", "Добре", "", body);
}

CMD:market(playerid, params[])
{
    #pragma unused params
    Economy_ShowMarket(playerid);
    return 1;
}
