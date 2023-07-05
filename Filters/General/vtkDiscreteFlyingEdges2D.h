// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDiscreteFlyingEdges2D
 * @brief   generate isoline(s) from 2D image data
 *
 * vtkDiscreteFlyingEdges2D creates output representations of label maps
 * (e.g., segmented images) using a variation of the flying edges
 * algorithm. The input is a 2D image where each point is labeled (integer
 * labels are preferred to real values), and the output data is polygonal
 * data representing labeled regions. (Note that on output each region
 * [corresponding to a different contour value] is represented independently;
 * i.e., points are not shared between regions even if they are coincident.)
 *
 * @warning
 * This filter is specialized to 2D images. This implementation can produce
 * degenerate line segments (i.e., zero-length line segments).
 *
 * @warning
 * Use vtkContourLoopExtraction if you wish to create polygons from the line
 * segments.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkDiscreteMarchingCubes vtkContourLoopExtraction
 */

#ifndef vtkDiscreteFlyingEdges2D_h
#define vtkDiscreteFlyingEdges2D_h

#include "vtkContourValues.h"        // Needed for direct access to ContourValues
#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;

class VTKFILTERSGENERAL_EXPORT vtkDiscreteFlyingEdges2D : public vtkPolyDataAlgorithm
{
public:
  /**
   * Standard methods for instantiation, printing, and type information.
   */
  static vtkDiscreteFlyingEdges2D* New();
  vtkTypeMacro(vtkDiscreteFlyingEdges2D, vtkPolyDataAlgorithm);
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

  ///@{
  /**
   * Generate numContours equally spaced contour values between specified
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
   * Option to set the point scalars of the output.  The scalars will be the
   * label values.  By default this flag is on.
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
  vtkDiscreteFlyingEdges2D();
  ~vtkDiscreteFlyingEdges2D() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkContourValues* ContourValues;
  vtkTypeBool ComputeScalars;
  int ArrayComponent;

private:
  vtkDiscreteFlyingEdges2D(const vtkDiscreteFlyingEdges2D&) = delete;
  void operator=(const vtkDiscreteFlyingEdges2D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
