// Промокодове при регистрация.
//
// Засега се създават директно в базата (на VPS-а):
//   sqlite3 /home/omp/Server/scriptfiles/hotfix.db \
//     "INSERT INTO promo_codes (code, money, max_uses) VALUES ('START2026', 10000, 100);"
// По-късно ще има админ команда и раздел в сайта.

// Проверява кода. Връща true и наградата в money, ако е валиден.
stock bool:Promo_Check(const code[], &money)
{
    money = 0;
    if (!IsAlphaNumeric(code) || strlen(code) > 32) return false;

    new q[256];
    format(q, sizeof (q),
        "SELECT money FROM promo_codes WHERE code = '%s' AND active = 1 AND (max_uses = 0 OR uses < max_uses) AND (expires = 0 OR expires > %d) LIMIT 1",
        code, gettime());

    new DBResult:result = Database_Query(q), bool:valid = false;
    if (result != DBResult:0) {
        if (DB_GetRowCount(result) > 0) {
            money = DB_GetFieldIntByName(result, "money");
            valid = true;
        }
        DB_FreeResultSet(result);
    }
    return valid;
}

// Отбелязва използване на кода.
stock Promo_Use(const code[])
{
    if (!IsAlphaNumeric(code)) return;
    new q[128];
    format(q, sizeof (q), "UPDATE promo_codes SET uses = uses + 1 WHERE code = '%s'", code);
    Database_Exec(q);
}
