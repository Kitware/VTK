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

#ifndef BasicDefinition_h
#define BasicDefinition_h

#include "Definition.h"

#define ID_64
//#define GRID_64
//#define POSVEL_64

typedef vtkTypeInt64 ID_T;      // Particle and halo ids
typedef float POSVEL_T;        // Position,velocity
typedef float POTENTIAL_T;     // Potential
typedef float GRID_T;          // Grid types
typedef vtkTypeInt32  STATUS_T; // Dead (which neighbor) or alive particles
typedef vtkTypeUInt16 MASK_T;   // Other particle information

///////////////////////////////////////////////////////////////////////////
namespace cosmologytools {

const float MAX_FLOAT   = 1.0e15;
const float MIN_FLOAT   = -1.0e15;

const int   RECORD      = 0;    // Input data is by particle record
const int   BLOCK       = 1;    // Input data is blocked by variable

const int   DIMENSION   = 3;
const int   BUF_SZ      = 512;  // Character buffer

// Constants for Spherical Over Dense calculation
const double CHAIN_SIZE         = 2.0;           // Size for bucket mesh
const double RHO_C              = 2.77536627e11; // Critical density
                                                 // in (M_sun/h) / (Mpc/h)^3
const double RHO_RATIO          = 200.0;         // density/critical density
const double SOD_MASS           = 1.0e14;        // for initial SOD radius
                                                 // in (M_sun/h)
const double MIN_RADIUS_FACTOR  = 0.5;           // Factor of initial SOD radius
const double MAX_RADIUS_FACTOR  = 2.0;           // Factor of initial SOD radius
const int    MIN_SOD_SIZE       = 1000;          // Min FOF halo for SOD
const float  MIN_SOD_MASS       = 5.0e12;        // Min FOF mass for SOD
const int    NUM_SOD_BINS       = 20;            // Log bins for SOD halo

// Constants for subhalo finding
const int    NUM_CHILDREN	= 8;             // Barnes Hut octree
const double GRAVITY_C          = 43.015e-10;
                                // Gravitional constant for potential energy
const double ALPHA_SUBHALO	= 1.0;
                                // Controls cut/grow of subhalo
                                // 1.0 / alphaFactor is the number of times
                                // larger a candidate must be in order for the
                                // smaller to be cut rather than allowed to grow
                                // Set to 1.0 means always cut as in SUBFIND
                                // Set to 0.2 main halo always wins cut/grow
                                // Set to 0.01 small structures grow
const double BETA_SUBHALO	= 0.0;
                                // Controls the Poisson noise significance test
                                // If average density of particles in a group
                                // is greater than (1 + beta) * density of
                                // saddlepoint particle, group is considered
                                // significant and stands on its own
                                // Set to 0.0 means always significant
                                // Set to 0.25 helps small scale connectivity

const int    NUM_SPH_DENSITY	= 64;
        // Number of neigbor particles used in
                                // calculating SPH smoothing length and density
const int    NUM_SUBHALO_NEIGHBOR = 20;
                                // Number of closest neighbors looked at
                                // in placing particle in a subgroup
const int    MIN_SUBHALO_SIZE	= 20;
                                // Smallest allowed subhalo
const int    MIN_FOF_SUBHALO	= 2000;
                                // Smallest FOF halo which will have
                                // subhalo finding run on it

// Constants for speeding up unbind calculation on very large subhalos
const int    MAX_UNBIND_1	= 100;
                                // When unbinding reaches less than this
                                // number of particles in subgroup remove
                                // only one particle before running unbind again
const int    MAX_UNBIND_2	= 2000;
                                // When unbinding reaches less than this
                                // number of particles in subgroup remove
                                // remove (1 / FACTOR_UNBIND_1) total positive
                                // energy particles before running unbind again
const int    MAX_UNBIND_3	= 40000;
                                // When unbinding reaches less than this
                                // number of particles in subgroup remove
                                // remove (1 / FACTOR_UNBIND_2) total positive
                                // energy particles before running unbind again
                                // Also maximum subhalo candidate for unbinding
                                // Used for development because that stage
                                // takes so long and normally this is only
                                // the main subhalo and all particles unbound
                                // would go to fuzz
const int    FACTOR_UNBIND_1	= 4;
        // Between MAX_UNBIND_1 and MAX_UNBIND_2
                                // remove 25% of the positive total energy
                                // particles
const int    FACTOR_UNBIND_2	= 2;
        // Betweend MAX_UNBIND_2 and MAX_UNBIND_3
                                // remove 50% of the positive total energy
                                // particles
const int    MAX_UNBIND_DELETE	= 20;
                                // To speed up unbinding when large candidate
                                // reaches this number of particles with
                                // positive total energy just quit


// Cosmology record data in .cosmo format
const int   COSMO_FLOAT = 7;    // x,y,z location and velocity plus mass
const int   COSMO_INT   = 1;    // Particle id
const int   RECORD_SIZE = sizeof(POSVEL_T) * COSMO_FLOAT +
                          sizeof(ID_T) * COSMO_INT;

const bool  ENFORCE_MAX_READ = false;
const int   MAX_READ    = 8000000;
                                // Maximum number of particles to read at a time
                                // Multipled by COSMO_FLOAT floats
                                // makes the largest MPI allowed buffer

const float DEAD_FACTOR = 1.20f; // Number of dead allocated is % more than max

const int   ALIVE       = -1;   // Particle belongs to this processor
const int   MIXED       = ALIVE - 1;
                                // For a trick to quickly know what
                                // particles should be output

const int   UNMARKED    = -1;   // Mixed halo needs MASTER to arbitrate
const int   INVALID     = 0;    // Mixed halo is not recorded on processor
const int   VALID       = 1;    // Mixed halo is recorded on processor

const int   MASTER      = 0;    // Processor to do merge step

const int   MERGE_COUNT = 20;   // Number of tags to merge on in mixed

// Parameters for center finding
const int   MBP_THRESHOLD = 5000; // Threshold between n^2 and AStar methods
const int   MCP_THRESHOLD = 8000;// Threshold between n^2 and Chain methods
const int   MCP_CHAIN_FACTOR = 5; // Subdivide bb for building chaining mesh

//
// Neighbors are enumerated so that particles can be attached to the correct
// neighbor, but these pairs must be preserved for the ParticleExchange.
// Every processor should be able to send and receive on every iteration of
// the exchange, so if everyone sends RIGHT and receives LEFT it works
//
// Do not change this pairing order.
//
enum NEIGHBOR
{
  X0,                   // Left face
  X1,                   // Right face

  Y0,                   // Bottom face
  Y1,                   // Top face

  Z0,                   // Front face
  Z1,                   // Back face

  X0_Y0,                // Left   bottom edge
  X1_Y1,                // Right  top    edge

  X0_Y1,                // Left   top    edge
  X1_Y0,                // Right  bottom edge

  Y0_Z0,                // Bottom front  edge
  Y1_Z1,                // Top    back   edge

  Y0_Z1,                // Bottom back   edge
  Y1_Z0,                // Top    front  edge

  Z0_X0,                // Front  left   edge
  Z1_X1,                // Back   right  edge

  Z0_X1,                // Front  right  edge
  Z1_X0,                // Back   left   edge

  X0_Y0_Z0,             // Left  bottom front corner
  X1_Y1_Z1,             // Right top    back  corner

  X0_Y0_Z1,             // Left  bottom back  corner
  X1_Y1_Z0,             // Right top    front corner

  X0_Y1_Z0,             // Left  top    front corner
  X1_Y0_Z1,             // Right bottom back  corner

  X0_Y1_Z1,             // Left  top    back  corner
  X1_Y0_Z0              // Right bottom front corner
};

const int NUM_OF_NEIGHBORS      = 26;

// Header for Gadget input files
const int GADGET_GAS            = 0;
const int GADGET_HALO           = 1;
const int GADGET_DISK           = 2;
const int GADGET_BULGE          = 3;
const int GADGET_STARS          = 4;
const int GADGET_BOUND          = 5;
const int NUM_GADGET_TYPES      = 6;    // Types of gadget particles

const int GADGET_HEADER_SIZE    = 256;  // Size when the endian matches
const int GADGET_HEADER_SIZE_SWP= 65536;// Size when the endian doesn't match
const int GADGET_FILL           = 60;   // Current fill to HEADER SIZE
const int GADGET_SKIP           = 4;    // Bytes the indicate block size
const int GADGET_2_SKIP         = 16;   // Extra bytes in gadget-2

const int GADGET_1              = 1;
const int GADGET_2              = 2;

struct GadgetHeader {
  int      npart[NUM_GADGET_TYPES];
  double   mass[NUM_GADGET_TYPES];
  double   time;
  double   redshift;
  int      flag_sfr;
  int      flag_feedback;
  int      npartTotal[NUM_GADGET_TYPES];
  int      flag_cooling;
  int      num_files;
  double   BoxSize;
  double   Omega0;
  double   OmegaLambda;
  double   HubbleParam;
  int      flag_stellarage;
  int      flag_metals;
  int      HighWord[NUM_GADGET_TYPES];
  int      flag_entropy;
  char     fill[GADGET_FILL];
};

} /* end namespace cosmologytools */
#endif
