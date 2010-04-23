/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaPolyDataMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMesaPolyDataMapper - a PolyDataMapper for the Mesa library
// .SECTION Description
// vtkMesaPolyDataMapper is a subclass of vtkPolyDataMapper.
// vtkMesaPolyDataMapper is a geometric PolyDataMapper for the Mesa 
// rendering library.

#ifndef __vtkMesaPolyDataMapper_h
#define __vtkMesaPolyDataMapper_h

#include "vtkPolyDataMapper.h"
#include "MangleMesaInclude/gl.h"  // Needed for GLenum

class vtkCellArray;
class vtkPoints;
class vtkProperty;
class vtkRenderWindow;
class vtkMesaRenderer;
class vtkTimerLog;
class vtkOpenGLTexture;

class VTK_RENDERING_EXPORT vtkMesaPolyDataMapper : public vtkPolyDataMapper
{
public:
  static vtkMesaPolyDataMapper *New();
  vtkTypeMacro(vtkMesaPolyDataMapper,vtkPolyDataMapper);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement superclass render method.
  virtual void RenderPiece(vtkRenderer *ren, vtkActor *a);

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Draw method for Mesa.
  virtual int Draw(vtkRenderer *ren, vtkActor *a);
  
protected:
  vtkMesaPolyDataMapper();
  ~vtkMesaPolyDataMapper();

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
  vtkRenderWindow *RenderWindow;   // RenderWindow used for the previous render
private:
  vtkMesaPolyDataMapper(const vtkMesaPolyDataMapper&);  // Not implemented.
  void operator=(const vtkMesaPolyDataMapper&);  // Not implemented.
};

#endif
