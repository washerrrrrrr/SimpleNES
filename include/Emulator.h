#ifndef EMULATOR_H
#define EMULATOR_H
#include <SFML/Graphics.hpp>
#include <chrono>
#include <filesystem>
#include <vector>
#include <iostream>
#include <string>
#include "APU/APU.h"
#include "AudioPlayer.h"
#include "CPU.h"
#include "Controller.h"
#include "MainBus.h"
#include "PPU.h"
#include "PictureBus.h"
#include <string>

extern bool noUnfocusPause;
extern int definedFps;
extern bool emuStats;
extern bool memEdit;

struct emulatorStatsUI {
    sf::Clock clock;
    int frames = 0;
    int fps = 0;

    sf::Font font;
    sf::Text statsText;
};


namespace sn
{

struct loadsave_State{
    std::string emu_Memory; //Memory from the emulator 
    std::string emu_registers; //Register info from the emulator
    std::filesystem::path saveDestination; //where the savestate file is saved.

    //CPU getting
    Address PC;
    Byte SP;
    Byte A_R;
    Byte X_R;
    Byte Y_R;

    //these are booleans.
    bool F_C;
    bool F_Z;
    bool F_I;
    bool F_D;
    bool F_V;
    bool F_N;

    std::vector<unsigned char> v_mem;
    //unsigned int temp;
};

using TimePoint          = std::chrono::high_resolution_clock::time_point;
using Duration           = std::chrono::high_resolution_clock::duration;

const int NESVideoWidth  = ScanlineVisibleDots;
const int NESVideoHeight = VisibleScanlines;

class Emulator
{
public:
    Emulator();
    void run(std::string rom_path, Emulator* emuAddress);
    void setVideoWidth(int width);
    void setVideoHeight(int height);
    void setVideoScale(float scale);
    void setKeys(std::vector<sf::Keyboard::Key>& p1, std::vector<sf::Keyboard::Key>& p2);
    void muteAudio();
    void savestate();
    void loadstate(bool isGame);
    CPU* passCPU();


private:
    void                    OAMDMA(Byte page);
    Byte                    DMCDMA(Address addr);

    CPU                     m_cpu;
    //int                     cpuAddress() { return &m_cpu }

    AudioPlayer             m_audioPlayer;

    PictureBus              m_pictureBus;
    PPU                     m_ppu;
    APU                     m_apu;
    Cartridge               m_cartridge;
    std::unique_ptr<Mapper> m_mapper;

    Controller              m_controller1, m_controller2;

    MainBus                 m_bus;

    sf::RenderWindow        m_window;
    VirtualScreen           m_emulatorScreen;
    float                   m_screenScale;

    TimePoint               m_lastWakeup;

    Duration                m_elapsedTime;
};
}
#endif // EMULATOR_H
