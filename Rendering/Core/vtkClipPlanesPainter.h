/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClipPlanesPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkClipPlanesPainter - abstract class defining interface for
// painter that manages clipping.

#ifndef __vtkClipPlanesPainter_h
#define __vtkClipPlanesPainter_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkPainter.h"

class vtkInformationObjectBaseKey;
class vtkPlaneCollection;

class VTKRENDERINGCORE_EXPORT vtkClipPlanesPainter : public vtkPainter
{
public:
  static vtkClipPlanesPainter* New();
  vtkTypeMacro(vtkClipPlanesPainter, vtkPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the vtkPlaneCollection which specifies the clipping planes.
  static vtkInformationObjectBaseKey* CLIPPING_PLANES();

protected:
  vtkClipPlanesPainter();
  ~vtkClipPlanesPainter();

  // Description:
  // Called before RenderInternal() if the Information has been changed
  // since the last time this method was called.
  virtual void ProcessInformation(vtkInformation*);

  void SetClippingPlanes(vtkPlaneCollection*);
  vtkPlaneCollection* ClippingPlanes;
private:
  vtkClipPlanesPainter(const vtkClipPlanesPainter&); // Not implemented.
  void operator=(const vtkClipPlanesPainter&); // Not implemented.

};

#endif
