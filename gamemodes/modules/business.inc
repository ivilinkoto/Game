// Компаниите като бизнеси: покупка, панел на собственика, продажба.
//
// ALT пеша в базата на компания (или GPS > Моето имущество > Моята компания отвсякъде, ако имаш).
// - Без собственик: инфо + "Купи" (от ниво 3, една компания на играч).
// - Твоя: каса, служители, молби, покани, облик, условия (firm.inc), бонус за шофьорите, статистика, продажба (60%).
// - Чужда (има собственик): инфо + "Кандидатствай" за работа; ако работиш там - панелът на служителя.
// - 14 дни без вход -> компанията се връща на държавата, собственикът получава 60% + касата.

#define BUSINESS_MIN_LEVEL      3
#define BUSINESS_SELL_PERCENT   60
#define BUSINESS_INACTIVE_DAYS  14

static const BusinessBonusSteps[] = {0, 5, 10, 20};

static BizMenuCompany[MAX_PLAYERS];

// Компанията на играча (или -1)
stock Business_GetOwned(playerid)
{
    if (!PlayerData[playerid][pLogged]) return -1;
    for (new c = 0; c < MAX_COMPANIES; c++) {
        if (Company[c][cpOwner] == PlayerData[playerid][pID]) return c;
    }
    return -1;
}

static stock Business_SizeName(c, dest[], size = sizeof (dest))
{
    switch (CompanyDefs[c][cdSize]) {
        case COMPANY_SMALL:  format(dest, size, "малка");
        case COMPANY_MEDIUM: format(dest, size, "средна");
        default:             format(dest, size, "голяма");
    }
}

// ---------- Прозорци ----------

stock Business_Show(playerid, c)
{
    BizMenuCompany[playerid] = c;
    if (Company[c][cpOwner] == PlayerData[playerid][pID]) {
        Business_ShowOwnerPanel(playerid, c);
    } else if (Company[c][cpOwner] != 0 && Firm_GetMemberCompany(playerid) == c) {
        Firm_ShowMemberPanel(playerid);
    } else {
        Business_ShowInfo(playerid, c);
    }
}

static stock Business_ShowInfo(playerid, c)
{
    new price[16], sizeName[16], produces[320], accepts[320], owner[96], status[160], button[16];
    FormatMoney(Company_GetPrice(c), price);
    if (Company[c][cpOwner] == 0) format(button, sizeof (button), "Купи");
    else if (Firm_GetCompany(playerid) == -1) format(button, sizeof (button), "Кандидатствай");
    else format(button, sizeof (button), "Добре");
    Business_SizeName(c, sizeName);
    Economy_GoodsLine(c, CompanyDefs[c][cdProduces], false, produces);
    Economy_GoodsLine(c, CompanyDefs[c][cdAccepts], true, accepts);

    if (Company[c][cpOwner] != 0) {
        format(owner, sizeof (owner), C_WHITE "Собственик: " C_BRAND "%s", Company[c][cpOwnerName]);
        new salary[16];
        FormatMoney(Company[c][cpSalary], salary);
        format(status, sizeof (status), C_GREY "Не се продава. Наема шофьори: %d/%d места, заплата $%s, комисионна %d%%.",
            Firm_CountMembers(c), Firm_Slots(c), salary, Company[c][cpCommission]);
    } else {
        format(owner, sizeof (owner), C_WHITE "Собственик: " C_GREEN "няма (държавна)");
        if (PlayerData[playerid][pLevel] < BUSINESS_MIN_LEVEL) {
            format(status, sizeof (status), C_RED "Компания се купува от ниво %d.", BUSINESS_MIN_LEVEL);
        } else if (Business_GetOwned(playerid) != -1) {
            format(status, sizeof (status), C_RED "Вече имаш компания (една на играч).");
        } else if (Money_Get(playerid) < Company_GetPrice(c)) {
            format(status, sizeof (status), C_RED "Нямаш достатъчно пари.");
        } else {
            format(status, sizeof (status), C_GREEN "Можеш да я купиш.");
        }
    }

    ShowPlayerDialog(playerid, DIALOG_BIZ_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "Компания",
        C_BRAND "%s" C_GREY " (%s, %s)\n\n" \
        "%s\n" \
        C_WHITE "Цена: " C_GREEN "$%s\n" \
        C_WHITE "Дава: " C_GREEN "%s\n" \
        C_WHITE "Приема: " C_BRAND "%s\n" \
        C_WHITE "Курсове досега: %d\n\n" \
        C_GREY "Собственикът получава %d%% от всеки курс от/до компанията.\n\n%s",
        button, "Затвори",
        CompanyDefs[c][cdName], CompanyDefs[c][cdPlace], sizeName, owner, price, produces, accepts,
        Company[c][cpDeliveries], COMPANY_COMMISSION, status);
}

// Редове: 0-5 фирмата (firm.inc), 6 бонус, 7 статистика, 8 продажба
static stock Business_ShowOwnerPanel(playerid, c)
{
    new earned[16], body[1024], line[160];
    body[0] = EOS;
    Firm_ShowOwnerExtras(playerid, c, body, sizeof (body));
    FormatMoney(Company[c][cpEarned], earned);
    format(line, sizeof (line), "Бонус за всички шофьори\t" C_BRAND "%d%%\t" C_GREY "смени\n", Company[c][cpBonus]);
    strcat(body, line);
    format(line, sizeof (line), "Статистика\t%d курса\t" C_GREEN "$%s общо\n", Company[c][cpDeliveries], earned);
    strcat(body, line);
    strcat(body, C_RED "Продай компанията\t\t");
    new title[96], col[12];
    Firm_ColorTag(c, col);
    format(title, sizeof (title), "%s%s" C_WHITE " | собственик", col, CompanyDefs[c][cdName]);
    ShowPlayerDialog(playerid, DIALOG_BIZ_OWNER, DIALOG_STYLE_TABLIST, title, "%s", "Избери", "Затвори", body);
}

static stock Business_ShowBonus(playerid, c)
{
    new body[400], line[128], now[16];
    body[0] = EOS;
    for (new i = 0; i < sizeof (BusinessBonusSteps); i++) {
        now[0] = EOS;
        if (Company[c][cpBonus] == BusinessBonusSteps[i]) format(now, sizeof (now), " (сега)");
        if (BusinessBonusSteps[i] == 0) format(line, sizeof (line), "Без бонус%s\n", now);
        else format(line, sizeof (line), "+%d%% към плащането%s\n", BusinessBonusSteps[i], now);
        strcat(body, line);
    }
    ShowPlayerDialog(playerid, DIALOG_BIZ_BONUS, DIALOG_STYLE_LIST, C_BRAND "Бонус за шофьорите", "%s", "Избери", "Назад", body);
}

// ---------- Действия ----------

static stock Business_Buy(playerid, c)
{
    if (Company[c][cpOwner] != 0) return SendError(playerid, "Компанията вече има собственик.");
    if (PlayerData[playerid][pLevel] < BUSINESS_MIN_LEVEL) return SendError(playerid, "Нивото ти не стига (нужно ниво 3).");
    if (Business_GetOwned(playerid) != -1) return SendError(playerid, "Вече имаш компания.");
    new price = Company_GetPrice(c);
    if (!Money_Take(playerid, price)) return SendError(playerid, "Нямаш достатъчно пари.");

    Company[c][cpOwner] = PlayerData[playerid][pID];
    format(Company[c][cpOwnerName], MAX_PLAYER_NAME + 1, "%s", PlayerData[playerid][pName]);
    Company[c][cpCash] = 0;
    Company[c][cpBonus] = 0;
    Company_ResetFirm(c);
    Company_Save(c);
    Firm_OnBecameOwner(playerid);
    new q[96];
    format(q, sizeof (q), "UPDATE companies SET owner_since = %d WHERE id = %d", gettime(), c);
    Database_Exec(q);
    Company_CreateMarkers(c);

    PlayerPlaySound(playerid, 1058, 0.0, 0.0, 0.0);
    GameTextForPlayer(playerid, "~g~COMPANY PURCHASED!", 4000, 1);
    SendClientMessageToAll(COLOR_BRAND, "[CARGO] " C_WHITE "%s купи компанията " C_BRAND "%s" C_WHITE "!", PlayerData[playerid][pName], CompanyDefs[c][cdName]);
    Business_ShowOwnerPanel(playerid, c);
    return 1;
}

// Връща компанията на държавата. ownerOnline: играчът (или INVALID_PLAYER_ID, ако е офлайн).
static stock Business_Release(c, ownerOnline)
{
    new payout = Company_GetPrice(c) * BUSINESS_SELL_PERCENT / 100 + Company[c][cpCash];
    if (ownerOnline != INVALID_PLAYER_ID) {
        Money_Give(ownerOnline, payout);
    } else {
        new q[128];
        format(q, sizeof (q), "UPDATE accounts SET money = money + %d WHERE id = %d", payout, Company[c][cpOwner]);
        Database_Exec(q);
    }
    Firm_OnCompanyReleased(c);
    Company_ResetFirm(c);
    Company[c][cpOwner] = 0;
    Company[c][cpOwnerName][0] = EOS;
    Company[c][cpCash] = 0;
    Company[c][cpBonus] = 0;
    Company_Save(c);
    Company_CreateMarkers(c);
    return payout;
}

static stock Business_AskSell(playerid, c)
{
    new value[16];
    FormatMoney(Company_GetPrice(c) * BUSINESS_SELL_PERCENT / 100 + Company[c][cpCash], value);
    ShowPlayerDialog(playerid, DIALOG_BIZ_SELL, DIALOG_STYLE_MSGBOX, C_BRAND "Продажба",
        C_WHITE "Да продам ли " C_BRAND "%s" C_WHITE " на държавата?\n\n" \
        "Получаваш " C_GREEN "$%s" C_WHITE " (%d%% от цената + парите в касата).",
        "Продай", "Назад", CompanyDefs[c][cdName], value, BUSINESS_SELL_PERCENT);
}

// Неактивни собственици (вика се при старт и на всеки час)
stock Business_CheckInactive()
{
    new limit = gettime() - BUSINESS_INACTIVE_DAYS * 86400;
    for (new c = 0; c < MAX_COMPANIES; c++) {
        if (Company[c][cpOwner] == 0) continue;
        // Онлайн ли е собственикът?
        new bool:online = false;
        for (new i = 0; i < MAX_PLAYERS; i++) {
            if (IsPlayerLogged(i) && PlayerData[i][pID] == Company[c][cpOwner]) {
                online = true;
                break;
            }
        }
        if (online) continue;

        new q[96], lastLogin = 0;
        format(q, sizeof (q), "SELECT last_login FROM accounts WHERE id = %d", Company[c][cpOwner]);
        new DBResult:r = Database_Query(q);
        if (r != DBResult:0) {
            if (DB_GetRowCount(r) > 0) lastLogin = DB_GetFieldIntByName(r, "last_login");
            DB_FreeResultSet(r);
        }
        if (lastLogin < limit) {
            printf("[BIZ] %s returned to the state (owner %d inactive).", CompanyDefs[c][cdName], Company[c][cpOwner]);
            Business_Release(c, INVALID_PLAYER_ID);
        }
    }
}

// ---------- Събития ----------

stock Business_OnKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
{
    if (!PlayerData[playerid][pLogged] || IsPlayerInAnyVehicle(playerid) || PlayerData[playerid][pInGarage]) return 0;
    if (!((newkeys & KEY_WALK) && !(oldkeys & KEY_WALK))) return 0;
    new c = Company_GetNear(playerid);
    if (c == -1) return 0;
    Business_Show(playerid, c);
    return 1;
}

stock Business_OnDialogResponse(playerid, dialogid, response, listitem)
{
    new c = BizMenuCompany[playerid];
    if (c < 0 || c >= MAX_COMPANIES) return 0;

    switch (dialogid) {
        case DIALOG_BIZ_INFO: {
            if (!response) return 1;
            if (Company[c][cpOwner] == 0) Business_Buy(playerid, c);
            else if (Firm_GetCompany(playerid) == -1) Firm_Apply(playerid, c);
            return 1;
        }
        case DIALOG_BIZ_OWNER: {
            if (!response || Company[c][cpOwner] != PlayerData[playerid][pID]) return 1;
            switch (listitem) {
                case 0, 1, 2, 3, 4, 5: Firm_OwnerPick(playerid, c, listitem);
                case 6: Business_ShowBonus(playerid, c);
                case 7: Business_ShowOwnerPanel(playerid, c);
                case 8: Business_AskSell(playerid, c);
            }
            return 1;
        }
        case DIALOG_BIZ_BONUS: {
            if (Company[c][cpOwner] != PlayerData[playerid][pID]) return 1;
            if (response && listitem >= 0 && listitem < sizeof (BusinessBonusSteps)) {
                Company[c][cpBonus] = BusinessBonusSteps[listitem];
                Company_Save(c);
                if (Company[c][cpBonus] > 0) {
                    SendClientMessage(playerid, COLOR_BRAND, "[КОМПАНИЯ] " C_WHITE "Бонусът за шофьорите е " C_BRAND "+%d%%" C_WHITE ". Плаща се от касата, докато има пари в нея.", Company[c][cpBonus]);
                } else {
                    SendClientMessage(playerid, COLOR_BRAND, "[КОМПАНИЯ] " C_WHITE "Бонусът за шофьорите е изключен.");
                }
            }
            Business_ShowOwnerPanel(playerid, c);
            return 1;
        }
        case DIALOG_BIZ_SELL: {
            if (!response) {
                Business_ShowOwnerPanel(playerid, c);
                return 1;
            }
            if (Company[c][cpOwner] != PlayerData[playerid][pID]) return 1;
            new payout = Business_Release(c, playerid), money[16];
            FormatMoney(payout, money);
            SendClientMessage(playerid, COLOR_BRAND, "[КОМПАНИЯ] " C_WHITE "Продаде " C_BRAND "%s" C_WHITE " за " C_GREEN "$%s" C_WHITE ".", CompanyDefs[c][cdName], money);
            return 1;
        }
    }
    return 0;
}

// GPS > Моето имущество > Моята компания (отвсякъде)
stock Business_OpenOwn(playerid)
{
    new c = Business_GetOwned(playerid);
    if (c == -1 && Firm_GetMemberCompany(playerid) != -1) {
        Firm_ShowMemberPanel(playerid);
        return;
    }
    if (c == -1) {
        ShowPlayerDialog(playerid, DIALOG_MENU_INFO, DIALOG_STYLE_MSGBOX, C_BRAND "Моята компания",
            C_WHITE "Нямаш компания.\n\nКомпаниите се купуват от ниво %d: отиди пеша до базата на\nкомпания без собственик и натисни " C_BRAND "ALT" C_WHITE ".\n\n" \
            C_WHITE "Или работи за някого: ALT на базата на компания със собственик -> " C_BRAND "Кандидатствай" C_WHITE ".",
            "Добре", "", BUSINESS_MIN_LEVEL);
        return;
    }
    Business_Show(playerid, c);
}

CMD:company(playerid, params[])
{
    #pragma unused params
    Business_OpenOwn(playerid);
    return 1;
}
