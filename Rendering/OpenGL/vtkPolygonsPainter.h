/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolygonsPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPolygonsPainter
 * @brief   this painter paints polygons.
 *
 * This painter renders Polys in vtkPolyData. It can render the polys
 * in any representation (VTK_POINTS, VTK_WIREFRAME, VTK_SURFACE).
*/

#ifndef vtkPolygonsPainter_h
#define vtkPolygonsPainter_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkPrimitivePainter.h"

class VTKRENDERINGOPENGL_EXPORT vtkPolygonsPainter : public vtkPrimitivePainter
{
public:
  static vtkPolygonsPainter* New();
  vtkTypeMacro(vtkPolygonsPainter, vtkPrimitivePainter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPolygonsPainter();
  ~vtkPolygonsPainter();

  /**
   * The actual rendering happens here. This method is called only when
   * SupportedPrimitive is present in typeflags when Render() is invoked.
   */
  virtual int RenderPrimitive(unsigned long flags, vtkDataArray* n,
    vtkUnsignedCharArray* c, vtkDataArray* t, vtkRenderer* ren);

private:
  vtkPolygonsPainter(const vtkPolygonsPainter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPolygonsPainter&) VTK_DELETE_FUNCTION;

};


#endif
