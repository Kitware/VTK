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

#include "Message.h"
#include "Partition.h"

#ifdef USE_SERIAL_COSMO
#include <string.h>
#endif

#include <iostream>

using namespace std;

////////////////////////////////////////////////////////////////////////////
//
// Create a Message for sending or receiving from MPI
//
////////////////////////////////////////////////////////////////////////////

Message::Message(int size)
{
  this->bufSize = size;
  this->buffer = new char[size];
  this->bufPos = 0;
}

void Message::manualPackAtPosition(char* data, int pos, int count, size_t size)
{
  for(int i = 0; i < count; i = i + 1) {
    for(size_t j = 0; j < size; j = j + 1) {
      this->buffer[pos++] = data[i * size + j];
    }
  }
}

void Message::manualPack(char* data, int count, size_t size)
{
  for(int i = 0; i < count; i = i + 1) {
    for(size_t j = 0; j < size; j = j + 1) {
      this->buffer[this->bufPos++] = data[i * size + j];
    }
  }
}

void Message::manualUnpack(char* data, int count, size_t size)
{
  for(int i = 0; i < count; i = i + 1) {
    for(size_t j = 0; j < size; j = j + 1) {
      data[i * size + j] = this->buffer[this->bufPos++];
    }
  }
}


////////////////////////////////////////////////////////////////////////////
//
// Destructor for a message
//
////////////////////////////////////////////////////////////////////////////
Message::~Message()
{
  delete [] this->buffer;
}

////////////////////////////////////////////////////////////////////////////
//
// Reset for another message of the same size
//
////////////////////////////////////////////////////////////////////////////
void Message::reset()
{
  this->bufPos = 0;
}


////////////////////////////////////////////////////////////////////////////
//
// Place an integer at a specific location in the buffer
// Used to set a counter of particles in the first position when it is
// only known after all the particles are packed
//
////////////////////////////////////////////////////////////////////////////
void Message::putValueAtPosition(int* data, int pos, int count)
{
  manualPackAtPosition((char*)data, pos, count, sizeof(int));
}
////////////////////////////////////////////////////////////////////////////
//
// Packing of the buffer
//
////////////////////////////////////////////////////////////////////////////
void Message::putValue(int* data, int count)
{
  manualPack((char*)data, count, sizeof(int));
}
void Message::putValue(unsigned short* data, int count)
{
  manualPack((char*)data, count, sizeof(unsigned short));
}
void Message::putValue(long int* data, int count)
{
  manualPack((char*)data, count, sizeof(long int));
}
void Message::putValue(long long* data, int count)
{
  manualPack((char*)data, count, sizeof(long long));
}
void Message::putValue(float* data, int count)
{
  manualPack((char*)data, count, sizeof(float));
}
void Message::putValue(double* data, int count)
{
  manualPack((char*)data, count, sizeof(double));
}
void Message::putValue(char* data, int count)
{
  manualPack((char*)data, count, sizeof(char));
}

////////////////////////////////////////////////////////////////////////////
//
// Unpacking of the buffer
//
////////////////////////////////////////////////////////////////////////////
void Message::getValue(int* data, int count)
{
  manualUnpack((char*)data, count, sizeof(int));
}
void Message::getValue(unsigned short* data, int count)
{
  manualUnpack((char*)data, count, sizeof(unsigned short));
}
void Message::getValue(long int* data, int count)
{
  manualUnpack((char*)data, count, sizeof(long int));
}
void Message::getValue(long long* data, int count)
{
  manualUnpack((char*)data, count, sizeof(long long));
}
void Message::getValue(float* data, int count)
{
  manualUnpack((char*)data, count, sizeof(float));
}
void Message::getValue(double* data, int count)
{
  manualUnpack((char*)data, count, sizeof(double));
}
void Message::getValue(char* data, int count)
{
  manualUnpack((char*)data, count, sizeof(char));
}

////////////////////////////////////////////////////////////////////////////
//
// Nonblocking send
//
////////////////////////////////////////////////////////////////////////////
void Message::send
#ifdef USE_SERIAL_COSMO
  (int , int )
#else
  (int mach, int tag)
#endif
{
#ifdef USE_SERIAL_COSMO
  char* in = new char[this->bufPos];
  memcpy(in, this->buffer, this->bufPos);
  q.push(in);
#else
  MPI_Request request;
  MPI_Isend(this->buffer, this->bufPos, MPI_PACKED, 
            mach, tag, Partition::getComm(), &request);
#endif
}


////////////////////////////////////////////////////////////////////////////
//
// Blocking receive
//
////////////////////////////////////////////////////////////////////////////
void Message::receive
#ifdef USE_SERIAL_COSMO
(int, int)
#else
(int mach, int tag)
#endif
{
#ifdef USE_SERIAL_COSMO
  char* out = q.front(); q.pop();
  memcpy(this->buffer, out, this->bufSize);
  delete [] out;
#else
  MPI_Status status;
  MPI_Recv(this->buffer, this->bufSize, MPI_PACKED, mach, tag,
           Partition::getComm(), &status);
#endif
}
