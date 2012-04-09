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
// .NAME vtkExtractArraysOverTime - extracts a selection over time.
// .SECTION Description
// vtkExtractArraysOverTime extracts a selection over time.
// The output is a multiblock dataset. If selection content type is  
// vtkSelection::Locations, then each output block corresponds to each probed
// location. Otherwise, each output block corresponds to an extracted cell/point
// depending on whether the selection field type is CELL or POINT.
// Each block is a vtkTable with a column named Time (or TimeData if Time exists
// in the input).
// When extracting point data, the input point coordinates are copied
// to a column named Point Coordinates or Points (if Point Coordinates
// exists in the input).
// This algorithm does not produce a TIME_STEPS or TIME_RANGE information
// because it works across time. 
// .Section Caveat
// This algorithm works only with source that produce TIME_STEPS().
// Continuous time range is not yet supported.

#ifndef __vtkExtractArraysOverTime_h
#define __vtkExtractArraysOverTime_h

#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkSelection;
class vtkDataSet;
class vtkTable;
class vtkExtractArraysOverTimeInternal;
class vtkDataSetAttributes;

class VTK_GRAPHICS_EXPORT vtkExtractArraysOverTime : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkExtractArraysOverTime *New();
  vtkTypeMacro(vtkExtractArraysOverTime, vtkMultiBlockDataSetAlgorithm);
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

//BTX
protected:
  vtkExtractArraysOverTime();
  ~vtkExtractArraysOverTime();

  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector, 
                                 vtkInformationVector* outputVector);
  virtual int RequestUpdateExtent(vtkInformation* request,
                                  vtkInformationVector** inputVector,
                                  vtkInformationVector* outputVector);
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  virtual void PostExecute(vtkInformation* request,
                           vtkInformationVector** inputVector,
                           vtkInformationVector* outputVector);

  // Description:
  // Determines the FieldType and ContentType for the selection. If the
  // selection is a vtkSelection::SELECTIONS selection, then this method ensures
  // that all child nodes have the same field type and content type otherwise,
  // it returns 0.
  int DetermineSelectionType(vtkSelection*);

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // This method doesn't care about the content type of the selection, 
  // just grabs the value. 
  bool UpdateFastPathIDs(
    vtkInformationVector** inputV, vtkInformation* outInfo);

  // Description:
  // This looks at the arrays in the vtkFieldData of input and copies those
  // whose names are in the form "XXXOverTime" to the output point data.
  void CopyFastPathDataToOutput(vtkDataSet *input, vtkTable *output);


  void ExecuteAtTimeStep(vtkInformationVector** inputV, 
    vtkInformation* outInfo);

  int CurrentTimeIndex;
  int NumberOfTimeSteps;

  int FieldType;
  int ContentType;

  bool WaitingForFastPathData;
  bool IsExecuting;
  bool UseFastPath;

  int Error;

  enum Errors
  {
    NoError,
    MoreThan1Indices
  };

private:
  vtkExtractArraysOverTime(const vtkExtractArraysOverTime&);  // Not implemented.
  void operator=(const vtkExtractArraysOverTime&);  // Not implemented.

  class vtkInternal;
  vtkInternal *Internal;

//ETX
};

#endif



