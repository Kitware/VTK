// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDiscreteFlyingEdgesClipper2D
 * @brief   generate filled regions from segmented 2D image data
 *
 * vtkDiscreteFlyingEdgesClipper2D creates filled polygons from a label map
 * (e.g., segmented image) using a variation of the flying edges algorithm
 * adapted for 2D clipping. The input is a 2D image where each pixel is
 * labeled (integer labels are preferred to real values), and the output data
 * is polygonal data representing labeled regions. (Note that on output each
 * region [corresponding to a different contour value] may share points on a
 * shared boundary.)
 *
 * While this filter is similar to a contouring operation, label maps do not
 * provide continuous function values meaning that usual interpolation along
 * edges is not possible. Instead, when the edge endpoints are labeled in
 * differing regions, the edge is split at its midpoint. In addition, besides
 * producing intersection points at the mid-point of edges, the filter may
 * also generate points interior to the pixel cells. For example, if the four
 * vertices of a pixel cell are labeled with different regions, then an
 * interior point is created and four rectangular "regions" are produced.
 *
 * Note that one nice feature of this filter is that algorithm execution
 * occurs only one time no matter the number of contour values. In many
 * contouring-like algorithms, each separate contour value requires an
 * additional algorithm execution with a new contour value. So in this filter
 * large numbers of contour values do not significantly affect overall speed.
 *
 * @warning This filter is specialized to 2D images.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkSurfaceNets2D vtkDiscreteFlyingEdges2D vtkDiscreteMarchingCubes
 * vtkContourLoopExtraction vtkFlyingEdges2D vtkFlyingEdges3D
 */

#ifndef vtkDiscreteFlyingEdgesClipper2D_h
#define vtkDiscreteFlyingEdgesClipper2D_h

#include "vtkContourValues.h"        // Needed for direct access to ContourValues
#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;

class VTKFILTERSGENERAL_EXPORT vtkDiscreteFlyingEdgesClipper2D : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, printing, and type information.
   */
  static vtkDiscreteFlyingEdgesClipper2D* New();
  vtkTypeMacro(vtkDiscreteFlyingEdgesClipper2D, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * The modified time is a function of the contour values because we delegate to
   * vtkContourValues.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Set a particular contour value at contour number i. The index i ranges
   * between 0 <= i <NumberOfContours. (Note: while contour values are
   * expressed as doubles, the underlying scalar data may be a different
   * type. During execution the contour values are static cast to the type of
   * the scalar values.)
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

  ///@{
  /**
   * Generate numContours equally spaced contour values between the specified
   * range. Contour values will include min/max range values.
   */
  void GenerateValues(int numContours, double range[2])
  {
    this->ContourValues->GenerateValues(numContours, range);
  }
  void GenerateValues(int numContours, double rangeStart, double rangeEnd)
  {
    this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
  }
  ///@}

  ///@{
  /**
   * Option to set the cell scalars of the output. The scalars will be the
   * contour values. By default this flag is on.
   */
  vtkSetMacro(ComputeScalars, vtkTypeBool);
  vtkGetMacro(ComputeScalars, vtkTypeBool);
  vtkBooleanMacro(ComputeScalars, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/get which component of a multi-component scalar array to contour on;
   * defaults to 0.
   */
  vtkSetMacro(ArrayComponent, int);
  vtkGetMacro(ArrayComponent, int);
  ///@}

protected:
  vtkDiscreteFlyingEdgesClipper2D();
  ~vtkDiscreteFlyingEdgesClipper2D() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkContourValues* ContourValues;
  vtkTypeBool ComputeScalars;
  int ArrayComponent;

private:
  vtkDiscreteFlyingEdgesClipper2D(const vtkDiscreteFlyingEdgesClipper2D&) = delete;
  void operator=(const vtkDiscreteFlyingEdgesClipper2D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
