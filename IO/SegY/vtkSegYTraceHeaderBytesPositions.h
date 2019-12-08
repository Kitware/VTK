/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSegYTraceHeaderBytesPositions.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkSegYTraceHeaderBytesPositions_h
#define vtkSegYTraceHeaderBytesPositions_h

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

public:
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

#endif // vtkSegYTraceHeaderBytesPositions_h
// VTK-HeaderTest-Exclude: vtkSegYTraceHeaderBytesPositions.h
