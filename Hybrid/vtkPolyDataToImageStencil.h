/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataToImageStencil.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPolyDataToImageStencil - clip an image with polydata
// .SECTION Description
// vtkPolyDataToImageStencil will convert a vtkPolyData into an image
// that can be used with vtkImageStecil or other vtk classes that apply
// a stencil to an image.
// .SECTION see also
// vtkPolyData vtkImageStencil vtkImplicitFunctionToImageStencil

#ifndef __vtkPolyDataToImageStencil_h
#define __vtkPolyDataToImageStencil_h


#include "vtkImageStencilSource.h"
#include "vtkPolyData.h"
#include "vtkOBBTree.h"

class VTK_HYBRID_EXPORT vtkPolyDataToImageStencil : public vtkImageStencilSource
{
public:
  static vtkPolyDataToImageStencil *New();
  vtkTypeRevisionMacro(vtkPolyDataToImageStencil, vtkImageStencilSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the polydata to convert into a stencil.
  void SetInput(vtkPolyData *input);
  vtkPolyData *GetInput();

  // Description:
  // Set the tolerance for doing spatial searches of the polydata.
  vtkSetMacro(Tolerance, float);
  vtkGetMacro(Tolerance, float);

protected:
  vtkPolyDataToImageStencil();
  ~vtkPolyDataToImageStencil();

  void ExecuteData(vtkDataObject *out);
  void ThreadedExecute(vtkImageStencilData *output,
                       int extent[6], int threadId);

  float Tolerance;
  vtkOBBTree *OBBTree;
private:
  vtkPolyDataToImageStencil(const vtkPolyDataToImageStencil&);  // Not implemented.
  void operator=(const vtkPolyDataToImageStencil&);  // Not implemented.
};

#endif
