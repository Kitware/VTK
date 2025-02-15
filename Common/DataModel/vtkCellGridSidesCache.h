// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkCellGridSidesCache_h
#define vtkCellGridSidesCache_h

#include "vtkObject.h"

#include "vtkCommonDataModelModule.h" // For export macro.
#include "vtkHashCombiner.h"          // For templated AddSide() method.
#include "vtkStringToken.h"           // For API.

#include <functional>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

class vtkIdTypeArray;

/**
 * @class   vtkCellGridSidesCache
 * @brief   Hold a map from hash-ids (representing sides of cells of multiple types)
 *          to details on the cells that claim the corresponding side.
 *
 * This class is created by filters such as vtkCellGridComputeSides and
 * vtkCellGridExtractCrinkle; it can be reused by the same filter and
 * any others that process the same input (since it is stored in a
 * cache available to them).
 */
class VTKCOMMONDATAMODEL_EXPORT vtkCellGridSidesCache : public vtkObject
{
public:
  static vtkCellGridSidesCache* New();
  vtkTypeMacro(vtkCellGridSidesCache, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Records held by a hash-entry that represent the side of one cell.
  ///
  /// All instances of Side owned by a single hash-entry have the same
  /// hash but correspond to distinct sides of different cells.
  struct Side
  {
    /// The type of cell whose side is hashed.
    vtkStringToken CellType;
    /// The shape of the side being hashed.
    vtkStringToken SideShape;
    /// The degree of freedom starting the hash sequence.
    vtkIdType DOF;
    /// The ID of the side being hashed.
    int SideId;

    /// Compare side-hashes to allow set insertion.
    bool operator<(const Side& other) const
    {
      return (this->CellType < other.CellType) ||
        (this->CellType == other.CellType &&
          ((this->DOF < other.DOF) || (this->DOF == other.DOF && this->SideId < other.SideId)));
    }
  };

  /// Each hash entry corresponds to one or more sides of one or more cells.
  struct Entry
  {
    std::set<Side> Sides;
  };

  /// Return the map of hashed side information.
  std::unordered_map<std::size_t, Entry>& GetHashes() { return this->Hashes; }

  /// Compute the hash of a side (but do not insert a side into the map).
  template <typename C, typename T = typename C::value_type>
  std::size_t HashSide(vtkStringToken shape, const C& conn)
  {
    std::size_t ss = 0;
    std::size_t NN = conn.size();
    if (NN == 0)
    {
      return 0;
    }

    T smin = conn[0];
    for (std::size_t jj = 1; jj < NN; ++jj)
    {
      if (conn[jj] < smin)
      {
        smin = conn[jj];
        ss = jj;
      }
    }
    bool forward = conn[(ss + 1) % NN] > conn[(ss + NN - 1) % NN];

    std::size_t hashedValue = std::hash<std::size_t>{}(NN);
    vtkHashCombiner()(hashedValue, shape.GetId());
    // std::cout << "Hash(" << (forward ? "F" : "R") << ")";
    if (forward)
    {
      for (std::size_t ii = 0; ii < NN; ++ii)
      {
        std::size_t hashedToken = std::hash<T>{}(conn[(ss + ii) % NN]);
        vtkHashCombiner()(hashedValue, hashedToken);
        // std::cout << " " << conn[(ss + ii) % NN];
      }
    }
    else // backward
    {
      for (std::size_t ii = 0; ii < NN; ++ii)
      {
        std::size_t hashedToken = std::hash<T>{}(conn[(ss + NN - ii) % NN]);
        // hashedValue = hashedValue ^ (hashedToken << (ii + 1));
        vtkHashCombiner()(hashedValue, hashedToken);
        // std::cout << " " << conn[(ss + NN - ii) % NN];
      }
    }
    // std::cout << " = " << std::hex << hashedValue << std::dec << "\n";
    return hashedValue;
  }

  /// Add a \a side with the given \a shape and connectivity to the request's state.
  ///
  /// The \a shape, \a conn size, and \a conn entries are hashed together into a key
  /// which is mapped to a set of all the matching sides.
  /// The \a cellType and \a cell ID are also stored with each matching side; these
  /// are used during Finalize() to generate the output map-of-maps returned
  /// by GetSides() so that the sides are reported by cell type, cell ID, and then side ID.
  ///
  /// Note that the \a conn entries are hashed in a particular canonical order so
  /// that the same hash is generated for sides with point IDs that have been shifted
  /// and/or reversed.
  /// The hash always starts at the smallest entry of \a conn and goes
  /// in the direction that has the largest next entry.
  /// Examples:
  ///   (3, 2, 0, 1) → starts at index 2 (0) and hashes backwards: (0, 2, 3, 1)
  ///   (4, 5, 6, 7) → starts at index 0 (4) and hashes backwards: (4, 7, 6, 5)
  ///   (7, 3, 6, 2) → starts at index 3 (2) and hashes forwards:  (2, 7, 3, 6)
  //
  /// By storing the \a cellType, we avoid requiring a global-to-local cell numbering
  /// in vtkCellGrid instances (as vtkPolyData incurs) which may hold multiple types of cells.
  //@{
  template <typename C, typename T = typename C::value_type>
  void AddSide(
    vtkStringToken cellType, vtkIdType cell, int side, vtkStringToken shape, const C& conn)
  {
    auto hashedValue = this->HashSide(shape, conn);
    this->Hashes[hashedValue].Sides.insert(Side{ cellType, shape, cell, side });
  }
  //@}

  /// Empty the cache of all hashes.
  void Initialize();

protected:
  vtkCellGridSidesCache() = default;
  ~vtkCellGridSidesCache() override = default;

  std::unordered_map<std::size_t, Entry> Hashes;

private:
  vtkCellGridSidesCache(const vtkCellGridSidesCache&) = delete;
  void operator=(const vtkCellGridSidesCache&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridSidesCache_h
