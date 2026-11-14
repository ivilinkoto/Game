// Динамично време. Сменя се на всеки WEATHER_CHANGE_MINUTES минути за целия сървър.
// Влияе на фермата: в жега растенията изсъхват по-бързо, дъждът ги полива.

#define WEATHER_CHANGE_MINUTES  20

#define WEATHER_KIND_SUNNY      0
#define WEATHER_KIND_CLOUDY     1
#define WEATHER_KIND_RAIN       2
#define WEATHER_KIND_HOT        3

enum E_WEATHER {
    wID,            // ID на времето в GTA
    wKind,
    wChance,        // относителна вероятност
    wName[24]
}

static const WeatherTypes[][E_WEATHER] = {
    {1,  WEATHER_KIND_SUNNY,  40, "Слънчево"},
    {4,  WEATHER_KIND_CLOUDY, 20, "Облачно"},
    {8,  WEATHER_KIND_RAIN,   15, "Дъжд"},
    {11, WEATHER_KIND_HOT,    15, "Жега"},
    {9,  WEATHER_KIND_CLOUDY, 10, "Мъгла"}
};

static gWeatherIndex;

stock Weather_Init()
{
    gWeatherIndex = 0;
    SetWeather(WeatherTypes[gWeatherIndex][wID]);
}

stock Weather_Change()
{
    new total = 0;
    for (new i = 0; i < sizeof (WeatherTypes); i++) total += WeatherTypes[i][wChance];

    new roll = random(total), acc = 0;
    for (new i = 0; i < sizeof (WeatherTypes); i++) {
        acc += WeatherTypes[i][wChance];
        if (roll < acc) {
            gWeatherIndex = i;
            break;
        }
    }
    SetWeather(WeatherTypes[gWeatherIndex][wID]);
}

stock Weather_OnTick(tick)
{
    if (tick % (WEATHER_CHANGE_MINUTES * 60) == 0) Weather_Change();
}

stock Weather_GetKind()
{
    return WeatherTypes[gWeatherIndex][wKind];
}

stock Weather_GetName(dest[], size = sizeof (dest))
{
    format(dest, size, "%s", WeatherTypes[gWeatherIndex][wName]);
}
