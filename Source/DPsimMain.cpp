#include <iostream>
#include <string>
#include "NetlistSim.h"
#include "Examples/SynchronGenUnitTest.h"
#include "Examples/SimpleCircuitTest.h"
#include "Examples/ReferenceCircuits.h"
#include <complex>

int main(int argc, char* argv[]) {



	simulationExampleIdealVS();
	//simulationExample3();

	//NetlistSim(argc, argv);
	
	//SynGenUnitTestBalancedResLoad();
	//SynGenUnitTestPhaseToPhaseFault();
	//SynGenUnitTestThreePhaseFault();	
	//SynGenDPUnitTestBalancedResLoad();

	//RXLineResLoad();
	//VarFreqRXLineResLoad();
	//RXLineResLoadEMT();
	//VarFreqRXLineResLoadEMT();
	
	//runDpEmtVarFreqStudy();

	//std::cin.get();
	return 0;	
}

