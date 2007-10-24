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
// .NAME vtkPainterPolyDataMapper - PolyDataMapper using painters
// .SECTION Description
// PolyDataMapper that uses painters to do the actual rendering.
// .SECTION Thanks
// Support for generic vertex attributes in VTK was contributed in
// collaboration with Stéphane Ploix at EDF.

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

  // Description:
  // Re-implement the superclass GetBounds method.
  void GetBounds(double bounds[6]);
  double *GetBounds();

  // Description:
  // Select a data array from the point/cell data
  // and map it to a generic vertex attribute. 
  // vertexAttributeName is the name of the vertex attribute.
  // dataArrayName is the name of the data array.
  // fieldAssociation indicates when the data array is a point data array or
  // cell data array (vtkDataObject::FIELD_ASSOCIATION_POINTS or
  // (vtkDataObject::FIELD_ASSOCIATION_CELLS).
  // componentno indicates which component from the data array must be passed as
  // the attribute. If -1, then all components are passed.
  virtual void MapDataArrayToVertexAttribute(
    const char* vertexAttributeName,
    const char* dataArrayName, int fieldAssociation, int componentno=-1);

  // Description:
  // Remove a vertex attribute mapping.
  virtual void RemoveVertexAttributeMapping(const char* vertexAttributeName);

  // Description:
  // Remove all vertex attributes.
  virtual void RemoveAllVertexAttributeMappings();
    
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

