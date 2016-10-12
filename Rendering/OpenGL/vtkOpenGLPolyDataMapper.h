/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLPolyDataMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLPolyDataMapper
 * @brief   a PolyDataMapper for the OpenGL library
 *
 * vtkOpenGLPolyDataMapper is a subclass of vtkPolyDataMapper.
 * vtkOpenGLPolyDataMapper is a geometric PolyDataMapper for the OpenGL
 * rendering library.
 * @deprecated Replaced by vtkPainterPolyDataMapper as of VTK 6.2.
*/

#ifndef vtkOpenGLPolyDataMapper_h
#define vtkOpenGLPolyDataMapper_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkPolyDataMapper.h"

#include "vtkOpenGL.h" // Needed for GLenum

class vtkCellArray;
class vtkPoints;
class vtkProperty;
class vtkRenderWindow;
class vtkOpenGLRenderer;
class vtkOpenGLTexture;
#if !defined(VTK_LEGACY_REMOVE)
class VTKRENDERINGOPENGL_EXPORT vtkOpenGLPolyDataMapper : public vtkPolyDataMapper
{
public:
  static vtkOpenGLPolyDataMapper *New();
  vtkTypeMacro(vtkOpenGLPolyDataMapper,vtkPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Implement superclass render method.
   */
  virtual void RenderPiece(vtkRenderer *ren, vtkActor *a);

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *);

  /**
   * Draw method for OpenGL.
   */
  virtual int Draw(vtkRenderer *ren, vtkActor *a);

protected:
  vtkOpenGLPolyDataMapper();
  ~vtkOpenGLPolyDataMapper();

  void DrawPoints(int idx,
                  vtkPoints *p,
                  vtkDataArray *n,
                  vtkUnsignedCharArray *c,
                  vtkDataArray *t,
                  vtkIdType &cellNum,
                  int &noAbort,
                  vtkCellArray *ca,
                  vtkRenderer *ren);

  void DrawLines(int idx,
                 vtkPoints *p,
                 vtkDataArray *n,
                 vtkUnsignedCharArray *c,
                 vtkDataArray *t,
                 vtkIdType &cellNum,
                 int &noAbort,
                 vtkCellArray *ca,
                 vtkRenderer *ren);

  void DrawPolygons(int idx,
                    vtkPoints *p,
                    vtkDataArray *n,
                    vtkUnsignedCharArray *c,
                    vtkDataArray *t,
                    vtkIdType &cellNum,
                    int &noAbort,
                    GLenum rep,
                    vtkCellArray *ca,
                    vtkRenderer *ren);

  void DrawTStrips(int idx,
                   vtkPoints *p,
                   vtkDataArray *n,
                   vtkUnsignedCharArray *c,
                   vtkDataArray *t,
                   vtkIdType &cellNum,
                   int &noAbort,
                   GLenum rep,
                   vtkCellArray *ca,
                   vtkRenderer *ren);

  vtkIdType TotalCells;
  int ListId;
  vtkOpenGLTexture* InternalColorTexture;

private:
  vtkOpenGLPolyDataMapper(const vtkOpenGLPolyDataMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLPolyDataMapper&) VTK_DELETE_FUNCTION;
};
#endif // VTK_LEGACY_REMOVE
#endif
