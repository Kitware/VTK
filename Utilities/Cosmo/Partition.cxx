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

#include <iostream>

#include "Partition.h"

/////////////////////////////////////////////////////////////////////////
//
// Static class to control MPI and the partitioning of processors in
// a Cartesian grid across the problem space.
//
/////////////////////////////////////////////////////////////////////////

#ifndef USE_SERIAL_COSMO
MPI_Comm Partition::cartComm;
#endif

int Partition::numProc = 0;
int Partition::myProc = -1;
int Partition::decompSize[DIMENSION];
int Partition::myPosition[DIMENSION];
int Partition::neighbor[NUM_OF_NEIGHBORS];
int Partition::initialized = 0;

Partition::Partition()
{
}

Partition::~Partition()
{
}

/////////////////////////////////////////////////////////////////////////
//
// Initialize MPI, allocate the processors across a Cartesian grid of
// DIMENSION size and record this processors position, id and neighbor ids
//
/////////////////////////////////////////////////////////////////////////

//void Partition::initialize(int& argc, char** argv)
void Partition::initialize()
{
  if(!initialized)
    {
#ifndef USE_SERIAL_COSMO

#ifdef USE_VTK_COSMO
    // this is for when it is compiled against MPI but single processor
    // on ParaView (client only, it won't MPI_Init itself)
    int temp;
    MPI_Initialized(&temp);
    if(!temp) 
      {
      temp = 0;
      MPI_Init(&temp, 0);
      }
#endif

    // Start up MPI
    //MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myProc);
    MPI_Comm_size(MPI_COMM_WORLD, &numProc);
#endif    

    for (int dim = 0; dim < DIMENSION; dim++)
      decompSize[dim] = 0;
    
#ifdef USE_SERIAL_COSMO
    myProc = 0;
    numProc = 1;

    for(int dim = 0; dim < DIMENSION; dim = dim + 1)
      {
      decompSize[dim] = 1;
      myPosition[dim] = 0;
      }
#else
    int periodic[] = {1, 1, 1};
    int reorder = 1;

    // Compute the number of processors in each dimension
    MPI_Dims_create(numProc, DIMENSION, decompSize);
    
    // Create the Cartesion communicator
    MPI_Cart_create(MPI_COMM_WORLD,
                    DIMENSION, decompSize, periodic, reorder, &cartComm);
    
    // Reset my rank if it changed
    MPI_Comm_rank(cartComm, &myProc);
    
    // Get this processor's position in the Cartesian topology
    MPI_Cart_coords(cartComm, myProc, DIMENSION, myPosition);
#endif    

    // Set all my neighbor processor ids for communication
    setNeighbors();
    
#ifndef USE_VTK_COSMO
    if (myProc == 0)
      cout << "Decomposition: [" << decompSize[0] << ":"
           << decompSize[1] << ":" << decompSize[2] << "]" << endl; 
#endif

    initialized = 1;
    }
}

/////////////////////////////////////////////////////////////////////////
//
// Return the decomposition size of this problem
//
/////////////////////////////////////////////////////////////////////////

void Partition::getDecompSize(int size[])
{
  for (int dim = 0; dim < DIMENSION; dim++)
    size[dim] = decompSize[dim];
}

/////////////////////////////////////////////////////////////////////////
//
// Return the position of this processor within the Cartesian grid
//
/////////////////////////////////////////////////////////////////////////

void Partition::getMyPosition(int pos[])
{
  for (int dim = 0; dim < DIMENSION; dim++)
    pos[dim] = myPosition[dim];
}

/////////////////////////////////////////////////////////////////////////
//
// Return the ranks of the neighbors of this processor using the
// description in Definition.h
//
/////////////////////////////////////////////////////////////////////////

void Partition::getNeighbors(int neigh[])
{
  for (int n = 0; n < NUM_OF_NEIGHBORS; n++)
    neigh[n] = neighbor[n];
}

/////////////////////////////////////////////////////////////////////////
//
// Get the id of a particular processor given its position in the topology
//
/////////////////////////////////////////////////////////////////////////

int Partition::getNeighbor
#ifdef USE_SERIAL_COSMO
  (int , int , int )
#else
  (int xpos, int ypos, int zpos)
#endif
{
#ifdef USE_SERIAL_COSMO
  return 0;
#else
  static int pos[DIMENSION];
  pos[0] = xpos;
  pos[1] = ypos;
  pos[2] = zpos;

  int neighborProc;
  MPI_Cart_rank(cartComm, pos, &neighborProc);
  return neighborProc;
#endif
}

/////////////////////////////////////////////////////////////////////////
//    
// Every processor will have 26 neighbors because the cosmology structure
// is a 3D torus.  Each will have 6 face neighbors, 12 edge neighbors and
// 8 corner neighbors.
//
/////////////////////////////////////////////////////////////////////////

void Partition::setNeighbors()
{ 
  // Where is this processor in the decomposition
  int xpos = myPosition[0];
  int ypos = myPosition[1];
  int zpos = myPosition[2];

  // Face neighbors
  neighbor[X0] = Partition::getNeighbor(xpos-1, ypos, zpos);
  neighbor[X1] = Partition::getNeighbor(xpos+1, ypos, zpos);
  neighbor[Y0] = Partition::getNeighbor(xpos, ypos-1, zpos);
  neighbor[Y1] = Partition::getNeighbor(xpos, ypos+1, zpos);
  neighbor[Z0] = Partition::getNeighbor(xpos, ypos, zpos-1);
  neighbor[Z1] = Partition::getNeighbor(xpos, ypos, zpos+1);

  // Edge neighbors
  neighbor[X0_Y0] = Partition::getNeighbor(xpos-1, ypos-1, zpos);
  neighbor[X0_Y1] = Partition::getNeighbor(xpos-1, ypos+1, zpos);
  neighbor[X1_Y0] = Partition::getNeighbor(xpos+1, ypos-1, zpos);
  neighbor[X1_Y1] = Partition::getNeighbor(xpos+1, ypos+1, zpos);
  
  neighbor[Y0_Z0] = Partition::getNeighbor(xpos, ypos-1, zpos-1);
  neighbor[Y0_Z1] = Partition::getNeighbor(xpos, ypos-1, zpos+1);
  neighbor[Y1_Z0] = Partition::getNeighbor(xpos, ypos+1, zpos-1);
  neighbor[Y1_Z1] = Partition::getNeighbor(xpos, ypos+1, zpos+1);
  
  neighbor[Z0_X0] = Partition::getNeighbor(xpos-1, ypos, zpos-1);
  neighbor[Z0_X1] = Partition::getNeighbor(xpos+1, ypos, zpos-1);
  neighbor[Z1_X0] = Partition::getNeighbor(xpos-1, ypos, zpos+1);
  neighbor[Z1_X1] = Partition::getNeighbor(xpos+1, ypos, zpos+1);
  
  // Corner neighbors
  neighbor[X0_Y0_Z0] = Partition::getNeighbor(xpos-1, ypos-1, zpos-1);
  neighbor[X1_Y0_Z0] = Partition::getNeighbor(xpos+1, ypos-1, zpos-1);
  neighbor[X0_Y1_Z0] = Partition::getNeighbor(xpos-1, ypos+1, zpos-1);
  neighbor[X1_Y1_Z0] = Partition::getNeighbor(xpos+1, ypos+1, zpos-1);
  neighbor[X0_Y0_Z1] = Partition::getNeighbor(xpos-1, ypos-1, zpos+1);
  neighbor[X1_Y0_Z1] = Partition::getNeighbor(xpos+1, ypos-1, zpos+1);
  neighbor[X0_Y1_Z1] = Partition::getNeighbor(xpos-1, ypos+1, zpos+1);
  neighbor[X1_Y1_Z1] = Partition::getNeighbor(xpos+1, ypos+1, zpos+1);
}

/////////////////////////////////////////////////////////////////////////
//
// Shut down MPI
//
/////////////////////////////////////////////////////////////////////////

void Partition::finalize()
{
  numProc = 0;
  myProc = -1;

  //MPI_Finalize();
}
