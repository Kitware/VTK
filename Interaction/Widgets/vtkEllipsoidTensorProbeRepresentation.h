/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEllipsoidTensorProbeRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkEllipsoidTensorProbeRepresentation
 * @brief   A concrete implementation of vtkTensorProbeRepresentation that renders tensors as ellipoids.
 *
 * vtkEllipsoidTensorProbeRepresentation is a concrete implementation of
 * vtkTensorProbeRepresentation. It renders tensors as ellipsoids. Locations
 * between two points when probed have the tensors linearly interpolated
 * from the neighboring locations on the polyline.
 *
 * @sa
 * vtkTensorProbeWidget
*/

#ifndef vtkEllipsoidTensorProbeRepresentation_h
#define vtkEllipsoidTensorProbeRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkTensorProbeRepresentation.h"

class vtkCellPicker;
class vtkTensorGlyph;
class vtkPolyDataNormals;

class VTKINTERACTIONWIDGETS_EXPORT vtkEllipsoidTensorProbeRepresentation :
               public vtkTensorProbeRepresentation
{
public:
  static vtkEllipsoidTensorProbeRepresentation *New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkEllipsoidTensorProbeRepresentation,
                                vtkTensorProbeRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  void BuildRepresentation() VTK_OVERRIDE;
  int RenderOpaqueGeometry(vtkViewport *) VTK_OVERRIDE;

  /**
   * Can we pick the tensor glyph at the current cursor pos
   */
  int SelectProbe( int pos[2] ) VTK_OVERRIDE;

  //@{
  /**
   * See vtkProp for details.
   */
  void GetActors(vtkPropCollection *) VTK_OVERRIDE;
  void ReleaseGraphicsResources(vtkWindow *) VTK_OVERRIDE;
  //@}

protected:
  vtkEllipsoidTensorProbeRepresentation();
  ~vtkEllipsoidTensorProbeRepresentation() VTK_OVERRIDE;

  // Get the interpolated tensor at the current position
  void EvaluateTensor( double t[9] );

  // Register internal Pickers within PickingManager
  void RegisterPickers() VTK_OVERRIDE;

  vtkActor           * EllipsoidActor;
  vtkPolyDataMapper  * EllipsoidMapper;
  vtkPolyData        * TensorSource;
  vtkTensorGlyph     * TensorGlypher;
  vtkCellPicker      * CellPicker;
  vtkPolyDataNormals * PolyDataNormals;

private:
  vtkEllipsoidTensorProbeRepresentation(const
      vtkEllipsoidTensorProbeRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const
      vtkEllipsoidTensorProbeRepresentation&) VTK_DELETE_FUNCTION;

};

#endif

