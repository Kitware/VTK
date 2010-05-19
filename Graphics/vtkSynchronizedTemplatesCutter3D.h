/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSynchronizedTemplatesCutter3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSynchronizedTemplatesCutter3D - generate cut surface from structured points

// .SECTION Description
// vtkSynchronizedTemplatesCutter3D is an implementation of the synchronized 
// template algorithm. Note that vtkCutFilter will automatically
// use this class when appropriate.

// .SECTION See Also
// vtkContourFilter vtkSynchronizedTemplates3D

#ifndef __vtkSynchronizedTemplatesCutter3D_h
#define __vtkSynchronizedTemplatesCutter3D_h

#include "vtkSynchronizedTemplates3D.h"

class vtkImplicitFunction;

class VTK_GRAPHICS_EXPORT vtkSynchronizedTemplatesCutter3D : public vtkSynchronizedTemplates3D
{
public:
  static vtkSynchronizedTemplatesCutter3D *New();

  vtkTypeMacro(vtkSynchronizedTemplatesCutter3D,vtkSynchronizedTemplates3D);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Needed by templated functions.
  void ThreadedExecute(vtkImageData *data, vtkInformation *outInfo,
                       int *exExt, int);

  // Description
  // Specify the implicit function to perform the cutting.
  virtual void SetCutFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(CutFunction,vtkImplicitFunction);

protected:
  vtkSynchronizedTemplatesCutter3D();
  ~vtkSynchronizedTemplatesCutter3D();

  vtkImplicitFunction *CutFunction;

  virtual int RequestData(vtkInformation *, 
                          vtkInformationVector **, 
                          vtkInformationVector *);

private:
  vtkSynchronizedTemplatesCutter3D(const vtkSynchronizedTemplatesCutter3D&);  // Not implemented.
  void operator=(const vtkSynchronizedTemplatesCutter3D&);  // Not implemented.
};

#endif

