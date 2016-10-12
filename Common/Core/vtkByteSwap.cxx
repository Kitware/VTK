/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkByteSwap.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkByteSwap.h"
#include <memory.h>
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkByteSwap);

//----------------------------------------------------------------------------
vtkByteSwap::vtkByteSwap()
{
}

//----------------------------------------------------------------------------
vtkByteSwap::~vtkByteSwap()
{
}

//----------------------------------------------------------------------------
// Define swap functions for each type size.
template <size_t s> struct vtkByteSwapper;
template<> struct vtkByteSwapper<1>
{
  static inline void Swap(char*) {}
};
template<> struct vtkByteSwapper<2>
{
  static inline void Swap(char* data)
  {
    char one_byte;
    one_byte = data[0]; data[0] = data[1]; data[1] = one_byte;
  }
};
template<> struct vtkByteSwapper<4>
{
  static inline void Swap(char* data)
  {
    char one_byte;
    one_byte = data[0]; data[0] = data[3]; data[3] = one_byte;
    one_byte = data[1]; data[1] = data[2]; data[2] = one_byte;
  }
};
template<> struct vtkByteSwapper<8>
{
  static inline void Swap(char* data)
  {
    char one_byte;
    one_byte = data[0]; data[0] = data[7]; data[7] = one_byte;
    one_byte = data[1]; data[1] = data[6]; data[6] = one_byte;
    one_byte = data[2]; data[2] = data[5]; data[5] = one_byte;
    one_byte = data[3]; data[3] = data[4]; data[4] = one_byte;
  }
};

//----------------------------------------------------------------------------
// Define range swap functions.
template <class T> inline void vtkByteSwapRange(T* first, size_t num)
{
  // Swap one value at a time.
  T* last = first + num;
  for(T* p=first; p != last; ++p)
  {
    vtkByteSwapper<sizeof(T)>::Swap(reinterpret_cast<char*>(p));
  }
}
inline bool vtkByteSwapRangeWrite(const char* first, size_t num,
                                  FILE* f, int)
{
  // No need to swap segments of 1 byte.
  size_t status=fwrite(first, sizeof(char), static_cast<size_t>(num), f);
  return status==static_cast<size_t>(num);
}
inline bool vtkByteSwapRangeWrite(const signed char* first, size_t num,
                                  FILE* f, int)
{
  // No need to swap segments of 1 byte.
  size_t status=fwrite(first, sizeof(signed char),static_cast<size_t>(num),f);
  return status==static_cast<size_t>(num);
}
inline bool vtkByteSwapRangeWrite(const unsigned char* first, size_t num,
                                  FILE* f, int)
{
  // No need to swap segments of 1 byte.
  size_t status=fwrite(first,sizeof(unsigned char),static_cast<size_t>(num),f);
  return status==static_cast<size_t>(num);
}
template <class T>
inline bool vtkByteSwapRangeWrite(const T* first, size_t num, FILE* f, long)
{
  // Swap and write one value at a time.  We do not need to do this in
  // blocks because the file stream is already buffered.
  const T* last = first + num;
  bool result=true;
  for(const T* p=first; p != last && result; ++p)
  {
    // Use a union to avoid breaking C++ aliasing rules.
    union { T value; char data[sizeof(T)]; } temp = {*p};
    vtkByteSwapper<sizeof(T)>::Swap(temp.data);
    size_t status=fwrite(temp.data, sizeof(T), 1, f);
    result=status==1;
  }
  return result;
}
inline void vtkByteSwapRangeWrite(const char* first, size_t num,
                                  ostream* os, int)
{
  // No need to swap segments of 1 byte.
  os->write(first, num*static_cast<size_t>(sizeof(char)));
}
inline void vtkByteSwapRangeWrite(const signed char* first, size_t num,
                                  ostream* os, int)
{
  // No need to swap segments of 1 byte.
  os->write(reinterpret_cast<const char*>(first),
            num*static_cast<size_t>(sizeof(signed char)));
}
inline void vtkByteSwapRangeWrite(const unsigned char* first, size_t num,
                                  ostream* os, int)
{
  // No need to swap segments of 1 byte.
  os->write(reinterpret_cast<const char*>(first),
            num*static_cast<size_t>(sizeof(unsigned char)));
}
template <class T>
inline void vtkByteSwapRangeWrite(const T* first, size_t num,
                                  ostream* os, long)
{
  // Swap and write one value at a time.  We do not need to do this in
  // blocks because the file stream is already buffered.
  const T* last = first + num;
  for(const T* p=first; p != last; ++p)
  {
    // Use a union to avoid breaking C++ aliasing rules.
    union { T value; char data[sizeof(T)]; } temp = {*p};
    vtkByteSwapper<sizeof(T)>::Swap(temp.data);
    os->write(temp.data, sizeof(T));
  }
}

//----------------------------------------------------------------------------
// Define swap functions for each endian-ness.
#if defined(VTK_WORDS_BIGENDIAN)
template <class T> inline void vtkByteSwapBE(T*) {}
template <class T> inline void vtkByteSwapBERange(T*, size_t) {}
template <class T>
inline bool vtkByteSwapBERangeWrite(const T* p, size_t num, FILE* f)
{
  size_t status=fwrite(p, sizeof(T), static_cast<size_t>(num), f);
  return status==static_cast<size_t>(num);
}
template <class T>
inline void vtkByteSwapBERangeWrite(const T* p, size_t num, ostream* os)
{
  os->write((char*)p, sizeof(T)*num);
}
template <class T> inline void vtkByteSwapLE(T* p)
{
  vtkByteSwapper<sizeof(T)>::Swap(reinterpret_cast<char*>(p));
}
template <class T> inline void vtkByteSwapLERange(T* p, size_t num)
{
  vtkByteSwapRange(p, num);
}
template <class T>
inline bool vtkByteSwapLERangeWrite(const T* p, size_t num, FILE* f)
{
  return vtkByteSwapRangeWrite(p, num, f, 1);
}
template <class T>
inline void vtkByteSwapLERangeWrite(const T* p, size_t num, ostream* os)
{
  vtkByteSwapRangeWrite(p, num, os, 1);
}
#else
template <class T> inline void vtkByteSwapBE(T* p)
{
  vtkByteSwapper<sizeof(T)>::Swap(reinterpret_cast<char*>(p));
}
template <class T> inline void vtkByteSwapBERange(T* p, size_t num)
{
  vtkByteSwapRange(p, num);
}
template <class T>
inline bool vtkByteSwapBERangeWrite(const T* p, size_t num, FILE* f)
{
  return vtkByteSwapRangeWrite(p, num, f, 1);
}
template <class T>
inline void vtkByteSwapBERangeWrite(const T* p, size_t num, ostream* os)
{
  vtkByteSwapRangeWrite(p, num, os, 1);
}
template <class T> inline void vtkByteSwapLE(T*) {}
template <class T> inline void vtkByteSwapLERange(T*, size_t) {}
template <class T>
inline bool vtkByteSwapLERangeWrite(const T* p, size_t num, FILE* f)
{
  size_t status=fwrite(p, sizeof(T), static_cast<size_t>(num), f);
  return status==static_cast<size_t>(num);
}
template <class T>
inline void vtkByteSwapLERangeWrite(const T* p, size_t num, ostream* os)
{
  os->write(reinterpret_cast<const char*>(p),
            static_cast<size_t>(sizeof(T))*num);
}
#endif

//----------------------------------------------------------------------------
#define VTK_BYTE_SWAP_IMPL(T)                                             \
  void vtkByteSwap::SwapLE(T* p) { vtkByteSwapLE(p); }                    \
  void vtkByteSwap::SwapBE(T* p) { vtkByteSwapBE(p); }                    \
  void vtkByteSwap::SwapLERange(T* p, size_t num)                         \
    { vtkByteSwapLERange(p, num); }                                       \
  void vtkByteSwap::SwapBERange(T* p, size_t num)                         \
    { vtkByteSwapBERange(p, num); }                                       \
  bool vtkByteSwap::SwapLERangeWrite(const T* p, size_t num, FILE* file)  \
    { return vtkByteSwapLERangeWrite(p, num, file); }                     \
  bool vtkByteSwap::SwapBERangeWrite(const T* p, size_t num, FILE* file)  \
    { return vtkByteSwapBERangeWrite(p, num, file); }                     \
  void vtkByteSwap::SwapLERangeWrite(const T* p, size_t num, ostream* os) \
    { vtkByteSwapLERangeWrite(p, num, os); }                              \
  void vtkByteSwap::SwapBERangeWrite(const T* p, size_t num, ostream* os) \
    { vtkByteSwapBERangeWrite(p, num, os); }
VTK_BYTE_SWAP_IMPL(float)
VTK_BYTE_SWAP_IMPL(double)
VTK_BYTE_SWAP_IMPL(char)
VTK_BYTE_SWAP_IMPL(short)
VTK_BYTE_SWAP_IMPL(int)
VTK_BYTE_SWAP_IMPL(long)
VTK_BYTE_SWAP_IMPL(long long)
VTK_BYTE_SWAP_IMPL(signed char)
VTK_BYTE_SWAP_IMPL(unsigned char)
VTK_BYTE_SWAP_IMPL(unsigned short)
VTK_BYTE_SWAP_IMPL(unsigned int)
VTK_BYTE_SWAP_IMPL(unsigned long)
VTK_BYTE_SWAP_IMPL(unsigned long long)
#undef VTK_BYTE_SWAP_IMPL

#if VTK_SIZEOF_SHORT == 2
typedef short vtkByteSwapType2;
#else
# error "..."
#endif

#if VTK_SIZEOF_INT == 4
typedef int vtkByteSwapType4;
#else
# error "..."
#endif

#if VTK_SIZEOF_DOUBLE == 8
typedef double vtkByteSwapType8;
#else
# error "..."
#endif

//----------------------------------------------------------------------------
#define VTK_BYTE_SWAP_SIZE(S)                                                   \
  void vtkByteSwap::Swap##S##LE(void* p)                                        \
    { vtkByteSwap::SwapLE(static_cast<vtkByteSwapType##S*>(p)); }               \
  void vtkByteSwap::Swap##S##BE(void* p)                                        \
    { vtkByteSwap::SwapBE(static_cast<vtkByteSwapType##S*>(p)); }               \
  void vtkByteSwap::Swap##S##LERange(void* p, size_t n)                         \
    { vtkByteSwap::SwapLERange(static_cast<vtkByteSwapType##S*>(p), n); }       \
  void vtkByteSwap::Swap##S##BERange(void* p, size_t n)                         \
    { vtkByteSwap::SwapBERange(static_cast<vtkByteSwapType##S*>(p), n); }       \
  bool vtkByteSwap::SwapWrite##S##LERange(void const* p, size_t n, FILE* f)     \
    { return vtkByteSwap::SwapLERangeWrite(                                     \
       static_cast<const vtkByteSwapType##S*>(p), n, f); }                      \
  bool vtkByteSwap::SwapWrite##S##BERange(void const* p, size_t n, FILE* f)     \
    { return vtkByteSwap::SwapBERangeWrite(                                     \
        static_cast<const vtkByteSwapType##S*>(p), n, f); }                     \
  void vtkByteSwap::SwapWrite##S##LERange(void const* p, size_t n, ostream* os) \
    { vtkByteSwap::SwapLERangeWrite(                                            \
        static_cast<const vtkByteSwapType##S*>(p), n, os); }                    \
  void vtkByteSwap::SwapWrite##S##BERange(void const* p, size_t n, ostream* os) \
    { vtkByteSwap::SwapBERangeWrite(                                            \
        static_cast<const vtkByteSwapType##S*>(p), n, os); }
VTK_BYTE_SWAP_SIZE(2)
VTK_BYTE_SWAP_SIZE(4)
VTK_BYTE_SWAP_SIZE(8)
#undef VTK_BYTE_SWAP_SIZE

//----------------------------------------------------------------------------
// Swaps the bytes of a buffer.  Uses an arbitrary word size, but
// assumes the word size is divisible by two.
void vtkByteSwap::SwapVoidRange(void *buffer, size_t numWords, size_t wordSize)
{
  unsigned char temp, *out, *buf;
  size_t idx1, idx2, inc, half;

  half = wordSize / 2;
  inc = wordSize - 1;
  buf = static_cast<unsigned char *>(buffer);

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
