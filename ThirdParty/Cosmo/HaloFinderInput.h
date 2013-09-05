/*=========================================================================
                                                                                
Copyright (c) 2007, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007. Los Alamos National Security, LLC. 
This software was produced under U.S. Government contract DE-AC52-06NA25396 
for Los Alamos National Laboratory (LANL), which is operated by 
Los Alamos National Security, LLC for the U.S. Department of Energy. 
The U.S. Government has rights to use, reproduce, and distribute this software. 
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  
If software is modified to produce derivative works, such modified software 
should be clearly marked, so as not to confuse it with the version available 
from LANL.
 
Additionally, redistribution and use in source and binary forms, with or 
without modification, are permitted provided that the following conditions 
are met:
-   Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer. 
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software 
    without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
                                                                                
=========================================================================*/

// .NAME HaloFinderInput - input parameters for HaloFinder
//

#ifndef HaloFinderInput_h
#define HaloFinderInput_h

#include <string>

using std::string;

namespace cosmologytools {

class HaloFinderInput {
public:
  HaloFinderInput();
  ~HaloFinderInput();

  void initialize(const string& inFile);

  void   getKeyword(char* inBuf, string& keyword, string& rest);

  string getInputBaseName()		{ return this->inputBaseName; }
  string getOutputBaseName()		{ return this->outputBaseName; }
  string getInputType()			{ return this->inputType; }
  string getDistributeType()		{ return this->distributeType; }

  float  getMassConvertFactor()		{ return this->massConvertFactor; }
  float  getDistConvertFactor()		{ return this->distConvertFactor; }
  float  getRHOCConvertFactor()		{ return this->rhocConvertFactor; }
  float  getSODMassConvertFactor()	{ return this->sodMassConvertFactor; }

  float  getBoxSize()			{ return this->boxSize; }
  float  getOverloadSize()		{ return this->overloadSize; }
  int    getNumberOfParticles()		{ return this->numberOfParticles; }

  int    getMinParticlesPerHalo()	{ return this->minParticlesPerHalo; }
  float  getMinParticleDistance()	{ return this->minParticleDistance; }
  int    getMinNeighForLinking()	{ return this->minNeighForLinking; }
  float  getOmegadm()			{ return this->omegadm; }
  float  getHubbleConstant()		{ return this->hubbleConstant; }
  float  getDeut()			{ return this->deut; }

  int    getNumSPHDensity()		{ return this->numSPHDensity; }
  int    getNumSubhaloNeighbors()	{ return this->numSubhaloNeighbors; }
  int    getMinSubhaloSize()		{ return this->minSubhaloSize; }
  int    getMinFOFSubhalo()		{ return this->minFOFSubhalo; }
  float  getAlphaSubhalo()		{ return this->alphaSubhalo; }
  float  getBetaSubhalo()		{ return this->betaSubhalo; }

  int    getUseMCPCenterFinder()	{ return this->useMCPCenterFinder; }
  int    getUseMBPCenterFinder()	{ return this->useMBPCenterFinder; }
  int    getUseMinimumPotential()	{ return this->useMinimumPotential; }

  int    getOutputParticles()		{ return this->outputParticles; }
  int    getOutputHaloCatalog()		{ return this->outputHaloCatalog; }
  int    getOutputFOFProperties()	{ return this->outputFOFProperties; }
  int    getOutputSODProperties()	{ return this->outputSODProperties; }
  int    getOutputSubhaloProperties()	{ return this->outputSubhaloProperties; }

  int    getMinHaloOutputSize()         { return this->minHaloOutputSize; }
  float  getOutputFrac()                { return this->outputFrac; }
  int    getOutputPosVel()              { return this->outputPosVel; }

private:
  string headerVersion;

  // Input and output
  string inputBaseName;		// Base name of input files ending in '.'
				// if processor id follows
  string outputBaseName;	// Base name of output files

  string inputType;		// RECORD for .cosmo, BLOCK for .gadget2
  string distributeType;	// ROUND_ROBIN or ONE_TO_ONE

  // Conversion factors on units of mass and distance
  // Defaults to Msun/h and Mpc/h but can use any units as long as factors match
  float  massConvertFactor;	// Every mass read is multiplied by this
  float  distConvertFactor;	// Every location read is multiplied by this
  float  rhocConvertFactor;	// RHO_C is changed if units are not defaults
  float  sodMassConvertFactor;	// SOD_MASS_FACTOR changed in units are not Msun

  // Halo finding parameters
  float  boxSize;		// Physical box of dataset
  float  overloadSize;		// Overloaded zone around each processor
  int    numberOfParticles;	// np^3 particles in dataset

  float  minParticleDistance;	// Distance between particles in halo (bb)
  int    minNeighForLinking;    // The number of neighbors needed for linking (nmin)
  int    minParticlesPerHalo;	// Minimum number of particles in halo (pmin)
  float  omegadm;
  float  hubbleConstant;	// Hubble constant
  float  deut;

  // Subhalo finding parameters
  int    numSPHDensity;		// Number of neighbors for calculating density
  int    numSubhaloNeighbors;	// Number of close neighbors used in subgrouping
  int    minSubhaloSize;	// Minimum particles in a subhalo
  int    minFOFSubhalo;		// Smallest FOF halo to have subfinding run on
  float  alphaSubhalo;		// Factor for cut/grow criteria
  float  betaSubhalo;		// Factor for Poisson noise significance

  // Options
  int    useMCPCenterFinder;	// Run the MCP algorithm for FOF centers
  int    useMBPCenterFinder;	// Run the MBP algorithm for FOF centers
  int    useMinimumPotential;	// Use the minimum potential array

  int    outputParticles;	// Output every particle with halo tags
  int    outputHaloCatalog;	// Output one item for every halo
  int    outputFOFProperties;	// Output FOF halo property summary
  int    outputSODProperties;	// Output SOD halo property summary
  int    outputSubhaloProperties; // Output subhalo property summary

  int    minHaloOutputSize;     // Don't output particles in halos smaller than
                                // this number of particles
  float  outputFrac;            // Tthe fraction of all particles in halos meeting
                                // the halo size cut to output
  int    outputPosVel;          // Output particle position and velocity
};

}
#endif
