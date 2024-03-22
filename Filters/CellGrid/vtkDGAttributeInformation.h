// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGAttributeInformation
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

#ifndef vtkDGAttributeInformation_h
#define vtkDGAttributeInformation_h

#include "vtkCellAttributeInformation.h"
#include "vtkVector.h" // For API.

VTK_ABI_NAMESPACE_BEGIN

class vtkDGCell;

class VTKFILTERSCELLGRID_EXPORT vtkDGAttributeInformation : public vtkCellAttributeInformation
{
public:
  vtkTypeMacro(vtkDGAttributeInformation, vtkCellAttributeInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkDGAttributeInformation* New();

  /// Return the polynomial order of the interpolant.
  int GetBasisOrder() const override { return this->BasisOrder; }

  /// Return the number of components of each basis function.
  ///
  /// For HGRAD function spaces, this is 1.
  /// For HCURL or HDIV function spaces, this is equal to the parametric dimension
  /// of the cell type in question.
  int GetBasisValueSize() const override { return this->BasisValueSize; }

  /// Return the number of basis functions for the cell type in question.
  int GetNumberOfBasisFunctions() const override { return this->NumberOfBasisFunctions; }

  /// Return the number of times each basis function should be applied to
  /// obtain an attribute value.
  ///
  /// For example, each HGRAD basis function for a hexahedron returns a single
  /// value per degree of freedom. However, when the output attribute has 3
  /// components (say, x, y, and z point coordinates), then the weighted sum
  /// of the basis functions must be applied 3 times (once for each point
  /// coordinate). This is how vector- or tensor-valued attributes may be
  /// composed from scalar basis functions.
  int GetDegreeOfFreedomSize() const override { return this->DegreeOfFreedomSize; }

  /// Return whether degrees of freedom are shared by multiple cells or not.
  ///
  /// This roughly translates to whether the attribute is allowed to express
  /// discontinuities at cell boundaries; when degrees of freedom are shared,
  /// continuity is presumed. Otherwise, the interpolation functions may not
  /// produce identical values where cell boundaries overlap.
  bool GetSharedDegreesOfFreedom() const override { return this->SharedDegreesOfFreedom; }

  /// Return the name of a basis function specific to the cell-metadata and
  /// cell-attribute.
  std::string GetBasisName() const override { return this->BasisName; }

  /// Given a cell type, return an abbreviated shape name.
  /// The returned value will always be a valid C identifier.
  /// When \a cellType is null, the returned string will be `None`.
  static std::string BasisShapeName(vtkDGCell* cellType);

  vtkSmartPointer<vtkCellAttributeCalculator> PrepareForGrid(
    vtkCellMetadata* cell, vtkCellAttribute* attribute) override;

protected:
  vtkDGAttributeInformation() = default;
  ~vtkDGAttributeInformation() override = default;

  int BasisOrder{ 0 };
  int BasisValueSize{ 1 };
  int NumberOfBasisFunctions{ 1 };
  int DegreeOfFreedomSize{ 1 };
  bool SharedDegreesOfFreedom{ true };
  std::string BasisName;

private:
  vtkDGAttributeInformation(const vtkDGAttributeInformation&) = delete;
  void operator=(const vtkDGAttributeInformation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGAttributeInformation_h
