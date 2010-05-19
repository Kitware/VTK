/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaScalarsToColorsPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkMesaScalarsToColorsPainter_h
#define __vtkMesaScalarsToColorsPainter_h

#include "vtkScalarsToColorsPainter.h"

class vtkOpenGLTexture;

class VTK_RENDERING_EXPORT vtkMesaScalarsToColorsPainter : 
  public vtkScalarsToColorsPainter
{
public:
  static vtkMesaScalarsToColorsPainter* New();
  vtkTypeMacro(vtkMesaScalarsToColorsPainter,
               vtkScalarsToColorsPainter);
  void PrintSelf(ostream& os, vtkIndent indent);
  

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release. 
  virtual void ReleaseGraphicsResources(vtkWindow *);
protected:
  vtkMesaScalarsToColorsPainter();
  ~vtkMesaScalarsToColorsPainter();
 
  vtkOpenGLTexture* InternalColorTexture;

  // Description:
  // Generates rendering primitives of appropriate type(s). Multiple types 
  // of preimitives can be requested by or-ring the primitive flags. 
  // Subclasses may override this method. Default implementation propagates
  // the call to Deletegate Painter, in any.
  virtual void RenderInternal(vtkRenderer* renderer, vtkActor* actor, 
    unsigned long typeflags);

private:
  vtkMesaScalarsToColorsPainter(const vtkMesaScalarsToColorsPainter&); // Not implemented.
  void operator=(const vtkMesaScalarsToColorsPainter&); // Not implemented.
};

#endif

