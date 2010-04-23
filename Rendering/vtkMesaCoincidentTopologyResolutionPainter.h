/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaCoincidentTopologyResolutionPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMesaCoincidentTopologyResolutionPainter
// .SECTION Description
// Implementation for vtkCoincidentTopologyResolutionPainter using Mesa.

#ifndef __vtkMesaCoincidentTopologyResolutionPainter_h
#define __vtkMesaCoincidentTopologyResolutionPainter_h

#include "vtkCoincidentTopologyResolutionPainter.h"

class VTK_RENDERING_EXPORT vtkMesaCoincidentTopologyResolutionPainter :
  public vtkCoincidentTopologyResolutionPainter
{
public:
  static vtkMesaCoincidentTopologyResolutionPainter* New();
  vtkTypeMacro(vtkMesaCoincidentTopologyResolutionPainter,
    vtkCoincidentTopologyResolutionPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkMesaCoincidentTopologyResolutionPainter();
  ~vtkMesaCoincidentTopologyResolutionPainter();

  // Description:
  // Performs the actual rendering. Subclasses may override this method.
  // default implementation merely call a Render on the DelegatePainter,
  // if any. When RenderInternal() is called, it is assured that the 
  // DelegatePainter is in sync with this painter i.e. UpdatePainter()
  // has been called.
  virtual void RenderInternal(vtkRenderer* renderer, vtkActor* actor, 
    unsigned long typeflags);
private:
  vtkMesaCoincidentTopologyResolutionPainter(
    const vtkMesaCoincidentTopologyResolutionPainter&); // Not implemented.
  void operator=(const vtkMesaCoincidentTopologyResolutionPainter&); // Not implemented.
};


#endif
