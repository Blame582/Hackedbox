#pragma once

#include <string>

#include <Style.hpp>

namespace HbTK {

class StyleEngine
{
public:
    StyleEngine();
    ~StyleEngine();

    StyleEngine(const StyleEngine&) = delete;
    StyleEngine& operator=(const StyleEngine&) = delete;

    bool load(const std::string& filename);

    bool loadString(const std::string& data);

    const Style& style() const;

    void reset();

private:
    bool parseLine(const std::string& line);

    bool setProperty(const std::string& key,
                     const std::string& value);

    static std::string trim(const std::string& value);

    static bool parseInteger(const std::string& value,
                             int& result);

    static bool parseColors(const std::string& value,
                           Colors& result);

    Style m_style;
};

} // namespace HbTK
