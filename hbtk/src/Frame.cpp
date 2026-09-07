#include <Frame.hpp>

#include <Painter.hpp>
#include <Style.hpp>

namespace HbTK {

Frame::Frame()
    : m_style(nullptr),
      m_title(),
      m_centralWidget(nullptr)
{
}

Frame::~Frame()
{
}

void Frame::setStyle(const Style& style)
{
    m_style = &style;

    updateCentralWidgetGeometry();
}

const Style* Frame::style() const
{
    return m_style;
}

void Frame::setTitle(const std::string& title)
{
    m_title = title;
}

const std::string& Frame::title() const
{
    return m_title;
}

void Frame::setCentralWidget(Widget* widget)
{
    if (m_centralWidget == widget)
        return;

    if (m_centralWidget)
        m_centralWidget->setParent(nullptr);

    m_centralWidget = widget;

    if (!m_centralWidget)
        return;

    m_centralWidget->setParent(this);

    updateCentralWidgetGeometry();
}

Widget* Frame::centralWidget() const
{
    return m_centralWidget;
}

void Frame::resize(int width, int height)
{
    Widget::resize(width, height);

    updateCentralWidgetGeometry();
}

void Frame::updateCentralWidgetGeometry()
{
    if (!m_centralWidget)
        return;

    int titleHeight = 0;

    if (m_style)
        titleHeight = m_style->titleHeight();

    const int contentHeight =
        height() - titleHeight;

    m_centralWidget->setGeometry(
        0,
        titleHeight,
        width(),
        contentHeight > 0 ? contentHeight : 0
    );
}

void Frame::paint(Painter& painter)
{
    if (!visible())
        return;

    if (m_style)
    {
        painter.fillRect(
            x(),
            y(),
            static_cast<unsigned int>(width()),
            static_cast<unsigned int>(height()),
            m_style->windowBackground()
        );

        const int titleHeight =
            m_style->titleHeight();

        if (titleHeight > 0)
        {
            painter.fillRect(
                x(),
                y(),
                static_cast<unsigned int>(width()),
                static_cast<unsigned int>(titleHeight),
                m_style->windowTitleBackground()
            );

            painter.drawText(
                x() + 10,
                y() + titleHeight - 6,
                m_title.c_str(),
                m_style->windowTitleText()
            );
        }

        if (m_style->frame.borderWidth > 0)
        {
            painter.drawRect(
                x(),
                y(),
                static_cast<unsigned int>(width()),
                static_cast<unsigned int>(height()),
                m_style->windowBorder()
            );
        }
    }

    if (m_centralWidget &&
        m_centralWidget->visible())
    {
        m_centralWidget->paint(painter);
    }
}

} // namespace HbTK
