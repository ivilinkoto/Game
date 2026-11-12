// Обработка на дарения.
//
// Как работи:
//   1. Уеб магазинът (сайтът) приема плащане през платежен доставчик.
//   2. След потвърдено плащане сайтът вмъква ред в таблица donations:
//        INSERT INTO donations (account_name, coins, provider, transaction_id, created_at)
//        VALUES ('Ivan_Petrov', 500, 'stripe', 'tx_123', strftime('%s','now'));
//   3. Сървърът проверява всяка минута и зачислява монетите.
//
// Сайтът НИКОГА не пипа accounts директно – така няма конфликти
// с данните на играчи, които са в играта.

#define MAX_DONATIONS_PER_RUN 20

// Намира онлайн играч по име на акаунт
stock Donations_FindOnline(const name[])
{
    for (new i = 0; i < MAX_PLAYERS; i++) {
        if (!IsPlayerConnected(i) || !PlayerData[i][pLogged]) continue;
        if (!strcmp(PlayerData[i][pName], name, true)) return i;
    }
    return INVALID_PLAYER_ID;
}

stock Donations_Process()
{
    new DBResult:result = Database_Query(
        "SELECT id, account_name, coins FROM donations WHERE processed = 0 ORDER BY id LIMIT 20");
    if (result == DBResult:0) return 0;

    // Първо четем всичко, после обработваме (за да не държим резултата отворен).
    new count = 0,
        ids[MAX_DONATIONS_PER_RUN],
        coins[MAX_DONATIONS_PER_RUN],
        names[MAX_DONATIONS_PER_RUN][MAX_PLAYER_NAME + 1];

    if (DB_GetRowCount(result) > 0) {
        do {
            ids[count] = DB_GetFieldIntByName(result, "id");
            coins[count] = DB_GetFieldIntByName(result, "coins");
            DB_GetFieldStringByName(result, "account_name", names[count], MAX_PLAYER_NAME + 1);
            count++;
        } while (count < MAX_DONATIONS_PER_RUN && DB_SelectNextRow(result));
    }
    DB_FreeResultSet(result);

    for (new i = 0; i < count; i++) {
        Donations_Credit(ids[i], names[i], coins[i]);
    }
    return count;
}

// Извиква се при вход – веднага зачислява чакащите дарения.
stock Donations_ProcessForPlayer(playerid)
{
    #pragma unused playerid
    Donations_Process();
}

stock Donations_Credit(donationId, const name[], amount)
{
    new eName[64], q[256];
    SQL_Escape(name, eName);

    // Отбелязваме като обработено ПРЕДИ зачисляване, за да няма двойно зачисляване.
    format(q, sizeof (q),
        "UPDATE donations SET processed = 1, processed_at = %d WHERE id = %d AND processed = 0",
        gettime(), donationId);
    Database_Exec(q);

    if (amount <= 0) return 0;

    new playerid = Donations_FindOnline(name);
    if (playerid != INVALID_PLAYER_ID) {
        PlayerData[playerid][pCoins] += amount;
        Accounts_Save(playerid, false);
        SendClientMessage(playerid, COLOR_VIP, "[Дарение] Получи %d монети! Благодарим ти за подкрепата. Виж /shop.", amount);
    } else {
        // Проверяваме дали акаунтът съществува
        format(q, sizeof (q), "SELECT id FROM accounts WHERE name = '%s' LIMIT 1", eName);
        new DBResult:result = Database_Query(q), bool:exists = false;
        if (result != DBResult:0) {
            exists = (DB_GetRowCount(result) > 0);
            DB_FreeResultSet(result);
        }

        if (!exists) {
            format(q, sizeof (q), "UPDATE donations SET processed = 2 WHERE id = %d", donationId);
            Database_Exec(q);
            printf("[ДАРЕНИЕ] #%d: няма акаунт '%s' - изисква ръчна проверка!", donationId, name);
            return 0;
        }

        format(q, sizeof (q), "UPDATE accounts SET coins = coins + %d WHERE name = '%s'", amount, eName);
        Database_Exec(q);
    }

    new details[64];
    format(details, sizeof (details), "дарение #%d: %d монети", donationId, amount);
    Log_Add("donation", "system", name, details);
    printf("[ДАРЕНИЕ] #%d: %d монети за %s", donationId, amount, name);
    return 1;
}
