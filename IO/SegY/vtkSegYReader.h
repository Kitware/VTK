/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSegYReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkSegYReader_h
#define __vtkSegYReader_h

#include <fstream>
#include <string>
#include <vector>

// Forward declarations
class vtkStructuredGrid;
class vtkImageData;
class vtkSegYTraceReader;
class vtkSegYTrace;
class vtkSegYBinaryHeaderBytesPositions;

class vtkSegYReader
{
public:
  vtkSegYReader();
  ~vtkSegYReader();

public:
  bool GetImageData(vtkImageData*); // export the data in segy file as 3D volume
  bool ExportData3D(vtkImageData*); // export the data in segy file as 3D volume
  bool LoadFromFile(std::string path);
  void ExportData2D(vtkStructuredGrid*);
  void AddScalars(vtkStructuredGrid*);

private:
  bool ReadHeader(std::ifstream& in);
  std::vector<vtkSegYTrace*> Traces;
  int FormatCode;
  vtkSegYBinaryHeaderBytesPositions* BinaryHeaderBytesPos;
  vtkSegYTraceReader* TraceReader;
  int SampleCountPerTrace;
};

#endif // __vtkSegYReader_h
