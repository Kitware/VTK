/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDepthSortPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDepthSortPolyData
 * @brief   sort poly data along camera view direction
 *
 * vtkDepthSortPolyData rearranges the order of cells so that certain
 * rendering operations (e.g., transparency or Painter's algorithms)
 * generate correct results. To use this filter you must specify the
 * direction vector along which to sort the cells. You can do this by
 * specifying a camera and/or prop to define a view direction; or
 * explicitly set a view direction.
 *
 * @warning
 * The sort operation will not work well for long, thin primitives, or cells
 * that intersect, overlap, or interpenetrate each other.
*/

#ifndef vtkDepthSortPolyData_h
#define vtkDepthSortPolyData_h

#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkCamera;
class vtkProp3D;
class vtkTransform;

class VTKFILTERSHYBRID_EXPORT vtkDepthSortPolyData : public vtkPolyDataAlgorithm
{
public:
  /**
   * Instantiate object.
   */
  static vtkDepthSortPolyData *New();

  vtkTypeMacro(vtkDepthSortPolyData,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum Directions
  {
    VTK_DIRECTION_BACK_TO_FRONT = 0,
    VTK_DIRECTION_FRONT_TO_BACK = 1,
    VTK_DIRECTION_SPECIFIED_VECTOR = 2
  };

  //@{
  /**
   * Specify the sort method for the polygonal primitives. By default, the
   * poly data is sorted from back to front.
   */
  vtkSetMacro(Direction,int);
  vtkGetMacro(Direction,int);
  void SetDirectionToFrontToBack()
    {this->SetDirection(VTK_DIRECTION_FRONT_TO_BACK);}
  void SetDirectionToBackToFront()
    {this->SetDirection(VTK_DIRECTION_BACK_TO_FRONT);}
  void SetDirectionToSpecifiedVector()
    {this->SetDirection(VTK_DIRECTION_SPECIFIED_VECTOR);}
  //@}

  enum SortMode
  {
    VTK_SORT_FIRST_POINT = 0,
    VTK_SORT_BOUNDS_CENTER = 1,
    VTK_SORT_PARAMETRIC_CENTER = 2
  };

  //@{
  /**
   * Specify the point to use when sorting. The fastest is to just
   * take the first cell point. Other options are to take the bounding
   * box center or the parametric center of the cell. By default, the
   * first cell point is used.
   */
  vtkSetMacro(DepthSortMode,int);
  vtkGetMacro(DepthSortMode,int);
  void SetDepthSortModeToFirstPoint()
    {this->SetDepthSortMode(VTK_SORT_FIRST_POINT);}
  void SetDepthSortModeToBoundsCenter()
    {this->SetDepthSortMode(VTK_SORT_BOUNDS_CENTER);}
  void SetDepthSortModeToParametricCenter()
    {this->SetDepthSortMode(VTK_SORT_PARAMETRIC_CENTER);}
  //@}

  //@{
  /**
   * Specify a camera that is used to define a view direction along which
   * the cells are sorted. This ivar only has effect if the direction is set
   * to front-to-back or back-to-front, and a camera is specified.
   */
  virtual void SetCamera(vtkCamera*);
  vtkGetObjectMacro(Camera,vtkCamera);
  //@}

  /**
   * Specify a transformation matrix (via the vtkProp3D::GetMatrix() method)
   * that is used to include the effects of transformation. This ivar only
   * has effect if the direction is set to front-to-back or back-to-front,
   * and a camera is specified. Specifying the vtkProp3D is optional.
   */
  void SetProp3D(vtkProp3D *);
  vtkProp3D *GetProp3D()
  { return this->Prop3D; }

  //@{
  /**
   * Set/Get the sort direction. This ivar only has effect if the sort
   * direction is set to SetDirectionToSpecifiedVector(). The sort occurs
   * in the direction of the vector.
   */
  vtkSetVector3Macro(Vector,double);
  vtkGetVectorMacro(Vector,double,3);
  //@}

  //@{
  /**
   * Set/Get the sort origin. This ivar only has effect if the sort
   * direction is set to SetDirectionToSpecifiedVector(). The sort occurs
   * in the direction of the vector, with this point specifying the
   * origin.
   */
  vtkSetVector3Macro(Origin,double);
  vtkGetVectorMacro(Origin,double,3);
  //@}

  //@{
  /**
   * Set/Get a flag that controls the generation of scalar values
   * corresponding to the sort order. If enabled, the output of this
   * filter will include scalar values that range from 0 to (ncells-1),
   * where 0 is closest to the sort direction.
   */
  vtkSetMacro(SortScalars, vtkTypeBool);
  vtkGetMacro(SortScalars, vtkTypeBool);
  vtkBooleanMacro(SortScalars, vtkTypeBool);
  //@}

  /**
   * Return MTime also considering the dependent objects: the camera
   * and/or the prop3D.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkDepthSortPolyData();
  ~vtkDepthSortPolyData() override;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  void ComputeProjectionVector(double vector[3], double origin[3]);

  int Direction;
  int DepthSortMode;
  vtkCamera *Camera;
  vtkProp3D *Prop3D;
  vtkTransform *Transform;
  double Vector[3];
  double Origin[3];
  vtkTypeBool SortScalars;

private:
  vtkDepthSortPolyData(const vtkDepthSortPolyData&) = delete;
  void operator=(const vtkDepthSortPolyData&) = delete;
};

#endif
