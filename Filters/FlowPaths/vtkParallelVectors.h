// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkParallelVectors
 * @brief   Compute polylines corresponding to locations where two vector fields
 *          are parallel
 *
 * vtkParallelVectors accepts a 3D dataset with two nodal 3-vector fields and
 * generates polylines along the paths where the vector fields are parallel.
 * This filter is an implementation of the concepts outlined in the following
 * article:
 *
 * R. Peikert and M. Roth, "The "Parallel Vectors" operator-a vector field
 * visualization primitive," Proceedings Visualization '99 (Cat. No.99CB37067),
 * San Francisco, CA, USA, 1999, pp. 263-532.
 *
 * @sa
 * vtkVortexCore
 */

#ifndef vtkParallelVectors_h
#define vtkParallelVectors_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#include "vtkNew.h" // for vtkNew

namespace detail
{
VTK_ABI_NAMESPACE_BEGIN
template <typename VArrayType, typename WArrayType>
class CollectValidCellSurfacePointsFunctor;
VTK_ABI_NAMESPACE_END
}

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSFLOWPATHS_EXPORT vtkParallelVectors : public vtkPolyDataAlgorithm
{
  template <typename, typename>
  friend class detail::CollectValidCellSurfacePointsFunctor;

public:
  static vtkParallelVectors* New();
  vtkTypeMacro(vtkParallelVectors, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the name of first vector field.
   */
  vtkSetStringMacro(FirstVectorFieldName);
  vtkGetStringMacro(FirstVectorFieldName);
  ///@}

  ///@{
  /**
   * Set/Get the name of second vector field.
   */
  vtkSetStringMacro(SecondVectorFieldName);
  vtkGetStringMacro(SecondVectorFieldName);
  ///@}

protected:
  vtkParallelVectors();
  ~vtkParallelVectors() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

  /**
   * Prefilter should resize the CriteriaArrays, initialize them and set their names.
   */
  virtual void Prefilter(vtkInformation*, vtkInformationVector**, vtkInformationVector*) {}
  virtual void Postfilter(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  virtual bool AcceptSurfaceTriangle(const vtkIdType surfaceSimplexIndices[3]);

  /**
   * Computes additional criteria to determine if a point should be added to a vortex core.
   * Criteria are returned in the criteria parameter.
   *
   * @note criterionArrayValues has the size of the number of the CriteriaArrays.
   */
  virtual bool ComputeAdditionalCriteria(const vtkIdType surfaceSimplexIndices[3], double s,
    double t, std::vector<double>& criterionArrayValues);

  /**
   * Contains the name of the first vector field to compare.
   */
  char* FirstVectorFieldName;

  /**
   * Contains the name of the second vector field to compare.
   */
  char* SecondVectorFieldName;

  // The arrays are used to store additional criteria related arrays with 1 component.
  // The size of this vector should be resized inside Prefilter.
  std::vector<vtkSmartPointer<vtkDoubleArray>> CriteriaArrays;

private:
  vtkParallelVectors(const vtkParallelVectors&) = delete;
  void operator=(const vtkParallelVectors&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
