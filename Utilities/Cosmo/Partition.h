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

// .NAME Partition - Partition MPI processors into cartesian grid
//
// .SECTION Description
// Partition allows MPI to divide the number of processors it is given and
// to set the position of this processor within the Cartesian grid.  Using
// that information with wraparound, all neighbors of a processor are
// also computed.  This class is static and will be shared by all classes
// within the infrastructure.

#ifndef Partition_h
#define Partition_h


#ifdef USE_VTK_COSMO
#include "CosmoDefinition.h"
#include "vtkstd/string"
#include "vtkstd/vector"

using namespace vtkstd;
#else
#include "Definition.h"
#include <string>
#include <vector>

using namespace std;
#endif

#ifdef USE_VTK_COSMO
class COSMO_EXPORT Partition {
#else
class Partition {
#endif
public:
  Partition();
  ~Partition();

  // Control MPI and the Cartesian topology
  //static void initialize(int& argc, char** argv);
  static void initialize();
  static void finalize();

  // Set the processor numbers of neighbors in all directions
  static void setNeighbors();

#ifndef USE_SERIAL_COSMO
  static MPI_Comm getComm()       { return cartComm; }
#endif

  static int  getMyProc()               { return myProc; }
  static int  getNumProc()              { return numProc; }

  static void getDecompSize(int size[]);
  static void getMyPosition(int pos[]);
  static void getNeighbors(int neigh[]);

  static int  getNeighbor(int xpos, int ypos, int zpos);

private:
  static int myProc;                    // My processor number
  static int numProc;                   // Total number of processors
  static int initialized;

#ifndef USE_SERIAL_COSMO
  static MPI_Comm cartComm;             // Cartesian communicator
#endif

  static int decompSize[DIMENSION];     // Number of processors in each dim
  static int myPosition[DIMENSION];     // My index in cartesian communicator

  static int neighbor[NUM_OF_NEIGHBORS];// Neighbor processor ids
};

#endif
