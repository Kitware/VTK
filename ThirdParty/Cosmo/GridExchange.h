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

// .NAME GridExchange - Send alive portions of grid on one processor to
//                      become dead portion of grid on a neighbor
//
// .SECTION Description
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

#ifndef GridExchange_h
#define GridExchange_h

#include "Definition.h"

namespace cosmologytools {


class COSMO_EXPORT GridExchange {
public:
  GridExchange(int* size, int ghost0, int ghost1);
  ~GridExchange();

  // Calculate the offsets and sizes for send and receive to neighbors
  void initialize();

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

  void dropBuffers();
  void resurrectBuffers();

private:
  int    myProc;		// My processor number
  int    numProc;		// Total number of processors

  int    layoutSize[DIMENSION];	// Decomposition of processors
  int    layoutPos[DIMENSION];	// Position of this processor in decomposition

  int    bufferSize;		// Max message size to send/receive
  GRID_T* sendBuffer;		// Message buffer
  GRID_T* recvBuffer;		// Message buffer

  int    totalSize[DIMENSION];	// Sizes with alive and dead grids
  int    alive[DIMENSION]; 	// Sizes of only alive grid
  int    dead0;			// Dead grid size on the front edge of dimension
  int    dead1;			// Dead grid size on the back edge of dimension

  // Processor id for each neighbor
  int    neighbor[NUM_OF_NEIGHBORS];

  // Region of data to send to each neighbor
  int    sendOrigin[NUM_OF_NEIGHBORS][DIMENSION];
  int    sendSize[NUM_OF_NEIGHBORS][DIMENSION];

  // Region of data to receive from each neighbor
  int    recvOrigin[NUM_OF_NEIGHBORS][DIMENSION];
  int    recvSize[NUM_OF_NEIGHBORS][DIMENSION];
};

}
#endif
