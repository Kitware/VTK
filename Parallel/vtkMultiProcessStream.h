/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiProcessStream.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiProcessStream - stream used to pass data across processes
// using vtkMultiProcessController.
// .SECTION Description
// vtkMultiProcessStream is used to pass data across processes. Using
// vtkMultiProcessStream it is possible to send data whose length is not known
// at the receiving end.

#ifndef __vtkMultiProcessStream_h
#define __vtkMultiProcessStream_h

#include "vtkObject.h"
#include <vtkstd/vector> // needed for vector.
#include <vtkstd/string> // needed for string.

class VTK_PARALLEL_EXPORT vtkMultiProcessStream
{
public:
  vtkMultiProcessStream();
  vtkMultiProcessStream(const vtkMultiProcessStream&);
  ~vtkMultiProcessStream();
  vtkMultiProcessStream& operator=(const vtkMultiProcessStream&);

  // Description:
  // Add-to-stream operators. Adds to the end of the stream.
  vtkMultiProcessStream& operator << (double value);
  vtkMultiProcessStream& operator << (float value);
  vtkMultiProcessStream& operator << (int value);
  vtkMultiProcessStream& operator << (char value);
  vtkMultiProcessStream& operator << (unsigned int value);
  vtkMultiProcessStream& operator << (unsigned char value);
  vtkMultiProcessStream& operator << (vtkTypeInt64 value);
  vtkMultiProcessStream& operator << (vtkTypeUInt64 value);
  vtkMultiProcessStream& operator << (const vtkstd::string& value);
  vtkMultiProcessStream& operator << (const vtkMultiProcessStream&);

  // Description:
  // Remove-from-stream operators. Removes from the head of the stream.
  vtkMultiProcessStream& operator >> (double &value);
  vtkMultiProcessStream& operator >> (float &value);
  vtkMultiProcessStream& operator >> (int &value);
  vtkMultiProcessStream& operator >> (char &value);
  vtkMultiProcessStream& operator >> (unsigned int &value);
  vtkMultiProcessStream& operator >> (unsigned char &value);
  vtkMultiProcessStream& operator >> (vtkTypeInt64 &value);
  vtkMultiProcessStream& operator >> (vtkTypeUInt64 &value);
  vtkMultiProcessStream& operator >> (vtkstd::string &value);
  vtkMultiProcessStream& operator >> (vtkMultiProcessStream&);

  // Description:
  // Clears everything in the stream.
  void Reset();

  // Description:
  // Serialization methods used to save/restore the stream to/from raw data.
  void GetRawData(vtkstd::vector<unsigned char>& data) const;
  void SetRawData(const vtkstd::vector<unsigned char>& data);
  void SetRawData(const unsigned char*, unsigned int size);

private:
  class vtkInternals;
  vtkInternals* Internals;
  unsigned char Endianness;
  enum 
    {
    BigEndian,
    LittleEndian
    };
};

#endif


