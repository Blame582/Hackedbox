#include <SWindow.hpp>

#include <MainWindow.hpp>
#include <Splitter.hpp>
#include <Widget.hpp>

HbSettingsWindow::HbSettingsWindow(Hackedbox* hackedbox)
    : m_hackedbox(hackedbox),
      m_window(nullptr),
      m_splitter(nullptr),
      m_navigation(nullptr),
      m_display(nullptr),
      m_visible(false)
{
}

HbSettingsWindow::~HbSettingsWindow()
{
    delete m_window;
}

bool HbSettingsWindow::show()
{
    if (m_visible)
        return true;

    if (!m_window)
    {
        m_window =
            new HbTK::Window(
                100,
                100,
                900,
                600
            );

        m_window->setTitle(
            "Hackedbox Settings"
        );

        m_splitter =
            new HbTK::Splitter(
                HbTK::Splitter::Orientation::Horizontal
            );

        m_navigation =
            new HbTK::Widget();

        m_display =
            new HbTK::Widget();

        m_splitter->addWidget(
            m_navigation
        );

        m_splitter->addWidget(
            m_display
        );

        m_window->setCentralWidget(
            m_splitter
        );

        m_splitter->setSizes(
            {
                225,
                675
            }
        );

        if (!m_window->create())
            return false;
    }

    m_window->show();

    m_visible = true;

    return true;
}

void HbSettingsWindow::hide()
{
    if (!m_window || !m_visible)
        return;

    m_window->hide();

    m_visible = false;
}
