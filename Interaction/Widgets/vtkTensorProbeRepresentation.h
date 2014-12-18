/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTensorProbeRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTensorProbeRepresentation - Abstract class that serves as a representation for vtkTensorProbeWidget
// .SECTION Description
// The class serves as an abstract geometrical representation for the
// vtkTensorProbeWidget. It is left to the concrete implementation to render
// the tensors as it desires. For instance,
// vtkEllipsoidTensorProbeRepresentation renders the tensors as ellipsoids.
//
// .SECTION See Also
// vtkTensorProbeWidget

#ifndef vtkTensorProbeRepresentation_h
#define vtkTensorProbeRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWidgetRepresentation.h"

class vtkActor;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkGenericCell;

class VTKINTERACTIONWIDGETS_EXPORT vtkTensorProbeRepresentation :
                           public vtkWidgetRepresentation
{
public:
  // Description:
  // Standard methods for instances of this class.
  vtkTypeMacro(vtkTensorProbeRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // See vtkWidgetRepresentation for details.
  virtual void BuildRepresentation();
  virtual int RenderOpaqueGeometry(vtkViewport *);

  // Description:
  // Set the position of the Tensor probe.
  vtkSetVector3Macro( ProbePosition, double );
  vtkGetVector3Macro( ProbePosition, double );
  vtkSetMacro( ProbeCellId, vtkIdType );
  vtkGetMacro( ProbeCellId, vtkIdType );

  // Description:
  // Set the trajectory that we are trying to probe tensors on
  virtual void SetTrajectory( vtkPolyData * );

  // Description:
  // Set the probe position to a reasonable location on the trajectory.
  void Initialize();

  // Description:
  // This method is invoked by the widget during user interaction.
  // Can we pick the tensor glyph at the current cursor pos
  virtual int SelectProbe( int pos[2] ) = 0;

  // Description:
  // INTERNAL - Do not use
  // This method is invoked by the widget during user interaction.
  // Move probe based on the position and the motion vector.
  virtual int Move( double motionVector[2] );

  // Description:
  // See vtkProp for details.
  virtual void GetActors(vtkPropCollection *);
  virtual void ReleaseGraphicsResources(vtkWindow *);

protected:
  vtkTensorProbeRepresentation();
  ~vtkTensorProbeRepresentation();

  void FindClosestPointOnPolyline(
    double displayPos[2], double closestWorldPos[3], vtkIdType &cellId,
    int maxSpeed = 10 );

  vtkActor           * TrajectoryActor;
  vtkPolyDataMapper  * TrajectoryMapper;
  vtkPolyData        * Trajectory;
  double               ProbePosition[3];
  vtkIdType            ProbeCellId;

private:
  vtkTensorProbeRepresentation(
      const vtkTensorProbeRepresentation&);  //Not implemented
  void operator=(const vtkTensorProbeRepresentation&);  //Not implemented

};

#endif

