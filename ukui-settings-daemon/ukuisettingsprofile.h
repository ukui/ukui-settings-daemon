#ifndef UKUISETTINGSPROFILE_H
#define UKUISETTINGSPROFILE_H

//
#define ukui_settings_profile_start(...)
#define ukui_settings_profile_end(...)
#define ukui_settings_profile_msg(...)

class UkuiSettingsProfile
{
public:
    UkuiSettingsProfile();

    static void ukuiSettingsProfileLog(const char* func, const char* note, const char* format, ...);

};

#endif // UKUISETTINGSPROFILE_H
