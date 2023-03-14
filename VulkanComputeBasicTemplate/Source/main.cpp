#include <stdexcept>
#include <iostream>

#include "Application.h"

int main()
{
    try
    {
        Application app;
        app.Run();
    }
    catch (const std::runtime_error& err)
    {
        std::cerr << err.what() << std::endl;
        return 1;
    }

    return 0;
}
