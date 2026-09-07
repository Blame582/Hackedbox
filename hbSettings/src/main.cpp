#include <Settings.hpp>

#include <unistd.h>

int main()
{
    HbSettings settings(nullptr);

    if (!settings.show())
        return 1;

    while (true)
        sleep(1);

    return 0;
}
