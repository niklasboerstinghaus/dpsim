#include "VoltSourceRes.h"

using namespace DPsim;

VoltSourceRes::VoltSourceRes(std::string name, int src, int dest, Real voltage, Real phase, Real resistance) : BaseComponent(src, dest) {
	this->mName = name;
	this->mVoltageDiffr = voltage*cos(phase);
	this->mVoltageDiffi = voltage*sin(phase);
	this->mResistance = resistance;		
	this->mConductance = 1. / resistance;
	this->mCurrentr = mVoltageDiffr / resistance;
	this->mCurrenti = mVoltageDiffi / resistance;
}

void VoltSourceRes::applySystemMatrixStamp(SystemModel& system) {
	// Apply matrix stamp for equivalent resistance
	if (mNode1 >= 0) {
		system.addCompToSystemMatrix(mNode1, mNode1, mConductance, 0);
	}
	if (mNode2 >= 0) {
		system.addCompToSystemMatrix(mNode2, mNode2, mConductance, 0);
	}
	if (mNode1 >= 0 && mNode2 >= 0) {
		system.addCompToSystemMatrix(mNode1, mNode2, -mConductance, 0);
		system.addCompToSystemMatrix(mNode2, mNode1, -mConductance, 0);
	}
}

void VoltSourceRes::applyRightSideVectorStamp(SystemModel& system) {
	// Apply matrix stamp for equivalent current source
	if (mNode1 >= 0) {
		system.addCompToRightSideVector(mNode1, mCurrentr, mCurrenti);
	}
	if (mNode2 >= 0) {
		system.addCompToRightSideVector(mNode2, -mCurrentr, -mCurrenti);
	}
}

void VoltSourceRes::step(SystemModel& system, Real time) {
	if (mNode1 >= 0) {
		system.addCompToRightSideVector(mNode1, mCurrentr, mCurrenti);
	}
	if (mNode2 >= 0) {
		system.addCompToRightSideVector(mNode2, -mCurrentr, -mCurrenti);
	}
}