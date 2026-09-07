#pragma once

class Hackedbox;
class HbSettingsWindow;

class HbSettings
{
public:
    explicit HbSettings(Hackedbox* hackedbox);
    ~HbSettings();

    bool show();
    void hide();

private:
    Hackedbox* m_hackedbox;
    HbSettingsWindow* m_window;
};
