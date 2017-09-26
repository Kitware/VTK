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
/**
 * @class   vtkClipPlanesPainter
 * @brief   abstract class defining interface for
 * painter that manages clipping.
*/

#ifndef vtkClipPlanesPainter_h
#define vtkClipPlanesPainter_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkPainter.h"

class vtkInformationObjectBaseKey;
class vtkPlaneCollection;

class VTKRENDERINGOPENGL_EXPORT vtkClipPlanesPainter : public vtkPainter
{
public:
  static vtkClipPlanesPainter* New();
  vtkTypeMacro(vtkClipPlanesPainter, vtkPainter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get/Set the vtkPlaneCollection which specifies the clipping planes.
   */
  static vtkInformationObjectBaseKey* CLIPPING_PLANES();

protected:
  vtkClipPlanesPainter();
  ~vtkClipPlanesPainter() override;

  /**
   * Called before RenderInternal() if the Information has been changed
   * since the last time this method was called.
   */
  void ProcessInformation(vtkInformation*) override;

  void SetClippingPlanes(vtkPlaneCollection*);
  vtkPlaneCollection* ClippingPlanes;
private:
  vtkClipPlanesPainter(const vtkClipPlanesPainter&) = delete;
  void operator=(const vtkClipPlanesPainter&) = delete;

};

#endif
