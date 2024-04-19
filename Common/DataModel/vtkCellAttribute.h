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
  using ArraysForCellType = std::unordered_map<vtkStringToken, vtkSmartPointer<vtkAbstractArray>>;
  using Arrays = std::unordered_map<vtkStringToken, ArraysForCellType>;

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
   * Return a (user-presentable) type for this attribute.
   *
   * The type should reflect the nature of the function and
   * may reflect the nature of the cell shapes supported.
   *
   * The type is distinct from the space in which values reside;
   * instead it describes the mathematical technique used to
   * interpolate values (e.g., "rational spline", "polynomial",
   * "partition of unity", "stochastic", etc.), behavior at cell
   * boundaries, and other relevant information.
   * For example, a quadratic field that allows discontinuities
   * at cell boundaries and uses H(Grad) Lagrange interpolation
   * of arbitrary order (i.e., order may differ per cell) might
   * return "DG HGRAD [CI]n", where "n" indicates the integration
   * order is arbitrary. The "C" or "I" preceding the order
   * indicates the basis is "complete (C)" or "incomplete (I)."
   *
   * Currently, this is just a free-form string but in the future
   * we may adopt a more rigorous standard.
   */
  virtual vtkStringToken GetAttributeType() const { return this->AttributeType; }

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
  virtual bool Initialize(vtkStringToken name, vtkStringToken attributeType, vtkStringToken space,
    int numberOfComponents);

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
  virtual ArraysForCellType GetArraysForCellType(vtkStringToken cellType) const;

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
   * TODO: Instead of accepting a fixed type (ArraysForCellType), this method
   *       should be templated to accept any object so that cell types can put
   *       whatever state is needed here in order to assist in evaluating the
   *       attribute.
   */
  virtual bool SetArraysForCellType(vtkStringToken cellType, const ArraysForCellType& arrays);

  /// Return a default colormap associated with the attribute.
  vtkScalarsToColors* GetColormap() const { return this->Colormap; }
  bool SetColormap(vtkScalarsToColors* colormap);

  /**
   * Copy data from an \a other attribute instance into this instance.
   *
   * Currently, the only difference between shallow and deep copies is
   * that the colormap is copied by reference when shallow-copying and
   * a cloned instance is created when deep-copying.
   *
   * Note that the list of array pointers is copied by reference
   * (even when deep-copying a vtkCellAttribute) unless you provide
   * DeepCopy() with a map of \a arrayRewrites pointers. The
   * vtkCellGrid owns the arrays, not the vtkCellAttribute, so the
   * when deep-copying a vtkCellGrid, it will have a map of array
   * copies it has created. If any array is mentioned in \a AllArrays
   * and is not present in \a arrayRewrites, it is copied by reference.
   */
  virtual void ShallowCopy(vtkCellAttribute* other);
  virtual void DeepCopy(vtkCellAttribute* other,
    const std::map<vtkAbstractArray*, vtkAbstractArray*>& arrayRewrites = {});

protected:
  vtkCellAttribute() = default;
  ~vtkCellAttribute() override = default;

  vtkStringToken Name;
  vtkStringToken AttributeType;
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
