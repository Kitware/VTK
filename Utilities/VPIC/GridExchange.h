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

// .NAME GridExchange - Send my data portions of grid on one processor to
//                      become ghost portion of grid on a neighbor
//
// .SECTION Description
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

#ifndef GridExchange_h
#define GridExchange_h

#include "VPICDefinition.h"

using namespace std;

class VPIC_EXPORT GridExchange {
public:
  GridExchange(
        int rank,		// Rank of this processor
        int total,		// Total processors (needed to find neighbors)
        int* decomp,		// Decomposition of processor in problem
        int* size,		// Size of total space to exchange
        int ghost0,		// Ghost cell count on the front plane
        int ghost1);		// Ghost cell count on the back plane
                                // ParaView wants overlap of 1 cell always
                                // so with a ghost in each direction overlap
                                // becomes 3 so ghost overlap on front plane
                                // will be 1 and on the back plane will be 2
  ~GridExchange();

  // Calculate the offsets and sizes for send and receive to neighbors
  void initialize();

  // Set the processor neighbors of this processor
  void setNeighbors();

  void setSendOrigin(int neighbor, int x, int y, int z);
  void setRecvOrigin(int neighbor, int x, int y, int z);
  void setSendSize(int neighbor, int x, int y, int z);
  void setRecvSize(int neighbor, int x, int y, int z);

  // Exchange sections of a grid with all neighbors
  void exchangeGrid(GRID_T* data);
  void exchange(
        int sendTo,		// Neighbor to send particles to
        int recvFrom,		// Neighbor to receive particles from
        GRID_T* data);		// Grid to share

private:
  int    myProc;		// My processor number
  int    numProc;		// Total number of processors

  int    layoutSize[DIMENSION];	// Decomposition of processors
  int    layoutPos[DIMENSION];	// Position of this processor in decomposition
  int*** layoutID;		// Layout of all processors

  int    bufferSize;		// Max message size to send/receive
  GRID_T* sendBuffer;		// Message buffer
  GRID_T* recvBuffer;		// Message buffer

  int    totalSize[DIMENSION];	// Sizes with mine and ghost grids
  int    mine[DIMENSION];       // Sizes of only my grid
  int    ghost0;		// Ghost grid size on front planes
  int    ghost1;		// Ghost grid size on back planes

  // Processor id for each neighbor
  int    neighbor[NUM_OF_NEIGHBORS];

  // Region of data to send to each neighbor
  int    sendOrigin[NUM_OF_NEIGHBORS][DIMENSION];
  int    sendSize[NUM_OF_NEIGHBORS][DIMENSION];

  // Region of data to receive from each neighbor
  int    recvOrigin[NUM_OF_NEIGHBORS][DIMENSION];
  int    recvSize[NUM_OF_NEIGHBORS][DIMENSION];
};

#endif
