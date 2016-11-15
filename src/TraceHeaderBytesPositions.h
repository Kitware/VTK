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
    int NumberSamples;
    int CrosslineNumber;
    int XCoordinate;
    int YCoordinate;
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
        NumberSamples = 114;
        CrosslineNumber = 20;
        XCoordinate = 72;
        YCoordinate = 76;
    }
};
#endif //SEGYVISUALIZER_TRACEHEADERBYTESPOSITIONS_H
