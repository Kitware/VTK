// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkFlyingEdges2D
 * @brief   generate isoline(s) from a structured points (image) dataset
 *
 * vtkFlyingEdges2D is a reference implementation of the 2D version of the
 * flying edges algorithm. It is designed to be highly scalable (i.e.,
 * parallelizable) for large data. It implements certain performance
 * optimizations including computational trimming to rapidly eliminate
 * processing of data regions, packed bit representation of case table
 * values, single edge intersection, elimination of point merging, and
 * elimination of any reallocs (due to dynamic data insertion). Note that
 * computational trimming is a method to reduce total computational cost in
 * which partial computational results can be used to eliminate future
 * computations.
 *
 * This is a four-pass algorithm. The first pass processes all x-edges and
 * builds x-edge case values (which, when the two x-edges defining a pixel
 * are combined, are equivalent to vertex-based case table except edge-based
 * approaches are separable to parallel computing). Next x-pixel rows are
 * processed to gather information from y-edges (basically to count the
 * number of edge intersections and lines generated). In the third pass a
 * prefix sum is used to count and allocate memory for the output
 * primitives. Finally in the fourth pass output primitives are generated into
 * pre-allocated arrays. This implementation uses pixel cell axes (a x-y dyad
 * located at the pixel origin) to ensure that each edge is intersected at
 * most one time.
 *
 * See the paper "Flying Edges: A High-Performance Scalable Isocontouring
 * Algorithm" by Schroeder, Maynard, Geveci. Proc. of LDAV 2015. Chicago, IL.
 *
 * @warning
 * This filter is specialized to 2D images. This implementation can produce
 * degenerate line segments (i.e., zero-length line segments).
 *
 * @warning
 * If you are interested in extracting segmented regions from a label mask,
 * consider using vtkDiscreteFlyingEdges2D.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkFlyingEdges3D vtkContourFilter vtkSynchronizedTemplates2D
 * vtkMarchingSquares vtkDiscreteFlyingEdges2D
 */

#ifndef vtkFlyingEdges2D_h
#define vtkFlyingEdges2D_h

#include "vtkContourValues.h"     // Needed for direct access to ContourValues
#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;

class VTKFILTERSCORE_EXPORT vtkFlyingEdges2D : public vtkPolyDataAlgorithm
{
public:
  static vtkFlyingEdges2D* New();
  vtkTypeMacro(vtkFlyingEdges2D, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Because we delegate to vtkContourValues.
   */
  vtkMTimeType GetMTime() override;

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
   * Option to set the point scalars of the output.  The scalars will be the
   * iso value of course.  By default this flag is on.
   */
  vtkSetMacro(ComputeScalars, vtkTypeBool);
  vtkGetMacro(ComputeScalars, vtkTypeBool);
  vtkBooleanMacro(ComputeScalars, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/get which component of the scalar array to contour on; defaults to 0.
   */
  vtkSetMacro(ArrayComponent, int);
  vtkGetMacro(ArrayComponent, int);
  ///@}

protected:
  vtkFlyingEdges2D();
  ~vtkFlyingEdges2D() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;
  vtkContourValues* ContourValues;

  vtkTypeBool ComputeScalars;
  int ArrayComponent;

private:
  vtkFlyingEdges2D(const vtkFlyingEdges2D&) = delete;
  void operator=(const vtkFlyingEdges2D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
