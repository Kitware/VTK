/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkX3DExporterFIWriterHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkX3DExporterFIWriterHelper -
// .SECTION Description

#ifndef __vtkX3DExporterFIWriterHelper_h
#define __vtkX3DExporterFIWriterHelper_h

//#include "vtkX3DExporterFIByteWriter.h"
#include "vtkZLibDataCompressor.h"
#include <cassert>

#define EXPONENT_MASK_32 0x7f800000
#define MANTISSA_MASK_32 0x007fffff

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
class vtkX3DExporterFIWriterHelper
{
public:
  union float_to_unsigned_int_to_bytes
      {
        float f;
        unsigned int ui;
        unsigned char ub[4]; // unsigned bytes
      };

  template<typename T>
    static inline void EncodeFloatFI(vtkX3DExporterFIByteWriter* writer, T* value, size_t size)
      {
      // We want to start at position 3
      assert(writer->CurrentBytePos == 2);

      // ITU C.19.3.4: If the alternative encoding-algorithm is present,
      // then the two bits '11' (discriminant) are appended
      writer->PutBits("11");
      // ITU 10.8.1: This encoding algorithm has a vocabulary table index of 7,
      writer->PutBits(7-1, 8);

      std::string bytes;
      char byte[4];
      for (size_t i = 0; i < size; i++)
        {
        float_to_unsigned_int_to_bytes v;
        v.f = value[i];

        // Avoid -0
        if (v.ui == 0x80000000)
          {
          v.f = 0;
          }

        byte[0] = v.ub[3];
        byte[1] = v.ub[2];
        byte[2] = v.ub[1];
        byte[3] = v.ub[0];

        bytes.append(byte, 4);
        }
      EncodeNonEmptyByteString5(writer, bytes);
      }

  template<typename T>
    static inline void EncodeIntegerFI(vtkX3DExporterFIByteWriter* writer, T* value, size_t size)
      {
      // We want to start at position 3
      assert(writer->CurrentBytePos == 2);

      // ITU C.19.3.4: If the alternative encoding-algorithm is present,
      // then the two bits '11' (discriminant) are appended
      writer->PutBits("11");
      // ITU 10.8.1: This encoding algorithm has a vocabulary table index of 4,
      writer->PutBits(4-1, 8);
      std::string bytes;
      for(size_t i = 0; i < size; i++)
        {
        int v = value[i];
        int f = ReverseBytes(&v);
        char *p = reinterpret_cast <char*> (&f);
        bytes.append(p, 4);
        }
      EncodeNonEmptyByteString5(writer, bytes);
      }

  static inline void EncodeCharacterString3(vtkX3DExporterFIByteWriter* writer, std::string value)
    {
    // We want to start at position 3
    assert(writer->CurrentBytePos == 2);

    // ITU C.19.3.1 If the alternative utf-8 is present, then the two bits '00'
    // are appended to the bit stream.
    writer->PutBits("00");
    // ITU C.19.4: The component bytes is encoded as described in C.23.
    EncodeNonEmptyByteString5(writer, value);
    }

  // ITU C.23: Encoding of the NonEmptyByteString starting
  // on the fifth bit of an byte
  static inline void EncodeNonEmptyByteString5(vtkX3DExporterFIByteWriter* writer, std::string value)
    {
    int length = static_cast<int>(value.length());
    if (length <= 8)
      {
      writer->PutBit(0);
      writer->PutBits(length - 1, 3);
      }
    else if (length <= 264)
      {
      writer->PutBits("1000");
      writer->PutBits(length - 9, 8);
      }
    else
      {
      writer->PutBits("1100");
      writer->PutBits(length - 265, 32);
      }
    writer->PutBytes(value.c_str(), length);
    }


  // ITU C.27: Encoding of integers in the range 1 to 2^20
  // starting on the third bit of an byte
  static inline void EncodeInteger3(vtkX3DExporterFIByteWriter* writer, unsigned int value)
    {
    // We want to start at position 3
    assert(writer->CurrentBytePos == 2);

    if (value <= 32) // ITU  C.27.2
      {
      writer->PutBit(0);
      writer->PutBits(value - 1, 5);
      }
    else if (value <= 2080) // ITU C.27.3
      {
      writer->PutBits("100");
      writer->PutBits(value - 33, 11);
      }
    else if (value < 526368) // ITU C.27.4
      {
      writer->PutBits("101");
      writer->PutBits(value - 2081, 19);
      }
    else // ITU C.27.5
      {
      writer->PutBits("1100000000");
      writer->PutBits(value - 526369, 20);
      }
    }

  // ITU C.25: Encoding of integers in the range 1 to 2^20
  // starting on the second bit of an byte
  static inline void EncodeInteger2(vtkX3DExporterFIByteWriter* writer, unsigned int value)
    {
    // We want to start at position 2
    assert(writer->CurrentBytePos == 1);

    if (value <= 64) // ITU  C.25.2
      {
      writer->PutBits("0");
      writer->PutBits(value - 1, 6);
      }
    else if (value <= 8256) // ITU C.25.3
      {
      writer->PutBits("10");
      writer->PutBits(value - 65, 13);
      }
    else // ITU C.25.4
      {
      writer->PutBits("110");
      writer->PutBits(value - 8257, 20);
      }
    }

  static inline void EncodeLineFeed(vtkX3DExporterFIByteWriter* writer)
    {
    static bool firstTime = true;
    writer->FillByte();
    if (firstTime)
      {
      writer->PutBits("1001000000001010");
      firstTime = false;
      }
    else
      {
      //cout << "Encode NOT the first time" << endl;
      writer->PutBits("10100000");
      }
    }

private:

  static int ReverseBytes(int* x) {
    /* break x apart, then put it back together backwards */
    int part1 = (*x)  & 0xFF;
    int part2 = ((*x) >> 8) & 0xFF;
    int part3 = ((*x) >> 16) & 0xFF;
    int part4 = ((*x) >> 24) & 0xFF;
    return (part1 << 24) | ( part2 << 16) | (part3 << 8) | part4;
  }


  friend class X3DEncoderFunctions;
};

class X3DEncoderFunctions {



public:
  template<typename T>
    static inline void EncodeIntegerDeltaZ(vtkX3DExporterFIByteWriter* writer, T* value, size_t size, vtkZLibDataCompressor* compressor,  bool image = false)
      {
      // We want to start at position 3
      assert(writer->CurrentBytePos == 2);

      // ITU C.19.3.4: If the alternative encoding-algorithm is present,
      // then the two bits '11' (discriminant) are appended
      writer->PutBits("11");
      // ITU 10.8.1: This encoding algorithm has a vocabulary table index of 33
      writer->PutBits(34-1, 8);

      // compute delta
      char span = 0;
      size_t i = 0;
      int f; unsigned char *p;
      std::vector<unsigned char> deltas;

      if (image)
        {
        span = 0;
        for(i = 0; i < size; i++)
          {
          int v = 1 + (value[i]);
          int *vp = reinterpret_cast<int*>(&v);
          f = vtkX3DExporterFIWriterHelper::ReverseBytes(vp);
          p = reinterpret_cast <unsigned char*> (&f);
          deltas.push_back(p[0]);
          deltas.push_back(p[1]);
          deltas.push_back(p[2]);
          deltas.push_back(p[3]);
          }
        compressor->SetCompressionLevel(9);
        }
      else
        {
        for (i = 0; i < 20; i++)
          {
          if (value[i] == -1)
            {
            span = static_cast<char>(i) + 1;
            break;
            }
          }
        if (!span) span = 4;

        for(i = 0; i < static_cast<size_t>(span); i++)
          {
          int v = 1 + value[i];
          int *vp = reinterpret_cast<int*>(&v);
          f = vtkX3DExporterFIWriterHelper::ReverseBytes(vp);

          p = reinterpret_cast <unsigned char*> (&f);
          deltas.push_back(p[0]);
          deltas.push_back(p[1]);
          deltas.push_back(p[2]);
          deltas.push_back(p[3]);
          }
        for(i = span; i < size; i++)
          {
          int v = 1 + (value[i] - value[i-span]);
          f = vtkX3DExporterFIWriterHelper::ReverseBytes(&v);

          p = reinterpret_cast <unsigned char*> (&f);
          deltas.push_back(p[0]);
          deltas.push_back(p[1]);
          deltas.push_back(p[2]);
          deltas.push_back(p[3]);
          }
        }

      size_t bufferSize = deltas.size() + static_cast<unsigned int>(ceil(deltas.size()*0.001)) + 12;
      unsigned char* buffer = new unsigned char[bufferSize];
      size_t newSize = compressor->Compress(&deltas[0],static_cast<unsigned long>(deltas.size()), buffer, static_cast<unsigned long>(bufferSize));

      std::string bytes;
      int size32 = static_cast<int>(size);
      int size32_reversed = vtkX3DExporterFIWriterHelper::ReverseBytes(&size32);
      char *s = reinterpret_cast <char*> (&size32_reversed);
      bytes.append(s, 4);
      bytes.append(&span, 1);

      for (i = 0; i < newSize; i++)
        {
        unsigned char c = buffer[i];
        bytes += c;
        }
      delete [] buffer;

      vtkX3DExporterFIWriterHelper::EncodeNonEmptyByteString5(writer, bytes);
      if (image)
        {
        compressor->SetCompressionLevel(5);
        }
      }

  static inline void EncodeQuantizedzlibFloatArray(vtkX3DExporterFIByteWriter* writer, const double* value, size_t size, vtkZLibDataCompressor* compressor)
    {
    // We want to start at position 3
    assert(writer->CurrentBytePos == 2);

    // ITU C.19.3.4: If the alternative encoding-algorithm is present,
    // then the two bits '11' (discriminant) are appended
    writer->PutBits("11");
    // ITU 10.8.1: This encoding algorithm has a vocabulary table index of 33
    writer->PutBits(34, 8);

    unsigned char* bytes = new unsigned char[size*4];
    unsigned char* bytepos = bytes;
    std::string bytesCompressed;
    size_t i;

    const double* vd = value;
    for (i = 0; i < size; i++)
      {
      union float_to_unsigned_int_to_bytes
      {
        float f;
        unsigned int ui;
        unsigned char ub[4]; // unsigned bytes
      };
      float_to_unsigned_int_to_bytes v;
      v.f = (*vd) * 2.0;

      // Avoid -0
      if (v.ui == 0x80000000)
        {
        v.f = 0.0f;
        }
      //vtkGenericWarningMacro(<< "value: " << v << " bytes: " << (int)s[0] << " " << (int)s[1] << " " << (int)s[2] << " " << (int)s[3]);
      *bytepos++ = v.ub[3];
      *bytepos++ = v.ub[2];
      *bytepos++ = v.ub[1];
      *bytepos++ = v.ub[0];
      vd++;
      }


    // Compress the data
    size_t bufferSize = (size * 4) + static_cast<size_t>(ceil((size * 4)*0.001)) + 12;
    unsigned char* buffer = new unsigned char[bufferSize];
    size_t newSize = compressor->Compress(bytes,
      static_cast<unsigned long>(size * 4), buffer,
      static_cast<unsigned long>(bufferSize));

    char *s;
    // Put the number of bits for exponent
    bytesCompressed += static_cast<char>(8);
    // Put the number of bits for mantissa
    bytesCompressed += static_cast<char>(23);
    // Put the length
    int length = static_cast<int>(size*4);
    int length_reversed = vtkX3DExporterFIWriterHelper::ReverseBytes(&length);
    s = reinterpret_cast <char*> (&length_reversed);
    bytesCompressed.append(s, 4);

    // Put the number of floats
    int numFloats = static_cast<int>(size);
    int numFloats_reversed = vtkX3DExporterFIWriterHelper::ReverseBytes(&numFloats);;
    s = reinterpret_cast <char*> (&numFloats_reversed);
    bytesCompressed.append(s, 4);

    for (i = 0; i < newSize; i++)
      {
      unsigned char c = buffer[i];
      bytesCompressed += c;
      }
    vtkX3DExporterFIWriterHelper::EncodeNonEmptyByteString5(writer, bytesCompressed);
    delete [] buffer;
    delete [] bytes;
    }

};

#endif
// VTK-HeaderTest-Exclude: vtkX3DExporterFIWriterHelper.h
