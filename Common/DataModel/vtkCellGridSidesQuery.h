// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridSidesQuery
 * @brief   Compute external faces of a cell-grid.
 */

#ifndef vtkCellGridSidesQuery_h
#define vtkCellGridSidesQuery_h

#include "vtkCellGridQuery.h"

#include "vtkStringToken.h" // For API.

#include <functional>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

class vtkIdTypeArray;

/**\brief A cell-grid query for enumerating sides of cells.
 *
 * As responders invoke the AddSides() method on this query,
 * entries are added to storage indicating the number of times
 * a given shape + connectivity appear. After the query is
 * finalized, calling GetSides() will return a map holding
 * sets of sides organized by cell ID for any shape + connectivity
 * repeated an odd number of times.
 */
class VTKCOMMONDATAMODEL_EXPORT vtkCellGridSidesQuery : public vtkCellGridQuery
{
public:
  static vtkCellGridSidesQuery* New();
  vtkTypeMacro(vtkCellGridSidesQuery, vtkCellGridQuery);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// An enum specifying the work responders should perform for each pass.
  enum PassWork : int
  {
    /// Responders should call AddSide() on each cell's sides to insert
    /// entries into this->Hashes.
    HashSides = 0,
    /// Responders should insert new side sets into their parent cell-grid.
    /// At the start of this pass, this->Hashes is transformed into this->Sides
    /// by calling this->SummarizeSides().
    GenerateSideSets = 1
  };

  /// A structure created by the GetSideSetArrays() method for responders to use.
  struct SideSetArray
  {
    /// The type of parent cell which created the sides.
    vtkStringToken CellType;
    /// The shape of all the sides in the \a Sides array.
    vtkStringToken SideShape;
    /// An array of tuples of (cell-id, side-id) specifying sides.
    vtkSmartPointer<vtkIdTypeArray> Sides;
  };

  /// Set/get whether renderable cells should be included in the output
  /// or the output should strictly contain sides of cells.
  ///
  /// A cell is renderable if it is of dimension 2 or less (i.e., surfaces,
  /// edges, and vertices are all renderable; volumetric cells are not).
  ///
  /// The default is false.
  vtkSetMacro(PreserveRenderableCells, vtkTypeBool);
  vtkGetMacro(PreserveRenderableCells, vtkTypeBool);
  vtkBooleanMacro(PreserveRenderableCells, vtkTypeBool);

  void Initialize() override;
  void StartPass() override;
  bool IsAnotherPassRequired() override;
  void Finalize() override;

  std::unordered_map<vtkStringToken,
    std::unordered_map<vtkStringToken, std::unordered_map<vtkIdType, std::set<int>>>>&
  GetSides()
  {
    return this->Sides;
  }
  // std::map<vtkStringToken, std::unordered_map<vtkIdType, std::set<int>>>& GetSides() { return
  // this->Sides; }

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
  ///
  /// By storing the \a cellType, we avoid requiring a global-to-local cell numbering
  /// in vtkCellGrid instances (as vtkPolyData incurs) which may hold multiple types of cells.
  //@{
  template <typename T, std::size_t NN>
  void AddSide(vtkStringToken cellType, vtkIdType cell, int side, vtkStringToken shape,
    const std::array<T, NN>& conn)
  {
    // Find where we should start hashing and in what direction.
    std::size_t ss = 0;
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
    vtkCellGridSidesQuery::HashCombiner()(hashedValue, shape.GetId());
    if (forward)
    {
      for (std::size_t ii = 0; ii < NN; ++ii)
      {
        std::size_t hashedToken = std::hash<T>{}(conn[(ss + ii) % NN]);
        vtkCellGridSidesQuery::HashCombiner()(hashedValue, hashedToken);
      }
    }
    else // backward
    {
      for (std::size_t ii = 0; ii < NN; ++ii)
      {
        std::size_t hashedToken = std::hash<T>{}(conn[(ss + NN - ii) % NN]);
        // hashedValue = hashedValue ^ (hashedToken << (ii + 1));
        vtkCellGridSidesQuery::HashCombiner()(hashedValue, hashedToken);
      }
    }
    this->Hashes[hashedValue].Sides.insert(Side{ cellType, shape, cell, side });
  }

  template <typename T>
  void AddSide(vtkStringToken cellType, vtkIdType cell, int side, vtkStringToken shape,
    const std::vector<T>& conn)
  {
    std::size_t ss = 0;
    std::size_t NN = conn.size();
    if (NN == 0)
    {
      return;
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
    vtkCellGridSidesQuery::HashCombiner()(hashedValue, shape.GetId());
    // std::cout << "Hash(" << (forward ? "F" : "R") << ")";
    if (forward)
    {
      for (std::size_t ii = 0; ii < NN; ++ii)
      {
        std::size_t hashedToken = std::hash<T>{}(conn[(ss + ii) % NN]);
        vtkCellGridSidesQuery::HashCombiner()(hashedValue, hashedToken);
        // std::cout << " " << conn[(ss + ii) % NN];
      }
    }
    else // backward
    {
      for (std::size_t ii = 0; ii < NN; ++ii)
      {
        std::size_t hashedToken = std::hash<T>{}(conn[(ss + NN - ii) % NN]);
        // hashedValue = hashedValue ^ (hashedToken << (ii + 1));
        vtkCellGridSidesQuery::HashCombiner()(hashedValue, hashedToken);
        // std::cout << " " << conn[(ss + NN - ii) % NN];
      }
    }
    // std::cout << " = " << std::hex << hashedValue << std::dec << "\n";
    this->Hashes[hashedValue].Sides.insert(Side{ cellType, shape, cell, side });
  }
  //@}

  /// Return arrays of cell+side IDs for the given \a cellType.
  std::vector<SideSetArray> GetSideSetArrays(vtkStringToken cellType);

  // Hash combiner adapted from boost::hash_combine
  class HashCombiner
  {
  public:
    template <typename T>
    void operator()(T& h, typename std::enable_if<sizeof(T) == 8, std::size_t>::type k)
    {
      constexpr T m = 0xc6a4a7935bd1e995ull;
      constexpr int r = 47;

      k *= m;
      k ^= k >> r;
      k *= m;

      h ^= k;
      h *= m;

      // Completely arbitrary number, to prevent 0's
      // from hashing to 0.
      h += 0xe6546b64;
    }

    template <typename T>
    void operator()(T& h, typename std::enable_if<sizeof(T) == 4, std::size_t>::type k)
    {
      constexpr std::uint32_t c1 = 0xcc9e2d51;
      constexpr std::uint32_t c2 = 0x1b873593;
      constexpr std::uint32_t r1 = 15;
      constexpr std::uint32_t r2 = 13;

      k *= c1;
      k = (k << r1) | (k >> (32 - r1));
      k *= c2;

      h ^= k;
      h = (h << r2) | (h >> (32 - r2));
      h = h * 5 + 0xe6546b64;
    }
  };

protected:
  vtkCellGridSidesQuery() = default;
  ~vtkCellGridSidesQuery() override = default;

  struct Side
  {
    vtkStringToken CellType;
    vtkStringToken SideShape;
    vtkIdType DOF;
    int SideId;
    bool operator<(const Side& other) const
    {
      return (this->CellType < other.CellType) ||
        (this->CellType == other.CellType &&
          ((this->DOF < other.DOF) || (this->DOF == other.DOF && this->SideId < other.SideId)));
    }
  };
  struct Entry
  {
    std::set<Side> Sides;
  };

  void SummarizeSides();

  vtkTypeBool PreserveRenderableCells{ false };
  std::unordered_map<std::size_t, Entry> Hashes;
  std::unordered_map<vtkStringToken,
    std::unordered_map<vtkStringToken, std::unordered_map<vtkIdType, std::set<int>>>>
    Sides;

private:
  vtkCellGridSidesQuery(const vtkCellGridSidesQuery&) = delete;
  void operator=(const vtkCellGridSidesQuery&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridSidesQuery_h
