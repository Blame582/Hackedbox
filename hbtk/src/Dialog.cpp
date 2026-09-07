#include <Dialog.hpp>

namespace HbTK {

Dialog::Dialog(int x,
               int y,
               int width,
               int height)
    : Window(x, y, width, height),
      m_modal(false)
{
}

Dialog::~Dialog() = default;

void Dialog::setModal(bool modal)
{
    m_modal = modal;
}

bool Dialog::isModal() const
{
    return m_modal;
}

} // namespace HbTK
