/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataToImageStencil.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPolyDataToImageStencil - clip an image with polydata
// .SECTION Description
// vtkPolyDataToImageStencil will convert a vtkPolyData into an image
// that can be used with vtkImageStencil or other vtk classes that apply
// a stencil to an image.
// .SECTION see also
// vtkPolyData vtkImageStencil vtkImplicitFunctionToImageStencil

#ifndef __vtkPolyDataToImageStencil_h
#define __vtkPolyDataToImageStencil_h


#include "vtkImageStencilSource.h"

class vtkPolyData;
class vtkOBBTree;

class VTK_HYBRID_EXPORT vtkPolyDataToImageStencil : public vtkImageStencilSource
{
public:
  static vtkPolyDataToImageStencil *New();
  vtkTypeRevisionMacro(vtkPolyDataToImageStencil, vtkImageStencilSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the polydata to convert into a stencil.
  virtual void SetInput(vtkPolyData *input);
  virtual vtkPolyData *GetInput();

  // Description:
  // Set the tolerance for doing spatial searches of the polydata.
  vtkSetMacro(Tolerance, double);
  vtkGetMacro(Tolerance, double);

protected:
  vtkPolyDataToImageStencil();
  ~vtkPolyDataToImageStencil();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  double Tolerance;
  vtkOBBTree *OBBTree;

  virtual void ReportReferences(vtkGarbageCollector*);
  virtual int FillInputPortInformation(int, vtkInformation*);
private:
  vtkPolyDataToImageStencil(const vtkPolyDataToImageStencil&);  // Not implemented.
  void operator=(const vtkPolyDataToImageStencil&);  // Not implemented.
};

#endif
