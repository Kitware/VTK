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
  bool LoadFromFile(std::string path);

  bool Is3DComputeParameters(int* extent, double* origin, double* spacing);
  void LoadTraces();

  void ExportData3D(vtkImageData*, int* extent, double* origin, double* spacing);
  void ExportData2D(vtkStructuredGrid*);

  void AddScalars(vtkStructuredGrid*);
  void SetXYCoordBytePositions(int x, int y);
  void SetVerticalCRS(int);

  std::ifstream In;

protected:
  bool ReadHeader();

private:
  std::vector<vtkSegYTrace*> Traces;
  vtkSegYBinaryHeaderBytesPositions* BinaryHeaderBytesPos;
  vtkSegYTraceReader* TraceReader;
  int VerticalCRS;
  // Binary Header
  short SampleInterval;
  int FormatCode;
  int SampleCountPerTrace;
};

#endif
#endif // vtkSegYReader_h
// VTK-HeaderTest-Exclude: vtkSegYReader.h
