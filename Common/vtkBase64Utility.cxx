/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBase64Utility.cxx
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
#include "vtkBase64Utility.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkBase64Utility, "1.1.2.1");
vtkStandardNewMacro(vtkBase64Utility);

//----------------------------------------------------------------------------
static const unsigned char vtkBase64UtilityEncodeTable[65] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

//----------------------------------------------------------------------------
inline static unsigned char vtkBase64UtilityEncodeChar(unsigned char c)
{
  return vtkBase64UtilityEncodeTable[c];
}

//----------------------------------------------------------------------------
inline void vtkBase64Utility::EncodeTriplet(unsigned char i0,
                                            unsigned char i1,
                                            unsigned char i2,
                                            unsigned char *o0,
                                            unsigned char *o1,
                                            unsigned char *o2,
                                            unsigned char *o3)
{
  *o0 = vtkBase64UtilityEncodeChar((i0 >> 2) & 0x3F);
  *o1 = vtkBase64UtilityEncodeChar(((i0 << 4) & 0x30)|((i1 >> 4) & 0x0F));
  *o2 = vtkBase64UtilityEncodeChar(((i1 << 2) & 0x3C)|((i2 >> 6) & 0x03));
  *o3 = vtkBase64UtilityEncodeChar(i2 & 0x3F);
}

//----------------------------------------------------------------------------
inline void vtkBase64Utility::EncodePair(unsigned char i0,
                                         unsigned char i1,
                                         unsigned char *o0,
                                         unsigned char *o1,
                                         unsigned char *o2,
                                         unsigned char *o3)
{
  *o0 = vtkBase64UtilityEncodeChar((i0 >> 2) & 0x3F);
  *o1 = vtkBase64UtilityEncodeChar(((i0 << 4) & 0x30)|((i1 >> 4) & 0x0F));
  *o2 = vtkBase64UtilityEncodeChar(((i1 << 2) & 0x3C));
  *o3 = '=';
}

//----------------------------------------------------------------------------
inline void vtkBase64Utility::EncodeSingle(unsigned char i0,
                                           unsigned char *o0,
                                           unsigned char *o1,
                                           unsigned char *o2,
                                           unsigned char *o3)
{
  *o0 = vtkBase64UtilityEncodeChar((i0 >> 2) & 0x3F);
  *o1 = vtkBase64UtilityEncodeChar(((i0 << 4) & 0x30));
  *o2 = '=';
  *o3 = '=';
}

//----------------------------------------------------------------------------
unsigned long vtkBase64Utility::Encode(const unsigned char *input, 
                                       unsigned long length, 
                                       unsigned char *output)
{
  
  const unsigned char *ptr = input;
  const unsigned char *end = input + length;
  unsigned char *optr = output;

  // Encode complete triplet

  while ((end - ptr) >= 3)
    {
    vtkBase64Utility::EncodeTriplet(ptr[0], ptr[1], ptr[2], 
                                    &optr[0], &optr[1], &optr[2], &optr[3]);
    ptr += 3;
    optr += 4;
    }

  // Encodes a 2-byte ending into 3 bytes and 1 pad byte and writes.

  if (end - ptr == 2)
    {
    vtkBase64Utility::EncodePair(ptr[0], ptr[1],
                                 &optr[0], &optr[1], &optr[2], &optr[3]);
    optr += 4;
    }

  // Encodes a 1-byte ending into 2 bytes and 2 pad bytes
  
  else if (end - ptr == 1)
    {
    vtkBase64Utility::EncodeSingle(ptr[0],
                                   &optr[0], &optr[1], &optr[2], &optr[3]);
    optr += 4;
    }

  return optr - output;
}

//----------------------------------------------------------------------------
static const unsigned char vtkBase64UtilityDecodeTable[256] =
{
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0x3E,0xFF,0xFF,0xFF,0x3F,
  0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,
  0x3C,0x3D,0xFF,0xFF,0xFF,0x00,0xFF,0xFF,
  0xFF,0x00,0x01,0x02,0x03,0x04,0x05,0x06,
  0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,
  0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,
  0x17,0x18,0x19,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,
  0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,
  0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30,
  0x31,0x32,0x33,0xFF,0xFF,0xFF,0xFF,0xFF,
  //-------------------------------------
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
};

//----------------------------------------------------------------------------
inline static unsigned char vtkBase64UtilityDecodeChar(unsigned char c)
{
  return vtkBase64UtilityDecodeTable[c];
}

//----------------------------------------------------------------------------
inline int vtkBase64Utility::DecodeTriplet(unsigned char i0,
                                           unsigned char i1,
                                           unsigned char i2,
                                           unsigned char i3,
                                           unsigned char *o0,
                                           unsigned char *o1,
                                           unsigned char *o2)
{
  unsigned char d0, d1, d2, d3;

  d0 = vtkBase64UtilityDecodeChar(i0);
  d1 = vtkBase64UtilityDecodeChar(i1);
  d2 = vtkBase64UtilityDecodeChar(i2);
  d3 = vtkBase64UtilityDecodeChar(i3);
  
  // Make sure all characters were valid

  if (d0 == 0xFF || d1 == 0xFF || d2 == 0xFF || d3 == 0xFF)
    { 
    return 0; 
    }
  
  // Decode the 3 bytes

  *o0 = ((d0 << 2) & 0xFC) | ((d1 >> 4) & 0x03);
  *o1 = ((d1 << 4) & 0xF0) | ((d2 >> 2) & 0x0F);
  *o2 = ((d2 << 6) & 0xC0) | ((d3 >> 0) & 0x3F);
  
  // Return the number of bytes actually decoded

  if (i2 == '=') 
    { 
    return 1; 
    }
  if (i3 == '=') 
    { 
    return 2; 
    }
  return 3;
}

//----------------------------------------------------------------------------
unsigned long vtkBase64Utility::Decode(const unsigned char *input, 
                                       unsigned long length, 
                                       unsigned char *output)
{
  const unsigned char *ptr = input;
  unsigned char *optr = output;
  unsigned char *oend = output + length;

  // Decode complete triplet

  while ((oend - optr) >= 3)
    {
    int len = vtkBase64Utility::DecodeTriplet(ptr[0], ptr[1], ptr[2], ptr[3], 
                                              &optr[0], &optr[1], &optr[2]);
    optr += len;
    if(len < 3)
      {
      return optr - output;
      }
    ptr += 4;
    }

  // Decode the last triplet
  
  if (oend - optr == 2)
    {
    unsigned char temp;
    int len = vtkBase64Utility::DecodeTriplet(ptr[0], ptr[1], ptr[2], ptr[3], 
                                              &optr[0], &optr[1], &temp);
    if (len > 2) 
      { 
      optr += 2; 
      }
    else 
      { 
      optr += len; 
      }
    }
  else if (oend - optr == 1)
    {
    unsigned char temp1, temp2;
    int len = vtkBase64Utility::DecodeTriplet(ptr[0], ptr[1], ptr[2], ptr[3], 
                                              &optr[0], &temp1, &temp2);
    if (len > 1) 
      { 
      optr += 1; 
      }
    else 
      { 
      optr += len; 
      }
    }

  return optr - output;
}
