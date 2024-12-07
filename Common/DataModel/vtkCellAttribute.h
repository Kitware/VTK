// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellAttribute
 * @brief   A function defined over the physical domain of a vtkCellGrid.
 *
 * This is a base class for attributes (functions) defined on the space
 * discretized by a vtkCellGrid. A vtkCellAttribute class must handle
 * cells of all types present in the grid.
 *
 * @sa vtkCellGrid
 */

#ifndef vtkCellAttribute_h
#define vtkCellAttribute_h

#include "vtkCommonDataModelModule.h" // for export
#include "vtkObject.h"
#include "vtkScalarsToColors.h" // for colormap
#include "vtkSmartPointer.h"    // for maps
#include "vtkStringToken.h"     // for vtkStringToken::Hash

#include <unordered_map>

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
class vtkCellAttribute;
class vtkCellGrid;
class vtkDataSetAttributes;

class VTKCOMMONDATAMODEL_EXPORT vtkCellAttribute : public vtkObject
{
public:
  /// A dictionary of arrays indexed by their roles in interpolation.
  using ArraysForCellType = std::unordered_map<vtkStringToken, vtkSmartPointer<vtkAbstractArray>>;

  struct CellTypeInfo
  {
    // The array-group name holding shared degree-of-freedom (DOF)
    // data if the attribute is shared. This is invalid for
    // discontinuous attributes
    vtkStringToken DOFSharing;
    /// The function space used to interpolate values of the attribute
    /// on cells of the matching type.
    ///
    /// Examples include "HGRAD", "HDIV", and "HCURL".
    vtkStringToken FunctionSpace;
    /// The interpolation scheme of the attribute on cells of the
    /// matching type.
    ///
    /// For polynomial interpolants, this is often used to
    /// indicate whether the basis covers the entire polynomial
    /// space or a particular subset of it.
    /// For example, serendipitity elements are often marked
    /// incomplete since they do not cover the entire space
    /// along each parametric coordinate axis.
    ///
    /// Examples include "I"ncomplete, "C"omplete, and "F"ull.
    vtkStringToken Basis;
    /// The interpolation order of the attribute on cells of the
    /// matching type.
    int Order;
    /// A dictionary of arrays indexed by their roles in interpolation.
    ///
    /// This is used by render-responders and interpolation calculators
    /// to interpolate attribute values.
    ArraysForCellType ArraysByRole;

    /// Return an array (or nullptr) given a role, cast to \a ArrayType.
    template <typename ArrayType>
    ArrayType* GetArrayForRoleAs(vtkStringToken role) const
    {
      auto it = this->ArraysByRole.find(role);
      if (it == this->ArraysByRole.end())
      {
        return nullptr;
      }
      return ArrayType::SafeDownCast(it->second);
    }

    /// Comparator used to test inequality.
    bool operator!=(const CellTypeInfo& other) const;

    /// Explicitly include assignment and copy-construction.
    CellTypeInfo() = default;
    CellTypeInfo(const CellTypeInfo& other) = default;
    CellTypeInfo& operator=(const CellTypeInfo& other) = default;
  };

  using Arrays = std::unordered_map<vtkStringToken, CellTypeInfo>;

  vtkTypeMacro(vtkCellAttribute, vtkObject);
  static vtkCellAttribute* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Return the (user-presentable) name of this attribute.
  virtual vtkStringToken GetName() const { return this->Name; }

  /**
   * Return a (cell-grid-assigned) integer identifier for this attribute.
   *
   * Do not call SetId() unless you represent a vtkCellGrid taking
   * ownership of this attribute instance.
   */
  vtkGetMacro(Id, int);
  vtkSetMacro(Id, int);

  /**
   * Return a token identifying the space containing all field values.
   *
   * Currently, this is just a free-form string but in the future
   * we may adopt a more rigorous standard.
   *
   * Some suggested values
   * + "ℝ¹" – single (scalar) values over the real numbers.
   * + "ℝ¹+" – single (scalar) values over the non-negative real numbers.
   * + "ℝ²" – 2-d vector values over the real numbers.
   * + "ℝ³" – 3-d vector values over the real numbers.
   * + "𝕊³" – points inside a unit 3-dimensional ball.
   * + "S²" – points on the surface of a unit 3-dimensional sphere.
   * + "SO(3)" – rotation matrices.
   * + "SU(2)" – special unitary group (homeomorphic to SO(3)).
   *
   * Note that the space might imply the number of components but
   * it also specifies how users should interpret operations such
   * as addition and/or multipliciation, especially in the case of
   * transforms applied to the domain.
   *
   * If you wish to encode/decode the exponents for a space like ℝ³,
   * see vtkCellAttribute::EncodeExponent/DecodeExponent().
   */
  virtual vtkStringToken GetSpace() const { return this->Space; }

  /**
   * Return the number of components this function provides
   * at each point in space.
   *
   * This should be consistent with the value returned by GetSpace().
   */
  virtual int GetNumberOfComponents() const { return this->NumberOfComponents; }

  /**
   * Initialize an attribute.
   *
   * Never call this method after a cell-attribute has been inserted
   * into an unordered container as it will change the reported hash,
   * which can cause crashes later.
   */
  virtual bool Initialize(vtkStringToken name, vtkStringToken space, int numberOfComponents);

  /**
   * Hash this attribute so it can be inserted into unordered containers.
   *
   * The hash includes the name, type, space, and number of components.
   */
  virtual vtkStringToken::Hash GetHash() const;

  /**
   * Return the arrays required to evaluate this attribute on
   * cells of the given type.
   */
  virtual CellTypeInfo GetCellTypeInfo(vtkStringToken cellType) const;

  /**
   * Return an array for the given cell type and role.
   *
   * This is a convenience for use in language bindings like Python.
   */
  vtkAbstractArray* GetArrayForCellTypeAndRole(
    vtkStringToken cellType, vtkStringToken arrayRole) const;

  /**
   * Set the arrays required to evaluate this attribute on cells
   * of the given type.
   *
   * TODO: Instead of accepting a fixed type (CellTypeInfo), this method
   *       should be templated to accept any object so that cell types can put
   *       whatever state is needed here in order to assist in evaluating the
   *       attribute.
   */
  virtual bool SetCellTypeInfo(vtkStringToken cellType, const CellTypeInfo& cellTypeInfo);

  /// Return a default colormap associated with the attribute.
  vtkScalarsToColors* GetColormap() const { return this->Colormap; }
  bool SetColormap(vtkScalarsToColors* colormap);

  /**
   * Copy data from an \a other attribute instance into this instance.
   *
   * The colormap is copied by reference when shallow-copying and
   * a cloned instance is created when deep-copying.
   * The shallow-copy method provides an option to omit copying any
   * arrays related to the attribute, while the deep-copy method
   * provides a map to look up replacements for arrays.
   *
   * Note that the list of array pointers is copied by reference
   * (even when deep-copying a vtkCellAttribute) unless you provide
   * DeepCopy() with a map of \a arrayRewrites pointers. The
   * vtkCellGrid owns the arrays, not the vtkCellAttribute, so the
   * when deep-copying a vtkCellGrid, it will have a map of array
   * copies it has created. If any array is mentioned in \a AllArrays
   * and is not present in \a arrayRewrites, it is copied by reference.
   */
  virtual void ShallowCopy(vtkCellAttribute* other, bool copyArrays = true);
  virtual void DeepCopy(vtkCellAttribute* other,
    const std::map<vtkAbstractArray*, vtkAbstractArray*>& arrayRewrites = {});

  /// Given a space string (e.g., ℝ³⁻ or ℚ¹), decode the base (e.g., ℝ resp. ℚ),
  /// exponent (e.g., 3 resp. 1), and halfspace indicator (-1 resp. 0).
  ///
  /// If parsing fails, return false.
  ///
  /// The halfspace indicator is either -1 (indicating only the negative halfspace),
  /// +1 (indicating only the positive halfspace), or 0 (indicating no restriction).
  ///
  /// If \a quiet is true, no parse errors will be printed. This is used to ensure
  /// tests with expected errors do not fail; you should generally pass false.
  static bool DecodeSpace(
    const std::string& space, std::string& base, double& exp, int& halfspace, bool quiet = false);

  /// Return a space string given a description of it via \a base, \a exp, and \a halfspace.
  static std::string EncodeSpace(const std::string& base, unsigned int, int halfspace = 0);

protected:
  vtkCellAttribute() = default;
  ~vtkCellAttribute() override = default;

  vtkStringToken Name;
  vtkStringToken Space;
  int NumberOfComponents = 1;
  Arrays AllArrays;
  int Id = -1;
  vtkSmartPointer<vtkScalarsToColors> Colormap;

private:
  vtkCellAttribute(const vtkCellAttribute&) = delete;
  void operator=(const vtkCellAttribute&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
