#include "Emulator.h"
#include "APU/Constants.h"
#include "Log.h"
#include <time.h>
#include <ctime>
#include "CPU.h"
#include <fstream>


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
        // LOG(Info) << m_cpu.getAreg()  << std::endl; 
        // Create and open a text file

        //stolen -- https://www.geeksforgeeks.org/linux-unix/create-directoryfolder-cc-program/
        if (std::filesystem::create_directory("savestate") == -1)
            std::cerr << "Error:  " << std::strerror(errno) << std::endl; //cerr can probably be changed.
        else
            LOG(Info) << "Directory Created." << std::endl;

          //Creating the file, creating a memory string, finding current directory we are in..
          std::ofstream memfile("memdump.txt");
          std::string mem;
          auto currDir =  std::filesystem::current_path();
          std::filesystem::path destination = currDir / "savestate" / "memdump.txt"; 

          //Writing current memory to the file
          for (auto i : m_bus.getRam())
          mem.push_back(i);
          memfile << mem;
          memfile.close();

          //Copying the memory file to the new directory.
          std::filesystem::copy_file("memdump.txt", destination, std::filesystem::copy_options::overwrite_existing);
}

void Emulator::run(std::string rom_path) {
    emulatorStatsUI ui;

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

}
