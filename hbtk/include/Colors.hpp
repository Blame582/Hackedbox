//hbtk/include/Colors.hpp

#pragma once

namespace HbTK {

class Colors {
public:
    Colors();
    Colors(unsigned char red,
          unsigned char green,
          unsigned char blue);

    unsigned char red() const;
    unsigned char green() const;
    unsigned char blue() const;

private:
    unsigned char m_red;
    unsigned char m_green;
    unsigned char m_blue;
};

} // namespace HbTK
