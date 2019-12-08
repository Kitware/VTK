/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSegYBinaryHeaderBytesPositions.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkSegYBinaryHeaderBytesPositions_h
#define vtkSegYBinaryHeaderBytesPositions_h

class vtkSegYBinaryHeaderBytesPositions
{
public:
  int JobID;
  int LineNumber;
  int ReelNumber;
  int NumberTracesPerEnsemble;
  int NumberAuxTracesPerEnsemble;
  int SampleInterval;
  int SampleIntervalOriginal;
  int NumSamplesPerTrace;
  int NumSamplesPerTraceOriginal;
  int FormatCode;
  int EnsembleType;
  int MeasurementSystem;
  int ByteOrderingDetection;
  int MajorVersion;
  int MinorVersion;
  int FixedLengthFlag;

public:
  vtkSegYBinaryHeaderBytesPositions() { initDefaultValues(); }

private:
  void initDefaultValues()
  {
    // Default data field positions
    JobID = 3200;
    LineNumber = 3204;
    ReelNumber = 3208;
    NumberTracesPerEnsemble = 3212;
    NumberAuxTracesPerEnsemble = 3214;
    SampleInterval = 3216;
    SampleIntervalOriginal = 3218;
    NumSamplesPerTrace = 3220;
    NumSamplesPerTraceOriginal = 3222;
    FormatCode = 3224;
    EnsembleType = 3228;
    MeasurementSystem = 3254;
    ByteOrderingDetection = 3296;
    MajorVersion = 3500;
    MinorVersion = 3501;
    FixedLengthFlag = 3502;
  }
};

#endif // vtkSegYBinaryHeaderBytesPositions_h
// VTK-HeaderTest-Exclude: vtkSegYBinaryHeaderBytesPositions.h
