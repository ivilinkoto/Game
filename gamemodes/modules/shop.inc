// Магазин за монети (coins). Монетите идват от дарения (виж donations.inc).
// За да добавиш продукт – добави ред в ShopItems и обработка в Shop_GiveItem.

enum E_SHOP_ITEM {
    siName[48],
    siPrice,
    siVipLevel,
    siDays
}

static const ShopItems[][E_SHOP_ITEM] = {
    {"VIP Bronze - 30 дни", 200, VIP_BRONZE, 30},
    {"VIP Silver - 30 дни", 350, VIP_SILVER, 30},
    {"VIP Gold - 30 дни",   500, VIP_GOLD,   30}
};

CMD:shop(playerid, params[])
{
    #pragma unused params
    new body[512], line[96];
    body[0] = EOS;
    for (new i = 0; i < sizeof (ShopItems); i++) {
        format(line, sizeof (line), "%s  {FFD700}(%d монети)\n", ShopItems[i][siName], ShopItems[i][siPrice]);
        strcat(body, line);
    }
    new title[64];
    format(title, sizeof (title), "Магазин (имаш %d монети)", PlayerData[playerid][pCoins]);
    ShowPlayerDialog(playerid, DIALOG_SHOP, DIALOG_STYLE_LIST, title, "%s", "Избери", "Затвори", body);
    return 1;
}

CMD:donate(playerid, params[])
{
    #pragma unused params
    SendClientMessage(playerid, COLOR_VIP, "Подкрепи сървъра и получи монети за /shop:");
    SendClientMessage(playerid, COLOR_WHITE, "Посети {FFC400}%s{FFFFFF} и въведи името си: {FFC400}%s", SERVER_WEBSITE, PlayerData[playerid][pName]);
    SendClientMessage(playerid, COLOR_GREY, "Монетите пристигат автоматично до 1 минута.");
    return 1;
}

CMD:coins(playerid, params[])
{
    #pragma unused params
    SendClientMessage(playerid, COLOR_VIP, "Имаш %d монети. Виж /shop и /donate.", PlayerData[playerid][pCoins]);
    return 1;
}

stock Shop_OnDialogResponse(playerid, dialogid, response, listitem, const inputtext[])
{
    #pragma unused inputtext
    switch (dialogid) {
        case DIALOG_SHOP: {
            if (!response) return 1;
            if (listitem < 0 || listitem >= sizeof (ShopItems)) return 1;

            PlayerData[playerid][pShopItem] = listitem;
            new body[256];
            format(body, sizeof (body),
                "{FFFFFF}Сигурен ли си, че искаш да купиш:\n\n{FFC400}%s{FFFFFF}\n\nЦена: {FFD700}%d монети{FFFFFF}\nТвоите монети: {FFD700}%d",
                ShopItems[listitem][siName], ShopItems[listitem][siPrice], PlayerData[playerid][pCoins]);
            ShowPlayerDialog(playerid, DIALOG_SHOP_CONFIRM, DIALOG_STYLE_MSGBOX, "Потвърждение", "%s", "Купи", "Отказ", body);
            return 1;
        }

        case DIALOG_SHOP_CONFIRM: {
            new item = PlayerData[playerid][pShopItem];
            PlayerData[playerid][pShopItem] = -1;
            if (!response || item < 0 || item >= sizeof (ShopItems)) return 1;

            Shop_Buy(playerid, item);
            return 1;
        }
    }
    return 0;
}

stock Shop_Buy(playerid, item)
{
    new price = ShopItems[item][siPrice];
    if (PlayerData[playerid][pCoins] < price) {
        SendError(playerid, "Нямаш достатъчно монети. Виж /donate.");
        return 0;
    }

    // Не позволяваме по-ниско VIP ниво да замени активно по-високо.
    new level = ShopItems[item][siVipLevel];
    if (level > VIP_NONE && Vip_IsActive(playerid) && PlayerData[playerid][pVip] > level) {
        SendError(playerid, "Вече имаш по-високо VIP ниво. Изчакай да изтече или купи същото/по-високо.");
        return 0;
    }

    PlayerData[playerid][pCoins] -= price;
    if (level > VIP_NONE) {
        Vip_Give(playerid, level, ShopItems[item][siDays]);
    }
    Accounts_Save(playerid, false);

    new details[96];
    format(details, sizeof (details), "%s за %d монети", ShopItems[item][siName], price);
    Log_Add("shop", PlayerData[playerid][pName], "", details);

    SendClientMessage(playerid, COLOR_VIP, "Покупката е успешна: %s. Остават ти %d монети.",
        ShopItems[item][siName], PlayerData[playerid][pCoins]);
    return 1;
}
