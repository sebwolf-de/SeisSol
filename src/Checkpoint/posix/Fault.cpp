/**
 * @file
 * This file is part of SeisSol.
 *
 * @author Sebastian Rettenberger (sebastian.rettenberger AT tum.de, http://www5.in.tum.de/wiki/index.php/Sebastian_Rettenberger)
 *
 * @section LICENSE
 * Copyright (c) 2015-2016, SeisSol Group
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @section DESCRIPTION
 */

#include "Fault.h"

#include "Kernels/precision.hpp"

bool seissol::checkpoint::posix::Fault::init(
		double* mu, double* slipRate1, double* slipRate2, double* slip, double* slip1, double* slip2,
		double* state, double* strength,
		unsigned int numSides, unsigned int numBndGP)
{
	seissol::checkpoint::Fault::init(mu, slipRate1, slipRate2, slip, slip1, slip2, state, strength,
			numSides, numBndGP);

	if (numSides == 0)
		return true;

	return exists();
}

void seissol::checkpoint::posix::Fault::load(int &timestepFault)
{
	if (numSides() == 0)
		return;

	logInfo(rank()) << "Loading fault checkpoint";

	seissol::checkpoint::CheckPoint::load();

	int file = open();
	checkErr(file);

	// Skip identifier
	checkErr(lseek64(file, sizeof(unsigned long), SEEK_SET));

	// Read header
	checkErr(read(file, &timestepFault, sizeof(timestepFault)),	sizeof(timestepFault));

	// Read data
	for (unsigned int i = 0; i < NUM_VARIABLES; i++)
		checkErr(read(file, data(i), numSides() * numBndGP() * sizeof(real)));

	// Close the file
	checkErr(::close(file));
}

void seissol::checkpoint::posix::Fault::write(int timestepFault)
{
	EPIK_TRACER("CheckPointFault_write");
	SCOREP_USER_REGION("CheckPointFault_write", SCOREP_USER_REGION_TYPE_FUNCTION);

	if (numSides() == 0)
		return;

	logInfo(rank()) << "Writing fault check point.";

	// Skip identifier
	checkErr(lseek64(file(), sizeof(unsigned long), SEEK_SET));

	// Write the header
	EPIK_USER_REG(r_write_header, "checkpoint_write_fault_header");
	SCOREP_USER_REGION_DEFINE(r_write_header);
	EPIK_USER_START(r_write_header);
	SCOREP_USER_REGION_BEGIN(r_write_header, "checkpoint_write_fault_header", SCOREP_USER_REGION_TYPE_COMMON);

	checkErr(::write(file(), &timestepFault, sizeof(timestepFault)), sizeof(timestepFault));

	EPIK_USER_END(r_write_header);
	SCOREP_USER_REGION_END(r_write_header);

	// Save data
	EPIK_USER_REG(r_write_wavefield, "checkpoint_write_fault");
	SCOREP_USER_REGION_DEFINE(r_write_fault);
	EPIK_USER_START(r_write_wavefield);
	SCOREP_USER_REGION_BEGIN(r_write_fault, "checkpoint_write_fault", SCOREP_USER_REGION_TYPE_COMMON);

	for (unsigned int i = 0; i < NUM_VARIABLES; i++)
		checkErr(::write(file(), data(i), numSides() * numBndGP() * sizeof(real)));

	EPIK_USER_END(r_write_fault);
	SCOREP_USER_REGION_END(r_write_fault);

	// Finalize the checkpoint
	finalizeCheckpoint();

	logInfo(rank()) << "Writing fault check point. Done.";
}
