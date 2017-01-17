/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataSilhouette.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPolyDataSilhouette
 * @brief   sort polydata along camera view direction
 *
 *
 * vtkPolyDataSilhouette extracts a subset of a polygonal mesh edges to
 * generate an outline (silhouette) of the corresponding 3D object. In
 * addition, this filter can also extracts sharp edges (aka feature angles).
 * In order to use this filter you must specify the a point of view (origin) or
 * a direction (vector).  given this direction or origin, a silhouette is
 * generated wherever the surface's normal is orthogonal to the view direction.
 *
 * @warning
 * when the active camera is used, almost everything is recomputed for each
 * frame, keep this in mind when dealing with extremely large surface data
 * sets.
 *
 * @par Thanks:
 * Contribution by Thierry Carrard <br>
 * CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br>
 * BP12, F-91297 Arpajon, France. <br>
*/

#ifndef vtkPolyDataSilhouette_h
#define vtkPolyDataSilhouette_h

#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"


class vtkCamera;
class vtkProp3D;
class vtkTransform;
class vtkPolyDataEdges;

class VTKFILTERSHYBRID_EXPORT vtkPolyDataSilhouette : public vtkPolyDataAlgorithm
{
public:
  /**
   * Instantiate object.
   */
  static vtkPolyDataSilhouette *New();

  vtkTypeMacro(vtkPolyDataSilhouette,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Enables or Disables generation of silhouette edges along sharp edges
   */
  vtkSetMacro(EnableFeatureAngle,int);
  vtkGetMacro(EnableFeatureAngle,int);
  //@}

  //@{
  /**
   * Sets/Gets minimal angle for sharp edges detection. Default is 60
   */
  vtkSetMacro(FeatureAngle,double);
  vtkGetMacro(FeatureAngle,double);
  //@}

  //@{
  /**
   * Enables or Disables generation of border edges. Note: borders exist only
   * in case of non closed surface
   */
  vtkSetMacro(BorderEdges,int);
  vtkGetMacro(BorderEdges,int);
  vtkBooleanMacro(BorderEdges,int);
  //@}

  //@{
  /**
   * Enables or Disables piece invariance. This is useful when dealing with
   * multi-block data sets. Note: requires one level of ghost cells
   */
  vtkSetMacro(PieceInvariant,int);
  vtkGetMacro(PieceInvariant,int);
  vtkBooleanMacro(PieceInvariant,int);
  //@}

  enum Directions
  {
    VTK_DIRECTION_SPECIFIED_VECTOR = 0,
    VTK_DIRECTION_SPECIFIED_ORIGIN = 1,
    VTK_DIRECTION_CAMERA_ORIGIN = 2,
    VTK_DIRECTION_CAMERA_VECTOR = 3
  };

  //@{
  /**
   * Specify how view direction is computed. By default, the
   * camera origin (eye) is used.
   */
  vtkSetMacro(Direction,int);
  vtkGetMacro(Direction,int);
  void SetDirectionToSpecifiedVector()
      {this->SetDirection( VTK_DIRECTION_SPECIFIED_VECTOR ); }
  void SetDirectionToSpecifiedOrigin()
      {this->SetDirection( VTK_DIRECTION_SPECIFIED_ORIGIN ); }
  void SetDirectionToCameraVector()
      {this->SetDirection( VTK_DIRECTION_CAMERA_VECTOR ); }
  void SetDirectionToCameraOrigin()
      {this->SetDirection( VTK_DIRECTION_CAMERA_ORIGIN ); }
  //@}

  //@{
  /**
   * Specify a camera that is used to define the view direction.  This ivar
   * only has effect if the direction is set to VTK_DIRECTION_CAMERA_ORIGIN or
   * VTK_DIRECTION_CAMERA_VECTOR, and a camera is specified.
   */
  virtual void SetCamera(vtkCamera VTK_WRAP_EXTERN*);
  vtkGetObjectMacro(Camera,vtkCamera VTK_WRAP_EXTERN);
  //@}

  //@{
  /**
   * Specify a transformation matrix (via the vtkProp3D::GetMatrix() method)
   * that is used to include the effects of transformation. This ivar only has
   * effect if the direction is set to VTK_DIRECTION_CAMERA_ORIGIN or
   * VTK_DIRECTION_CAMERA_VECTOR, and a camera is specified. Specifying the
   * vtkProp3D is optional.
   */
  void SetProp3D(vtkProp3D VTK_WRAP_EXTERN*);
  vtkProp3D VTK_WRAP_EXTERN*GetProp3D();
  //@}

  //@{
  /**
   * Set/Get the sort direction. This ivar only has effect if the sort
   * direction is set to SetDirectionToSpecifiedVector(). The edge detection
   * occurs in the direction of the vector.
   */
  vtkSetVector3Macro(Vector,double);
  vtkGetVectorMacro(Vector,double,3);
  //@}

  //@{
  /**
   * Set/Get the sort origin. This ivar only has effect if the sort direction
   * is set to SetDirectionToSpecifiedOrigin(). The edge detection occurs in
   * the direction of the origin to each edge's center.
   */
  vtkSetVector3Macro(Origin,double);
  vtkGetVectorMacro(Origin,double,3);
  //@}

  /**
   * Return MTime also considering the dependent objects: the camera
   * and/or the prop3D.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

protected:
  vtkPolyDataSilhouette();
  ~vtkPolyDataSilhouette() VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  void ComputeProjectionVector(double vector[3], double origin[3]);

  int Direction;
  vtkCamera *Camera;
  vtkProp3D *Prop3D;
  vtkTransform *Transform;
  double Vector[3];
  double Origin[3];

  int EnableFeatureAngle;
  double FeatureAngle;

  int BorderEdges;
  int PieceInvariant;

  vtkPolyDataEdges* PreComp; // precomputed data for a given point of view

private:
  vtkPolyDataSilhouette(const vtkPolyDataSilhouette&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPolyDataSilhouette&) VTK_DELETE_FUNCTION;
};

#endif
