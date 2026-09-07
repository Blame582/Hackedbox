#pragma once

#include "MainWindow.hpp"

namespace HbTK {

class Dialog : public Window
{
public:
    Dialog(int x,
           int y,
           int width,
           int height);

    ~Dialog() override;

    void setModal(bool modal);
    bool isModal() const;

private:
    bool m_modal;
};

} // namespace HbTK
