/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolygonalSurfacePointPlacer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME - Place points on the surface of polygonal data.
//
// .SECTION Description
// vtkPolygonalSurfacePointPlacer places points on polygonal data and is
// meant to be used in conjunction with
// vtkPolygonalSurfaceContourLineInterpolator.
//
// .SECTION Usage
//
// .SECTION Caveats
// You should have computed cell normals for the input polydata if you are
// specifying a distance offset.
//
// .SECTION See Also
// vtkPointPlacer vtkPolyDataNormals

#ifndef __vtkPolygonalSurfacePointPlacer_h
#define __vtkPolygonalSurfacePointPlacer_h

#include "vtkPolyDataPointPlacer.h"

class  vtkPolyDataCollection;
class  vtkCellPicker;
class  vtkPolygonalSurfacePointPlacerInternals;
class  vtkPolyData;

//BTX
// The Node stores information about the point. This information is used by
// the interpolator. Reusing this information avoids the need for a second
// pick operation to regenerate it. (Cellpickers are slow).
struct vtkPolygonalSurfacePointPlacerNode
{
  double       WorldPosition[3];
  double       SurfaceWorldPosition[3];
  vtkIdType    CellId;
  vtkIdType    PointId;
  double       ParametricCoords[3]; // parametric coords within cell
  vtkPolyData  *PolyData;
};
//ETX

class VTK_WIDGETS_EXPORT vtkPolygonalSurfacePointPlacer
                                  : public vtkPolyDataPointPlacer
{
public:
  // Description:
  // Instantiate this class.
  static vtkPolygonalSurfacePointPlacer *New();

  // Description:
  // Standard methods for instances of this class.
  vtkTypeMacro(vtkPolygonalSurfacePointPlacer,vtkPolyDataPointPlacer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Descuription:
  // Add /remove a prop, to place points on
  virtual void AddProp( vtkProp * );
  virtual void RemoveViewProp(vtkProp *prop);
  virtual void RemoveAllProps();

  // Description:
  // Given a renderer and a display position in pixel coordinates,
  // compute the world position and orientation where this point
  // will be placed. This method is typically used by the
  // representation to place the point initially.
  // For the Terrain point placer this computes world points that
  // lie at the specified height above the terrain.
  virtual int ComputeWorldPosition( vtkRenderer *ren,
                                    double displayPos[2],
                                    double worldPos[3],
                                    double worldOrient[9] );

  // Description:
  // Given a renderer, a display position, and a reference world
  // position, compute the new world position and orientation
  // of this point. This method is typically used by the
  // representation to move the point.
  virtual int ComputeWorldPosition( vtkRenderer *ren,
                                    double displayPos[2],
                                    double refWorldPos[3],
                                    double worldPos[3],
                                    double worldOrient[9] );

  // Description:
  // Given a world position check the validity of this
  // position according to the constraints of the placer
  virtual int ValidateWorldPosition( double worldPos[3] );

  // Description:
  // Give the node a chance to update its auxiliary point id.
  virtual int UpdateNodeWorldPosition( double worldPos[3],
                                       vtkIdType nodePointId );

  // Description:
  // Given a display position, check the validity of this position.
  virtual int ValidateDisplayPosition( vtkRenderer *, double displayPos[2] );

  // Description:
  // Given a world position and a world orientation,
  // validate it according to the constraints of the placer.
  virtual int ValidateWorldPosition( double worldPos[3],
                                     double worldOrient[9] );

  // Description:
  // Get the Prop picker.
  vtkGetObjectMacro( CellPicker, vtkCellPicker );

  // Description:
  // Be sure to add polydata on which you wish to place points to this list
  // or they will not be considered for placement.
  vtkGetObjectMacro( Polys, vtkPolyDataCollection );

  // Description:
  // Height offset at which points may be placed on the polygonal surface.
  // If you specify a non-zero value here, be sure to compute cell normals
  // on your input polygonal data (easily done with vtkPolyDataNormals).
  vtkSetMacro( DistanceOffset, double );
  vtkGetMacro( DistanceOffset, double );

  // Description:
  // Snap to the closest point on the surface ?
  // This is useful for the vtkPolygonalSurfaceContourLineInterpolator, when
  // drawing contours along the edges of a surface mesh.
  // OFF by default.
  vtkSetMacro( SnapToClosestPoint, int );
  vtkGetMacro( SnapToClosestPoint, int );
  vtkBooleanMacro( SnapToClosestPoint, int );

//BTX
  // Description:
  // Internally used by the interpolator.
  typedef vtkPolygonalSurfacePointPlacerNode Node;
  Node *GetNodeAtWorldPosition( double worldPos[3] );
//ETX

protected:
  vtkPolygonalSurfacePointPlacer();
  ~vtkPolygonalSurfacePointPlacer();

  // The props that represents the terrain data (one or more) in a rendered
  // scene
  vtkCellPicker                           *CellPicker;
  vtkPolyDataCollection                   *Polys;
  vtkPolygonalSurfacePointPlacerInternals *Internals;
  double                                   DistanceOffset;
  int                                      SnapToClosestPoint;

private:
  vtkPolygonalSurfacePointPlacer(const vtkPolygonalSurfacePointPlacer&);  //Not implemented
  void operator=(const vtkPolygonalSurfacePointPlacer&);  //Not implemented
};

#endif
