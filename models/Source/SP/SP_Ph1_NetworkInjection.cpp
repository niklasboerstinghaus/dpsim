/* Copyright 2017-2020 Institute for Automation of Complex Power Systems,
 *                     EONERC, RWTH Aachen University
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *********************************************************************************/

#include <cps/SP/SP_Ph1_NetworkInjection.h>

using namespace CPS;


SP::Ph1::externalGridInjection::externalGridInjection(String uid, String name,
    Logger::Level logLevel) : SimPowerComp<Complex>(uid, name, logLevel) {

	mSLog->info("Create {} of type {}", mName, this->type());
	mSLog->flush();
	mIntfVoltage = MatrixComp::Zero(1, 1);
	mIntfCurrent = MatrixComp::Zero(1, 1);
	setVirtualNodeNumber(1);
	setTerminalNumber(1);

    addAttribute<Real>("V_set", &mVoltageSetPoint, Flags::read | Flags::write);
    addAttribute<Real>("V_set_pu", &mVoltageSetPointPerUnit, Flags::read | Flags::write);
	addAttribute<Real>("p_inj", &mActivePowerInjection, Flags::read | Flags::write);
	addAttribute<Real>("q_inj", &mReactivePowerInjection, Flags::read | Flags::write);

	// MNA attributes
	addAttribute<Complex>("V_ref", Flags::read | Flags::write);
	addAttribute<Real>("f_src", Flags::read | Flags::write);
}

// #### Powerflow section ####

void SP::Ph1::externalGridInjection::setParameters(Real voltageSetPoint) {
	mVoltageSetPoint = voltageSetPoint;

	mSLog->info("Voltage Set-Point ={} [V]", mVoltageSetPoint);
	mSLog->flush();

	mParametersSet = true;
}

void SP::Ph1::externalGridInjection::setBaseVoltage(Real baseVoltage) {
    mBaseVoltage = baseVoltage;
}

void SP::Ph1::externalGridInjection::calculatePerUnitParameters(Real baseApparentPower, Real baseOmega) {
    mSLog->info("#### Calculate Per Unit Parameters for {}", mName);
	mSLog->info("Base Voltage={} [V]", mBaseVoltage);

    mVoltageSetPointPerUnit = mVoltageSetPoint / mBaseVoltage;

	mSLog->info("Voltage Set-Point ={} [pu]", mVoltageSetPointPerUnit);
	mSLog->flush();
}

void SP::Ph1::externalGridInjection::modifyPowerFlowBusType(PowerflowBusType powerflowBusType) {
	mPowerflowBusType = powerflowBusType;
}

void SP::Ph1::externalGridInjection::updatePowerInjection(Complex powerInj) {
	mActivePowerInjection = powerInj.real();
	mReactivePowerInjection = powerInj.imag();
}

// #### MNA Section ####

void SP::Ph1::externalGridInjection::setParameters(Complex voltageRef, Real srcFreq) {
	attribute<Complex>("V_ref")->set(voltageRef);
	attribute<Real>("f_src")->set(srcFreq);
	mParametersSet = true;
}

SimPowerComp<Complex>::Ptr SP::Ph1::externalGridInjection::clone(String name) {
	auto copy = externalGridInjection::make(name, mLogLevel);
	copy->setParameters(attribute<Complex>("V_ref")->get());
	return copy;
}

void SP::Ph1::externalGridInjection::initializeFromNodesAndTerminals(Real frequency) {
	mVoltageRef = attribute<Complex>("V_ref");
	mSrcFreq = attribute<Real>("f_src");
	if (mVoltageRef->get() == Complex(0, 0))
		//mVoltageRef->set(Complex(std::abs(initialSingleVoltage(0).real()), std::abs(initialSingleVoltage(0).imag())));
		mVoltageRef->set(initialSingleVoltage(0));
	mSLog->info(
		"\n--- Initialization from node voltages ---"
		"\nVoltage across: {:e}<{:e}"
		"\nTerminal 0 voltage: {:e}<{:e}"
		"\n--- Initialization from node voltages ---",
		std::abs(mVoltageRef->get()), std::arg(mVoltageRef->get()),
		std::abs(initialSingleVoltage(0)), std::arg(initialSingleVoltage(0)));
}

// #### MNA functions ####

void SP::Ph1::externalGridInjection::mnaInitialize(Real omega, Real timeStep, Attribute<Matrix>::Ptr leftVector) {
	MNAInterface::mnaInitialize(omega, timeStep);
	updateMatrixNodeIndices();

	mIntfVoltage(0, 0) = mVoltageRef->get();
	mMnaTasks.push_back(std::make_shared<MnaPreStep>(*this));
	mMnaTasks.push_back(std::make_shared<MnaPostStep>(*this, leftVector));
	mRightVector = Matrix::Zero(leftVector->get().rows(), 1);
}

void SP::Ph1::externalGridInjection::mnaApplySystemMatrixStamp(Matrix& systemMatrix) {
	Math::setMatrixElement(systemMatrix, mVirtualNodes[0]->matrixNodeIndex(), matrixNodeIndex(0), Complex(1, 0));
	Math::setMatrixElement(systemMatrix, matrixNodeIndex(0), mVirtualNodes[0]->matrixNodeIndex(), Complex(1, 0));
	mSLog->info("-- Matrix Stamp ---");
	mSLog->info("Add {:f} to system at ({:d},{:d})", 1., matrixNodeIndex(0), mVirtualNodes[0]->matrixNodeIndex());
	mSLog->info("Add {:f} to system at ({:d},{:d})", 1., mVirtualNodes[0]->matrixNodeIndex(), matrixNodeIndex(0));
}


void SP::Ph1::externalGridInjection::mnaApplyRightSideVectorStamp(Matrix& rightVector) {
	// TODO: Is this correct with two nodes not gnd?
	Math::setVectorElement(rightVector, mVirtualNodes[0]->matrixNodeIndex(), mIntfVoltage(0, 0));
	SPDLOG_LOGGER_DEBUG(mSLog, "Add {:s} to source vector at {:d}",
		Logger::complexToString(mIntfVoltage(0, 0)), mVirtualNodes[0]->matrixNodeIndex());
}

void SP::Ph1::externalGridInjection::updateVoltage(Real time) {
	if (mSrcFreq->get() < 0) {
		mIntfVoltage(0, 0) = mVoltageRef->get();
	}
	else {
		mIntfVoltage(0, 0) = Complex(
			Math::abs(mVoltageRef->get()) * cos(time * 2. * PI * mSrcFreq->get() + Math::phase(mVoltageRef->get())),
			Math::abs(mVoltageRef->get()) * sin(time * 2. * PI * mSrcFreq->get() + Math::phase(mVoltageRef->get())));
	}
}

void SP::Ph1::externalGridInjection::MnaPreStep::execute(Real time, Int timeStepCount) {
	mExternalGridInjection.updateVoltage(time);
	mExternalGridInjection.mnaApplyRightSideVectorStamp(mExternalGridInjection.mRightVector);
}


void SP::Ph1::externalGridInjection::MnaPostStep::execute(Real time, Int timeStepCount) {
	mExternalGridInjection.mnaUpdateCurrent(*mLeftVector);
}

void SP::Ph1::externalGridInjection::mnaUpdateCurrent(const Matrix& leftVector) {
	mIntfCurrent(0, 0) = Math::complexFromVectorElement(leftVector, mVirtualNodes[0]->matrixNodeIndex());
}

void SP::Ph1::externalGridInjection::daeResidual(double ttime, const double state[], const double dstate_dt[], double resid[], std::vector<int>& off) {
	/* new state vector definintion:
		state[0]=node0_voltage
		state[1]=node1_voltage
		....
		state[n]=noden_voltage
		state[n+1]=component0_voltage
		state[n+2]=component0_inductance (not yet implemented)
		...
		state[m-1]=componentm_voltage
		state[m]=componentm_inductance
	*/

	int Pos1 = matrixNodeIndex(0);
	int Pos2 = matrixNodeIndex(1);
	int c_offset = off[0] + off[1]; //current offset for component
	int n_offset_1 = c_offset + Pos1 + 1;// current offset for first nodal equation
	int n_offset_2 = c_offset + Pos2 + 1;// current offset for second nodal equation
	resid[c_offset] = (state[Pos2] - state[Pos1]) - state[c_offset]; // Voltage equation for Resistor
	//resid[++c_offset] = ; //TODO : add inductance equation
	resid[n_offset_1] += mIntfCurrent(0, 0).real();
	resid[n_offset_2] += mIntfCurrent(0, 0).real();
	off[1] += 1;
}

Complex SP::Ph1::externalGridInjection::daeInitialize() {
	mIntfVoltage(0, 0) = mVoltageRef->get();
	return mVoltageRef->get();
}
