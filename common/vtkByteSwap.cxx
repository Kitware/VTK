/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkByteSwap.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkByteSwap.h"
#include <memory.h>
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkByteSwap* vtkByteSwap::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkByteSwap");
  if(ret)
    {
    return (vtkByteSwap*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkByteSwap;
}




// Swap four byte word.
#ifdef VTK_WORDS_BIGENDIAN
void vtkByteSwap::Swap4BE(char *){}
#else
void vtkByteSwap::Swap4BE(char *mem_ptr1)
{
  char one_byte;

  one_byte    = mem_ptr1[0];
  mem_ptr1[0] = mem_ptr1[3];
  mem_ptr1[3] = one_byte;

  one_byte    = mem_ptr1[1];
  mem_ptr1[1] = mem_ptr1[2];
  mem_ptr1[2] = one_byte;
}
#endif

// Swap eight byte word.
#ifdef VTK_WORDS_BIGENDIAN
void vtkByteSwap::Swap8BE(char *){}
#else
void vtkByteSwap::Swap8BE(char *mem_ptr1)
{
  char one_byte;

  one_byte    = mem_ptr1[0];
  mem_ptr1[0] = mem_ptr1[7];
  mem_ptr1[7] = one_byte;

  one_byte    = mem_ptr1[1];
  mem_ptr1[1] = mem_ptr1[6];
  mem_ptr1[6] = one_byte;

  one_byte    = mem_ptr1[2];
  mem_ptr1[2] = mem_ptr1[5];
  mem_ptr1[5] = one_byte;

  one_byte    = mem_ptr1[3];
  mem_ptr1[3] = mem_ptr1[4];
  mem_ptr1[4] = one_byte;

}
#endif

// Swap bunch of bytes. Num is the number of four byte words to swap.
#ifdef VTK_WORDS_BIGENDIAN
void vtkByteSwap::Swap4BERange(char *,int){}
#else
void vtkByteSwap::Swap4BERange(char *mem_ptr1,int num)
{
  char one_byte;
  char *pos;
  int i;
  
  pos = mem_ptr1;
  
  for (i = 0; i < num; i++)
    {
    one_byte = pos[0];
    pos[0] = pos[3];
    pos[3] = one_byte;
    
    one_byte = pos[1];
    pos[1] = pos[2];
    pos[2] = one_byte;
    pos = pos + 4;
    }
  
}
#endif

// Swap bunch of bytes. Num is the number of eight byte words to swap.
#ifdef VTK_WORDS_BIGENDIAN
void vtkByteSwap::Swap8BERange(char *,int){}
#else
void vtkByteSwap::Swap8BERange(char *mem_ptr1, int num)
{
  char one_byte;
  char *pos;
  int i;
  
  pos = mem_ptr1;
  
  for (i = 0; i < num; i++)
    {
      one_byte    = pos[0];
      pos[0] = pos[7];
      pos[7] = one_byte;

      one_byte    = pos[1];
      pos[1] = pos[6];
      pos[6] = one_byte;

      one_byte    = pos[2];
      pos[2] = pos[5];
      pos[5] = one_byte;

      one_byte    = pos[3];
      pos[3] = pos[4];
      pos[4] = one_byte;

      pos += 8;
    }
  
}
#endif



// Swap bunch of bytes. Num is the number of four byte words to swap.
void vtkByteSwap::SwapWrite4BERange(char *mem_ptr1,int num, ostream *fp)
{
#ifndef VTK_WORDS_BIGENDIAN
  char one_byte;
  char *pos;
  int i;
  char *cpy;
  int chunkSize = 1000000;

  if (num < chunkSize)
    {
    chunkSize = num;
    }
  cpy = new char [chunkSize * 4];
 
  while (num)
    {
    memcpy(cpy, mem_ptr1, chunkSize * 4);
    
    pos = cpy;   
    for (i = 0; i < chunkSize; i++)
      {
      one_byte = pos[0];
      pos[0] = pos[3];
      pos[3] = one_byte;
      
      one_byte = pos[1];
      pos[1] = pos[2];
      pos[2] = one_byte;
      pos = pos + 4;
      }
    fp->write((char *)cpy, 4*chunkSize);
    mem_ptr1 += chunkSize*4;
    num -= chunkSize;
    if (num < chunkSize)
      {
      chunkSize = num;
      }
    }

  delete [] cpy;
#else
  fp->write((char *)mem_ptr1, 4*num);
#endif
}

// Swap bunch of bytes. Num is the number of four byte words to swap.
void vtkByteSwap::SwapWrite4BERange(char *mem_ptr1,int num, FILE *fp)
{
#ifndef VTK_WORDS_BIGENDIAN
  char one_byte;
  char *pos;
  int i;
  char *cpy;
  int chunkSize = 1000000;

  if (num < chunkSize)
    {
    chunkSize = num;
    }
  cpy = new char [chunkSize * 4];
 
  while (num)
    {
    memcpy(cpy, mem_ptr1, chunkSize * 4);
  
    pos = cpy;    
    for (i = 0; i < chunkSize; i++)
      {
      one_byte = pos[0];
      pos[0] = pos[3];
      pos[3] = one_byte;
      
      one_byte = pos[1];
      pos[1] = pos[2];
      pos[2] = one_byte;
      pos = pos + 4;
      }
    fwrite(cpy,4,chunkSize,fp);
    mem_ptr1 += chunkSize*4;
    num -= chunkSize;
    if (num < chunkSize)
      {
      chunkSize = num;
      }
    }
  delete [] cpy;
  
#else
  fwrite(mem_ptr1,4,num,fp);
#endif
}

// Swap bunch of bytes. Num is the number of eight byte words to swap.
void vtkByteSwap::SwapWrite8BERange(char *mem_ptr1,int num, ostream *fp)
{
#ifndef VTK_WORDS_BIGENDIAN
  char one_byte;
  char *pos;
  int i;
  char *cpy;
  int chunkSize = 1000000;

  if (num < chunkSize)
    {
    chunkSize = num;
    }
  cpy = new char [chunkSize * 8];
 
  while (num)
    {
    memcpy(cpy, mem_ptr1, chunkSize * 8);
    
    pos = cpy;   
    for (i = 0; i < chunkSize; i++)
      {

	one_byte    = pos[0];
	pos[0] = pos[7];
	pos[7] = one_byte;

	one_byte    = pos[1];
	pos[1] = pos[6];
	pos[6] = one_byte;

	one_byte    = pos[2];
	pos[2] = pos[5];
	pos[5] = one_byte;

	one_byte    = pos[3];
	pos[3] = pos[4];
	pos[4] = one_byte;

	pos += 8;

      }
    fp->write((char *)cpy, 8*chunkSize);
    mem_ptr1 += chunkSize*8;
    num -= chunkSize;
    if (num < chunkSize)
      {
      chunkSize = num;
      }
    }

  delete [] cpy;
#else
  fp->write((char *)mem_ptr1, 8*num);
#endif
}

// Swap bunch of bytes. Num is the number of eight byte words to swap.
void vtkByteSwap::SwapWrite8BERange(char *mem_ptr1,int num, FILE *fp)
{
#ifndef VTK_WORDS_BIGENDIAN
  char one_byte;
  char *pos;
  int i;
  char *cpy;
  int chunkSize = 1000000;

  if (num < chunkSize)
    {
    chunkSize = num;
    }
  cpy = new char [chunkSize * 8];
 
  while (num)
    {
    memcpy(cpy, mem_ptr1, chunkSize * 8);
  
    pos = cpy;    
    for (i = 0; i < chunkSize; i++)
      {

	one_byte    = pos[0];
	pos[0] = pos[7];
	pos[7] = one_byte;

	one_byte    = pos[1];
	pos[1] = pos[6];
	pos[6] = one_byte;

	one_byte    = pos[2];
	pos[2] = pos[5];
	pos[5] = one_byte;

	one_byte    = pos[3];
	pos[3] = pos[4];
	pos[4] = one_byte;

	pos += 8;

      }
    fwrite(cpy,8,chunkSize,fp);
    mem_ptr1 += chunkSize*8;
    num -= chunkSize;
    if (num < chunkSize)
      {
      chunkSize = num;
      }
    }
  delete [] cpy;
  
#else
  fwrite(mem_ptr1,8,num,fp);
#endif
}

// Swap 2 byte word.
#ifdef VTK_WORDS_BIGENDIAN
void vtkByteSwap::Swap2LE(short *mem_ptr)
{
  unsigned short h1,h2;

  h1 = (unsigned short) *mem_ptr << 8;
  h2 = (unsigned short) *mem_ptr >> 8;
  *mem_ptr = (short) h1 | h2;

}
#else
void vtkByteSwap::Swap2LE(short *) {}
#endif

// Swap four byte word.
#ifdef VTK_WORDS_BIGENDIAN
void vtkByteSwap::Swap4LE(char *mem_ptr1)
{
  char one_byte;

  one_byte    = mem_ptr1[0];
  mem_ptr1[0] = mem_ptr1[3];
  mem_ptr1[3] = one_byte;

  one_byte    = mem_ptr1[1];
  mem_ptr1[1] = mem_ptr1[2];
  mem_ptr1[2] = one_byte;
}
#else
void vtkByteSwap::Swap4LE(char *){}
#endif

// Swap eight byte word.
#ifdef VTK_WORDS_BIGENDIAN
void vtkByteSwap::Swap8LE(char *mem_ptr1)
{
  char one_byte;

  one_byte    = mem_ptr1[0];
  mem_ptr1[0] = mem_ptr1[3];
  mem_ptr1[3] = one_byte;

  one_byte    = mem_ptr1[1];
  mem_ptr1[1] = mem_ptr1[2];
  mem_ptr1[2] = one_byte;
}
#else
void vtkByteSwap::Swap8LE(char *){}
#endif

// Swap bunch of bytes. Num is the number of four byte words to swap.
#ifdef VTK_WORDS_BIGENDIAN
void vtkByteSwap::Swap4LERange(char *mem_ptr1,int num)
{
  char one_byte;
  char *pos;
  int i;
  
  pos = mem_ptr1;
  
  for (i = 0; i < num; i++)
    {
    one_byte    = pos[0];
    pos[0] = pos[3];
    pos[3] = one_byte;
    
    one_byte    = pos[1];
    pos[1] = pos[2];
    pos[2] = one_byte;
    pos = pos + 4;
    }
  
}
#else
void vtkByteSwap::Swap4LERange(char *,int) {}
#endif

// Swap bunch of bytes. Num is the number of eight byte words to swap.
#ifdef VTK_WORDS_BIGENDIAN
void vtkByteSwap::Swap8LERange(char *mem_ptr1,int num)
{
  char one_byte;
  char *pos;
  int i;
  
  pos = mem_ptr1;
  
  for (i = 0; i < num; i++)
    {
	one_byte    = pos[0];
	pos[0] = pos[7];
	pos[7] = one_byte;

	one_byte    = pos[1];
	pos[1] = pos[6];
	pos[6] = one_byte;

	one_byte    = pos[2];
	pos[2] = pos[5];
	pos[5] = one_byte;

	one_byte    = pos[3];
	pos[3] = pos[4];
	pos[4] = one_byte;

	pos += 8;

    }
  
}
#else
void vtkByteSwap::Swap8LERange(char *,int) {}
#endif

// Swap 2 byte word.
#ifdef VTK_WORDS_BIGENDIAN
void vtkByteSwap::Swap2BE(short *) {}
#else
void vtkByteSwap::Swap2BE(short *mem_ptr)
{
  unsigned short h1,h2;

  h1 = (unsigned short)*mem_ptr << 8;
  h2 = (unsigned short)*mem_ptr >> 8;
  *mem_ptr = (short) h1 | h2;

}
#endif

// Swap bunch of bytes. Num is the number of two byte words to swap.
#ifdef VTK_WORDS_BIGENDIAN
void vtkByteSwap::Swap2BERange(char *,int) {}
#else
void vtkByteSwap::Swap2BERange(char *mem_ptr1,int num)
{
  char one_byte;
  char *pos;
  int i;
  
  pos = mem_ptr1;
  
  for (i = 0; i < num; i++)
    {
    one_byte = pos[0];
    pos[0] = pos[1];
    pos[1] = one_byte;
    pos = pos + 2;
    }
  
}
#endif

// Swap bunch of bytes. Num is the number of two byte words to swap.
#ifdef VTK_WORDS_BIGENDIAN
void vtkByteSwap::Swap2LERange(char *mem_ptr1,int num)
{
  char one_byte;
  char *pos;
  int i;
  
  pos = mem_ptr1;
  
  for (i = 0; i < num; i++)
    {
    one_byte = pos[0];
    pos[0] = pos[1];
    pos[1] = one_byte;
    pos = pos + 2;
    }
  
}
#else
void vtkByteSwap::Swap2LERange(char *mem_ptr1,int num){}
#endif



// Swap bunch of bytes. Num is the number of four byte words to swap.
void vtkByteSwap::SwapWrite2BERange(char *mem_ptr1,int num, ostream *fp)
{
#ifndef VTK_WORDS_BIGENDIAN
  char one_byte;
  char *pos;
  int i;
  char *cpy;
  int chunkSize = 1000000;

  if (num < chunkSize)
    {
    chunkSize = num;
    }
  cpy = new char [chunkSize * 2];
 
  while (num)
    {
    memcpy(cpy, mem_ptr1, chunkSize * 2);
 
    pos = cpy; 
    for (i = 0; i < chunkSize; i++)
      {
      one_byte = pos[0];
      pos[0] = pos[1];
      pos[1] = one_byte;
      pos = pos + 2;
      }
    fp->write((char *)cpy, 2*chunkSize);
    mem_ptr1 += chunkSize * 2;
    num -= chunkSize;
    if (num < chunkSize)
      {
      chunkSize = num;
      }
    }
    
  delete [] cpy;
  
#else
  fp->write((char *)mem_ptr1, 2*num);
#endif
}

// Swap bunch of bytes. Num is the number of four byte words to swap.
void vtkByteSwap::SwapWrite2BERange(char *mem_ptr1,int num, FILE *fp)
{
#ifndef VTK_WORDS_BIGENDIAN
  char one_byte;
  char *pos;
  int i;
  char *cpy;
  int chunkSize = 1000000;

  if (num < chunkSize)
    {
    chunkSize = num;
    }
  cpy = new char [chunkSize * 2];
 
  while (num)
    {
    memcpy(cpy, mem_ptr1, chunkSize * 2);
  
    pos = cpy; 
    for (i = 0; i < chunkSize; i++)
      {
      one_byte = pos[0];
      pos[0] = pos[1];
      pos[1] = one_byte;
      pos = pos + 2;
      }
    fwrite(cpy,2,chunkSize,fp);
    mem_ptr1 += chunkSize*2;
    num -= chunkSize;
    if (num < chunkSize)
      {
      chunkSize = num;
      }
    }
  delete [] cpy;
  
#else
  fwrite(mem_ptr1,2,num,fp);
#endif
}

//----------------------------------------------------------------------------
// Swaps the bytes of a buffer.  Uses an arbitrary word size, but
// assumes the word size is divisible by two.
void vtkByteSwap::SwapVoidRange(void *buffer, int numWords, int wordSize)
{
  unsigned char temp, *out, *buf;
  int idx1, idx2, inc, half;
  
  half = wordSize / 2;
  inc = wordSize - 1;
  buf = (unsigned char *)(buffer);
  
  for (idx1 = 0; idx1 < numWords; ++idx1)
    {
    out = buf + inc;
    for (idx2 = 0; idx2 < half; ++idx2)
      {
      temp = *out;
      *out = *buf;
      *buf = temp;
      ++buf;
      --out;
      }
    buf += half;
    }
}

  
    
