/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDSRdr.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkDataSetReader - class to read any type of vtk dataset
// .SECTION Description
// vtkDataSetReader is a class that provides instance variables 
// and methods to read any type of dataset in visualization library format. 
// The output type of this class will vary depending upon the type of data
// file. Note: these formats are not standard. Use other more standard 
// formats when you can.

#ifndef __vtkDataSetReader_h
#define __vtkDataSetReader_h

#include "DSSrc.hh"
#include "vtkDataR.hh"

class vtkDataSetReader : public vtkDataSetSource
{
public:
  vtkDataSetReader();
  ~vtkDataSetReader();
  char *GetClassName() {return "vtkDataSetReader";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetFilename(char *name);
  char *GetFilename();

  int GetFileType();

  void SetScalarsName(char *name);
  char *GetScalarsName();

  void SetVectorsName(char *name);
  char *GetVectorsName();

  void SetTensorsName(char *name);
  char *GetTensorsName();

  void SetNormalsName(char *name);
  char *GetNormalsName();

  void SetTCoordsName(char *name);
  char *GetTCoordsName();

  void SetLookupTableName(char *name);
  char *GetLookupTableName();

protected:
  void Execute();
  vtkDataReader Reader;
};

#endif


