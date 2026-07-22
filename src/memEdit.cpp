#include "memEdit.h"
#include "Emulator.h"

//https://en.sfml-dev.org/forums/index.php?topic=9490.0

namespace sn{

template <typename T>
std::string toString(T arg)
{
    std::stringstream ss;
    ss << arg;
    return ss.str();
}

void memEditor(Emulator *emulator)
{

    CPU *cpu = emulator->passCPU();



    sf::RenderWindow window;
    window.create(sf::VideoMode(800, 600), "Memory Editor", sf::Style::Default);

    sf::Font font;
    if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf"))
        std::cout << "Could not create the font!\n";
    sf::Text text("Memory Editor", font, 20);
    text.setPosition(10, 10);
    text.setFillColor(sf::Color::Red);
    text.setStyle(sf::Text::Bold);





     //std::string pcString = std::to_string(ss);


    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        //Keep this looped so the debugger can get the latest cpu values.
        registersStr rs;
        rs.PC = cpu->getPC();
        rs.SP = cpu->getSP();
        rs.A_R = cpu->getAreg();
        rs.X_R = cpu->getXreg();
        rs.Y_R = cpu->getYreg();
        rs.F_C = cpu->getf_c();
        rs.F_Z = cpu->getf_z();
        rs.F_I = cpu->getf_I();
        rs.F_D = cpu->getf_D();
        rs.F_V = cpu->getf_V();
        rs.F_N = cpu->getf_N();


        std::stringstream ssCPU;
        ssCPU << "PC: 0x"
           << std::hex
           << (std::to_string(rs.PC))
           << "\n"
           << "SP: "
           << (std::to_string(rs.SP))
           << "\n"
           << "A: "
           << (std::to_string(rs.A_R))
           << "\n"
           << "X: "
           << (std::to_string(rs.X_R))
           << "\n"
           << "Y: "
           << (std::to_string(rs.Y_R))
           << "\n"
           << "Carry: "
           << rs.F_C 
           << "\n"
           << "Zero: "
           << rs.F_Z
           << "\n"
           << "Interrupt: "
           << rs.F_I
           << "\n"
           << "Decimal: "
           << rs.F_D
           << "\n"
           << "Overflow(V): "
           << rs.F_V
           << "\n"
           << "Negative: "
           << rs.F_N
           << "\n";

        std::stringstream ssMem;
        ssMem << "Memory: ";

        sf::Text memInfo(ssMem.str(), font, 20);
        memInfo.setPosition(250, 50);
        memInfo.setFillColor(sf::Color::Black);
        memInfo.setStyle(sf::Text::Bold);

        sf::Text registerInfo(ssCPU.str(), font, 20);
        registerInfo.setPosition(10, 50);
        registerInfo.setFillColor(sf::Color::Black);
        registerInfo.setStyle(sf::Text::Bold);

        window.clear(sf::Color::White);
        window.setVerticalSyncEnabled(true);
        window.draw(text);
        window.draw(registerInfo);
        window.draw(memInfo);
        window.display();

    }
}
//};
}
