/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkByteSwap.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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
#ifndef vtkByteSwap_h
#define vtkByteSwap_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

class VTKCOMMONCORE_EXPORT vtkByteSwap : public vtkObject
{
public:
  static vtkByteSwap *New();
  vtkTypeMacro(vtkByteSwap,vtkObject);

  //BTX
  // Description:
  // Type-safe swap signatures to swap for storage in either Little
  // Endian or Big Endian format.  Swapping is performed according to
  // the true size of the type given.
#define VTK_BYTE_SWAP_DECL(T)                                           \
  static void SwapLE(T* p);                                             \
  static void SwapBE(T* p);                                             \
  static void SwapLERange(T* p, size_t num);                            \
  static void SwapBERange(T* p, size_t num);                            \
  static bool SwapLERangeWrite(const T* p, size_t num, FILE* file);     \
  static bool SwapBERangeWrite(const T* p, size_t num, FILE* file);     \
  static void SwapLERangeWrite(const T* p, size_t num, ostream* os);    \
  static void SwapBERangeWrite(const T* p, size_t num, ostream* os)
  VTK_BYTE_SWAP_DECL(float);
  VTK_BYTE_SWAP_DECL(double);
  VTK_BYTE_SWAP_DECL(char);
  VTK_BYTE_SWAP_DECL(short);
  VTK_BYTE_SWAP_DECL(int);
  VTK_BYTE_SWAP_DECL(long);
  VTK_BYTE_SWAP_DECL(signed char);
  VTK_BYTE_SWAP_DECL(unsigned char);
  VTK_BYTE_SWAP_DECL(unsigned short);
  VTK_BYTE_SWAP_DECL(unsigned int);
  VTK_BYTE_SWAP_DECL(unsigned long);
#if defined(VTK_DECL_USE_LONG_LONG)
  VTK_BYTE_SWAP_DECL(long long);
  VTK_BYTE_SWAP_DECL(unsigned long long);
#endif
#if defined(VTK_DECL_USE___INT64)
  VTK_BYTE_SWAP_DECL(__int64);
  VTK_BYTE_SWAP_DECL(unsigned __int64);
#endif
#undef VTK_BYTE_SWAP_DECL
  //ETX

  // Description:
  // Swap 2, 4, or 8 bytes for storage as Little Endian.
  static void Swap2LE(void* p);
  static void Swap4LE(void* p);
  static void Swap8LE(void* p);

  // Description:
  // Swap a block of 2-, 4-, or 8-byte segments for storage as Little Endian.
  static void Swap2LERange(void* p, size_t num);
  static void Swap4LERange(void* p, size_t num);
  static void Swap8LERange(void* p, size_t num);

  // Description:
  // Swap a block of 2-, 4-, or 8-byte segments for storage as Little Endian.
  // The results are written directly to a file to avoid temporary storage.
  static bool SwapWrite2LERange(void const* p, size_t num, FILE* f);
  static bool SwapWrite4LERange(void const* p, size_t num, FILE* f);
  static bool SwapWrite8LERange(void const* p, size_t num, FILE* f);
  static void SwapWrite2LERange(void const* p, size_t num, ostream* os);
  static void SwapWrite4LERange(void const* p, size_t num, ostream* os);
  static void SwapWrite8LERange(void const* p, size_t num, ostream* os);

  // Description:
  // Swap 2, 4, or 8 bytes for storage as Big Endian.
  static void Swap2BE(void* p);
  static void Swap4BE(void* p);
  static void Swap8BE(void* p);

  // Description:
  // Swap a block of 2-, 4-, or 8-byte segments for storage as Big Endian.
  static void Swap2BERange(void* p, size_t num);
  static void Swap4BERange(void* p, size_t num);
  static void Swap8BERange(void* p, size_t num);

  // Description:
  // Swap a block of 2-, 4-, or 8-byte segments for storage as Big Endian.
  // The results are written directly to a file to avoid temporary storage.
  static bool SwapWrite2BERange(void const* p, size_t num, FILE* f);
  static bool SwapWrite4BERange(void const* p, size_t num, FILE* f);
  static bool SwapWrite8BERange(void const* p, size_t num, FILE* f);
  static void SwapWrite2BERange(void const* p, size_t num, ostream* os);
  static void SwapWrite4BERange(void const* p, size_t num, ostream* os);
  static void SwapWrite8BERange(void const* p, size_t num, ostream* os);

  // Description:
  // Swaps the bytes of a buffer.  Uses an arbitrary word size, but
  // assumes the word size is divisible by two.
  static void SwapVoidRange(void *buffer, size_t numWords, size_t wordSize);

protected:
  vtkByteSwap();
  ~vtkByteSwap();

private:
  vtkByteSwap(const vtkByteSwap&);  // Not implemented.
  void operator=(const vtkByteSwap&);  // Not implemented.
};

#endif
// VTK-HeaderTest-Exclude: vtkByteSwap.h
