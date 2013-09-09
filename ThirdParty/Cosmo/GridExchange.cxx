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
#include <iomanip>

#include "Partition.h"
#include "GridExchange.h"
#include "bigchunk.h"

using namespace std;

namespace cosmologytools {
/////////////////////////////////////////////////////////////////////////
//
// GridExchange will take a pointer to contiguous memory, the size of
// memory in each dimension, and the amount of dead grid information
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
			int* size,
			int ghost0,
			int ghost1)
{
  // Get the number of processors running this problem and rank
  this->numProc = Partition::getNumProc();
  this->myProc = Partition::getMyProc();

  // Get the number of processors in each dimension
  Partition::getDecompSize(this->layoutSize);

  // Get my position within the Cartesian topology
  Partition::getMyPosition(this->layoutPos);

  // Get neighbors of this processor including the wraparound
  Partition::getNeighbors(this->neighbor);

  // Store sizes for this exchange which depend on alive and dead grid zones
  this->dead0 = ghost0;
  this->dead1 = ghost1; 

  int maxGridSize = 0;
  for (int dim = 0; dim < DIMENSION; dim++) {
    this->totalSize[dim] = size[dim];
    this->alive[dim] = size[dim] - this->dead0 - this->dead1;
    if (alive[dim] > maxGridSize)
      maxGridSize = alive[dim];
  }

  // Calculate the MPI message size for biggest grid region
  this->bufferSize = maxGridSize * maxGridSize * max(this->dead0, this->dead1);
  resurrectBuffers();

  // Initialize this exchanger with a given size to save the calculation
  // every time data is to be sent
  initialize();
}

GridExchange::~GridExchange()
{
  dropBuffers();
}

void GridExchange::dropBuffers()
{
  // delete [] this->sendBuffer;
  // delete [] this->recvBuffer;
  bigchunk_free(this->sendBuffer);
  bigchunk_free(this->recvBuffer);
  this->sendBuffer = this->recvBuffer = 0;
}

void GridExchange::resurrectBuffers()
{
  size_t bs = this->bufferSize*sizeof(GRID_T);
  this->sendBuffer = (GRID_T*) bigchunk_malloc(bs);
  this->recvBuffer = (GRID_T*) bigchunk_malloc(bs);
  // this->sendBuffer = new GRID_T[this->bufferSize];
  // this->recvBuffer = new GRID_T[this->bufferSize];
}

/////////////////////////////////////////////////////////////////////////
//
// Calculate information needed for neighbor exchange of dead grids
// For each neighbor sent to their is an offset into the alive grid and
// a size of the region to be sent.  For each neighbor received from there
// is an offset into the dead grid and a size of the region where the
// data will be unpacked.
//
/////////////////////////////////////////////////////////////////////////

void GridExchange::initialize()
{
  /////////////////////////////////////////////////////////////////////////
  //
  // Send left and receive right face
  setSendOrigin(X0, 
                this->dead0, 
                this->dead0, 
                this->dead0);
  setRecvOrigin(X1, 
                this->dead0 + this->alive[0], 
                this->dead0, 
                this->dead0);
  setSendSize(X0, this->dead1, this->alive[1], this->alive[2]);
  setRecvSize(X1, this->dead1, this->alive[1], this->alive[2]);

  // Send right and receive left face
  setSendOrigin(X1,
                this->dead0 + this->alive[0] - this->dead0,
                this->dead0,
                this->dead0);
  setRecvOrigin(X0, 
                0, 
                this->dead0, 
                this->dead0);
  setSendSize(X1, this->dead0, this->alive[1], this->alive[2]);
  setRecvSize(X0, this->dead0, this->alive[1], this->alive[2]);

  /////////////////////////////////////////////////////////////////////////
  //
  // Send bottom and receive top face
  setSendOrigin(Y0, 
                this->dead0, 
                this->dead0, 
                this->dead0);
  setRecvOrigin(Y1, 
                this->dead0, 
                this->dead0 + this->alive[1], 
                this->dead0);
  setSendSize(Y0, this->alive[0], this->dead1, this->alive[2]);
  setRecvSize(Y1, this->alive[0], this->dead1, this->alive[2]);

  // Send top and receive bottom face
  setSendOrigin(Y1,
                this->dead0,
                this->dead0 + this->alive[1] - this->dead0,
                this->dead0);
  setRecvOrigin(Y0, 
                this->dead0, 
                0, 
                this->dead0);
  setSendSize(Y1, this->alive[0], this->dead0, this->alive[2]);
  setRecvSize(Y0, this->alive[0], this->dead0, this->alive[2]);

  /////////////////////////////////////////////////////////////////////////
  //
  // Send front and receive back face
  setSendOrigin(Z0, 
                this->dead0, 
                this->dead0, 
                this->dead0);
  setRecvOrigin(Z1, 
                this->dead0, 
                this->dead0, 
                this->dead0 + this->alive[2]);
  setSendSize(Z0, this->alive[0], this->alive[1], this->dead1);
  setRecvSize(Z1, this->alive[0], this->alive[1], this->dead1);

  // Send back and receive front face
  setSendOrigin(Z1,
                this->dead0,
                this->dead0,
                this->dead0 + this->alive[2] - this->dead0);
  setRecvOrigin(Z0, 
                this->dead0, 
                this->dead0, 
                0);
  setSendSize(Z1, this->alive[0], this->alive[1], this->dead0);
  setRecvSize(Z0, this->alive[0], this->alive[1], this->dead0);

  /////////////////////////////////////////////////////////////////////////
  //
  // Send left bottom edge and receive right top edge
  setSendOrigin(X0_Y0, 
                this->dead0, 
                this->dead0, 
                this->dead0);
  setRecvOrigin(X1_Y1,
                this->dead0 + this->alive[0],
                this->dead0 + this->alive[1],
                this->dead0);
  setSendSize(X0_Y0, this->dead1, this->dead1, this->alive[2]);
  setRecvSize(X1_Y1, this->dead1, this->dead1, this->alive[2]);

  // Send right top edge and receive left bottom edge
  setSendOrigin(X1_Y1,
                this->dead0 + this->alive[0] - this->dead0,
                this->dead0 + this->alive[1] - this->dead0,
                this->dead0);
  setRecvOrigin(X0_Y0, 
                0, 
                0, 
                this->dead0);
  setSendSize(X1_Y1, this->dead0, this->dead0, this->alive[2]);
  setRecvSize(X0_Y0, this->dead0, this->dead0, this->alive[2]);

  /////////////////////////////////////////////////////////////////////////
  //
  // Send left top edge and receive right bottom edge
  setSendOrigin(X0_Y1, 
                this->dead0, 
                this->dead0 + this->alive[1] - this->dead0, 
                this->dead0);
  setRecvOrigin(X1_Y0, 
                this->dead0 + this->alive[0], 
                0,
                this->dead0);
  setSendSize(X0_Y1, this->dead1, this->dead0, this->alive[2]);
  setRecvSize(X1_Y0, this->dead1, this->dead0, this->alive[2]);

  // Send right bottom edge and receive left top edge
  setSendOrigin(X1_Y0,
                this->dead0 + this->alive[0] - this->dead0,
                this->dead0,
                this->dead0);
  setRecvOrigin(X0_Y1, 
                0, 
                this->dead0 + this->alive[1],
                this->dead0);
  setSendSize(X1_Y0, this->dead0, this->dead1, this->alive[2]);
  setRecvSize(X0_Y1, this->dead0, this->dead1, this->alive[2]);

  /////////////////////////////////////////////////////////////////////////
  //
  // Send bottom front edge and receive top back edge
  setSendOrigin(Y0_Z0,
                this->dead0,
                this->dead0,
                this->dead0);
  setRecvOrigin(Y1_Z1,
                this->dead0,
                this->dead0 + this->alive[1],
                this->dead0 + this->alive[2]);
  setSendSize(Y0_Z0, this->alive[0], this->dead1, this->dead1);
  setRecvSize(Y1_Z1, this->alive[0], this->dead1, this->dead1);

  // Send top back edge and receive bottom front edge
  setSendOrigin(Y1_Z1,
                this->dead0,
                this->dead0 + this->alive[1] - this->dead0,
                this->dead0 + this->alive[2] - this->dead0);
  setRecvOrigin(Y0_Z0,
                this->dead0,
                0,
                0);
  setSendSize(Y1_Z1, this->alive[0], this->dead0, this->dead0);
  setRecvSize(Y0_Z0, this->alive[0], this->dead0, this->dead0);

  /////////////////////////////////////////////////////////////////////////
  //
  // Send bottom back edge and receive top front edge
  setSendOrigin(Y0_Z1,
                this->dead0,
                this->dead0,
                this->dead0 + this->alive[2] - this->dead0);
  setRecvOrigin(Y1_Z0,
                this->dead0,
                this->dead0 + this->alive[1],
                0);
  setSendSize(Y0_Z1, this->alive[0], this->dead1, this->dead0);
  setRecvSize(Y1_Z0, this->alive[0], this->dead1, this->dead0);

  // Send top front edge and receive bottom back edge
  setSendOrigin(Y1_Z0,
                this->dead0,
                this->dead0 + this->alive[1] - this->dead0,
                this->dead0);
  setRecvOrigin(Y0_Z1,
                this->dead0,
                0,
                this->dead0 + this->alive[2]);
  setSendSize(Y1_Z0, this->alive[0], this->dead0, this->dead1);
  setRecvSize(Y0_Z1, this->alive[0], this->dead0, this->dead1);

  /////////////////////////////////////////////////////////////////////////
  //
  // Send front left edge and receive back right edge
  setSendOrigin(Z0_X0,
                this->dead0,
                this->dead0,
                this->dead0);
  setRecvOrigin(Z1_X1,
                this->dead0 + this->alive[0],
                this->dead0,
                this->dead0 + this->alive[2]);
  setSendSize(Z0_X0, this->dead1, this->alive[1], this->dead1);
  setRecvSize(Z1_X1, this->dead1, this->alive[1], this->dead1);

  // Send back right edge and receive front left edge
  setSendOrigin(Z1_X1,
                this->dead0 + this->alive[0] - this->dead0,
                this->dead0,
                this->dead0 + this->alive[2] - this->dead0);
  setRecvOrigin(Z0_X0,
                0,
                this->dead0,
                0);
  setSendSize(Z1_X1, this->dead0, this->alive[1], this->dead0);
  setRecvSize(Z0_X0, this->dead0, this->alive[1], this->dead0);

  /////////////////////////////////////////////////////////////////////////
  //
  // Send front right edge and receive back left edge
  setSendOrigin(Z0_X1,
                this->dead0 + this->alive[0] - this->dead0,
                this->dead0,
                this->dead0);
  setRecvOrigin(Z1_X0,
                0,
                this->dead0,
                this->dead0 + this->alive[2]);
  setSendSize(Z0_X1, this->dead0, this->alive[1], this->dead1);
  setRecvSize(Z1_X0, this->dead0, this->alive[1], this->dead1);

  // Send back left edge and receive front right edge
  setSendOrigin(Z1_X0,
                this->dead0,
                this->dead0,
                this->dead0 + this->alive[2] - this->dead0);
  setRecvOrigin(Z0_X1,
                this->dead0 + this->alive[0],
                this->dead0,
                0);
  setSendSize(Z1_X0, this->dead1, this->alive[1], this->dead0);
  setRecvSize(Z0_X1, this->dead1, this->alive[1], this->dead0);

  /////////////////////////////////////////////////////////////////////////
  //
  // Send left bottom front corner and receive right top back corner
  setSendOrigin(X0_Y0_Z0,
                this->dead0,
                this->dead0,
                this->dead0);
  setRecvOrigin(X1_Y1_Z1,
                this->dead0 + this->alive[0],
                this->dead0 + this->alive[1],
                this->dead0 + this->alive[2]);
  setSendSize(X0_Y0_Z0, this->dead1, this->dead1, this->dead1);
  setRecvSize(X1_Y1_Z1, this->dead1, this->dead1, this->dead1);

  // Send right top back corner and receive left bottom front corner
  setSendOrigin(X1_Y1_Z1,
                this->dead0 + this->alive[0] - this->dead0,
                this->dead0 + this->alive[1] - this->dead0,
                this->dead0 + this->alive[2] - this->dead0);
  setRecvOrigin(X0_Y0_Z0,
                0,
                0,
                0);
  setSendSize(X1_Y1_Z1, this->dead0, this->dead0, this->dead0);
  setRecvSize(X0_Y0_Z0, this->dead0, this->dead0, this->dead0);

  /////////////////////////////////////////////////////////////////////////
  //
  // Send left bottom back corner and receive right top front corner
  setSendOrigin(X0_Y0_Z1,
                this->dead0,
                this->dead0,
                this->dead0 + this->alive[2] - this->dead0);
  setRecvOrigin(X1_Y1_Z0,
                this->dead0 + this->alive[0],
                this->dead0 + this->alive[1],
                0);
  setSendSize(X0_Y0_Z1, this->dead1, this->dead1, this->dead0);
  setRecvSize(X1_Y1_Z0, this->dead1, this->dead1, this->dead0);

  // Send right top front corner and receive left bottom back corner
  setSendOrigin(X1_Y1_Z0,
                this->dead0 + this->alive[0] - this->dead0,
                this->dead0 + this->alive[1] - this->dead0,
                this->dead0);
  setRecvOrigin(X0_Y0_Z1,
                0,
                0,
                this->dead0 + this->alive[2]);
  setSendSize(X1_Y1_Z0, this->dead0, this->dead0, this->dead1);
  setRecvSize(X0_Y0_Z1, this->dead0, this->dead0, this->dead1);

  /////////////////////////////////////////////////////////////////////////
  //
  // Send left top front corner and receive right bottom back corner
  setSendOrigin(X0_Y1_Z0,
                this->dead0,
                this->dead0 + this->alive[1] - this->dead0,
                this->dead0);
  setRecvOrigin(X1_Y0_Z1,
                this->dead0 + this->alive[0],
                0,
                this->dead0 + this->alive[2]);
  setSendSize(X0_Y1_Z0, this->dead1, this->dead0, this->dead1);
  setRecvSize(X1_Y0_Z1, this->dead1, this->dead0, this->dead1);

  // Send right bottom back corner and receive left top front corner
  setSendOrigin(X1_Y0_Z1,
                this->dead0 + this->alive[0] - this->dead0,
                this->dead0,
                this->dead0 + this->alive[2] - this->dead0);
  setRecvOrigin(X0_Y1_Z0,
                0,
                this->dead0 + this->alive[1],
                0);
  setSendSize(X1_Y0_Z1, this->dead0, this->dead1, this->dead0);
  setRecvSize(X0_Y1_Z0, this->dead0, this->dead1, this->dead0);

  /////////////////////////////////////////////////////////////////////////
  //
  // Send left top back corner and receive right bottom front corner
  setSendOrigin(X0_Y1_Z1,
                this->dead0,
                this->dead0 + this->alive[1] - this->dead0,
                this->dead0 + this->alive[2] - this->dead0);
  setRecvOrigin(X1_Y0_Z0,
                this->dead0 + this->alive[0],
                0,
                0);
  setSendSize(X0_Y1_Z1, this->dead1, this->dead0, this->dead0);
  setRecvSize(X1_Y0_Z0, this->dead1, this->dead0, this->dead0);

  // Send right bottom front corner and receive left top back corner
  setSendOrigin(X1_Y0_Z0,
              this->dead0 + this->alive[0] - this->dead0,
              this->dead0,
              this->dead0);
  setRecvOrigin(X0_Y1_Z1,
              0,
              this->dead0 + this->alive[1],
              this->dead0 + this->alive[2]);
  setSendSize(X1_Y0_Z0, this->dead0, this->dead1, this->dead1);
  setRecvSize(X0_Y1_Z1, this->dead0, this->dead1, this->dead1);
}

/////////////////////////////////////////////////////////////////////////////
//
// Short cuts for setting send/receive origins and sizes to make the
// code more readable
//
/////////////////////////////////////////////////////////////////////////////

void GridExchange::setSendOrigin(int whichNeighbor, int x, int y, int z)
{
  this->sendOrigin[whichNeighbor][0] = x;
  this->sendOrigin[whichNeighbor][1] = y;
  this->sendOrigin[whichNeighbor][2] = z;
}

void GridExchange::setRecvOrigin(int whichNeighbor, int x, int y, int z)
{
  this->recvOrigin[whichNeighbor][0] = x;
  this->recvOrigin[whichNeighbor][1] = y;
  this->recvOrigin[whichNeighbor][2] = z;
}

void GridExchange::setSendSize(int whichNeighbor, int x, int y, int z)
{
  this->sendSize[whichNeighbor][0] = x;
  this->sendSize[whichNeighbor][1] = y;
  this->sendSize[whichNeighbor][2] = z;
}

void GridExchange::setRecvSize(int whichNeighbor, int x, int y, int z)
{
  this->recvSize[whichNeighbor][0] = x;
  this->recvSize[whichNeighbor][1] = y;
  this->recvSize[whichNeighbor][2] = z;
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
  // alive part of the grid and when it is received it is unpacked into the
  // dead part of the grid.

  for (int n = 0; n < NUM_OF_NEIGHBORS; n=n+2) {
    exchange(n, n+1, data);
    exchange(n+1, n, data);
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// Pack grid data for the indicated neighbor into MPI message
// Send that message and receive from opposite neighbor
// Unpack the received grid data and write into the dead part of the grid.
//
/////////////////////////////////////////////////////////////////////////////

void GridExchange::exchange(int sendTo, int recvFrom, GRID_T* data)
{
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

  int planeSize = this->totalSize[1] * this->totalSize[2];
  int rowSize = this->totalSize[2];

  // Pack the send buffer
  int messageIndex = 0;
  for (int i = 0; i < this->sendSize[sendTo][0]; i++) {
    for (int j = 0; j < this->sendSize[sendTo][1]; j++) {
      for (int k = 0; k < this->sendSize[sendTo][2]; k++) {
        int dataIndex = (this->sendOrigin[sendTo][0] + i) * planeSize +
                        (this->sendOrigin[sendTo][1] + j) * rowSize +
                         this->sendOrigin[sendTo][2] + k;
        sendBuffer[messageIndex++] = data[dataIndex];
      }
    }
  }
     
  // Send the buffer
  MPI_Request mpiRequest;
#ifdef GRID_64
  MPI_Isend(sendBuffer, bufferSize, MPI_DOUBLE, this->neighbor[sendTo],
            0, Partition::getComm(), &mpiRequest);
#else
  MPI_Isend(sendBuffer, bufferSize, MPI_FLOAT, this->neighbor[sendTo],
            0, Partition::getComm(), &mpiRequest);
#endif

  // Receive the buffer from neighbor on other side
  MPI_Status mpiStatus;
#ifdef GRID_64
  MPI_Recv(recvBuffer, bufferSize, MPI_DOUBLE, this->neighbor[recvFrom],
           0, Partition::getComm(), &mpiStatus);
#else
  MPI_Recv(recvBuffer, bufferSize, MPI_FLOAT, this->neighbor[recvFrom],
           0, Partition::getComm(), &mpiStatus);
#endif

  MPI_Barrier(Partition::getComm());

  // Unpack the received buffer
  messageIndex = 0;
  for (int i = 0; i < this->recvSize[recvFrom][0]; i++) {
    for (int j = 0; j < this->recvSize[recvFrom][1]; j++) {
      for (int k = 0; k < this->recvSize[recvFrom][2]; k++) {
        int dataIndex = (this->recvOrigin[recvFrom][0] + i) * planeSize +
                        (this->recvOrigin[recvFrom][1] + j) * rowSize +
                         this->recvOrigin[recvFrom][2] + k;
        data[dataIndex] = recvBuffer[messageIndex++];
      }
    }
  }
}

}
