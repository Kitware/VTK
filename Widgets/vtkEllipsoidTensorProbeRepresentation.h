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
// .NAME vtkEllipsoidTensorProbeRepresentation - A concrete implementation of vtkTensorProbeRepresentation that renders tensors as ellipoids.
// .SECTION Description
// vtkEllipsoidTensorProbeRepresentation is a concrete implementation of 
// vtkTensorProbeRepresentation. It renders tensors as ellipsoids. Locations
// between two points when probed have the tensors linearly interpolated 
// from the neighboring locations on the polyline.
//
// .SECTION See Also
// vtkTensorProbeWidget

#ifndef __vtkEllipsoidTensorProbeRepresentation_h
#define __vtkEllipsoidTensorProbeRepresentation_h

#include "vtkTensorProbeRepresentation.h"

class vtkCellPicker;
class vtkTensorGlyph;
class vtkPolyDataNormals;

class VTK_WIDGETS_EXPORT vtkEllipsoidTensorProbeRepresentation :
               public vtkTensorProbeRepresentation
{
public:
  static vtkEllipsoidTensorProbeRepresentation *New();

  // Description:
  // Standard methods for instances of this class.
  vtkTypeMacro(vtkEllipsoidTensorProbeRepresentation,
                                vtkTensorProbeRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void BuildRepresentation();
  virtual int RenderOpaqueGeometry(vtkViewport *);

  // Description:
  // Can we pick the tensor glyph at the current cursor pos
  virtual int SelectProbe( int pos[2] );

  // Description:
  // See vtkProp for details.  
  virtual void GetActors(vtkPropCollection *);
  virtual void ReleaseGraphicsResources(vtkWindow *);
  
protected:
  vtkEllipsoidTensorProbeRepresentation();
  ~vtkEllipsoidTensorProbeRepresentation();

  // Get the interpolated tensor at the current position
  void EvaluateTensor( double t[9] ); 

  vtkActor           * EllipsoidActor;
  vtkPolyDataMapper  * EllipsoidMapper;
  vtkPolyData        * TensorSource;
  vtkTensorGlyph     * TensorGlypher;
  vtkCellPicker      * CellPicker;
  vtkPolyDataNormals * PolyDataNormals;

private:
  vtkEllipsoidTensorProbeRepresentation(const 
      vtkEllipsoidTensorProbeRepresentation&);  //Not implemented
  void operator=(const 
      vtkEllipsoidTensorProbeRepresentation&);  //Not implemented

};

#endif

