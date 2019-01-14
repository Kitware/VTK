/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSegYReaderInternal.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkSegYReaderInternal_h
#define vtkSegYReaderInternal_h
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

class vtkSegYReaderInternal
{
public:
  vtkSegYReaderInternal();
  vtkSegYReaderInternal(const vtkSegYReaderInternal& other) = delete;
  vtkSegYReaderInternal& operator=(const vtkSegYReaderInternal& other) = delete;
  ~vtkSegYReaderInternal();

public:
  bool Is3DComputeParameters(int* extent, double origin[3], double spacing[3][3], int* spacingSign);
  void LoadTraces(int *extent);

  void ExportData(vtkImageData*, int* extent,
                  double origin[3], double spacing[3][3], int* spacingSign);
  void ExportData(vtkStructuredGrid*, int* extent,
    double origin[3], double spacing[3][3]);

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
#endif // vtkSegYReaderInternal_h
// VTK-HeaderTest-Exclude: vtkSegYReaderInternal.h
