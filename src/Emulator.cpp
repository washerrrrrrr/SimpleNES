#include "Emulator.h"
#include "APU/Constants.h"
#include "Log.h"
#include <time.h>
#include <ctime>
#include "CPU.h"
#include <fstream>
#include <iomanip>
#include "memEdit.h"
#include <thread>
#include <chrono>


namespace sn {

    using std::chrono::high_resolution_clock;

    Emulator::Emulator()
      : m_cpu(m_bus)
      , m_audioPlayer(static_cast<int>(1.0 / apu_clock_period_s.count()))
      , m_ppu(m_pictureBus, m_emulatorScreen)
      , m_apu(m_audioPlayer, m_cpu.createIRQHandler(), [&](Address addr) { return DMCDMA(addr); })
      , m_bus(m_ppu, m_apu, m_controller1, m_controller2, [&](Byte b) { OAMDMA(b); })
      , m_screenScale(3.f)
      , m_lastWakeup()
{
    m_ppu.setInterruptCallback([&]() { m_cpu.nmiInterrupt(); });
}



void Emulator::savestate() {
    sn::loadsave_State state;
        m_cpu;

        //stolen -- https://www.geeksforgeeks.org/linux-unix/create-directoryfolder-cc-program/
        if (std::filesystem::create_directory("savestate") == -1)
            std::cerr << "Error:  " << std::strerror(errno) << std::endl; //cerr can probably be changed.
        else
            LOG(Info) << "Directory Created." << std::endl;


        //Creating the file, creating a memory string, finding current directory we are in..
        //std::ofstream savestateFP("savestate.txt");
        //state.emu_Memory;
        //auto currentDirectory = std::filesystem::current_path();
        //state.saveDestination = currentDirectory / "savestate" / "savestate.txt"; 

        std::ofstream file("savestate//savestate.txt", std::ios::binary);

        const auto& ram = m_bus.getRam();

        file.write(
            reinterpret_cast<const char*>(ram.data()),
            ram.size()
        );

        //Registers:
        state.PC = m_cpu.getPC();
        state.SP = m_cpu.getSP();
        state.A_R = m_cpu.getAreg();
        state.X_R = m_cpu.getXreg();
        state.Y_R = m_cpu.getYreg();

        //these are booleans.
        state.F_C = m_cpu.getf_c();
        state.F_Z = m_cpu.getf_z();
        state.F_I = m_cpu.getf_I();
        state.F_D = m_cpu.getf_D();
        state.F_V = m_cpu.getf_V();
        state.F_N = m_cpu.getf_N();

        //state.emu_registers;
        //Apparently this way works better than what I was doing previously sooooo....
        //Passing the addresses instead
        // Registers
        file.write(reinterpret_cast<const char*>(&state.PC), sizeof(state.PC));
        file.write(reinterpret_cast<const char*>(&state.SP), sizeof(state.SP));
        file.write(reinterpret_cast<const char*>(&state.A_R), sizeof(state.A_R));
        file.write(reinterpret_cast<const char*>(&state.X_R), sizeof(state.X_R));
        file.write(reinterpret_cast<const char*>(&state.Y_R), sizeof(state.Y_R));

        // Flags
        file.write(reinterpret_cast<const char*>(&state.F_C), sizeof(state.F_C));
        file.write(reinterpret_cast<const char*>(&state.F_Z), sizeof(state.F_Z));
        file.write(reinterpret_cast<const char*>(&state.F_I), sizeof(state.F_I));
        file.write(reinterpret_cast<const char*>(&state.F_D), sizeof(state.F_D));
        file.write(reinterpret_cast<const char*>(&state.F_V), sizeof(state.F_V));
        file.write(reinterpret_cast<const char*>(&state.F_N), sizeof(state.F_N));
        std::cout << "PC: 0x" << std::hex << state.PC << "\n";
        std::cout << "SP: 0x" << (int)state.SP << "\n";
        std::cout << "A: 0x" << (int)state.A_R << "\n";
        std::cout << "X: 0x" << (int)state.A_R << "\n";
        std::cout << "Y: 0x" << (int)state.Y_R << "\n";
        std::cout << "F_C: " << (bool)state.F_C << "\n";
        std::cout << "F_Z: " << (bool)state.F_Z << "\n";
        std::cout << "F_I: " << (bool)state.F_I << "\n";
        std::cout << "F_D: " << (bool)state.F_D << "\n";
        std::cout << "F_V: " << (bool)state.F_V << "\n";
        std::cout << "F_N: " << (bool)state.F_N << "\n";

        file.close();

       // std::filesystem::copy_file("savestate.txt", state.saveDestination, std::filesystem::copy_options::overwrite_existing);

}

void Emulator::loadstate(bool isGame) {
    if(isGame) {
        loadsave_State state;
        m_cpu;
        m_bus;

        std::ifstream savestateFP("savestate//savestate.txt", std::ios::binary);
       //state.emu_Memory;
       //auto currentDirectory = std::filesystem::current_path();
       //state.saveDestination = currentDirectory / "savestate" / "savestate.txt"; 
        std::vector<Byte> ram;
        auto& tmp = m_bus.getRam();

        ram.resize(tmp.size());

        savestateFP.read(
            reinterpret_cast<char*>(ram.data()),
            tmp.size()
        );

        savestateFP.read(reinterpret_cast<char*>(&state.PC), sizeof(state.PC));
        savestateFP.read(reinterpret_cast<char*>(&state.SP), sizeof(state.SP));
        savestateFP.read(reinterpret_cast<char*>(&state.A_R), sizeof(state.A_R));
        savestateFP.read(reinterpret_cast<char*>(&state.X_R), sizeof(state.X_R));
        savestateFP.read(reinterpret_cast<char*>(&state.Y_R), sizeof(state.Y_R));


        savestateFP.read(reinterpret_cast<char*>(&state.F_C), sizeof(state.F_C));
        savestateFP.read(reinterpret_cast<char*>(&state.F_Z), sizeof(state.F_Z));
        savestateFP.read(reinterpret_cast<char*>(&state.F_I), sizeof(state.F_I));
        savestateFP.read(reinterpret_cast<char*>(&state.F_D), sizeof(state.F_D));
        savestateFP.read(reinterpret_cast<char*>(&state.F_V), sizeof(state.F_V));
        savestateFP.read(reinterpret_cast<char*>(&state.F_N), sizeof(state.F_N));


    //     std::string lines;
    //     std::stringstream ss(lines); 


    //     int i;

    //     for(i = 0; i < 128; i++) {
    //         while (std::getline(savestateFP, lines)) {
    //             unsigned char temp;
    //             if (ss >> temp) { 
    //                 std::cout << "Memory data parsed: Ok\n";
    //                 state.v_mem.push_back(temp);
    //             }else { LOG(Error) << "Couldn't parse correctly: " << lines << "(memdump.txt)" <<"\n";
    //         }
    //     }
    //     //Loading in memory:
           m_bus.repl_Ram(ram); //load ram with the memory extracted from the savefile.
    //     ss.str("");
    //     }

    //     if (i == 128){
    //         for(i < 128; i++;){
    //                 while (std::getline(savestateFP, lines)) {
    //                 std::stringstream ss(lines);
                    
    //                 state.PC = 0;
    //                 state.SP = 0;
    //                 state.A_R = 0;
    //                 state.X_R = 0;
    //                 state.Y_R = 0;

    //                 state.F_C;
    //                 state.F_Z;
    //                 state.F_I;
    //                 state.F_D;
    //                 state.F_V;
    //                 state.F_N;

    //                 ss >> state.PC;
    //                 ss >> state.SP;
    //                 ss >> state.A_R;
    //                 ss >> state.X_R;
    //                 ss >> state.Y_R;

    //                 ss >> state.F_C;
    //                 ss >> state.F_Z;
    //                 ss >> state.F_I;
    //                 ss >> state.F_D;
    //                 ss >> state.F_V;
    //                 ss >> state.F_N;


    //                 //std::cout << "Register data parsed: " << PC << " " << SP << " "  << A_R << " " << X_R << " " << Y_R
    //                                                         // << " " << F_C << " " << F_Z << " " << F_I << " " << F_D << " " << F_V << " " << F_N << "\n";

    //                    // LOG(Error) << "Couldn't parse correctly:  " << lines << "(registers.txt)" <<"\n";
    //                 savestateFP.close();

                    m_cpu.repl_PC(state.PC);
                    m_cpu.repl_SP(state.SP);
                    m_cpu.repl_A(state.A_R);
                    m_cpu.repl_X(state.X_R);
                    m_cpu.repl_Y(state.Y_R);
                    m_cpu.repl_FC(state.F_C);
                    m_cpu.repl_FZ(state.F_Z);
                    m_cpu.repl_FI(state.F_I);
                    m_cpu.repl_FD(state.F_D);
                    m_cpu.repl_FV(state.F_V);
                    m_cpu.repl_FN(state.F_N);
        std::cout << "PC: 0x" << std::hex << state.PC << "\n";
        std::cout << "SP: 0x" << (int)state.SP << "\n";
        std::cout << "A: 0x" << (int)state.A_R << "\n";
        std::cout << "X: 0x" << (int)state.A_R << "\n";
        std::cout << "Y: 0x" << (int)state.Y_R << "\n";
        std::cout << "F_C: " << (bool)state.F_C << "\n";
        std::cout << "F_Z: " << (bool)state.F_Z << "\n";
        std::cout << "F_I: " << (bool)state.F_I << "\n";
        std::cout << "F_D: " << (bool)state.F_D << "\n";
        std::cout << "F_V: " << (bool)state.F_V << "\n";
        std::cout << "F_N: " << (bool)state.F_N << "\n";
    //                 ss.str(""); //clear the stringstream
    //                 }
    //             }
    //         }
    //     }else {LOG(Error) << "Loadstate error: Cannot find the game mentioned in the savestate files." << std::endl; 
    }
}

void Emulator::run(std::string rom_path, Emulator* emuAddress) {
    emulatorStatsUI ui;

    //auto getEmulatorAddress = emuAddress;

    if (!m_cartridge.loadFromFile(rom_path))
        return;

    m_mapper = Mapper::createMapper(static_cast<Mapper::Type>(m_cartridge.getMapper()),
                                    m_cartridge,
                                    m_cpu.createIRQHandler(),
                                    [&]() { m_pictureBus.updateMirroring(); });
    if (!m_mapper)
    {
        LOG(Error) << "Creating Mapper failed. Probably unsupported." << std::endl;
        return;
    }

    if (!m_bus.setMapper(m_mapper.get()) || !m_pictureBus.setMapper(m_mapper.get()))
    {
        return;
    }

    m_cpu.reset();
    m_ppu.reset();

    m_window.create(sf::VideoMode(NESVideoWidth * m_screenScale, NESVideoHeight * m_screenScale),
                    "SimpleNES",
                    sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize);


    if (definedFps != 0 || definedFps > 0)
        m_window.setFramerateLimit(definedFps);
    else
        m_window.setVerticalSyncEnabled(true);

    m_emulatorScreen.create(NESVideoWidth, NESVideoHeight, m_screenScale, sf::Color::White);

    m_lastWakeup  = high_resolution_clock::now();
    m_elapsedTime = m_lastWakeup - m_lastWakeup;

    //For FPS
    if(emuStats) {
        ui.font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf");

        ui.statsText.setFont(ui.font);
        ui.statsText.setCharacterSize(15);
        ui.statsText.setPosition(10, 10);
    }

    if(memEdit){
        std::thread me(memEditor, emuAddress);
        me.detach();
    }

    m_audioPlayer.start();

    sf::Event event;
    bool focus = true, pause = false;
    while (m_window.isOpen())
    {

        while (m_window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed ||
                (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape))
            {
                m_window.close();
                return;
            }
            else if (event.type == sf::Event::GainedFocus)
            {
                focus          = true;
                const auto now = high_resolution_clock::now();
                LOG(Info) << "Gained focus. Removing " << (now - m_lastWakeup).count() << "ns from timers" << std::endl;
                m_lastWakeup = now;
            }
            else if (event.type == sf::Event::LostFocus)
            {
                if (noUnfocusPause) 
                    focus = true;
                else {
                    focus = false;
                    LOG(Info) << "Losing focus; paused." << std::endl;
                }

            }
            else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F2)
            {
                pause = !pause;
                if (!pause)
                {
                    const auto now = high_resolution_clock::now();
                    LOG(Info) << "Unpaused. Removing " << (now - m_lastWakeup).count() << "ns from timers" << std::endl;
                    m_lastWakeup = now;
                }
                else
                {
                    LOG(Info) << "Paused." << std::endl;
                }
            }
            else if (pause && event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::F3)
            {
                for (int i = 0; i < 29781; ++i) // Around one frame
                {
                    // PPU
                    m_ppu.step();
                    m_ppu.step();
                    m_ppu.step();
                    // CPU
                    m_cpu.step();
                    // APU
                    m_apu.step();
                }
            }
            else if (focus && event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::F4)
            {

                Log::get().setLevel(Info);
            }
            else if (focus && event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::F5)
            {
                Log::get().setLevel(InfoVerbose);
            }
            else if (focus && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Tab))
            {
                savestate();
            }
            else if (focus && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::T)) { //Change to something else
                auto game = m_cartridge;
                loadstate(true); //change this as well
            }

            else if (focus && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F12))
            { 
                //This took me at least 10 minutes to figure out for some reason.
                time_t timestamp;
                time(&timestamp); 
                std::string screenshotName =  std::string(ctime(&timestamp))+".png";

                //Stolen screenshot code from r*ddit because i cant code.
                sf::Vector2u window_size = m_window.getSize();
                sf::Texture sstexture;
                sstexture.create(window_size.x, window_size.y);
                sstexture.update(m_window);
                sf::Image screenshot = sstexture.copyToImage();
                if (screenshot.saveToFile(screenshotName)) {
                    LOG(Info) << "Screenshot taken." << std::endl;
                }
            }

        }

        if (focus && !pause)
        {
            const auto now  = high_resolution_clock::now();
            m_elapsedTime  += now - m_lastWakeup;
            m_lastWakeup    = now;

            while (m_elapsedTime > cpu_clock_period_ns)
            {
                // PPU
                m_ppu.step();
                m_ppu.step();
                m_ppu.step();
                // CPU
                m_cpu.step();
                // APU
                m_apu.step();

                m_elapsedTime -= cpu_clock_period_ns;
            }


            m_window.draw(m_emulatorScreen);
            ui.frames++;

            if (ui.clock.getElapsedTime().asSeconds() >= 1.0f)
            {
                ui.fps = ui.frames;
                ui.frames = 0;
                ui.clock.restart();

                ui.statsText.setString("FPS: " + std::to_string(ui.fps));
            }

            m_window.draw(ui.statsText);
            m_window.display();
        }
        else
        {
            sf::sleep(sf::milliseconds(1000 / 60));
        }
    }
}

void Emulator::OAMDMA(Byte page)
{
    m_cpu.skipOAMDMACycles();
    auto page_ptr = m_bus.getPagePtr(page);
    if (page_ptr != nullptr)
    {
        m_ppu.doDMA(page_ptr);
    }
    else
    {
        LOG(Error) << "Can't get pageptr for DMA" << std::endl;
    }
}

Byte Emulator::DMCDMA(Address addr)
{
    m_cpu.skipDMCDMACycles();
    return m_bus.read(addr);
};

void Emulator::setVideoHeight(int height)
{
    m_screenScale = height / float(NESVideoHeight);
    LOG(Info) << "Scale: " << m_screenScale << " set. Screen: " << int(NESVideoWidth * m_screenScale) << "x"
              << int(NESVideoHeight * m_screenScale) << std::endl;
}

void Emulator::setVideoWidth(int width)
{
    m_screenScale = width / float(NESVideoWidth);
    LOG(Info) << "Scale: " << m_screenScale << " set. Screen: " << int(NESVideoWidth * m_screenScale) << "x"
              << int(NESVideoHeight * m_screenScale) << std::endl;
}
void Emulator::setVideoScale(float scale)
{
    m_screenScale = scale;
    LOG(Info) << "Scale: " << m_screenScale << " set. Screen: " << int(NESVideoWidth * m_screenScale) << "x"
              << int(NESVideoHeight * m_screenScale) << std::endl;
}

void Emulator::setKeys(std::vector<sf::Keyboard::Key>& p1, std::vector<sf::Keyboard::Key>& p2)
{
    m_controller1.setKeyBindings(p1);
    m_controller2.setKeyBindings(p2);
}

void Emulator::muteAudio()
{
    m_audioPlayer.mute();
}

CPU* Emulator::passCPU() 
{ 
    return &m_cpu;
}

}
