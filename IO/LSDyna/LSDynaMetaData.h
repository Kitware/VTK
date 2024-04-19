// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

// .NAME LSDynaMetaData - Read LS-Dyna databases (d3plot)
// .SECTION Description
//    A class to hold metadata about a particular file (such as time steps,
//    the start of state information for each time step, the number of
//    adaptive remeshes, and the large collection of constants that determine
//    the available attributes). It contains an LSDynaFamily instance.

#ifndef __LSDynaMetaData_h
#define __LSDynaMetaData_h

#include "LSDynaFamily.h"

#include <map>
#include <set>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class LSDynaMetaData
{
public:
  LSDynaMetaData();

  bool AddPointArray(const std::string& name, int numComponents, int status);

  bool AddCellArray(int cellType, const std::string& name, int numComponents, int status);

  vtkIdType GetTotalMaterialCount();

  void Reset();

  /** LS-Dyna cell types.
   * These may be used as values for the \a cellType argument in member functions.
   * One dataset is created for each cell type so that cells can have different
   * attributes (temperature, pressure, etc.) defined over them.
   * Note that \a NUM_CELL_TYPES is not a cell type, but an enumerant that
   * specifies the total number of cell types. It is used to size arrays.
   */
  enum LSDYNA_TYPES
  {
    PARTICLE = 0,
    BEAM = 1,
    SHELL = 2,
    THICK_SHELL = 3,
    SOLID = 4,
    RIGID_BODY = 5,
    ROAD_SURFACE = 6,
    NUM_CELL_TYPES
  };

  // If this is 0, the rest of the members have undefined
  // values (although "derived-value" arrays will be
  // initialized to nullptr)
  int FileIsValid;
  int FileSizeFactor;      // scale factor used to compute MaxFileLength
  vtkIdType MaxFileLength; // Maximum size of any file (data too big is split into multiple files)

  LSDynaFamily Fam; // file family I/O aggregator

  char Title[41];
  char ReleaseNumber[16];
  float CodeVersion;
  int Dimensionality;
  vtkIdType CurrentState; // time step
  vtkIdType NumberOfNodes;
  vtkIdType NumberOfCells[LSDynaMetaData::NUM_CELL_TYPES];
  int ReadRigidRoadMvmt;    // Are some of the quads rigid? (eliminating a lot of state)
  int ConnectivityUnpacked; // Is the connectivity packed, 3 to a word?
  std::map<std::string, vtkIdType> Dict;

  /// List of material IDs that indicate the associated shell element is rigid (and has no state
  /// data)
  std::set<int> RigidMaterials;
  /// List of material IDs that indicate the associated solid element represents an Eulerian or ALE
  /// fluid.
  std::set<int> FluidMaterials;

  std::vector<std::string> PointArrayNames;
  std::vector<int> PointArrayComponents;
  std::vector<int> PointArrayStatus;

  std::map<int, std::vector<std::string>> CellArrayNames;
  std::map<int, std::vector<int>> CellArrayComponents;
  std::map<int, std::vector<int>> CellArrayStatus;

  std::vector<std::string> PartNames;
  std::vector<int> PartIds;
  std::vector<int> PartMaterials;
  std::vector<int> PartStatus;

  std::vector<int> MaterialsOrdered;
  std::vector<int> MaterialsUnordered;
  std::vector<int> MaterialsLookup;

  std::vector<vtkIdType> RigidSurfaceSegmentSizes;
  std::vector<double> TimeValues;

  // For the current time value, what file contains this state (0=d3plot,1=d3plot01, ...)
  vtkIdType FileNumberThisState;
  // For the current time value, what is the byte offset of the state in file FileNumberThisState?
  vtkIdType FileOffsetThisState;
  // Size of all data that appears before first state
  vtkIdType PreStateSize;
  // Number of bytes required to store a single timestep
  vtkIdType StateSize;

  // Number of words into the state that the element deletion starts at
  vtkIdType ElementDeletionOffset;

  // Number of words into the state that the SPH state data starts at
  vtkIdType SPHStateOffset;
};

VTK_ABI_NAMESPACE_END
#endif // __LSDynaMetaData_h
