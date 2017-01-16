/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWeightedTransformFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWeightedTransformFilter
 * @brief   transform based on per-point or per-cell weighting functions.
 *
 *
 * vtkWeightedTransformFilter is a filter that can be used to "skin"
 * structures and to create new and complex shapes.  Unlike a
 * traditional transform filter (which has one transform for a data
 * set) or an assembly (which has one transform per part or group of
 * parts), a weighted transform produces the weighted sum of
 * transforms on a per-point or per-cell basis.
 *
 * Each point or cell in the filter's input has an attached DataArray
 * that contains tuples of weighting functions, one per point or cell.
 * The filter also has a set of fixed transforms.  When the filter
 * executes, each input point/cell is transformed by each of the
 * transforms.  These results are weighted by the point/cell's
 * weighting factors to produce final output data.
 *
 * Linear transforms are performance-optimized.  Using arbitrary
 * transforms will work, but performance may suffer.
 *
 * As an example of the utility of weighted transforms, here's how
 * this filter can be used for "skinning."  Skinning is the process of
 * putting a mesh cover over an underlying structure, like skin over
 * bone.  Joints are difficult to skin because deformation is hard to
 * do.  Visualize skin over an elbow joint.  Part of the skin moves
 * with one bone, part of the skin moves with the other bone, and the
 * skin in the middle moves a little with each.
 *
 * Weighted filtering can be used for a simple and efficient kind of
 * skinning.  Begin with a cylindrical mesh.  Create a FloatArray with
 * two components per tuple, and one tuple for each point in the mesh.
 * Assign transform weights that linear interpolate the distance along
 * the cylinder (one component is the distance along the cylinder, the
 * other is one minus that distance).  Set the filter up to use two
 * transforms, the two used to transform the two bones.  Now, when the
 * transforms change, the mesh will deform so as to, hopefully,
 * continue to cover the bones.
 *
 * vtkWeightedTransformFilter is also useful for creating "strange and
 * complex" shapes using pinching, bending, and blending.
 *
 * @warning
 * Weighted combination of normals and vectors are probably not appropriate
 * in many cases.  Surface normals are treated somewhat specially, but
 * in many cases you may need to regenerate the surface normals.
 *
 * @warning
 * Cell data can only be transformed if all transforms are linear.
 *
 *
 * @sa
 * vtkAbstractTransform vtkLinearTransform vtkTransformPolyDataFilter vtkActor
*/

#ifndef vtkWeightedTransformFilter_h
#define vtkWeightedTransformFilter_h

#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

class vtkAbstractTransform;

class VTKFILTERSHYBRID_EXPORT vtkWeightedTransformFilter : public vtkPointSetAlgorithm
{
public:
  static vtkWeightedTransformFilter *New();
  vtkTypeMacro(vtkWeightedTransformFilter,vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Return the MTime also considering the filter's transforms.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  //@{
  /**
   * WeightArray is the string name of the DataArray in the input's
   * FieldData that holds the weighting coefficients for each point.
   * The filter will first look for the array in the input's PointData
   * FieldData.  If the array isn't there, the filter looks in the
   * input's FieldData.  The WeightArray can have tuples of any length,
   * but must have a tuple for every point in the input data set.
   * This array transforms points, normals, and vectors.
   */
  vtkSetStringMacro(WeightArray);
  vtkGetStringMacro(WeightArray);
  //@}

  //@{
  /**
   * TransformIndexArray is the string name of the DataArray in the input's
   * FieldData that holds the indices for the transforms for each point.
   * These indices are used to select which transforms each weight of
   * the DataArray refers.  If the TransformIndexArray is not specified,
   * the weights of each point are assumed to map directly to a transform.
   * This DataArray must be of type UnsignedShort, which effectively
   * limits the number of transforms to 65536 if a transform index
   * array is used.

   * The filter will first look for the array in the input's PointData
   * FieldData.  If the array isn't there, the filter looks in the
   * input's FieldData.  The TransformIndexArray can have tuples of any
   * length, but must have a tuple for every point in the input data set.
   * This array transforms points, normals, and vectors.
   */
  vtkSetStringMacro(TransformIndexArray);
  vtkGetStringMacro(TransformIndexArray);
  //@}

  //@{
  /**
   * The CellDataWeightArray is analogous to the WeightArray, except
   * for CellData.  The array is searched for first in the CellData
   * FieldData, then in the input's FieldData.  The data array must have
   * a tuple for each cell.  This array is used to transform only normals
   * and vectors.
   */
  vtkSetStringMacro(CellDataWeightArray);
  vtkGetStringMacro(CellDataWeightArray);
  //@}

  //@{
  /**
   * The CellDataTransformIndexArray is like a TransformIndexArray,
   * except for cell data.  The array must have type UnsignedShort.
   */
  vtkSetStringMacro(CellDataTransformIndexArray);
  vtkGetStringMacro(CellDataTransformIndexArray);
  //@}

  //@{
  /**
   * Set or Get one of the filter's transforms. The transform number must
   * be less than the number of transforms allocated for the object.  Setting
   * a transform slot to NULL is equivalent to assigning an overriding weight
   * of zero to that filter slot.
   */
  virtual void SetTransform(vtkAbstractTransform *transform, int num);
  virtual vtkAbstractTransform *GetTransform(int num);
  //@}

  //@{
  /**
   * Set the number of transforms for the filter.  References to non-existent
   * filter numbers in the data array is equivalent to a weight of zero
   * (i.e., no contribution of that filter or weight).  The maximum number of
   * transforms is limited to 65536 if transform index arrays are used.
   */
  virtual void SetNumberOfTransforms(int num);
  vtkGetMacro(NumberOfTransforms, int);
  //@}

  //@{
  /**
   * If AddInputValues is true, the output values of this filter will be
   * offset from the input values.  The effect is exactly equivalent to
   * having an identity transform of weight 1 added into each output point.
   */
  vtkBooleanMacro(AddInputValues, int);
  vtkSetMacro(AddInputValues, int);
  vtkGetMacro(AddInputValues, int);
  //@}

protected:
  vtkAbstractTransform **Transforms;
  int NumberOfTransforms;
  int AddInputValues;

  char *CellDataWeightArray;
  char *WeightArray;

  char *CellDataTransformIndexArray;
  char *TransformIndexArray;

  vtkWeightedTransformFilter();
  ~vtkWeightedTransformFilter() VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
private:
  vtkWeightedTransformFilter(const vtkWeightedTransformFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkWeightedTransformFilter&) VTK_DELETE_FUNCTION;
};

#endif
