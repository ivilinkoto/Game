/*
 *  HOTFIX ONLINE – главен файл на gamemode-а
 *
 *  Кодът е в UTF-8. scripts/build.sh го конвертира към Windows-1251
 *  преди компилация, за да се вижда кирилицата в играта.
 */

#include <open.mp>

main()
{
    print("HOTFIX ONLINE gamemode loaded.");
}

public OnGameModeInit()
{
    SetGameModeText("HOTFIX ONLINE");
    AddPlayerClass(26, 1742.9, -1861.0, 13.58, 0.0);
    return 1;
}

public OnPlayerConnect(playerid)
{
    SendClientMessage(playerid, 0xFFFFFFFF, "Добре дошъл в {FF6A00}HOTFIX ONLINE{FFFFFF}! Сървърът е в разработка.");
    SendClientMessage(playerid, 0xAFAFAFFF, "Проверка на кирилицата: АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЬЮЯ абвгдежзийклмнопрстуфхцчшщъьюя");
    return 1;
}
