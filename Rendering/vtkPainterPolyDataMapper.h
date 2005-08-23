/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPainterPolyDataMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPainterPolyDataMapper
// .SECTION Description
// PolyDataMapper that uses painters to do the actual rendering.

#ifndef __vtkPainterPolyDataMapper_h
#define __vtkPainterPolyDataMapper_h

#include "vtkPolyDataMapper.h"

class vtkPainterPolyDataMapperObserver;
class vtkPolyDataPainter;

class VTK_RENDERING_EXPORT vtkPainterPolyDataMapper : public vtkPolyDataMapper
{
public:
  static vtkPainterPolyDataMapper* New();
  vtkTypeRevisionMacro(vtkPainterPolyDataMapper, vtkPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implemented by sub classes. Actual rendering is done here.
  virtual void RenderPiece(vtkRenderer *ren, vtkActor *act);

  // Description:
  // Get/Set the painter used to do the actual rendering.
  // By default, vtkDefaultPainter is used to build the rendering 
  // painter chain for color mapping/clipping etc. followed by 
  // a vtkChooserPainter which renders the primitives.
  vtkGetObjectMacro(Painter, vtkPolyDataPainter);
  void SetPainter(vtkPolyDataPainter*);

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release. Merely propagates the call to the painter.
  void ReleaseGraphicsResources(vtkWindow *);
  
protected:
  vtkPainterPolyDataMapper();
  ~vtkPainterPolyDataMapper();

  // Description:
  // Called when the PainterInformation becomes obsolete. 
  // It is called before the Render is initiated on the Painter.
  void UpdatePainterInformation();

  // Description:
  // Take part in garbage collection.
  void ReportReferences(vtkGarbageCollector *collector);

  vtkInformation* PainterInformation;
  vtkTimeStamp PainterUpdateTime;
  vtkPolyDataPainter* Painter;
  vtkPainterPolyDataMapperObserver* Observer;
private:
  vtkPainterPolyDataMapper(const vtkPainterPolyDataMapper&); // Not implemented.
  void operator=(const vtkPainterPolyDataMapper&); // Not implemented.
};

#endif

