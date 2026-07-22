#include <iostream>
#include "Emulator.h"
#include <SFML/Window.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include "CPU.h"

namespace sn{

    class Emulator;

	template <typename T>
	std::string toString(T arg);

	struct registersStr {         
	    //m_cpu;

	    char PC;
	    Byte SP;
	    Byte A_R;
	    Byte X_R;
	    Byte Y_R;

	    bool F_C;
	    bool F_Z;
	    bool F_I;
	    bool F_D;
	    bool F_V;
	    bool F_N;
	} ;

	//just this for now it seems.
	void memEditor(Emulator *emulator);
}

