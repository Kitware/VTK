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

#ifdef USE_PARALLEL_VPIC
#include <mpi.h>
#endif

#include "GridExchange.h"

#include <iostream>
#include <iomanip>

using namespace std;

/////////////////////////////////////////////////////////////////////////
//
// GridExchange will take a pointer to contiguous memory, the size of
// memory in each dimension, and the amount of ghost grid information
// to be shared on the front and back of every dimension.  From this
// GridExchange can calculate what part of the contiguous memory must be
// packed to share with each of the neighbors, and what part of its own
// memory must be used to unpack similar information from each neighbor.
//
// This can be accomplished by recording for every neighbor the send
// origin and send size, the receive origin and receive size.
//
/////////////////////////////////////////////////////////////////////////

GridExchange::GridExchange(
                        int rank,
                        int totalRank,
                        int* decomp,
                        int* size,
                        int ghostSize0,
                        int ghostSize1)
{
  // Get the number of processors running this problem and rank
  this->numProc = totalRank;
  this->myProc = rank;

  // Get the number of processors in each dimension
  for (int dim = 0; dim < DIMENSION; dim++) {
    this->layoutSize[dim] = decomp[dim];
  }

  // Layout a 3D decomposition grid for processor ids
  this->layoutID = new int**[this->layoutSize[0]];
  for (int i = 0; i < this->layoutSize[0]; i++) {
    this->layoutID[i] = new int*[this->layoutSize[1]];
    for (int j = 0; j < this->layoutSize[1]; j++)
      this->layoutID[i][j] = new int[this->layoutSize[2]];
  }

  for (int k = 0; k < this->layoutSize[2]; k++)
    for (int j = 0; j < this->layoutSize[1]; j++)
      for (int i = 0; i < this->layoutSize[0]; i++) {
        int procID =
              k * (this->layoutSize[0] * this->layoutSize[1]) +
              j * this->layoutSize[0] + i;
        this->layoutID[i][j][k] = procID;
        if (procID == this->myProc) {
          this->layoutPos[0] = i;
          this->layoutPos[1] = j;
          this->layoutPos[2] = k;
        }
      }

  // Set neighbors of this processor including the wraparound
  setNeighbors();

  // Store sizes for this exchange which depend on mine and ghost grid zones
  this->ghost0 = ghostSize0;
  this->ghost1 = ghostSize1;

  int maxGridSize = 0;
  for (int dim = 0; dim < DIMENSION; dim++) {
    this->totalSize[dim] = size[dim];
    this->mine[dim] = size[dim] - this->ghost0 - this->ghost1;
    if (mine[dim] > maxGridSize)
      maxGridSize = mine[dim];
  }

  // Calculate the MPI message size for biggest grid region
  this->bufferSize = maxGridSize * maxGridSize *
                     max(this->ghost0, this->ghost1);
  this->sendBuffer = new GRID_T[this->bufferSize];
  this->recvBuffer = new GRID_T[this->bufferSize];

  // Initialize this exchanger with a given size to save the calculation
  // every time data is to be sent
  initialize();
}

GridExchange::~GridExchange()
{
  delete [] this->sendBuffer;
  delete [] this->recvBuffer;
  for (int i = 0; i < this->layoutSize[0]; i++) {
    for (int j = 0; j < this->layoutSize[1]; j++)
      delete [] this->layoutID[i][j];
    delete [] this->layoutID[i];
  }
  delete this->layoutID;
}

/////////////////////////////////////////////////////////////////////////
//
// Every processor will have 26 neighbors because the cosmology structure
// is a 3D torus.  Each will have 6 face neighbors, 12 edge neighbors and
// 8 corner neighbors.
//
/////////////////////////////////////////////////////////////////////////

void GridExchange::setNeighbors()
{
  // Where is this processor in the decomposition
  int xpos = this->layoutPos[0];
  int ypos = this->layoutPos[1];
  int zpos = this->layoutPos[2];

  int xpos0 = xpos - 1;
  int xpos1 = xpos + 1;
  int ypos0 = ypos - 1;
  int ypos1 = ypos + 1;
  int zpos0 = zpos - 1;
  int zpos1 = zpos + 1;

  if (xpos0 == -1) xpos0 = this->layoutSize[0] - 1;
  if (ypos0 == -1) ypos0 = this->layoutSize[1] - 1;
  if (zpos0 == -1) zpos0 = this->layoutSize[2] - 1;
  if (xpos1 == this->layoutSize[0]) xpos1 = 0;
  if (ypos1 == this->layoutSize[1]) ypos1 = 0;
  if (zpos1 == this->layoutSize[2]) zpos1 = 0;

  // Face neighborz
  this->neighbor[X0] = this->layoutID[xpos0][ypos][zpos];
  this->neighbor[X1] = this->layoutID[xpos1][ypos][zpos];
  this->neighbor[Y0] = this->layoutID[xpos][ypos0][zpos];
  this->neighbor[Y1] = this->layoutID[xpos][ypos1][zpos];
  this->neighbor[Z0] = this->layoutID[xpos][ypos][zpos0];
  this->neighbor[Z1] = this->layoutID[xpos][ypos][zpos1];

  // Edge neighbors
  this->neighbor[X0_Y0] = this->layoutID[xpos0][ypos0][zpos];
  this->neighbor[X0_Y1] = this->layoutID[xpos0][ypos1][zpos];
  this->neighbor[X1_Y0] = this->layoutID[xpos1][ypos0][zpos];
  this->neighbor[X1_Y1] = this->layoutID[xpos1][ypos1][zpos];

  this->neighbor[Y0_Z0] = this->layoutID[xpos][ypos0][zpos0];
  this->neighbor[Y0_Z1] = this->layoutID[xpos][ypos0][zpos1];
  this->neighbor[Y1_Z0] = this->layoutID[xpos][ypos1][zpos0];
  this->neighbor[Y1_Z1] = this->layoutID[xpos][ypos1][zpos1];

  this->neighbor[Z0_X0] = this->layoutID[xpos0][ypos][zpos0];
  this->neighbor[Z0_X1] = this->layoutID[xpos1][ypos][zpos0];
  this->neighbor[Z1_X0] = this->layoutID[xpos0][ypos][zpos1];
  this->neighbor[Z1_X1] = this->layoutID[xpos1][ypos][zpos1];

  // Corner neighbors
  this->neighbor[X0_Y0_Z0] = this->layoutID[xpos0][ypos0][zpos0];
  this->neighbor[X1_Y0_Z0] = this->layoutID[xpos1][ypos0][zpos0];
  this->neighbor[X0_Y1_Z0] = this->layoutID[xpos0][ypos1][zpos0];
  this->neighbor[X1_Y1_Z0] = this->layoutID[xpos1][ypos1][zpos0];
  this->neighbor[X0_Y0_Z1] = this->layoutID[xpos0][ypos0][zpos1];
  this->neighbor[X1_Y0_Z1] = this->layoutID[xpos1][ypos0][zpos1];
  this->neighbor[X0_Y1_Z1] = this->layoutID[xpos0][ypos1][zpos1];
  this->neighbor[X1_Y1_Z1] = this->layoutID[xpos1][ypos1][zpos1];
}

/////////////////////////////////////////////////////////////////////////
//
// Calculate information needed for neighbor exchange of ghost grids
// For each neighbor sent to their is an offset into the mine grid and
// a size of the region to be sent.  For each neighbor received from there
// is an offset into the ghost grid and a size of the region where the
// data will be unpacked.
//
/////////////////////////////////////////////////////////////////////////

void GridExchange::initialize()
{
  /////////////////////////////////////////////////////////////////////////
  //
  // Send left and receive right face
  setSendOrigin(X0,
                this->ghost0,
                this->ghost0,
                this->ghost0);
  setRecvOrigin(X1,
                this->ghost0 + this->mine[0],
                this->ghost0,
                this->ghost0);
  setSendSize(X0, this->ghost1, this->mine[1], this->mine[2]);
  setRecvSize(X1, this->ghost1, this->mine[1], this->mine[2]);

  // Send right and receive left face
  setSendOrigin(X1,
                this->ghost0 + this->mine[0] - this->ghost0,
                this->ghost0,
                this->ghost0);
  setRecvOrigin(X0,
                0,
                this->ghost0,
                this->ghost0);
  setSendSize(X1, this->ghost0, this->mine[1], this->mine[2]);
  setRecvSize(X0, this->ghost0, this->mine[1], this->mine[2]);

  /////////////////////////////////////////////////////////////////////////
  //
  // Send bottom and receive top face
  setSendOrigin(Y0,
                this->ghost0,
                this->ghost0,
                this->ghost0);
  setRecvOrigin(Y1,
                this->ghost0,
                this->ghost0 + this->mine[1],
                this->ghost0);
  setSendSize(Y0, this->mine[0], this->ghost1, this->mine[2]);
  setRecvSize(Y1, this->mine[0], this->ghost1, this->mine[2]);

  // Send top and receive bottom face
  setSendOrigin(Y1,
                this->ghost0,
                this->ghost0 + this->mine[1] + this->ghost0,
                this->ghost0);
  setRecvOrigin(Y0,
                this->ghost0,
                0,
                this->ghost0);
  setSendSize(Y1, this->mine[0], this->ghost0, this->mine[2]);
  setRecvSize(Y0, this->mine[0], this->ghost0, this->mine[2]);

  /////////////////////////////////////////////////////////////////////////
  //
  // Send front and receive back face
  setSendOrigin(Z0,
                this->ghost0,
                this->ghost0,
                this->ghost0);
  setRecvOrigin(Z1,
                this->ghost0,
                this->ghost0,
                this->ghost0 + this->mine[2]);
  setSendSize(Z0, this->mine[0], this->mine[1], this->ghost1);
  setRecvSize(Z1, this->mine[0], this->mine[1], this->ghost1);

  // Send back and receive front face
  setSendOrigin(Z1,
                this->ghost0,
                this->ghost0,
                this->ghost0 + this->mine[2] - this->ghost0);
  setRecvOrigin(Z0,
                this->ghost0,
                this->ghost0,
                0);
  setSendSize(Z1, this->mine[0], this->mine[1], this->ghost0);
  setRecvSize(Z0, this->mine[0], this->mine[1], this->ghost0);

  /////////////////////////////////////////////////////////////////////////
  //
  // Send left bottom edge and receive right top edge
  setSendOrigin(X0_Y0,
                this->ghost0,
                this->ghost0,
                this->ghost0);
  setRecvOrigin(X1_Y1,
                this->ghost0 + this->mine[0],
                this->ghost0 + this->mine[1],
                this->ghost0);
  setSendSize(X0_Y0, this->ghost1, this->ghost1, this->mine[2]);
  setRecvSize(X1_Y1, this->ghost1, this->ghost1, this->mine[2]);

  // Send right top edge and receive left bottom edge
  setSendOrigin(X1_Y1,
                this->ghost0 + this->mine[0] - this->ghost0,
                this->ghost0 + this->mine[1] - this->ghost0,
                this->ghost0);
  setRecvOrigin(X0_Y0,
                0,
                0,
                this->ghost0);
  setSendSize(X1_Y1, this->ghost0, this->ghost0, this->mine[2]);
  setRecvSize(X0_Y0, this->ghost0, this->ghost0, this->mine[2]);

  /////////////////////////////////////////////////////////////////////////
  //
  // Send left top edge and receive right bottom edge
  setSendOrigin(X0_Y1,
                this->ghost0,
                this->ghost0 + this->mine[1] - this->ghost0,
                this->ghost0);
  setRecvOrigin(X1_Y0,
                this->ghost0 + this->mine[0],
                0,
                this->ghost0);
  setSendSize(X0_Y1, this->ghost1, this->ghost0, this->mine[2]);
  setRecvSize(X1_Y0, this->ghost1, this->ghost0, this->mine[2]);

  // Send right bottom edge and receive left top edge
  setSendOrigin(X1_Y0,
                this->ghost0 + this->mine[0] - this->ghost0,
                this->ghost0,
                this->ghost0);
  setRecvOrigin(X0_Y1,
                0,
                this->ghost0 + this->mine[1],
                this->ghost0);
  setSendSize(X1_Y0, this->ghost0, this->ghost1, this->mine[2]);
  setRecvSize(X0_Y1, this->ghost0, this->ghost1, this->mine[2]);

  /////////////////////////////////////////////////////////////////////////
  //
  // Send bottom front edge and receive top back edge
  setSendOrigin(Y0_Z0,
                this->ghost0,
                this->ghost0,
                this->ghost0);
  setRecvOrigin(Y1_Z1,
                this->ghost0,
                this->ghost0 + this->mine[1],
                this->ghost0 + this->mine[2]);
  setSendSize(Y0_Z0, this->mine[0], this->ghost1, this->ghost1);
  setRecvSize(Y1_Z1, this->mine[0], this->ghost1, this->ghost1);

  // Send top back edge and receive bottom front edge
  setSendOrigin(Y1_Z1,
                this->ghost0,
                this->ghost0 + this->mine[1] - this->ghost0,
                this->ghost0 + this->mine[2] - this->ghost0);
  setRecvOrigin(Y0_Z0,
                this->ghost0,
                0,
                0);
  setSendSize(Y1_Z1, this->mine[0], this->ghost0, this->ghost0);
  setRecvSize(Y0_Z0, this->mine[0], this->ghost0, this->ghost0);

  /////////////////////////////////////////////////////////////////////////
  //
  // Send bottom back edge and receive top front edge
  setSendOrigin(Y0_Z1,
                this->ghost0,
                this->ghost0,
                this->ghost0 + this->mine[2] - this->ghost0);
  setRecvOrigin(Y1_Z0,
                this->ghost0,
                this->ghost0 + this->mine[1],
                0);
  setSendSize(Y0_Z1, this->mine[0], this->ghost1, this->ghost0);
  setRecvSize(Y1_Z0, this->mine[0], this->ghost1, this->ghost0);

  // Send top front edge and receive bottom back edge
  setSendOrigin(Y1_Z0,
                this->ghost0,
                this->ghost0 + this->mine[1] - this->ghost0,
                this->ghost0);
  setRecvOrigin(Y0_Z1,
                this->ghost0,
                0,
                this->ghost0 + this->mine[2]);
  setSendSize(Y1_Z0, this->mine[0], this->ghost0, this->ghost1);
  setRecvSize(Y0_Z1, this->mine[0], this->ghost0, this->ghost1);

  /////////////////////////////////////////////////////////////////////////
  //
  // Send front left edge and receive back right edge
  setSendOrigin(Z0_X0,
                this->ghost0,
                this->ghost0,
                this->ghost0);
  setRecvOrigin(Z1_X1,
                this->ghost0 + this->mine[0],
                this->ghost0,
                this->ghost0 + this->mine[2]);
  setSendSize(Z0_X0, this->ghost1, this->mine[1], this->ghost1);
  setRecvSize(Z1_X1, this->ghost1, this->mine[1], this->ghost1);

  // Send back right edge and receive front left edge
  setSendOrigin(Z1_X1,
                this->ghost0 + this->mine[0] - this->ghost0,
                this->ghost0,
                this->ghost0 + this->mine[2] - this->ghost0);
  setRecvOrigin(Z0_X0,
                0,
                this->ghost0,
                0);
  setSendSize(Z1_X1, this->ghost0, this->mine[1], this->ghost0);
  setRecvSize(Z0_X0, this->ghost0, this->mine[1], this->ghost0);

  /////////////////////////////////////////////////////////////////////////
  //
  // Send front right edge and receive back left edge
  setSendOrigin(Z0_X1,
                this->ghost0 + this->mine[0] - this->ghost0,
                this->ghost0,
                this->ghost0);
  setRecvOrigin(Z1_X0,
                0,
                this->ghost0,
                this->ghost0 + this->mine[2]);
  setSendSize(Z0_X1, this->ghost0, this->mine[1], this->ghost1);
  setRecvSize(Z1_X0, this->ghost0, this->mine[1], this->ghost1);

  // Send back left edge and receive front right edge
  setSendOrigin(Z1_X0,
                this->ghost0,
                this->ghost0,
                this->ghost0 + this->mine[2] - this->ghost0);
  setRecvOrigin(Z0_X1,
                this->ghost0 + this->mine[0],
                this->ghost0,
                0);
  setSendSize(Z1_X0, this->ghost1, this->mine[1], this->ghost0);
  setRecvSize(Z0_X1, this->ghost1, this->mine[1], this->ghost0);

  /////////////////////////////////////////////////////////////////////////
  //
  // Send left bottom front corner and receive right top back corner
  setSendOrigin(X0_Y0_Z0,
                this->ghost0,
                this->ghost0,
                this->ghost0);
  setRecvOrigin(X1_Y1_Z1,
                this->ghost0 + this->mine[0],
                this->ghost0 + this->mine[1],
                this->ghost0 + this->mine[2]);
  setSendSize(X0_Y0_Z0, this->ghost1, this->ghost1, this->ghost1);
  setRecvSize(X1_Y1_Z1, this->ghost1, this->ghost1, this->ghost1);

  // Send right top back corner and receive left bottom front corner
  setSendOrigin(X1_Y1_Z1,
                this->ghost0 + this->mine[0] - this->ghost0,
                this->ghost0 + this->mine[1] - this->ghost0,
                this->ghost0 + this->mine[2] - this->ghost0);
  setRecvOrigin(X0_Y0_Z0,
                0,
                0,
                0);
  setSendSize(X1_Y1_Z1, this->ghost0, this->ghost0, this->ghost0);
  setRecvSize(X0_Y0_Z0, this->ghost0, this->ghost0, this->ghost0);

  /////////////////////////////////////////////////////////////////////////
  //
  // Send left bottom back corner and receive right top front corner
  setSendOrigin(X0_Y0_Z1,
                this->ghost0,
                this->ghost0,
                this->ghost0 + this->mine[2] - this->ghost0);
  setRecvOrigin(X1_Y1_Z0,
                this->ghost0 + this->mine[0],
                this->ghost0 + this->mine[1],
                0);
  setSendSize(X0_Y0_Z1, this->ghost1, this->ghost1, this->ghost0);
  setRecvSize(X1_Y1_Z0, this->ghost1, this->ghost1, this->ghost0);

  // Send right top front corner and receive left bottom back corner
  setSendOrigin(X1_Y1_Z0,
                this->ghost0 + this->mine[0] - this->ghost0,
                this->ghost0 + this->mine[1] - this->ghost0,
                this->ghost0);
  setRecvOrigin(X0_Y0_Z1,
                0,
                0,
                this->ghost0 + this->mine[2]);
  setSendSize(X1_Y1_Z0, this->ghost0, this->ghost0, this->ghost1);
  setRecvSize(X0_Y0_Z1, this->ghost0, this->ghost0, this->ghost1);

  /////////////////////////////////////////////////////////////////////////
  //
  // Send left top front corner and receive right bottom back corner
  setSendOrigin(X0_Y1_Z0,
                this->ghost0,
                this->ghost0 + this->mine[1] - this->ghost0,
                this->ghost0);
  setRecvOrigin(X1_Y0_Z1,
                this->ghost0 + this->mine[0],
                0,
                this->ghost0 + this->mine[2]);
  setSendSize(X0_Y1_Z0, this->ghost1, this->ghost0, this->ghost1);
  setRecvSize(X1_Y0_Z1, this->ghost1, this->ghost0, this->ghost1);

  // Send right bottom back corner and receive left top front corner
  setSendOrigin(X1_Y0_Z1,
                this->ghost0 + this->mine[0] - this->ghost0,
                this->ghost0,
                this->ghost0 + this->mine[2] - this->ghost0);
  setRecvOrigin(X0_Y1_Z0,
                0,
                this->ghost0 + this->mine[1],
                0);
  setSendSize(X1_Y0_Z1, this->ghost0, this->ghost1, this->ghost0);
  setRecvSize(X0_Y1_Z0, this->ghost0, this->ghost1, this->ghost0);

  /////////////////////////////////////////////////////////////////////////
  //
  // Send left top back corner and receive right bottom front corner
  setSendOrigin(X0_Y1_Z1,
                this->ghost0,
                this->ghost0 + this->mine[1] - this->ghost0,
                this->ghost0 + this->mine[2] - this->ghost0);
  setRecvOrigin(X1_Y0_Z0,
                this->ghost0 + this->mine[0],
                0,
                0);
  setSendSize(X0_Y1_Z1, this->ghost1, this->ghost0, this->ghost0);
  setRecvSize(X1_Y0_Z0, this->ghost1, this->ghost0, this->ghost0);

  // Send right bottom front corner and receive left top back corner
  setSendOrigin(X1_Y0_Z0,
              this->ghost0 + this->mine[0] - this->ghost0,
              this->ghost0,
              this->ghost0);
  setRecvOrigin(X0_Y1_Z1,
              0,
              this->ghost0 + this->mine[1],
              this->ghost0 + this->mine[2]);
  setSendSize(X1_Y0_Z0, this->ghost0, this->ghost1, this->ghost1);
  setRecvSize(X0_Y1_Z1, this->ghost0, this->ghost1, this->ghost1);
}

/////////////////////////////////////////////////////////////////////////////
//
// Short cuts for setting send/receive origins and sizes to make the
// code more readable
//
/////////////////////////////////////////////////////////////////////////////

void GridExchange::setSendOrigin(int Neighbor, int x, int y, int z)
{
  this->sendOrigin[Neighbor][0] = x;
  this->sendOrigin[Neighbor][1] = y;
  this->sendOrigin[Neighbor][2] = z;
}

void GridExchange::setRecvOrigin(int Neighbor, int x, int y, int z)
{
  this->recvOrigin[Neighbor][0] = x;
  this->recvOrigin[Neighbor][1] = y;
  this->recvOrigin[Neighbor][2] = z;
}

void GridExchange::setSendSize(int Neighbor, int x, int y, int z)
{
  this->sendSize[Neighbor][0] = x;
  this->sendSize[Neighbor][1] = y;
  this->sendSize[Neighbor][2] = z;
}

void GridExchange::setRecvSize(int Neighbor, int x, int y, int z)
{
  this->recvSize[Neighbor][0] = x;
  this->recvSize[Neighbor][1] = y;
  this->recvSize[Neighbor][2] = z;
}

/////////////////////////////////////////////////////////////////////////////
//
// Exchange the appropriate grid regions with neighbors
// Use the Cartesian communicator for neighbor exchange
//
/////////////////////////////////////////////////////////////////////////////

void GridExchange::exchangeGrid(GRID_T* data)
{
  // Exchange with each neighbor, with everyone sending in one direction and
  // receiving from the other direction in pairs.  Data is packed from the
  // mine part of the grid and when it is received it is unpacked into the
  // ghost part of the grid.
#ifdef USE_PARALLEL_VPIC
  MPI_Barrier(MPI_COMM_WORLD);
#endif

  for (int n = 0; n < NUM_OF_NEIGHBORS; n=n+2) {
    exchange(n, n+1, data);
    exchange(n+1, n, data);
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// Pack grid data for the indicated neighbor into MPI message
// Send that message and receive from opposite neighbor
// Unpack the received grid data and write into the ghost part of the grid.
//
/////////////////////////////////////////////////////////////////////////////

void GridExchange::exchange(int sendTo, int recvFrom, GRID_T* data)
{
#ifdef USE_PARALLEL_VPIC
  if (myProc == neighbor[recvFrom] || myProc == neighbor[sendTo])
    return;

#ifdef DEBUG
  if (myProc == 0) {
    cout << "Neighbor " << sendTo << " Send origin "
         << setw(3) << sendOrigin[sendTo][0] << ","
         << setw(3) << sendOrigin[sendTo][1] << ","
         << setw(3) << sendOrigin[sendTo][2] << "\tsize "
         << sendSize[sendTo][0] << ":"
         << sendSize[sendTo][1] << ":"
         << sendSize[sendTo][2] << endl;
    cout << "Neighbor " << recvFrom << " Recv origin "
         << setw(3) << recvOrigin[recvFrom][0] << ","
         << setw(3) << recvOrigin[recvFrom][1] << ","
         << setw(3) << recvOrigin[recvFrom][2] << "\tsize "
         << recvSize[recvFrom][0] << ":"
         << recvSize[recvFrom][1] << ":"
         << recvSize[recvFrom][2] << endl;
     cout << "------------------------------------" << endl;
  }
#endif

  int planeSize = this->totalSize[0] * this->totalSize[1];
  int rowSize = this->totalSize[0];

  // Pack the send buffer
  int messageIndex = 0;
  for (int k = 0; k < this->sendSize[sendTo][2]; k++) {
    for (int j = 0; j < this->sendSize[sendTo][1]; j++) {
      for (int i = 0; i < this->sendSize[sendTo][0]; i++) {
        int dataIndex = (this->sendOrigin[sendTo][2] + k) * planeSize +
                        (this->sendOrigin[sendTo][1] + j) * rowSize +
                         this->sendOrigin[sendTo][0] + i;
        sendBuffer[messageIndex++] = data[dataIndex];
      }
    }
  }

  // Send the buffer
  MPI_Request mpiRequest;
  MPI_Isend(sendBuffer, bufferSize, MPI_FLOAT, this->neighbor[sendTo],
            0, MPI_COMM_WORLD, &mpiRequest);

  // Receive the buffer from neighbor on other side
  MPI_Status mpiStatus;
  MPI_Recv(recvBuffer, bufferSize, MPI_FLOAT, this->neighbor[recvFrom],
           0, MPI_COMM_WORLD, &mpiStatus);

  MPI_Barrier(MPI_COMM_WORLD);

  // Unpack the received buffer
  messageIndex = 0;
  for (int k = 0; k < this->recvSize[recvFrom][2]; k++) {
    for (int j = 0; j < this->recvSize[recvFrom][1]; j++) {
      for (int i = 0; i < this->recvSize[recvFrom][0]; i++) {
        int dataIndex = (this->recvOrigin[recvFrom][2] + k) * planeSize +
                        (this->recvOrigin[recvFrom][1] + j) * rowSize +
                         this->recvOrigin[recvFrom][0] + i;
        data[dataIndex] = recvBuffer[messageIndex++];
      }
    }
  }
#else
  (void)sendTo;
  (void)recvFrom;
  (void)data;
#endif
}
