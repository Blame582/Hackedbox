#include <StyleEngine.hpp>

#include <fstream>
#include <sstream>

namespace HbTK {

StyleEngine::StyleEngine()
{
    reset();
}

StyleEngine::~StyleEngine() = default;

bool StyleEngine::load(const std::string& filename)
{
    std::ifstream file(filename);

    if (!file.is_open())
        return false;

    reset();

    std::string line;

    while (std::getline(file, line))
    {
        if (!parseLine(line))
            return false;
    }

    return true;
}

bool StyleEngine::loadString(const std::string& data)
{
    reset();

    std::istringstream stream(data);

    std::string line;

    while (std::getline(stream, line))
    {
        if (!parseLine(line))
            return false;
    }

    return true;
}

const Style& StyleEngine::style() const
{
    return m_style;
}

void StyleEngine::reset()
{
    m_style = Style();
}

bool StyleEngine::parseLine(const std::string& input)
{
    std::string line = trim(input);

    if (line.empty())
        return true;

    if (line[0] == '#')
        return true;

    const std::size_t separator = line.find(':');

    if (separator == std::string::npos)
        return false;

    const std::string key =
        trim(line.substr(0, separator));

    const std::string value =
        trim(line.substr(separator + 1));

    if (key.empty())
        return false;

    return setProperty(key, value);
}

bool StyleEngine::setProperty(const std::string& key,
                              const std::string& value)
{
    if (key == "window.frame.color")
        return parseColors(value, m_style.frame.color);

    if (key == "window.frame.borderColors")
        return parseColors(value, m_style.frame.borderColors);

    if (key == "window.frame.borderWidth")
        return parseInteger(value, m_style.frame.borderWidth);

    if (key == "window.frame.radius")
        return parseInteger(value, m_style.frame.radius);

    if (key == "window.frame.alpha")
        return parseInteger(value, m_style.frame.alpha);

    if (key == "window.title.color")
        return parseColors(value, m_style.title.color);

    if (key == "window.title.textColors")
        return parseColors(value, m_style.title.textColors);

    if (key == "window.title.height")
        return parseInteger(value, m_style.title.height);

    if (key == "window.title.alpha")
        return parseInteger(value, m_style.title.alpha);

    if (key == "window.title.layout")
    {
        m_style.title.layout = value;
        return true;
    }

    if (key == "window.button.color")
        return parseColors(value, m_style.button.color);

    if (key == "window.button.hoverColors")
        return parseColors(value, m_style.button.hoverColors);

    if (key == "window.button.pressedColors")
        return parseColors(value, m_style.button.pressedColors);

    if (key == "window.button.borderColors")
        return parseColors(value, m_style.button.borderColors);

    if (key == "window.button.textColors")
        return parseColors(value, m_style.button.textColors);

    if (key == "window.button.width")
        return parseInteger(value, m_style.button.width);

    if (key == "window.button.height")
        return parseInteger(value, m_style.button.height);

    if (key == "window.button.borderWidth")
        return parseInteger(value, m_style.button.borderWidth);

    if (key == "window.button.radius")
        return parseInteger(value, m_style.button.radius);

    if (key == "window.button.alpha")
        return parseInteger(value, m_style.button.alpha);

    if (key == "window.background")
        return parseColors(value, m_style.background);

    if (key == "window.foreground")
        return parseColors(value, m_style.foreground);

    if (key == "window.alpha")
        return parseInteger(value, m_style.alpha);

    /*
     * Unknown properties are ignored for now.
     *
     * This lets the .hbs format grow without making
     * older versions fail when they encounter a newer
     * property.
     */

    return true;
}

std::string StyleEngine::trim(const std::string& value)
{
    const std::size_t first = value.find_first_not_of(" \t\r\n");

    if (first == std::string::npos)
        return {};

    const std::size_t last = value.find_last_not_of(" \t\r\n");

    return value.substr(first, last - first + 1);
}

bool StyleEngine::parseInteger(const std::string& value,
                               int& result)
{
    try
    {
        std::size_t position = 0;

        const int number =
            std::stoi(value, &position);

        if (position != value.size())
            return false;

        result = number;

        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool StyleEngine::parseColors(const std::string& value,
                             Colors& result)
{
    if (value.size() != 7 || value[0] != '#')
        return false;

    try
    {
        const unsigned int red =
            std::stoul(value.substr(1, 2), nullptr, 16);

        const unsigned int green =
            std::stoul(value.substr(3, 2), nullptr, 16);

        const unsigned int blue =
            std::stoul(value.substr(5, 2), nullptr, 16);

        result = Colors(
            static_cast<unsigned char>(red),
            static_cast<unsigned char>(green),
            static_cast<unsigned char>(blue)
        );

        return true;
    }
    catch (...)
    {
        return false;
    }
}

} // namespace HbTK
