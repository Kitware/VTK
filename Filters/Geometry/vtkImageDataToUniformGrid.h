/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataToUniformGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageDataToUniformGrid - convert vtkImageData to vtkUniformGrid
// .SECTION Description
// Convert a vtkImageData to vtkUniformGrid and set blanking based on
// specified by named arrays.

#ifndef __vtkImageDataToUniformGrid_h
#define __vtkImageDataToUniformGrid_h

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkDataObjectAlgorithm.h"

class vtkDataArray;
class vtkFieldData;
class vtkImageData;
class vtkUniformGrid;

class VTKFILTERSGEOMETRY_EXPORT vtkImageDataToUniformGrid
: public vtkDataObjectAlgorithm
{
 public:
  static vtkImageDataToUniformGrid *New();
  vtkTypeMacro(vtkImageDataToUniformGrid,vtkDataObjectAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent);

protected:
  vtkImageDataToUniformGrid();
  ~vtkImageDataToUniformGrid();

  virtual int RequestData(vtkInformation *req,
                          vtkInformationVector **inV,
                          vtkInformationVector *outV);
  virtual int RequestDataObject(vtkInformation *req,
                                vtkInformationVector **inV,
                                vtkInformationVector *outV);

  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual int FillOutputPortInformation(int port, vtkInformation* info);

  /* virtual int Process(vtkImageData* input, vtkInformationVector** inV, */
  /*                     vtkUniformGrid* output); */
  virtual int Process(vtkImageData* input, int association, const char* arrayName,
                      vtkUniformGrid* output);

private:
  vtkImageDataToUniformGrid(const vtkImageDataToUniformGrid&);  // Not implemented.
  void operator=(const vtkImageDataToUniformGrid&);  // Not implemented.
};

#endif
