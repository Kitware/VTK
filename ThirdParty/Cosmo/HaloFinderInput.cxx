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

#include "HaloFinderInput.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include <stdlib.h>

using namespace std;

namespace cosmologytools {


const int LINESIZE = 512;

///////////////////////////////////////////////////////////////////////////
//
// Read parameter input file and store results
//
///////////////////////////////////////////////////////////////////////////

HaloFinderInput::HaloFinderInput()
{
  this->massConvertFactor = 1.0;
  this->distConvertFactor = 1.0;
  this->rhocConvertFactor = 1.0;
  this->sodMassConvertFactor = 1.0;

  this->hubbleConstant = 0.5;
  this->omegadm = 1.0;
  this->deut = 0.0;

  this->numSPHDensity = 1;
  this->numSubhaloNeighbors = 1;
  this->minSubhaloSize = 1;
  this->minFOFSubhalo = 1;
  this->alphaSubhalo = 1.0;
  this->betaSubhalo = 0.0;

  this->useMCPCenterFinder = 0;
  this->useMBPCenterFinder = 0;
  this->useMinimumPotential = 0;

  this->outputParticles = 0;
  this->outputHaloCatalog = 0;
  this->outputFOFProperties = 0;
  this->outputSODProperties = 0;
  this->outputSubhaloProperties = 0;

  this->minNeighForLinking = 1;
  this->minHaloOutputSize = 0;
  this->outputFrac = 1.0;
  this->outputPosVel = 1;
}

void HaloFinderInput::initialize(const string& inFile)
{
  ifstream inStr(inFile.c_str());
  if (!inStr) {
    cout << "Could not open input file " << inFile << endl;
    exit(-1);
  }

  char inBuf[LINESIZE];
  string keyword;
  string rest;

  while (inStr.getline(inBuf, LINESIZE)) {
    if (inBuf[0] != '#' && inStr.gcount() > 1) {

      getKeyword(inBuf, keyword, rest);
      istringstream line(rest.c_str());

      // Input/Output information
      if (keyword == "HALOFINDER_HEADER_VERSION")
        line >> this->headerVersion;
      else if (keyword == "INPUT_BASE_NAME")
        line >> this->inputBaseName;
      else if (keyword == "OUTPUT_BASE_NAME")
        line >> this->outputBaseName;
      else if (keyword == "INPUT_TYPE")
        line >> this->inputType;
      else if (keyword == "DISTRIBUTE_TYPE")
        line >> this->distributeType;

      // Units information
      else if (keyword == "MASS_CONVERT_FACTOR")
        line >> this->massConvertFactor;
      else if (keyword == "DIST_CONVERT_FACTOR")
        line >> this->distConvertFactor;
      else if (keyword == "RHOC_CONVERT_FACTOR")
        line >> this->rhocConvertFactor;
      else if (keyword == "SOD_MASS_CONVERT_FACTOR")
        line >> this->sodMassConvertFactor;

      // Problem size information
      else if (keyword == "BOX_SIZE")
        line >> this->boxSize;
      else if (keyword == "OVERLOAD_SIZE")
        line >> this->overloadSize;
      else if (keyword == "NUMBER_OF_PARTICLES")
        line >> this->numberOfParticles;

      // Halo finding parameters
      else if (keyword == "MINIMUM_PARTICLE_DISTANCE")
        line >> this->minParticleDistance;
      else if (keyword == "MINIMUM_NEIGH_FOR_LINKING")
        line >> this->minNeighForLinking;
      else if (keyword == "MINIMUM_PARTICLES_PER_HALO")
        line >> this->minParticlesPerHalo;
      else if (keyword == "OMEGADM")
        line >> this->omegadm;
      else if (keyword == "HUBBLE_CONSTANT")
        line >> this->hubbleConstant;
      else if (keyword == "DEUT")
        line >> this->deut;

      // Subhalo finding parameters
      else if (keyword == "NUM_SPH_DENSITY")
        line >> this->numSPHDensity;
      else if (keyword == "NUM_SUBHALO_NEIGHBORS")
        line >> this->numSubhaloNeighbors;
      else if (keyword == "MIN_SUBHALO_SIZE")
        line >> this->minSubhaloSize;
      else if (keyword == "MIN_FOF_SUBHALO")
        line >> this->minFOFSubhalo;
      else if (keyword == "ALPHA_SUBHALO")
        line >> this->alphaSubhalo;
      else if (keyword == "BETA_SUBHALO")
        line >> this->betaSubhalo;

      // Options
      else if (keyword == "USE_MCP_CENTER_FINDER")
        line >> this->useMCPCenterFinder;
      else if (keyword == "USE_MBP_CENTER_FINDER")
        line >> this->useMBPCenterFinder;
      else if (keyword == "USE_MINIMUM_POTENTIAL")
        line >> this->useMinimumPotential;

      else if (keyword == "OUTPUT_PARTICLES")
        line >> this->outputParticles;
      else if (keyword == "OUTPUT_HALO_CATALOG")
        line >> this->outputHaloCatalog;
      else if (keyword == "OUTPUT_FOF_PROPERTIES")
        line >> this->outputFOFProperties;
      else if (keyword == "OUTPUT_SOD_PROPERTIES")
        line >> this->outputSODProperties;
      else if (keyword == "OUTPUT_SUBHALO_PROPERTIES")
        line >> this->outputSubhaloProperties;

      else if (keyword == "MINIMUM_PARTICLES_PER_OUTPUT_HALO")
        line >> this->minHaloOutputSize;
      else if (keyword == "OUTPUT_PARTICLE_FRACTION")
        line >> this->outputFrac;
      else if (keyword == "OUTPUT_PARTICLE_POS_VEL")
        line >> this->outputPosVel;
    }
  }
  // Only one center finder can be set
  int center = this->useMinimumPotential + this->useMBPCenterFinder +
               this->useMCPCenterFinder;
  if (center > 1) {
    cout << "More than one center finder was selected in input" << endl;

    if (this->useMinimumPotential == 1) {
      this->useMBPCenterFinder = 0;
      this->useMCPCenterFinder = 0;
      cout << "Using minimum potential array" << endl;
    } else if (this->useMCPCenterFinder == 1) { 
      this->useMBPCenterFinder = 0;
      cout << "Using most connect particle" << endl;
    }
  }
}

HaloFinderInput::~HaloFinderInput()
{
}


/////////////////////////////////////////////////////////////////////////////
//
// Keywords start in position 0 and are delimited by white space
//
/////////////////////////////////////////////////////////////////////////////

void HaloFinderInput::getKeyword(char* inBuf, string& keyword, string& rest)
{
  string line(inBuf);
  string::size_type keyPos = line.find(' ');
  keyword = line.substr(0, keyPos);
  rest = line.substr(keyPos + 1);
}

}
