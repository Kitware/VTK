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

// .NAME Message - create, send and receive MPI messages
//
// .SECTION Description
// Message class packs and unpacks data into an MPI buffer

#ifndef MESSAGE_H
#define MESSAGE_H

#ifdef USE_VTK_COSMO
#include "CosmoDefinition.h"
#include <queue>

using namespace std;
#else
#include "Definition.h"
#include <queue>

using namespace std;
#endif


class Message {
public:
  Message(int size = BUF_SZ);

   ~Message();

  // Put values into the MPI buffer
  void putValueAtPosition(int* data, int pos, int count = 1);
  void putValue(int* data, int count = 1);
  void putValue(unsigned short* data, int count = 1);
  void putValue(long int* data, int count = 1);
  void putValue(long long* data, int count = 1);
  void putValue(float* data, int count = 1);
  void putValue(double* data, int count = 1);
  void putValue(char* data, int count = 1);

  // Get values from the MPI buffer
  void getValue(int* data, int count = 1);
  void getValue(unsigned short* data, int count = 1);
  void getValue(long int* data, int count = 1);
  void getValue(long long* data, int count = 1);
  void getValue(float* data, int count = 1);
  void getValue(double* data, int count = 1);
  void getValue(char* data, int count = 1);

  int getBufPos() { return this->bufPos; }

  void manualPackAtPosition(char* data, int pos, int count, size_t size);
  void manualPack(char* data, int count, size_t size);
  void manualUnpack(char* data, int count, size_t size);

  // Send nonblocking
  void send(
        int mach,                       // Where to send message
        int tag = 0                     // Identifying tag
  );

  // Receive blocking
  void receive(
#ifdef USE_SERIAL_COSMO
        int mach = 0,
#else
        int mach = MPI_ANY_SOURCE,      // From where to receive
#endif
        int tag = 0                     // Identifying tag
  );

#ifdef USE_SERIAL_COSMO // message queue hack for serial
  queue<char*> q;
#endif

  // Reset the buffer for another set of data
  void reset();

private:
  char* buffer;         // Buffer to pack
  int   bufSize;        // Size of buffer
  int   bufPos;         // Position in buffer
};

#endif
