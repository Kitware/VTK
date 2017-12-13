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

#ifndef vtkSegYReader_h
#define vtkSegYReader_h
#ifndef __VTK_WRAP__

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
  void SetXYCoordBytePositions(int x, int y);
  void SetVerticalCRS(int);

private:
  bool ReadHeader(std::ifstream& in);
  std::vector<vtkSegYTrace*> Traces;
  int FormatCode;
  vtkSegYBinaryHeaderBytesPositions* BinaryHeaderBytesPos;
  vtkSegYTraceReader* TraceReader;
  int SampleCountPerTrace;
  int VerticalCRS;
};

#endif
#endif // vtkSegYReader_h
// VTK-HeaderTest-Exclude: vtkSegYReader.h
