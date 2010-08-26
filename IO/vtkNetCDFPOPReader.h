/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNetCDFPOPReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkNetCDFPOPReader - read NetCDF files
// .Author Joshua Wu 09.15.2009
// .SECTION Description
// vtkNetCDFPOPReader is a source object that reads NetCDF files.
// It should be able to read most any NetCDF file that wants to output
// rectilinear grid
//

#ifndef __vtkNetCDFPOPReader_h
#define __vtkNetCDFPOPReader_h

#include "vtkRectilinearGridAlgorithm.h"
#include "vtkSmartPointer.h" // For SmartPointer

class vtkDataArraySelection;
class vtkCallbackCommand;

//TODO: get rid of these completely arbitrary limits
//or at least protect against overflow
#define NCDFPOP_MAX_ARRAYS 100
#define NCDFPOP_MAX_NAMELEN 100

class VTK_IO_EXPORT vtkNetCDFPOPReader : public vtkRectilinearGridAlgorithm
{
public:
  vtkTypeMacro(vtkNetCDFPOPReader,vtkRectilinearGridAlgorithm);
  static vtkNetCDFPOPReader *New();
  void PrintSelf(ostream& os, vtkIndent indent);

  //Description:
  //The file to open
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  //Description:
  //Enable subsampling in i,j and k dimensions
  vtkSetVector3Macro(Stride, int);
  vtkGetVector3Macro(Stride, int);

  // Description:
  // Variable array selection.
  virtual int GetNumberOfVariableArrays();
  virtual const char *GetVariableArrayName(int idx);
  virtual int GetVariableArrayStatus(const char *name);
  virtual void SetVariableArrayStatus(const char *name, int status);

protected:
  vtkNetCDFPOPReader();
  ~vtkNetCDFPOPReader();
//BTX
  vtkSmartPointer<vtkDataArraySelection> VariableArraySelection;

//ETX
  int RequestData(vtkInformation*,vtkInformationVector**,
                  vtkInformationVector*);
  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);

  static void SelectionModifiedCallback(vtkObject *caller, unsigned long eid,
                                        void *clientdata, void *calldata);

  static void EventCallback(vtkObject* caller, unsigned long eid,
                                void* clientdata, void* calldata);

  //TODO: these hardcoded limits must be removed
  char VariableArrayInfo[NCDFPOP_MAX_ARRAYS][NCDFPOP_MAX_NAMELEN];
  // name of variables that user wants
  char VariableName[NCDFPOP_MAX_ARRAYS][NCDFPOP_MAX_NAMELEN];
  //name of all variables
  int Draw[NCDFPOP_MAX_ARRAYS];
  //if 0, don't draw, if set to 1, draw out the rectilinear grid
  vtkCallbackCommand* SelectionObserver;

  char *FileName;

  int NCDFFD; //netcdf file descriptor
  int NVarsp; //number of variables
  int NVarspw; //number of variables we list

  int Stride[3];

private:
  vtkNetCDFPOPReader(const vtkNetCDFPOPReader&);  // Not implemented.
  void operator=(const vtkNetCDFPOPReader&);  // Not implemented.
};
#endif
