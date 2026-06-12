// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkSegYReaderInternal_h
#define vtkSegYReaderInternal_h

#include "vtkABINamespace.h"
#include "vtkSetGet.h"

#include <fstream>
#include <string>
#include <vector>
#include <vtksys/FStream.hxx>

// Forward declarations
VTK_ABI_NAMESPACE_BEGIN
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

  bool Is3DComputeParameters(
    int extent[6], double origin[3], double spacing[3][3], int* spacingSign, bool force2D);
  void LoadTraces(VTK_FUTURE_CONST int extent[6]);

  void ExportData(vtkImageData*, VTK_FUTURE_CONST int extent[6], double origin[3],
    double spacing[3][3], int* spacingSign);
  void ExportData(
    vtkStructuredGrid*, VTK_FUTURE_CONST int extent[6], double origin[3], double spacing[3][3]);

  void SetXYCoordBytePositions(int x, int y);
  void SetVerticalCRS(int);

  vtksys::ifstream In;

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

VTK_ABI_NAMESPACE_END
#endif // vtkSegYReaderInternal_h
// VTK-HeaderTest-Exclude: vtkSegYReaderInternal.h
