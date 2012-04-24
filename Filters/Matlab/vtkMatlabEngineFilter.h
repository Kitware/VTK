/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatlabEngineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

// .NAME vtkMatlabEngineFilter
//
// .SECTION Description
//
// This VTK uses the vtkMatlabEngineInterface class to perform calculations on
// VTK array input using the Matlab Engine.
//
// .SECTION See Also
//  vtkMatlabMexAdapter vtkMatlabMexInterface
//
// .SECTION Thanks
//  Developed by Thomas Otahal at Sandia National Laboratories.
//

#ifndef __vtkMatlabEngineFilter_h
#define __vtkMatlabEngineFilter_h

#include "vtkDataObjectAlgorithm.h"

class vtkMatlabEngineInterface;
class vtkMatlabEngineFilterInternals;
class vtkDataSet;
class vtkDoubleArray;

class VTK_GRAPHICS_EXPORT vtkMatlabEngineFilter : public vtkDataObjectAlgorithm
{

public:

  static vtkMatlabEngineFilter *New();

  vtkTypeMacro(vtkMatlabEngineFilter, vtkDataObjectAlgorithm );
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Copies vtkDataArray named NameOfVTKArray to the Matlab engine with Matlab variable
  // name NameOfMatVar.  The array must exist in the input data set.
  //
  // Note: for vtkArray use "0","1","2",... for NameOfVTKArray to specify the index of
  // the vtkArray to pass to the Matlab Engine.
  void PutArray(const char* NameOfVTKArray, const char* NameOfMatVar);

  // Description:
  // Copies Matlab variable NameOfMatVar from the Matlab Engine to the vtkDataArray named
  // NameOfVTKArray.  Will replace existing vtkDataArray with the same name.
  //
  // Note: for vtkArray use any string for NameOfVTKArray.  The array will be appended
  // to the list of vtkArrays on the output.
  void GetArray(const char* NameOfVTKArray, const char* NameOfMatVar);

  // Description:
  // Clears the list of variables to be copied to the Matlab engine.
  void RemoveAllPutVariables();

  // Description:
  // Clears the list of variables to be copied from the Matlab engine.
  void RemoveAllGetVariables();

  // Description:
  // Matlab script executed by the Matlab Engine.  Can also be set from a file.
  vtkSetStringMacro(MatlabScript);
  vtkGetStringMacro(MatlabScript);

  // Description:
  // Provide Matlab script executed by the Matlab Engine from an input file.
  vtkSetStringMacro(ScriptFname);
  vtkGetStringMacro(ScriptFname);

  // Description:
  // Make Matlab Engine console visible.  Default is off.
  vtkSetMacro(EngineVisible,int);
  vtkGetMacro(EngineVisible,int);

  // Description:
  // Write Matlab Engine text output to standard output.
  vtkSetMacro(EngineOutput,int);
  vtkGetMacro(EngineOutput,int);

  // Description:
  // Pass VTK time information to Matlab.
  // If turned turned on, the filter will create three variables on the
  // Matlab engine.  The variables will be update automatically as time
  // changes in the VTK pipeline.
  // VTK_TIME_STEPS - array of all available time values.
  // VTK_TIME_RANGE- array of minimum and maximum time values.
  // VTK_CURRENT_TIME - floating point time value at the current time index.
  vtkSetMacro(TimeOutput,int);
  vtkGetMacro(TimeOutput,int);

  // Description:
  // Create VTK_BLOCK_ID and VTK_NUMBER_OF_BLOCKS variables in Matlab
  // when processing composite data sets.
  vtkSetMacro(BlockInfoOutput,int);
  vtkGetMacro(BlockInfoOutput,int);

  // Description:
  // This is required to capture REQUEST_DATA_OBJECT requests.
  virtual int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inputVector,
                             vtkInformationVector* outputVector);

protected:

  int SetMatlabScriptFromFile(const char* fname);

  virtual int RequestData(vtkInformation *vtkNotUsed(request),
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

  // Description:
  // Creates the same output type as the input type.
  virtual int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);

  vtkMatlabEngineFilter();
  ~vtkMatlabEngineFilter();

private:

  vtkMatlabEngineFilter(const vtkMatlabEngineFilter&);  // Not implemented.
  void operator=(const vtkMatlabEngineFilter&);  // Not implemented.

  // Implementation details
  vtkMatlabEngineFilterInternals* mefi;

  int ProcessDataSet(vtkDataSet* dsinp, vtkDataSet* dsout);

  vtkMatlabEngineInterface* mengi;
  char* MatlabScript;
  char* MatlabFileScript;
  char* ScriptFname;
  int EngineVisible;
  int EngineOutput;
  int TimeOutput;
  int BlockInfoOutput;
  char* OutputBuffer;
  vtkDoubleArray* CurrentTime;
  vtkDoubleArray* TimeRange;
  vtkDoubleArray* TimeSteps;
  vtkDoubleArray* BlockId;
  vtkDoubleArray* NumBlocks;

};

#endif

