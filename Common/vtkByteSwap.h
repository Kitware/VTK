/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkByteSwap.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkByteSwap - perform machine dependent byte swapping
// .SECTION Description
// vtkByteSwap is used by other classes to perform machine dependent byte
// swapping. Byte swapping is often used when reading or writing binary 
// files.
#ifndef __vtkByteSwap_h
#define __vtkByteSwap_h

#include "vtkObject.h"

class VTK_COMMON_EXPORT vtkByteSwap : public vtkObject
{
public:
  static vtkByteSwap *New();
  vtkTypeRevisionMacro(vtkByteSwap,vtkObject);

  // Description:
  // Swap 2 byte word to be LE.
  static void Swap2LE(short *s);
  static void Swap2LE(unsigned short *s);

  // Description:
  // Swap four byte word to be LE.
  static void Swap4LE(char *c);
  static void Swap4LE(float *p) { vtkByteSwap::Swap4LE((char *)p);};
  static void Swap4LE(int *i)   { vtkByteSwap::Swap4LE((char *)i);};
  static void Swap4LE(unsigned long *i) { vtkByteSwap::Swap4LE((char *)i);};
  static void Swap4LE(long *i) { vtkByteSwap::Swap4LE((char *)i);};

  // Description:
  // Swap eight byte word to be LE.  Currently implemented for doubles, but
  // will be necessary for 64bit integers? 16bit chars?
  static void Swap8LE(char *c);
  static void Swap8LE(double *d) { vtkByteSwap::Swap8LE((char *)d);};

  // Description:
  // Swap bunch of bytes to LE. Num is the number of two byte words to swap.
  static void Swap2LERange(char *c,int num);
  static void Swap2LERange(short *i,int num) 
  { vtkByteSwap::Swap2LERange((char *)i,num);};

  // Description:
  // Swap bunch of bytes to be LE. Num is the number of four byte words to swap.
  static void Swap4LERange(char *c,int num);
  static void Swap4LERange(unsigned char *c,int num) 
  { vtkByteSwap::Swap4LERange((char *)c,num);};
  static void Swap4LERange(float *p,int num) 
  { vtkByteSwap::Swap4LERange((char *)p,num);};
  static void Swap4LERange(int *i,int num) 
  { vtkByteSwap::Swap4LERange((char *)i,num);};
  static void Swap4LERange(unsigned long *i,int num) 
  { vtkByteSwap::Swap4LERange((char *)i,num);};

  // Description:
  // Swap bunch of bytes to be LE. Num is the number of eight byte words to swap.
  // Currently implemented for doubles...
  static void Swap8LERange(char *c, int num);
  static void Swap8LERange(double *d, int num) 
  { vtkByteSwap::Swap8LERange((char *)d, num);};

  // Description:
  // Swap 2 byte word to BE.
  static void Swap2BE(short *s);
  static void Swap2BE(unsigned short *s);

  // Description:
  // For writing, swap four byte word to be BE.
  // For reading, swap four byte word from BE to machine's internal
  // representation.
  static void Swap4BE(char *c);
  static void Swap4BE(float *p) { vtkByteSwap::Swap4BE((char *)p);};
  static void Swap4BE(int *i)   { vtkByteSwap::Swap4BE((char *)i);};
  static void Swap4BE(unsigned long *i) { vtkByteSwap::Swap4BE((char *)i);};

  // Description:
  // For writing, swap eight byte word to be BE.
  // For reading, swap eight byte word from BE to machine's internal
  // representation.
  static void Swap8BE(char *c);
  static void Swap8BE(double *d) { vtkByteSwap::Swap8BE((char *)d);};

  // Description:
  // Swap bunch of bytes to BE. Num is the number of two byte words to swap.
  static void Swap2BERange(char *c,int num);
  static void Swap2BERange(short *i,int num) 
  { vtkByteSwap::Swap2BERange((char *)i,num);};

    // Description:
  // Swap bunch of bytes to be BE. Num is the number of four byte words to swap.
  static void Swap4BERange(char *c,int num);
  static void Swap4BERange(float *p,int num) 
  { vtkByteSwap::Swap4BERange((char *)p,num); };
  static void Swap4BERange(int *i,int num) 
  { vtkByteSwap::Swap4BERange((char *)i,num); };
  static void Swap4BERange(unsigned long *i,int num) 
  { vtkByteSwap::Swap4BERange((char *)i,num); };

#ifdef VTK_USE_64BIT_IDS
  static void Swap4BERange(vtkIdType *i,int num) 
  { vtkByteSwap::Swap4BERange((char *)i,num); };
#endif

  // Description:
  // Swap bunch of bytes to be BE. Num is the number of eight byte words to swap.
  static void Swap8BERange(char *c,int num);
  static void Swap8BERange(double *d,int num) 
  { vtkByteSwap::Swap8BERange((char *)d,num); };

  // Description:
  // Swap bunch of bytes to BE. Num is the number of two byte words to swap.
  // The results are written out to file to prevent having to keep the swapped
  // copy in memory.
  static void SwapWrite2BERange(char *c,int num,FILE *fp);
  static void SwapWrite2BERange(short *i,int num, FILE *fp) 
  {vtkByteSwap::SwapWrite2BERange((char *)i,num,fp);};

  // Description:
  // Swap bunch of bytes to BE. Num is the number of four byte words to swap.
  // The results are written out to file to prevent having to keep the swapped
  // copy in memory.
  static void SwapWrite4BERange(char *c,int num,FILE *fp);
  static void SwapWrite4BERange(float *p,int num, FILE *fp) 
  { vtkByteSwap::SwapWrite4BERange((char *)p,num,fp);};
  static void SwapWrite4BERange(int *i,int num,FILE *fp) 
  { vtkByteSwap::SwapWrite4BERange((char *)i,num,fp);};
  static void SwapWrite4BERange(unsigned long *i,int num, FILE *fp) 
  { vtkByteSwap::SwapWrite4BERange((char *)i,num,fp);};
#ifdef VTK_USE_64BIT_IDS
  static void SwapWrite4BERange(vtkIdType *i,int num, FILE *fp) 
  { vtkByteSwap::SwapWrite4BERange((char *)i,num,fp);};
#endif
  // Description:
  // Swap bunch of bytes to BE. Num is the number of eight byte words to swap.
  // The results are written out to file to prevent having to keep the swapped
  // copy in memory.  Implemented for doubles for now.
  static void SwapWrite8BERange(char *c,int num,FILE *fp);
  static void SwapWrite8BERange(double *d,int num, FILE *fp) 
  { vtkByteSwap::SwapWrite8BERange((char *)d,num,fp);};

  // Description:
  // Swap bunch of bytes to BE. Num is the number of two byte words to swap.
  // The results are written out to stream to prevent having to keep the swapped
  // copy in memory.
  static void SwapWrite2BERange(char *c,int num, ostream *fp);
  static void SwapWrite2BERange(short *i,int num, ostream *fp) 
  {vtkByteSwap::SwapWrite2BERange((char *)i,num,fp);};

  // Description:
  // Swap bunch of bytes to BE. Num is the number of four byte words to swap.
  // The results are written out to stream to prevent having to keep the swapped
  // copy in memory.
  static void SwapWrite4BERange(char *c,int num, ostream *fp);
  static void SwapWrite4BERange(float *p,int num, ostream *fp) 
  { vtkByteSwap::SwapWrite4BERange((char *)p,num,fp);};
  static void SwapWrite4BERange(int *i,int num, ostream *fp) 
  { vtkByteSwap::SwapWrite4BERange((char *)i,num,fp);};
  static void SwapWrite4BERange(unsigned long *i,int num, ostream *fp) 
  { vtkByteSwap::SwapWrite4BERange((char *)i,num,fp);};
#ifdef VTK_USE_64BIT_IDS
  static void SwapWrite4BERange(vtkIdType *i,int num, ostream *fp) 
  { vtkByteSwap::SwapWrite4BERange((char *)i,num,fp);};
#endif
  // Description:
  // Swap bunch of bytes to BE. Num is the number of eight byte words to swap.
  // The results are written out to stream to prevent having to keep the swapped
  // copy in memory.
  static void SwapWrite8BERange(char *c,int num, ostream *fp);
  static void SwapWrite8BERange(double *d,int num, ostream *fp) 
  { vtkByteSwap::SwapWrite8BERange((char *)d,num,fp);};

  // Description:
  // Swaps the bytes of a buffer.  Uses an arbitrary word size, but
  // assumes the word size is divisible by two.
  static void SwapVoidRange(void *buffer, int numWords, int wordSize);


protected:
  vtkByteSwap() {};
  ~vtkByteSwap() {};

private:
  // Description:
  // Swaps bytes. 
  static void Swap2Bytes(char* &data);

  static void Swap4Bytes(char* &data);
 
  static void Swap8Bytes(char* &data);

private:
  vtkByteSwap(const vtkByteSwap&);  // Not implemented.
  void operator=(const vtkByteSwap&);  // Not implemented.
};


inline void 
vtkByteSwap::Swap2Bytes(char* &data)
{ 
  char one_byte;  
  one_byte = data[0]; data[0] = data[1]; data[1] = one_byte;
}

inline void 
vtkByteSwap::Swap4Bytes(char* &data)
{ 
  char one_byte; 
  one_byte = data[0]; data[0] = data[3]; data[3] = one_byte;
  one_byte = data[1]; data[1] = data[2]; data[2] = one_byte; 
}

inline void 
vtkByteSwap::Swap8Bytes(char* &data)
{ 
  char one_byte;
  one_byte = data[0]; data[0] = data[7]; data[7] = one_byte;
  one_byte = data[1]; data[1] = data[6]; data[6] = one_byte;
  one_byte = data[2]; data[2] = data[5]; data[5] = one_byte;
  one_byte = data[3]; data[3] = data[4]; data[4] = one_byte; 
}

#endif
