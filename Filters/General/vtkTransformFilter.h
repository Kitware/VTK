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
 * associated point normals and vectors. Other point data is passed
 * through the filter.
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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Return the MTime also considering the transform.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  //@{
  /**
   * Specify the transform object used to transform points.
   */
  virtual void SetTransform(vtkAbstractTransform*);
  vtkGetObjectMacro(Transform,vtkAbstractTransform);
  //@}

  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);
  //@}

protected:
  vtkTransformFilter();
  ~vtkTransformFilter() VTK_OVERRIDE;

  int RequestDataObject(vtkInformation *request,
                        vtkInformationVector **inputVector,
                        vtkInformationVector *outputVector) VTK_OVERRIDE;
  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *) VTK_OVERRIDE;

  vtkAbstractTransform *Transform;
  int OutputPointsPrecision;
private:
  vtkTransformFilter(const vtkTransformFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTransformFilter&) VTK_DELETE_FUNCTION;
};

#endif
