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

// .NAME ParticleDistribute - distribute particles to processors
//
// .SECTION Description
// ParticleDistribute takes a series of data files containing RECORD style
// .cosmo data or Gadget style BLOCK data
// along with parameters defining the box size for the data and for
// determining halos within the particle data.  It distributes the data
// across processors including a healthy dead zone of particles belonging
// to neighbor processors.  By definition all halos can be determined
// completely for any processor because of this dead zone.  The serial
// halo finder is called on each processor.
//

#ifndef ParticleDistribute_h
#define ParticleDistribute_h

#include "CosmoDefinition.h"
#include "Message.h"

#include <cstdlib>

#ifdef USE_VTK_COSMO 
#include "vtkstd/string"
#include "vtkstd/vector"

using namespace vtkstd;
#else 
#include <string>
#include <vector>

using namespace std;
#endif

class COSMO_EXPORT ParticleDistribute {
public:
  ParticleDistribute();
  ~ParticleDistribute();

  // Set parameters particle distribution
  void setParameters(
        const string& inName,   // Base file name to read from
        POSVEL_T rL,            // Box size of the physical problem
        string dataType);       // BLOCK or RECORD structured input data

  // Set neighbor processor numbers and calculate dead regions
  void initialize();

  // Read particle files per processor and share round robin with others
  // extracting only the alive particles
  void readParticlesRoundRobin(int reserveQ=0);
  void partitionInputFiles();

  // Read one particle file per processor with alive particles 
  // and correct topology
  void readParticlesOneToOne(int reserveQ=0);

  // Get particle counts for allocating buffers
  void findFileParticleCount();

  // Round robin version must buffer for MPI sends to other processors
  void readFromRecordFile(
        ifstream* inStream,     // Stream to read from
        int firstParticle,      // First particle index to read in this chunk
        int numberOfParticles,  // Number of particles to read in this chunk
        POSVEL_T* fblock,       // Buffer for read in data
        ID_T* iblock,           // Buffer for read in data
        Message* message);      // Message buffer for distribution

  void readFromBlockFile(
        ifstream* inStream,     // Stream to read from
        int firstParticle,      // First particle index to read in this chunk
        int numberOfParticles,  // Number of particles to read in this chunk
        int totParticles,       // Total particles (used to get offset)
        POSVEL_T* lblock,       // Buffer for read in location data
        POSVEL_T* vblock,       // Buffer for read in velocity data
        ID_T* iblock,           // Buffer for read in data
        Message* message);      // Message buffer for distribution

  // One to one version of read is simpler with no MPI buffering
  void readFromRecordFile();
  void readFromBlockFile();

  // Collect local alive particles from the input buffers
  void distributeParticles(
        Message* message1,      // Double buffering for reads
        Message* message2);     // Double buffering for reads
  void collectLocalParticles(Message* message);

  // Return data needed by other software
  int     getParticleCount()    { return this->particleCount; }

  void setParticles(vector<POSVEL_T>* xx,
                    vector<POSVEL_T>* yy,
                    vector<POSVEL_T>* zz,
                    vector<POSVEL_T>* vx,
                    vector<POSVEL_T>* vy,
                    vector<POSVEL_T>* vz,
                    vector<POSVEL_T>* mass,
                    vector<ID_T>* tag);

  vector<POSVEL_T>* getXLocation()      { return this->xx; }
  vector<POSVEL_T>* getYLocation()      { return this->yy; }
  vector<POSVEL_T>* getZLocation()      { return this->zz; }
  vector<POSVEL_T>* getXVelocity()      { return this->vx; }
  vector<POSVEL_T>* getYVelocity()      { return this->vy; }
  vector<POSVEL_T>* getZVelocity()      { return this->vz; }
  vector<POSVEL_T>* getMass()           { return this->ms; }
  vector<ID_T>* getTag()                { return this->tag; }

private:
  int    myProc;                // My processor number
  int    numProc;               // Total number of processors

  string baseFile;              // Base name of input particle files
  int    inputType;             // BLOCK or RECORD structure
  int    maxFiles;              // Maximum number of files per processor
  vector<string> inFiles;       // Files read by this processor
  vector<long> fileParticles;   // Number of particles in files on processor

  struct CosmoHeader cosmoHeader;// Gadget file header

  long   maxParticles;          // Largest number of particles in any file
  long   maxRead;               // Largest number of particles read at one time
  int    maxReadsPerFile;       // Max number of reads per file

  long   totalParticles;        // Number of particles on all files
  int    headerSize;            // For BLOCK files

  int    nextProc;              // Where to send buffers to be shared
  int    prevProc;              // Where to receive buffers from be shared
  int    numberOfFiles;         // Number of input files total
  int    processorsPerFile;     // Multiple processors read same file
  int    numberOfFileSends;     // Number of round robin sends to share buffers
  int    maxFileSends;          // Max of round robin sends to share buffers

  int    layoutSize[DIMENSION]; // Decomposition of processors
  int    layoutPos[DIMENSION];  // Position of this processor in decomposition

  long   np;                    // Number of particles in the problem
  POSVEL_T boxSize;             // Physical box size (rL)

  long   numberOfAliveParticles;

  long   particleCount;         // Running index used to store data
                                // Ends up as the number of alive plus dead

  POSVEL_T minAlive[DIMENSION]; // Minimum alive particle location on processor
  POSVEL_T maxAlive[DIMENSION]; // Maximum alive particle location on processor

  int    neighbor[NUM_OF_NEIGHBORS];            // Neighbor processor ids

  vector<POSVEL_T>* xx;         // X location for particles on this processor
  vector<POSVEL_T>* yy;         // Y location for particles on this processor
  vector<POSVEL_T>* zz;         // Z location for particles on this processor
  vector<POSVEL_T>* vx;         // X velocity for particles on this processor
  vector<POSVEL_T>* vy;         // Y velocity for particles on this processor
  vector<POSVEL_T>* vz;         // Z velocity for particles on this processor
  vector<POSVEL_T>* ms;         // Mass for particles on this processor
  vector<ID_T>* tag;            // Id tag for particles on this processor
};

#endif
