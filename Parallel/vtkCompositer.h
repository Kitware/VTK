/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositer.h
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
// .NAME vtkCompositer - Super class for composite algorthms.
// .SECTION Description
// vtkCompositer operates in multiple processes.  Each compositer has 
// a render window.  They use vtkMultiProcessControllers to comunicate 
// the color and depth buffer to process 0's render window.
// It will not handle transparency well.
// vtkCompositeManager.

#ifndef __vtkCompositer_h
#define __vtkCompositer_h

#include "vtkObject.h"

class vtkMultiProcessController;
class vtkCompositer;
class vtkDataArray;
class vtkFloatArray;

class VTK_PARALLEL_EXPORT vtkCompositer : public vtkObject
{
public:
  static vtkCompositer *New();
  vtkTypeRevisionMacro(vtkCompositer,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method gets called on every process.  The final image gets
  // put into pBuf and zBuf.
  virtual void CompositeBuffer(vtkDataArray *pBuf, vtkFloatArray *zBuf,
                               vtkDataArray *pTmp, vtkFloatArray *zTmp);

  // Description:
  // Access to the controller.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller,vtkMultiProcessController);

protected:
  vtkCompositer();
  ~vtkCompositer();
  vtkCompositer(const vtkCompositer&);
  void operator=(const vtkCompositer&);
  
  vtkMultiProcessController *Controller;

};

#endif
