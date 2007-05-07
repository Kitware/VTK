/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractArraysOverTime.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractArraysOverTime - extract point or cell data over time
// .SECTION Description
// vtkExtractArraysOverTime extracts point or cell data of one point or
// cell over time. The output is a 1D rectilinear grid where the 
// XCoordinates correspond to time (the same array is also copied to
// a point array named Time or TimeData (if Time exists in the input).
// When extracting point data, the input point coordinates are copied
// to a point array named Point Coordinates or Points (if Point Coordinates
// exists in the input).
// This algorithm does not produce a TIME_STEPS or TIME_RANGE information
// because it works across time. 
// .Section Caveat
// vtkExtractArraysOverTime puts a vtkOnePieceExtentTranslator in the
// output during RequestInformation(). As a result, the same whole 
// extented is produced independent of the piece request.
// This algorithm works only with source that produce TIME_STEPS().
// Continuous time range is not yet supported.

#ifndef __vtkExtractArraysOverTime_h
#define __vtkExtractArraysOverTime_h

#include "vtkRectilinearGridAlgorithm.h"

class vtkDataSet;
class vtkRectilinearGrid;

class VTK_GRAPHICS_EXPORT vtkExtractArraysOverTime : public vtkRectilinearGridAlgorithm
{
public:
  static vtkExtractArraysOverTime *New();
  vtkTypeRevisionMacro(vtkExtractArraysOverTime,vtkRectilinearGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the number of time steps
  vtkGetMacro(NumberOfTimeSteps,int);    

  // Description:
  // Convenience method to specify the selection connection (2nd input
  // port)
  void SetSelectionConnection(vtkAlgorithmOutput* algOutput)
  {
    this->SetInputConnection(1, algOutput);
  }

protected:
  vtkExtractArraysOverTime();
  ~vtkExtractArraysOverTime() {};

  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector, 
                                 vtkInformationVector* outputVector);
  virtual int RequestUpdateExtent(vtkInformation* request,
                                  vtkInformationVector** inputVector,
                                  vtkInformationVector* outputVector);

  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

  virtual void PostExecute(vtkInformation* request,
                           vtkInformationVector** inputVector,
                           vtkInformationVector* outputVector);

  int AllocateOutputData(vtkDataSet *input, vtkRectilinearGrid *output);

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  vtkIdType GetIndex(vtkIdType selIndex, vtkDataSet* input);

  int CurrentTimeIndex;
  int NumberOfTimeSteps;

  int FieldType;
  int ContentType;

  void ExecuteTimeStep(vtkInformationVector** inputV, 
                       vtkInformation* outInfo);

  int Error;

  //BTX
  enum Errors
  {
    NoError,
    MoreThan1Indices
  };
  //ETX

  //Returns a copy of "source" with all points marked as invalid removed.
  vtkRectilinearGrid *RemoveInvalidPoints(vtkRectilinearGrid *source);

private:
  vtkExtractArraysOverTime(const vtkExtractArraysOverTime&);  // Not implemented.
  void operator=(const vtkExtractArraysOverTime&);  // Not implemented.
};

#endif



