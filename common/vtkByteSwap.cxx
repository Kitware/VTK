/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkByteSwap.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkByteSwap.h"
#include <memory.h>

// Description:
// Swap four byte word.
void vtkByteSwap::Swap4BE(char *mem_ptr1)
{
#ifndef VTK_WORDS_BIGENDIAN
  char one_byte;

  one_byte    = mem_ptr1[0];
  mem_ptr1[0] = mem_ptr1[3];
  mem_ptr1[3] = one_byte;

  one_byte    = mem_ptr1[1];
  mem_ptr1[1] = mem_ptr1[2];
  mem_ptr1[2] = one_byte;
#endif
}

// Description:
// Swap bunch of bytes. Num is the number of four byte words to swap.
void vtkByteSwap::Swap4BERange(char *mem_ptr1,int num)
{
#ifndef VTK_WORDS_BIGENDIAN
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
  
#endif
}

// Description:
// Swap bunch of bytes. Num is the number of four byte words to swap.
void vtkByteSwap::SwapWrite4BERange(char *mem_ptr1,int num, FILE *fp)
{
#ifndef VTK_WORDS_BIGENDIAN
  char one_byte;
  char *pos;
  int i;
  char *cpy;
  
  cpy = new char [num*4];
  memcpy(cpy, mem_ptr1,num*4);
  
  pos = cpy;
  
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
  fwrite(cpy,4,num,fp);
  delete [] cpy;
  
#else
  fwrite(mem_ptr1,4,num,fp);
#endif
}
// Description:
// Swap 2 byte word.
void vtkByteSwap::Swap2LE(short *mem_ptr)
{
#if VTK_WORDS_BIGENDIAN
  unsigned h1,h2;

  h1 = *mem_ptr << 8;
  h2 = *mem_ptr >> 8;
  *mem_ptr = (short) h1 | h2;

#endif
}

// Description:
// Swap four byte word.
void vtkByteSwap::Swap4LE(char *mem_ptr1)
{
#if VTK_WORDS_BIGENDIAN
  char one_byte;

  one_byte    = mem_ptr1[0];
  mem_ptr1[0] = mem_ptr1[3];
  mem_ptr1[3] = one_byte;

  one_byte    = mem_ptr1[1];
  mem_ptr1[1] = mem_ptr1[2];
  mem_ptr1[2] = one_byte;
#endif
}

// Description:
// Swap bunch of bytes. Num is the number of four byte words to swap.
void vtkByteSwap::Swap4LERange(char *mem_ptr1,int num)
{
#ifdef VTK_WORDS_BIGENDIAN
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
  
#endif
}

// Description:
// Swap 2 byte word.
void vtkByteSwap::Swap2BE(short *mem_ptr)
{
#ifndef VTK_WORDS_BIGENDIAN
  unsigned h1,h2;

  h1 = *mem_ptr << 8;
  h2 = *mem_ptr >> 8;
  *mem_ptr = (short) h1 | h2;

#endif
}
// Description:
// Swap bunch of bytes. Num is the number of two byte words to swap.
void vtkByteSwap::Swap2BERange(char *mem_ptr1,int num)
{
#ifndef VTK_WORDS_BIGENDIAN
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
  
#endif
}

// Description:
// Swap bunch of bytes. Num is the number of two byte words to swap.
void vtkByteSwap::Swap2LERange(char *mem_ptr1,int num)
{
#ifdef VTK_WORDS_BIGENDIAN
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
  
#endif
}

// Description:
// Swap bunch of bytes. Num is the number of four byte words to swap.
void vtkByteSwap::SwapWrite2BERange(char *mem_ptr1,int num, FILE *fp)
{
#ifndef VTK_WORDS_BIGENDIAN
  char one_byte;
  char *pos;
  int i;
  char *cpy;
  
  cpy = new char [num*2];
  memcpy(cpy, mem_ptr1,num*2);
  
  pos = cpy;
  
  for (i = 0; i < num; i++)
    {
    one_byte = pos[0];
    pos[0] = pos[1];
    pos[1] = one_byte;
    pos = pos + 2;
    }
  fwrite(cpy,2,num,fp);
  delete [] cpy;
  
#else
  fwrite(mem_ptr1,2,num,fp);
#endif
}

