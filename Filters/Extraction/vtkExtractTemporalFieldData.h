/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractTemporalFieldData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractTemporalFieldData - Extract temporal arrays from input field data
// .SECTION Description
// vtkExtractTemporalFieldData extracts arrays from the input vtkFieldData. 
// These arrays are assumed to contain temporal data, where the nth tuple 
// contains the value for the nth timestep. 
// The output is a 1D rectilinear grid where the 
// XCoordinates correspond to time (the same array is also copied to
// a point array named Time or TimeData (if Time exists in the input).
// This algorithm does not produce a TIME_STEPS or TIME_RANGE information
// because it works across time. 
// .Section Caveat
// vtkExtractTemporalFieldData puts a vtkOnePieceExtentTranslator in the
// output during RequestInformation(). As a result, the same whole 
// extented is produced independent of the piece request.
// This algorithm works only with source that produce TIME_STEPS().
// Continuous time range is not yet supported.

#ifndef __vtkExtractTemporalFieldData_h
#define __vtkExtractTemporalFieldData_h

#include "vtkTableAlgorithm.h"

class vtkDataSet;
class vtkTable;
class vtkDataSetAttributes;

class VTK_GRAPHICS_EXPORT vtkExtractTemporalFieldData : public vtkTableAlgorithm
{
public:
  static vtkExtractTemporalFieldData *New();
  vtkTypeMacro(vtkExtractTemporalFieldData,vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the number of time steps
  vtkGetMacro(NumberOfTimeSteps,int);    

protected:
  vtkExtractTemporalFieldData();
  ~vtkExtractTemporalFieldData();

  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector, 
                                 vtkInformationVector* outputVector);
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);


  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // This looks at the arrays in the vtkFieldData of input and copies them 
  // to the output point data.
  void CopyDataToOutput(vtkInformation* inInfo,
    vtkDataSet *input, vtkTable *output);

  int NumberOfTimeSteps;

private:
  vtkExtractTemporalFieldData(const vtkExtractTemporalFieldData&);  // Not implemented.
  void operator=(const vtkExtractTemporalFieldData&);  // Not implemented.
};

#endif



