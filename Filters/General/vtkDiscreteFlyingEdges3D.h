// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDiscreteFlyingEdges3D
 * @brief   generate isosurface from 3D image data (volume)
 *
 * vtkDiscreteFlyingEdges3D creates output representations of label maps
 * (e.g., segmented volumes) using a variation of the flying edges
 * algorithm. The input is a 3D image (volume( where each point is labeled
 * (integer labels are preferred to real values), and the output data is
 * polygonal data representing labeled regions. (Note that on output each
 * region [corresponding to a different contour value] is represented
 * independently; i.e., points are not shared between regions even if they
 * are coincident.)
 *
 * This filter is similar to but produces different results than the filter
 * vtkDiscreteMarchingCubes. This filter can produce output normals, and each
 * labeled region is completely disconnected from neighboring regions
 * (coincident points are not merged). Both algorithms interpolate edges at
 * the halfway point between vertices with different segmentation labels.
 *
 * See the paper "Flying Edges: A High-Performance Scalable Isocontouring
 * Algorithm" by Schroeder, Maynard, Geveci. Proc. of LDAV 2015. Chicago, IL.
 *
 * @warning
 * This filter is specialized to 3D volumes. This implementation can produce
 * degenerate triangles (i.e., zero-area triangles).
 *
 * @warning
 * See also vtkPackLabels which is a utility class for renumbering the labels
 * found in the input segmentation mask to contiguous forms of smaller type.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkDiscreteMarchingCubes vtkDiscreteFlyingEdges2D vtkDiscreteFlyingEdges3D
 * vtkPackLabels
 */

#ifndef vtkDiscreteFlyingEdges3D_h
#define vtkDiscreteFlyingEdges3D_h

#include "vtkContourValues.h"        // Passes calls through
#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;

class VTKFILTERSGENERAL_EXPORT vtkDiscreteFlyingEdges3D : public vtkPolyDataAlgorithm
{
public:
  static vtkDiscreteFlyingEdges3D* New();
  vtkTypeMacro(vtkDiscreteFlyingEdges3D, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Because we delegate to vtkContourValues.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set/Get the computation of normals. Normal computation is fairly
   * expensive in both time and storage. If the output data will be processed
   * by filters that modify topology or geometry, it may be wise to turn
   * Normals and Gradients off.
   */
  vtkSetMacro(ComputeNormals, vtkTypeBool);
  vtkGetMacro(ComputeNormals, vtkTypeBool);
  vtkBooleanMacro(ComputeNormals, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the computation of gradients. Gradient computation is fairly
   * expensive in both time and storage. Note that if ComputeNormals is on,
   * gradients will have to be calculated, but will not be stored in the
   * output dataset. If the output data will be processed by filters that
   * modify topology or geometry, it may be wise to turn Normals and
   * Gradients off.
   */
  vtkSetMacro(ComputeGradients, vtkTypeBool);
  vtkGetMacro(ComputeGradients, vtkTypeBool);
  vtkBooleanMacro(ComputeGradients, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the computation of scalars.
   */
  vtkSetMacro(ComputeScalars, vtkTypeBool);
  vtkGetMacro(ComputeScalars, vtkTypeBool);
  vtkBooleanMacro(ComputeScalars, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Indicate whether to interpolate other attribute data. That is, as the
   * isosurface is generated, interpolate all point attribute data across
   * the edge. This is independent of scalar interpolation, which is
   * controlled by the ComputeScalars flag.
   */
  vtkSetMacro(InterpolateAttributes, vtkTypeBool);
  vtkGetMacro(InterpolateAttributes, vtkTypeBool);
  vtkBooleanMacro(InterpolateAttributes, vtkTypeBool);
  ///@}

  /**
   * Set a particular contour value at contour number i. The index i ranges
   * between 0<=i<NumberOfContours.
   */
  void SetValue(int i, double value) { this->ContourValues->SetValue(i, value); }

  /**
   * Get the ith contour value.
   */
  double GetValue(int i) { return this->ContourValues->GetValue(i); }

  /**
   * Get a pointer to an array of contour values. There will be
   * GetNumberOfContours() values in the list.
   */
  double* GetValues() { return this->ContourValues->GetValues(); }

  /**
   * Fill a supplied list with contour values. There will be
   * GetNumberOfContours() values in the list. Make sure you allocate
   * enough memory to hold the list.
   */
  void GetValues(double* contourValues) { this->ContourValues->GetValues(contourValues); }

  /**
   * Set the number of contours to place into the list. You only really
   * need to use this method to reduce list size. The method SetValue()
   * will automatically increase list size as needed.
   */
  void SetNumberOfContours(int number) { this->ContourValues->SetNumberOfContours(number); }

  /**
   * Get the number of contours in the list of contour values.
   */
  vtkIdType GetNumberOfContours() { return this->ContourValues->GetNumberOfContours(); }

  /**
   * Generate numContours equally spaced contour values between specified
   * range. Contour values will include min/max range values.
   */
  void GenerateValues(int numContours, double range[2])
  {
    this->ContourValues->GenerateValues(numContours, range);
  }

  /**
   * Generate numContours equally spaced contour values between specified
   * range. Contour values will include min/max range values.
   */
  void GenerateValues(int numContours, double rangeStart, double rangeEnd)
  {
    this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
  }

  ///@{
  /**
   * Set/get which component of the scalar array to contour on; defaults to 0.
   */
  vtkSetMacro(ArrayComponent, int);
  vtkGetMacro(ArrayComponent, int);
  ///@}

protected:
  vtkDiscreteFlyingEdges3D();
  ~vtkDiscreteFlyingEdges3D() override;

  vtkTypeBool ComputeNormals;
  vtkTypeBool ComputeGradients;
  vtkTypeBool ComputeScalars;
  vtkTypeBool InterpolateAttributes;
  int ArrayComponent;
  vtkContourValues* ContourValues;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkDiscreteFlyingEdges3D(const vtkDiscreteFlyingEdges3D&) = delete;
  void operator=(const vtkDiscreteFlyingEdges3D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
