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
// It should be able to read most any NetCDF file that wants to output rectilinear grid
//

#ifndef __vtkNetCDFPOPReader_h
#define __vtkNetCDFPOPReader_h

#include "vtkRectilinearGridAlgorithm.h"
#include "vtkSmartPointer.h" // For SmartPointer

class vtkDataArraySelection;
class vtkCallbackCommand;

class VTK_IO_EXPORT vtkNetCDFPOPReader : public vtkRectilinearGridAlgorithm 
{
public:
  vtkTypeMacro(vtkNetCDFPOPReader,vtkRectilinearGridAlgorithm);
  static vtkNetCDFPOPReader *New();
  void PrintSelf(ostream& os, vtkIndent indent);
  
  vtkSetStringMacro(Filename);
  vtkGetStringMacro(Filename);

  vtkSetVector6Macro(WholeExtent, int);
  vtkGetVector6Macro(WholeExtent, int);

  vtkSetVector6Macro(SubExtent, int);
  vtkGetVector6Macro(SubExtent, int);

  vtkSetVector3Macro(Origin, double);
  vtkGetVector3Macro(Origin, double);

  vtkSetVector3Macro(Spacing, double);
  vtkGetVector3Macro(Spacing, double);

  vtkSetVector3Macro(Stride, int);
  vtkGetVector3Macro(Stride, int);

  vtkSetMacro(BlockReadSize, int);
  vtkGetMacro(BlockReadSize, int);

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
  //virtual void ExecuteData(vtkDataObject *out);
  int RequestData(vtkInformation*,vtkInformationVector**,vtkInformationVector*);
    virtual int RequestInformation(vtkInformation* request,
                   vtkInformationVector** inputVector,
                   vtkInformationVector* outputVector);
//  virtual int RequestUpdateExtent(vtkInformation* request,
//                  vtkInformationVector** inputVector,
//                  vtkInformationVector* outputVector);

  static void SelectionModifiedCallback(vtkObject *caller, unsigned long eid,
                                        void *clientdata, void *calldata);

  static void EventCallback(vtkObject* caller, unsigned long eid,
                                void* clientdata, void* calldata);
  int ncFD; //file descriptor
  char VariableArrayInfo[100][100]; // name of variables that user wants
  char VariableName[100][100]; //name of all variables
  char *Filename; //file name
  int WholeExtent[6]; //extents of the rectilinear grid (not implemented)
  int SubExtent[6]; //extents of the rectilinear grid
  double Origin[3]; //default is 0,0,0
  double Spacing[3]; //default is 1,1,1
  int Stride[3]; // not implemented
  int BlockReadSize; //not implemented
  int nvarsp; //number of variables
  int nvarspw; //number of variables we list
  int draw[100]; //if 0, don't draw, if set to 1, draw out the rectilinear grid
  vtkCallbackCommand* SelectionObserver;

private:
  vtkNetCDFPOPReader(const vtkNetCDFPOPReader&);  // Not implemented.
  void operator=(const vtkNetCDFPOPReader&);  // Not implemented.
};
#endif

