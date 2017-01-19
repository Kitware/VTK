/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPicker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPicker
 * @brief   superclass for 3D geometric pickers (uses ray cast)
 *
 * vtkPicker is used to select instances of vtkProp3D by shooting a ray
 * into a graphics window and intersecting with the actor's bounding box.
 * The ray is defined from a point defined in window (or pixel) coordinates,
 * and a point located from the camera's position.
 *
 * vtkPicker may return more than one vtkProp3D, since more than one bounding
 * box may be intersected. vtkPicker returns an unsorted list of props that
 * were hit, and a list of the corresponding world points of the hits.
 * For the vtkProp3D that is closest to the camera, vtkPicker returns the
 * pick coordinates in world and untransformed mapper space, the prop itself,
 * the data set, and the mapper.  For vtkPicker the closest prop is the one
 * whose center point (i.e., center of bounding box) projected on the view
 * ray is closest to the camera.  Subclasses of vtkPicker use other methods
 * for computing the pick point.
 *
 * @sa
 * vtkPicker is used for quick geometric picking. If you desire more precise
 * picking of points or cells based on the geometry of any vtkProp3D, use the
 * subclasses vtkPointPicker or vtkCellPicker.  For hardware-accelerated
 * picking of any type of vtkProp, use vtkPropPicker or vtkWorldPointPicker.
*/

#ifndef vtkPicker_h
#define vtkPicker_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkAbstractPropPicker.h"

class vtkAbstractMapper3D;
class vtkCompositeDataSet;
class vtkDataSet;
class vtkTransform;
class vtkActorCollection;
class vtkProp3DCollection;
class vtkPoints;

class VTKRENDERINGCORE_EXPORT vtkPicker : public vtkAbstractPropPicker
{
public:
  static vtkPicker *New();
  vtkTypeMacro(vtkPicker, vtkAbstractPropPicker);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Specify tolerance for performing pick operation. Tolerance is specified
   * as fraction of rendering window size. (Rendering window size is measured
   * across diagonal.)
   */
  vtkSetMacro(Tolerance, double);
  vtkGetMacro(Tolerance, double);
  //@}

  //@{
  /**
   * Return position in mapper (i.e., non-transformed) coordinates of
   * pick point.
   */
  vtkGetVectorMacro(MapperPosition, double, 3);
  //@}

  //@{
  /**
   * Return mapper that was picked (if any).
   */
  vtkGetObjectMacro(Mapper, vtkAbstractMapper3D);
  //@}

  //@{
  /**
   * Get a pointer to the dataset that was picked (if any). If nothing
   * was picked then NULL is returned.
   */
  vtkGetObjectMacro(DataSet, vtkDataSet);
  //@}

  //@{
  /**
   * Get a pointer to the composite dataset that was picked (if any). If nothing
   * was picked or a non-composite data object was picked then NULL is returned.
   */
  vtkGetObjectMacro(CompositeDataSet, vtkCompositeDataSet);
  //@}

  //@{
  /**
   * Get the flat block index of the vtkDataSet in the composite dataset
   * that was picked (if any). If nothing
   * was picked or a non-composite data object was picked then -1 is returned.
   */
  vtkGetMacro(FlatBlockIndex, vtkIdType);
  //@}

  /**
   * Return a collection of all the prop 3D's that were intersected
   * by the pick ray. This collection is not sorted.
   */
  vtkProp3DCollection *GetProp3Ds()
    { return this->Prop3Ds; }

  /**
   * Return a collection of all the actors that were intersected.
   * This collection is not sorted. (This is a convenience method
   * to maintain backward compatibility.)
   */
  vtkActorCollection *GetActors();

  /**
   * Return a list of the points the the actors returned by GetProp3Ds
   * were intersected at. The order of this list will match the order of
   * GetProp3Ds.
   */
  vtkPoints *GetPickedPositions()
    { return this->PickedPositions; }

  /**
   * Perform pick operation with selection point provided. Normally the
   * first two values for the selection point are x-y pixel coordinate, and
   * the third value is =0. Return non-zero if something was successfully
   * picked.
   */
  int Pick(double selectionX, double selectionY, double selectionZ,
                   vtkRenderer *renderer) VTK_OVERRIDE;

  /**
   * Perform pick operation with selection point provided. Normally the first
   * two values for the selection point are x-y pixel coordinate, and the
   * third value is =0. Return non-zero if something was successfully picked.
   */
  int Pick(double selectionPt[3], vtkRenderer *ren)
    { return this->Pick(selectionPt[0], selectionPt[1], selectionPt[2], ren); }

protected:
  vtkPicker();
  ~vtkPicker() VTK_OVERRIDE;

  void MarkPicked(vtkAssemblyPath *path, vtkProp3D *p, vtkAbstractMapper3D *m,
                  double tMin, double mapperPos[3]);
  void MarkPickedData(vtkAssemblyPath *path,
                  double tMin, double mapperPos[3], vtkAbstractMapper3D* mapper,
                  vtkDataSet* input, vtkIdType flatBlockIndex = -1);
  virtual double IntersectWithLine(double p1[3], double p2[3], double tol,
                                  vtkAssemblyPath *path, vtkProp3D *p,
                                  vtkAbstractMapper3D *m);
  void Initialize() VTK_OVERRIDE;
  static bool CalculateRay(double p1[3], double p2[3],
                           double ray[3], double &rayFactor);

  double Tolerance;  //tolerance for computation (% of window)
  double MapperPosition[3]; //selection point in untransformed coordinates

  vtkAbstractMapper3D *Mapper; //selected mapper (if the prop has a mapper)
  vtkDataSet *DataSet; //selected dataset (if there is one)
  vtkCompositeDataSet* CompositeDataSet;
  vtkIdType FlatBlockIndex; // flat block index, for a composite data set

  double GlobalTMin; //parametric coordinate along pick ray where hit occurred
  vtkTransform *Transform; //use to perform ray transformation
  vtkActorCollection *Actors; //candidate actors (based on bounding box)
  vtkProp3DCollection *Prop3Ds; //candidate actors (based on bounding box)
  vtkPoints *PickedPositions; // candidate positions

private:
  vtkPicker(const vtkPicker&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPicker&) VTK_DELETE_FUNCTION;
};

#endif
