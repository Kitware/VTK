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
/**
 * @class   vtkPainterPolyDataMapper
 * @brief   PolyDataMapper using painters
 *
 * PolyDataMapper that uses painters to do the actual rendering.
 * @par Thanks:
 * Support for generic vertex attributes in VTK was contributed in
 * collaboration with Stephane Ploix at EDF.
*/

#ifndef vtkPainterPolyDataMapper_h
#define vtkPainterPolyDataMapper_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkPolyDataMapper.h"

class vtkPainterPolyDataMapperObserver;
class vtkPainter;

class VTKRENDERINGOPENGL_EXPORT vtkPainterPolyDataMapper : public vtkPolyDataMapper
{
public:
  static vtkPainterPolyDataMapper* New();
  vtkTypeMacro(vtkPainterPolyDataMapper, vtkPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Implemented by sub classes. Actual rendering is done here.
   */
  void RenderPiece(vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the painter used to do the actual rendering.
   * By default, vtkDefaultPainter is used to build the rendering
   * painter chain for color mapping/clipping etc. followed by
   * a vtkChooserPainter which renders the primitives.
   */
  vtkGetObjectMacro(Painter, vtkPainter);
  void SetPainter(vtkPainter*);
  //@}

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release. Merely propagates the call to the painter.
   */
  void ReleaseGraphicsResources(vtkWindow *) VTK_OVERRIDE;

  /**
   * Select a data array from the point/cell data
   * and map it to a generic vertex attribute.
   * vertexAttributeName is the name of the vertex attribute.
   * dataArrayName is the name of the data array.
   * fieldAssociation indicates when the data array is a point data array or
   * cell data array (vtkDataObject::FIELD_ASSOCIATION_POINTS or
   * (vtkDataObject::FIELD_ASSOCIATION_CELLS).
   * componentno indicates which component from the data array must be passed as
   * the attribute. If -1, then all components are passed.
   */
  void MapDataArrayToVertexAttribute(
    const char* vertexAttributeName,
    const char* dataArrayName, int fieldAssociation, int componentno=-1) VTK_OVERRIDE;

  void MapDataArrayToMultiTextureAttribute(
    int unit,
    const char* dataArrayName, int fieldAssociation, int componentno=-1) VTK_OVERRIDE;

  /**
   * Remove a vertex attribute mapping.
   */
  void RemoveVertexAttributeMapping(const char* vertexAttributeName) VTK_OVERRIDE;

  /**
   * Remove all vertex attributes.
   */
  void RemoveAllVertexAttributeMappings() VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the painter used when rendering the selection pass.
   */
  vtkGetObjectMacro(SelectionPainter, vtkPainter);
  void SetSelectionPainter(vtkPainter*);
  //@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Used by vtkHardwareSelector to determine if the prop supports hardware
   * selection.
   */
  bool GetSupportsSelection() VTK_OVERRIDE
    { return (this->SelectionPainter != 0); }

  /**
   * Returns if the mapper does not expect to have translucent geometry. This
   * may happen when using ScalarMode is set to not map scalars i.e. render the
   * scalar array directly as colors and the scalar array has opacity i.e. alpha
   * component. Note that even if this method returns true, an actor may treat
   * the geometry as translucent since a constant translucency is set on the
   * property, for example.
   * Overridden to use the actual data and ScalarMode to determine if we have
   * opaque geometry.
   */
  bool GetIsOpaque() VTK_OVERRIDE;

protected:
  vtkPainterPolyDataMapper();
  ~vtkPainterPolyDataMapper() VTK_OVERRIDE;

  /**
   * Called in GetBounds(). When this method is called, the consider the input
   * to be updated depending on whether this->Static is set or not. This method
   * simply obtains the bounds from the data-object and returns it.
   */
  void ComputeBounds() VTK_OVERRIDE;

  /**
   * Called when the PainterInformation becomes obsolete.
   * It is called before UpdateBounds or Render is initiated on the Painter
   */
  virtual void UpdatePainterInformation();

  /**
   * Take part in garbage collection.
   */
  void ReportReferences(vtkGarbageCollector *collector) VTK_OVERRIDE;

  vtkInformation* PainterInformation;
  vtkTimeStamp PainterUpdateTime;
  vtkPainter* Painter;
  // Painter used when rendering for hardware selection
  // (look at vtkHardwareSelector).
  vtkPainter* SelectionPainter;
  vtkPainterPolyDataMapperObserver* Observer;
private:
  vtkPainterPolyDataMapper(const vtkPainterPolyDataMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPainterPolyDataMapper&) VTK_DELETE_FUNCTION;
};

#endif
