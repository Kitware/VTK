// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkSegYTraceHeaderBytesPositions_h
#define vtkSegYTraceHeaderBytesPositions_h

VTK_ABI_NAMESPACE_BEGIN
class vtkSegYTraceHeaderBytesPositions
{
public:
  int TraceNumber;
  int InlineNumber;
  int CrosslineNumber;
  int TraceNumberWithinEnsemble;
  int CoordinateMultiplier;
  int CoordinateUnits;
  int NumberSamples;
  int SampleInterval;

  vtkSegYTraceHeaderBytesPositions() { initDefaultValues(); }

private:
  void initDefaultValues()
  {
    TraceNumber = 0;
    InlineNumber = 8;
    CrosslineNumber = 20;
    TraceNumberWithinEnsemble = 24;
    CoordinateMultiplier = 70;
    CoordinateUnits = 88;
    NumberSamples = 114;
    SampleInterval = 116;
  }
};

VTK_ABI_NAMESPACE_END
#endif // vtkSegYTraceHeaderBytesPositions_h
// VTK-HeaderTest-Exclude: vtkSegYTraceHeaderBytesPositions.h
