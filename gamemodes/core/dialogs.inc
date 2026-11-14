// Уникални ID-та на диалозите
enum {
    DIALOG_NONE,

    // Вход
    DIALOG_LOGIN,

    // Регистрация (в този ред)
    DIALOG_REG_WELCOME,
    DIALOG_REG_PASSWORD,
    DIALOG_REG_PASSWORD_CONFIRM,
    DIALOG_REG_EMAIL,
    DIALOG_REG_GENDER,
    DIALOG_REG_AGE,
    DIALOG_REG_SOURCE,
    DIALOG_REG_REFERRAL,
    DIALOG_REG_PROMO,
    DIALOG_REG_SUMMARY,

    // Главно меню
    DIALOG_MENU,
    DIALOG_MENU_BACK,   // информационен прозорец с бутон "Назад" към менюто

    // Админ вход
    DIALOG_ALOGIN,
    DIALOG_ALOGIN_CREATE,
    DIALOG_ALOGIN_CONFIRM,

    // Репорти
    DIALOG_REPORT_OPEN,
    DIALOG_REPORT_ADD,
    DIALOG_REPORT_CATEGORY,
    DIALOG_REPORT_TARGET,
    DIALOG_REPORT_TEXT,
    DIALOG_REPORT_RATE,
    DIALOG_REPORTS_LIST,
    DIALOG_REPORT_VIEW,
    DIALOG_REPORT_ACTIONS,
    DIALOG_REPORT_REPLY,

    // Информационни
    DIALOG_INFO,
    DIALOG_INFO_KICK    // след затваряне играчът се изгонва
}
