/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericProbeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGenericProbeFilter
 * @brief   sample data values at specified point locations
 *
 * vtkGenericProbeFilter is a filter that computes point attributes (e.g., scalars,
 * vectors, etc.) at specified point positions. The filter has two inputs:
 * the Input and Source. The Input geometric structure is passed through the
 * filter. The point attributes are computed at the Input point positions
 * by interpolating into the source data. For example, we can compute data
 * values on a plane (plane specified as Input) from a volume (Source).
 *
 * This filter can be used to resample data, or convert one dataset form into
 * another. For example, a generic dataset can be probed with a volume
 * (three-dimensional vtkImageData), and then volume rendering techniques can
 * be used to visualize the results. Another example: a line or curve can be
 * used to probe data to produce x-y plots along that line or curve.
 *
 * This filter has been implemented to operate on generic datasets, rather
 * than the typical vtkDataSet (and subclasses). vtkGenericDataSet is a more
 * complex cousin of vtkDataSet, typically consisting of nonlinear,
 * higher-order cells. To process this type of data, generic cells are
 * automatically tessellated into linear cells prior to isocontouring.
 *
 * @sa
 * vtkGenericProbeFilter vtkProbeFilter vtkGenericDataSet
*/

#ifndef vtkGenericProbeFilter_h
#define vtkGenericProbeFilter_h

#include "vtkFiltersGenericModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class vtkIdTypeArray;
class vtkGenericDataSet;

class VTKFILTERSGENERIC_EXPORT vtkGenericProbeFilter : public vtkDataSetAlgorithm
{
public:
  static vtkGenericProbeFilter *New();
  vtkTypeMacro(vtkGenericProbeFilter,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify the point locations used to probe input. A generic dataset
   * type is assumed.
   */
  void SetSourceData(vtkGenericDataSet *source);
  vtkGenericDataSet *GetSource();
  //@}

  //@{
  /**
   * Get the list of point ids in the output that contain attribute data
   * interpolated from the source.
   */
  vtkGetObjectMacro(ValidPoints, vtkIdTypeArray);
  //@}

protected:
  vtkGenericProbeFilter();
  ~vtkGenericProbeFilter() override;

  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  int FillInputPortInformation(int, vtkInformation*) override;

  vtkIdTypeArray *ValidPoints;

private:
  vtkGenericProbeFilter(const vtkGenericProbeFilter&) = delete;
  void operator=(const vtkGenericProbeFilter&) = delete;
};

#endif
