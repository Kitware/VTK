// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellAttributeInformation
 * @brief   Provide information about a cell attribute's basis functions on one cell type.
 *
 * Note that some of the methods may be unable to return meaningful
 * information about some attribute types; this class has methods
 * that may be specific to subclasses of vtkDGCell. If this API does
 * not fit your cell type or attribute type; do not register a
 * calculator subclass of this type.
 * Instead, it is up to consuming code to ensure a non-null calculator
 * is returned.
 *
 * This calculator assumes interpolation is performed as a weighted sum
 * of basis functions evaluated at some parameter value.
 *
 * \f[
 *    f_j = \sum_i B_i(r) w_{i,j}
 * \f]
 *
 * where \f$i\in[0, M - 1]\f$ and \f$j\in[0,N - 1[\f$.
 * + \f$M\f$ is GetNumberOfBasisFunctions().
 * + \f$N\f$ is NumberOfComponentsPerDegreeOfFreedom().
 */

#ifndef vtkCellAttributeInformation_h
#define vtkCellAttributeInformation_h

#include "vtkCellAttributeCalculator.h"
#include "vtkFiltersCellGridModule.h" // For export macro.
#include "vtkVector.h"                // For API.

#include <array>
#include <string>

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSCELLGRID_EXPORT vtkCellAttributeInformation : public vtkCellAttributeCalculator
{
public:
  vtkTypeMacro(vtkCellAttributeInformation, vtkCellAttributeCalculator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Return the polynomial order of the interpolant.
  ///
  /// Subclasses must override this method to perform evaluation.
  virtual int GetBasisOrder() const = 0;

  /// Return the number of basis functions for the cell type in question.
  virtual int GetNumberOfBasisFunctions() const = 0;

  /// Return the number of components generated for each basis function.
  ///
  /// For HGRAD function spaces, this is 1.
  /// For HCURL or HDIV function spaces, this is equal to the parametric dimension
  /// of the cell type in question.
  virtual int GetBasisValueSize() const = 0;

  /// Return the number of times each basis function should be applied to
  /// obtain an attribute value.
  ///
  /// For example, each HGRAD basis function for a hexahedron returns a single
  /// value per degree of freedom. However, when the output attribute has 3
  /// components (say, x, y, and z point coordinates), then the weighted sum
  /// of the basis functions must be applied 3 times (once for each point
  /// coordinate). This is how vector- or tensor-valued attributes may be
  /// composed from scalar basis functions.
  virtual int GetDegreeOfFreedomSize() const = 0;

  /// Return whether degrees of freedom are shared by multiple cells or not.
  ///
  /// This roughly translates to whether the attribute is allowed to express
  /// discontinuities at cell boundaries; when degrees of freedom are shared,
  /// continuity is presumed. Otherwise, the interpolation functions may not
  /// produce identical values where cell boundaries overlap.
  virtual bool GetSharedDegreesOfFreedom() const = 0;

  /// Return a name for a basis function specific to both the cell-metadata
  /// and the cell-attribute type.
  ///
  /// This is used by some render responders and interpolation calculators
  /// to choose a function to invoke.
  virtual std::string GetBasisName() const = 0;

protected:
  vtkCellAttributeInformation() = default;
  ~vtkCellAttributeInformation() override = default;

private:
  vtkCellAttributeInformation(const vtkCellAttributeInformation&) = delete;
  void operator=(const vtkCellAttributeInformation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellAttributeInformation_h
