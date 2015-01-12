
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRInterface.h

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

// .NAME vtkRInterface
//
// .SECTION Description
//
// This class defines a VTK interface to an embedded GNU R intepreter instance.  An
// instance of the R interpreter is created when this class is instantiated.  Additional
// instances of this class will share access the same R interpreter.  The R interpreter will
// be shutdown when the application using this class exits.
//
// .SECTION See Also
//  vtkRadapter vtkRcalculatorFilter
//
// .SECTION Thanks
//  Developed by Thomas Otahal at Sandia National Laboratories.
//


#ifndef vtkRInterface_h
#define vtkRInterface_h

#include "vtkFiltersStatisticsGnuRModule.h" // For export macro
#include "vtkObject.h"

class vtkDataArray;
class vtkArray;
class vtkTree;
class vtkTable;
class vtkImplementationRSingleton;
class vtkRAdapter;

class VTKFILTERSSTATISTICSGNUR_EXPORT vtkRInterface : public vtkObject
{
public:

  static vtkRInterface* New();
  vtkTypeMacro(vtkRInterface,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Evaluate an R command on the embedded R interpreter that takes one integer argument.
  int EvalRcommand(const char *commandName, int param);

  // Description:
  // Evaluate an R script given in string on the embedded R interpreter.  Set showRoutput
  // to turn on and off output from R.
  int EvalRscript(const char *string, bool showRoutput = true);

  // Description:
  // Provide a character buffer in p of length n.  All output from the R interpreter instance
  // will be written to p by default.
  int OutputBuffer(char* p, int n);

  // Description:
  // Copies vtkDataArray da into the R interpreter instance as a variable named RVariableName.
  // If RVariableName already exists, it will be overwritten.
  void AssignVTKDataArrayToRVariable(vtkDataArray* da, const char* RVariableName);

  // Description:
  // Copies vtkArray da into the R interpreter instance as a variable named RVariableName.
  // If RVariableName already exists, it will be overwritten.
  void AssignVTKArrayToRVariable(vtkArray* da, const char* RVariableName);

  // Description:
  // Copies vtkTree tr into the R interpreter instance as a variable named RVariableName.
  // If RVariableName already exists, it will be overwritten.
  void AssignVTKTreeToRVariable(vtkTree* tr, const char* RVariableName);

  // Description:
  // Copies the R variable RVariableName to the returned vtkDataArray.  If the operation fails,
  // the method will return NULL.
  vtkTree* AssignRVariableToVTKTree(const char* RVariableName);

  // Description:
  // Copies the R variable RVariableName to the returned vtkDataArray.  If the operation fails,
  // the method will return NULL.
  vtkDataArray* AssignRVariableToVTKDataArray(const char* RVariableName);

  // Description:
  // Copies the R variable RVariableName to the returned vtkArray.  If the operation fails,
  // the method will return NULL.  The returned vtkArray is currently always a vtkDenseArray
  // of type double.
  vtkArray* AssignRVariableToVTKArray(const char* RVariableName);

  // Description:
  // Copies the R matrix or R list in RVariableName to the returned vtkTable.  If the operation fails,
  // the method will return NULL.  If RVariableName is an R list, each list entry must be a vector of
  // the same length.
  vtkTable* AssignRVariableToVTKTable(const char* RVariableName);

  // Description:
  // Copies the vtkTable given in table to an R list structure name RVariableName.  The R list
  // will be length of the number of columns in table.  Each member of the list will contain a
  // column of table.
  void AssignVTKTableToRVariable(vtkTable* table, const char* RVariableName);

protected:
  vtkRInterface();
  ~vtkRInterface();

private:
  int FillOutputBuffer();
  vtkRInterface(const vtkRInterface&);    // Not implemented
  void operator=(const vtkRInterface&);   // Not implemented

  vtkImplementationRSingleton* rs;

  char* buffer;
  int buffer_size;
  vtkRAdapter* vra;

};

#endif

