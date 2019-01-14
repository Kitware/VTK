/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTransformFilter
 * @brief   transform points and associated normals and vectors
 *
 * vtkTransformFilter is a filter to transform point coordinates, and
 * associated point normals and vectors, as well as cell normals and vectors.
 * Transformed data array will be stored in a float array or a double array.
 * Other point and cel data are passed through the filter, unless TransformAllInputVectors
 * is set to true, in this case all other 3 components arrays from point and cell data
 * will be transformed as well.
 *
 * An alternative method of transformation is to use vtkActor's methods
 * to scale, rotate, and translate objects. The difference between the
 * two methods is that vtkActor's transformation simply effects where
 * objects are rendered (via the graphics pipeline), whereas
 * vtkTransformFilter actually modifies point coordinates in the
 * visualization pipeline. This is necessary for some objects
 * (e.g., vtkProbeFilter) that require point coordinates as input.
 *
 * @sa
 * vtkAbstractTransform vtkTransformPolyDataFilter vtkActor
*/

#ifndef vtkTransformFilter_h
#define vtkTransformFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

class vtkAbstractTransform;

class VTKFILTERSGENERAL_EXPORT vtkTransformFilter : public vtkPointSetAlgorithm
{
public:
  static vtkTransformFilter *New();
  vtkTypeMacro(vtkTransformFilter,vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return the MTime also considering the transform.
   */
  vtkMTimeType GetMTime() override;

  //@{
  /**
   * Specify the transform object used to transform points.
   */
  virtual void SetTransform(vtkAbstractTransform*);
  vtkGetObjectMacro(Transform,vtkAbstractTransform);
  //@}

  int FillInputPortInformation(int port, vtkInformation *info) override;

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);
  //@}

  //@{
  /**
   * If off (the default), only Vectors and Normals will be transformed.
   * If on, all 3-component data arrays ( considered as 3D vectors) will be transformed
   * All other won't be flipped and will only be copied.
   */
  vtkSetMacro(TransformAllInputVectors, bool);
  vtkGetMacro(TransformAllInputVectors, bool);
  vtkBooleanMacro(TransformAllInputVectors, bool);
  //@}

protected:
  vtkTransformFilter();
  ~vtkTransformFilter() override;

  int RequestDataObject(vtkInformation *request,
                        vtkInformationVector **inputVector,
                        vtkInformationVector *outputVector) override;
  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *) override;

  vtkDataArray* CreateNewDataArray();

  vtkAbstractTransform *Transform;
  int OutputPointsPrecision;
  bool TransformAllInputVectors;

private:
  vtkTransformFilter(const vtkTransformFilter&) = delete;
  void operator=(const vtkTransformFilter&) = delete;
};

#endif
