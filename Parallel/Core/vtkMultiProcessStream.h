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
//
// .SECTION Caveats
// Note, stream operators cannot be combined with the Push/Pop array operators.
// For example, if you push an array to the stream,

#ifndef __vtkMultiProcessStream_h
#define __vtkMultiProcessStream_h

#include "vtkParallelCoreModule.h" // For export macro
#include "vtkObject.h"
#include <vector> // needed for vector.
#include <string> // needed for string.

class VTKPARALLELCORE_EXPORT vtkMultiProcessStream
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
  vtkMultiProcessStream& operator << (const std::string& value);
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
  vtkMultiProcessStream& operator >> (std::string &value);
  vtkMultiProcessStream& operator >> (vtkMultiProcessStream&);

  // Description:
  // Add-array-to-stream methods. Adds to the end of the stream
  void Push(double array[], unsigned int size);
  void Push(float array[], unsigned int size);
  void Push(int array[], unsigned int size);
  void Push(char array[], unsigned int size);
  void Push(unsigned int array[], unsigned int size );
  void Push(unsigned char array[], unsigned int size );
  void Push(vtkTypeInt64 array[], unsigned int size );
  void Push(vtkTypeUInt64 array[], unsigned int size );

  // Description:
  // Remove-array-to-stream methods. Removes from the head of the stream.
  // Note: If the input array is NULL, the array will be allocated internally
  // and the calling application is responsible for properly de-allocating it.
  // If the input array is not NULL, it is expected to match the size of the
  // data internally, and this method would just fill in the data.
  void Pop(double*& array, unsigned int& size);
  void Pop(float*& array, unsigned int& size);
  void Pop(int*& array, unsigned int& size);
  void Pop(char*& array, unsigned int& size);
  void Pop(unsigned int*& array, unsigned int& size );
  void Pop(unsigned char*& array, unsigned int& size );
  void Pop(vtkTypeInt64*& array, unsigned int& size );
  void Pop(vtkTypeUInt64*& array, unsigned int& size );


  // Description:
  // Clears everything in the stream.
  void Reset();

  // Description:
  // Returns the size of the stream.
  int Size();

  // Description:
  // Returns the size of the raw data returned by GetRawData. This
  // includes 1 byte to store the endian type.
  int RawSize()
    {return(this->Size()+1);};

  // Description:
  // Returns true iff the stream is empty.
  bool Empty();

  // Description:
  // Serialization methods used to save/restore the stream to/from raw data.
  // Note: The 1st byte of the raw data buffer consists of the endian type.
  void GetRawData(std::vector<unsigned char>& data) const;
  void GetRawData( unsigned char*& data, unsigned int &size );
  void SetRawData(const std::vector<unsigned char>& data);
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


// VTK-HeaderTest-Exclude: vtkMultiProcessStream.h
