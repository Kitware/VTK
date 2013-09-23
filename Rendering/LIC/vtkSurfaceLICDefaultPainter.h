/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSurfaceLICDefaultPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSurfaceLICDefaultPainter - vtkDefaultPainter replacement that
//  inserts the vtkSurfaceLICPainter at the correct position in the painter
//  chain.
//
// .SECTION Description
//  vtkSurfaceLICDefaultPainter is a vtkDefaultPainter replacement
//  that inserts the vtkSurfaceLICPainter at the correct position in the painter
//  chain.
//
// .SECTION See Also
//  vtkDefaultPainter vtkSurfaceLICPainter

#ifndef __vtkSurfaceLICDefaultPainter_h
#define __vtkSurfaceLICDefaultPainter_h

#include "vtkRenderingLICModule.h" // For export macro
#include "vtkDefaultPainter.h"

class vtkSurfaceLICPainter;

class VTKRENDERINGLIC_EXPORT vtkSurfaceLICDefaultPainter
  : public vtkDefaultPainter
{
public:
  static vtkSurfaceLICDefaultPainter* New();
  vtkTypeMacro(vtkSurfaceLICDefaultPainter, vtkDefaultPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the Surface LIC painter.
  void SetSurfaceLICPainter(vtkSurfaceLICPainter*);
  vtkGetObjectMacro(SurfaceLICPainter, vtkSurfaceLICPainter);

//BTX
protected:
  vtkSurfaceLICDefaultPainter();
  ~vtkSurfaceLICDefaultPainter();

  // Description:
  // Setup the the painter chain.
  virtual void BuildPainterChain();

  // Description:
  // Take part in garbage collection.
  virtual void ReportReferences(vtkGarbageCollector *collector);

  // Description:
  // Override.
  virtual void UpdateBounds(double bounds[6]);

protected:
  vtkSurfaceLICPainter* SurfaceLICPainter;

private:
  vtkSurfaceLICDefaultPainter(const vtkSurfaceLICDefaultPainter&); // Not implemented.
  void operator=(const vtkSurfaceLICDefaultPainter&); // Not implemented.
//ETX
};

#endif
