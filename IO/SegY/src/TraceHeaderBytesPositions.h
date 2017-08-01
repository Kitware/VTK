/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlane.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef SEGYVISUALIZER_TRACEHEADERBYTESPOSITIONS_H
#define SEGYVISUALIZER_TRACEHEADERBYTESPOSITIONS_H
class TraceHeaderBytesPositions
{
public:
    int TraceNumber;
    int InlineNumber;
    int CrosslineNumber;
    int TraceNumberWithinEnsemble;
    int CoordinateMultiplier;
    int XCoordinate;
    int YCoordinate;
    int CoordinateUnits;
    int NumberSamples;
    int SampleInterval;
public:
    TraceHeaderBytesPositions()
    {
        initDefaultValues();
    }
private:
    void initDefaultValues()
    {
        TraceNumber = 0;
        InlineNumber = 8;
        CrosslineNumber = 20;
        TraceNumberWithinEnsemble = 24;
        CoordinateMultiplier = 70;
        XCoordinate = 72;
        YCoordinate = 76;
        CoordinateUnits = 88;
        NumberSamples = 114;
        SampleInterval = 116;
    }
};
#endif //SEGYVISUALIZER_TRACEHEADERBYTESPOSITIONS_H
