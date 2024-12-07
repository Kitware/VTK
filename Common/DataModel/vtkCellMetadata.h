// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellMetadata
 * @brief   Metadata for a particular type of cell (finite element).
 *
 * This is a base class for metadata on cell types held by a vtkCellGrid
 * instance (not for subclasses of vtkCell).
 * A vtkCellGrid holds one instance of a vtkCellMetadata-subclass for
 * each *type* of cell present in the grid.
 *
 * This class intentionally provides very little functionality; instead,
 * it is intended to serve as a key or index into a set of registered
 * responder classes which are able respond to queries about cells of
 * this type. This pattern makes it possible to extend VTK with both
 * new cell types and new types of queries for existing cell types.
 *
 * @sa vtkCellGrid
 */

#ifndef vtkCellMetadata_h
#define vtkCellMetadata_h

#include "vtkCellGridResponders.h"    // For ivar.
#include "vtkCommonDataModelModule.h" // for export macro
#include "vtkInherits.h"              // for vtk::Inherits<>()
#include "vtkNew.h"                   // for queries
#include "vtkObject.h"
#include "vtkSmartPointer.h" // for constructor return values
#include "vtkStringToken.h"  // for vtkStringToken::Hash
#include "vtkTypeName.h"     // for vtk::TypeName<>()

#include <token/Singletons.h> // Increment Schwarz counter for initialization.

#include <functional>
#include <set>
#include <unordered_map>
#include <unordered_set>

VTK_ABI_NAMESPACE_BEGIN
class vtkCellGrid;
class vtkCellGridQuery;
class vtkDataSetAttributes;
class vtkCellAttribute;

class VTKCOMMONDATAMODEL_EXPORT vtkCellMetadata : public vtkObject
{
public:
  using CellTypeId = vtkStringToken::Hash;
  using DOFType = vtkStringToken;
  using MetadataConstructor = std::function<vtkSmartPointer<vtkCellMetadata>(vtkCellGrid*)>;
  using ConstructorMap = std::unordered_map<vtkStringToken, MetadataConstructor>;

  vtkTypeMacro(vtkCellMetadata, vtkObject);
  vtkInheritanceHierarchyBaseMacro(vtkCellMetadata);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Register a subclass of vtkCellMetadata.
   */
  template <typename Subclass>
  static bool RegisterType()
  {
    vtkStringToken name = vtk::TypeName<Subclass>();
    auto status = vtkCellMetadata::Constructors().insert(std::make_pair(name,
      [](vtkCellGrid* grid)
      {
        auto result = vtkSmartPointer<Subclass>::New();
        if (result)
        {
          result->SetCellGrid(grid);
        }
        return result;
      }));
    return status.second; // true if insertion occurred.
  }

  template <typename Subclass>
  static vtkSmartPointer<Subclass> NewInstance(vtkCellGrid* grid = nullptr)
  {
    vtkStringToken name = vtk::TypeName<Subclass>();
    auto metadata = vtkCellMetadata::NewInstance(name, grid);
    vtkSmartPointer<Subclass> result(Subclass::SafeDownCast(metadata));
    return result;
  }

  static vtkSmartPointer<vtkCellMetadata> NewInstance(
    vtkStringToken className, vtkCellGrid* grid = nullptr);

  static std::unordered_set<vtkStringToken> CellTypes();

  /**
   * Return a hash of the cell type.
   *
   * WARNING: If you change this method, you must also update
   * vtkCellGrid::AddCellMetadata() and vtkCellGrid::GetCellsOfType().
   */
  CellTypeId Hash() { return vtkStringToken(this->GetClassName()).GetId(); }

  /**
   * Set the vtkCellGrid holding DOF arrays required by this cell.
   *
   * If the given vtkCellGrid does not have the required degrees
   * of freedom arrays (as provided by GetDOFTypes()), then this
   * method will return false.
   *
   * If this method returns true, the \a parent grid will have
   * incremented the reference count of \a this class instance.
   */
  virtual bool SetCellGrid(vtkCellGrid* parent);

  /// Return the parent vtkCellGrid that owns this instance (or nullptr).
  vtkGetObjectMacro(CellGrid, vtkCellGrid);

  /// Return the number of cells of this type in the parent cell-grid object.
  /// Subclasses must override this method.
  virtual vtkIdType GetNumberOfCells() { return 0; }

  /**
   * Respond to a query on cells of this type.
   *
   * This returns true on success and false on failure.
   * If no responder has been registered for queries of this type,
   * that is considered a failure.
   */
  bool Query(vtkCellGridQuery* query);

  /**
   * Copy \a other into this instance (which must be of the same type).
   *
   * These methods exist so subclasses know when they are being copied;
   * the base class has no data to copy, so both ShallowCopy and DeepCopy
   * do nothing.
   */
  virtual void ShallowCopy(vtkCellMetadata* vtkNotUsed(other)) {}
  virtual void DeepCopy(vtkCellMetadata* vtkNotUsed(other)) {}

  /// Return the set of registered responder types.
  static vtkCellGridResponders* GetResponders();

  /// Clear all of the registered responders.
  static void ClearResponders();

  /// Return the set of registered responder types.
  ///
  /// This method is not static on purpose; even when VTK is compiled
  /// as a set of static libraries, calling this method will always
  /// return the proper class-static member. It returns the same value
  /// as vtkCellMetadata::GetResponders() but will produce the correct
  /// result across static-library boundaries (unlike GetResponders above).
  vtkCellGridResponders* GetCaches();

protected:
  vtkCellMetadata() = default;
  ~vtkCellMetadata() override;

  vtkCellGrid* CellGrid{ nullptr };

  static ConstructorMap& Constructors();

private:
  vtkCellMetadata(const vtkCellMetadata&) = delete;
  void operator=(const vtkCellMetadata&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
