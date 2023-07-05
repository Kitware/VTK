// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkLSDynaPartCollection_h
#define vtkLSDynaPartCollection_h

#include "LSDynaMetaData.h"    //needed for LSDynaMetaData::LSDYNA_TYPES enum
#include "vtkIOLSDynaModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkUnstructuredGrid;
class vtkPoints;
class vtkUnsignedCharArray;
class vtkLSDynaPart;

class VTKIOLSDYNA_EXPORT vtkLSDynaPartCollection : public vtkObject
{
public:
  class LSDynaPart;
  static vtkLSDynaPartCollection* New();

  vtkTypeMacro(vtkLSDynaPartCollection, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Pass in the metadata to setup this collection.
  // The optional min and max cell Id are used when in parallel to load balance the nodes.
  // Meaning the collection will only store subsections of parts that fall within
  // the range of the min and max
  // Note: min is included, and max is excluded from the valid range of cells.
  void InitCollection(
    LSDynaMetaData* metaData, vtkIdType* mins = nullptr, vtkIdType* maxs = nullptr);

  // Description:
  // For a given part type returns the number of cells to read and the number
  // of cells to skip first to not read
  void GetPartReadInfo(const int& partType, vtkIdType& numberOfCells, vtkIdType& numCellsToSkip,
    vtkIdType& numCellsToSkipEnd) const;

  // Description:
  // Finalizes the cell topology by mapping the cells point indexes
  // to a relative number based on the cells this collection is storing
  void FinalizeTopology();

  // Description: Register a cell of a given type and material index to the
  // correct part
  // NOTE: the cellIndex is relative to the collection. So in parallel
  // the cellIndex will be from 0 to MaxId-MinId
  void RegisterCellIndexToPart(const int& partType, const vtkIdType& matIdx,
    const vtkIdType& cellIndex, const vtkIdType& npts);

  void InitCellInsertion();

  void AllocateParts();

  // Description: Insert a cell of a given type and material index to the
  // collection.
  // NOTE: the cellIndex is relative to the collection. So in parallel
  // the cellIndex will be from 0 to MaxId-MinId
  void InsertCell(const int& partType, const vtkIdType& matIdx, const int& cellType,
    const vtkIdType& npts, vtkIdType conn[8]);

  // Description:
  // Set for each part type what cells are deleted/dead
  void SetCellDeadFlags(
    const int& partType, vtkUnsignedCharArray* death, const int& deadCellsAsGhostArray);

  bool IsActivePart(const int& id) const;

  // Description:
  // Given a part will return the unstructured grid for the part.
  // Note: You must call finalize before using this method
  vtkUnstructuredGrid* GetGridForPart(const int& index) const;

  int GetNumberOfParts() const;

  void DisbleDeadCells();

  // Description:
  void ReadPointUserIds(const vtkIdType& numTuples, const char* name);

  // Description:
  void ReadPointProperty(const vtkIdType& numTuples, const vtkIdType& numComps, const char* name,
    const bool& isProperty = true, const bool& isGeometryPoints = false,
    const bool& isRoadPoints = false);

  // Description:
  // Adds a property for all parts of a certain type
  void AddProperty(const LSDynaMetaData::LSDYNA_TYPES& type, const char* name, const int& offset,
    const int& numComps);
  void FillCellProperties(float* buffer, const LSDynaMetaData::LSDYNA_TYPES& type,
    const vtkIdType& startId, const vtkIdType& numCells, const int& numPropertiesInCell);
  void FillCellProperties(double* buffer, const LSDynaMetaData::LSDYNA_TYPES& type,
    const vtkIdType& startId, const vtkIdType& numCells, const int& numPropertiesInCell);

  // Description:
  // Adds User Ids for all parts of a certain type
  void ReadCellUserIds(const LSDynaMetaData::LSDYNA_TYPES& type, const int& status);

  template <typename T>
  void FillCellUserId(T* buffer, const LSDynaMetaData::LSDYNA_TYPES& type, const vtkIdType& startId,
    const vtkIdType& numCells)
  {
    this->FillCellUserIdArray(buffer, type, startId, numCells);
  }

protected:
  vtkLSDynaPartCollection();
  ~vtkLSDynaPartCollection() override;

  vtkIdType* MinIds;
  vtkIdType* MaxIds;

  // Builds up the basic meta information needed for topology storage
  void BuildPartInfo();

  // Description:
  // Breaks down the buffer of cell properties to the cell properties we
  // are interested in. This will remove all properties that aren't active or
  // for parts we are not loading
  template <typename T>
  void FillCellArray(T* buffer, const LSDynaMetaData::LSDYNA_TYPES& type, const vtkIdType& startId,
    vtkIdType numCells, const int& numTuples);

  template <typename T>
  void FillCellUserIdArray(T* buffer, const LSDynaMetaData::LSDYNA_TYPES& type,
    const vtkIdType& startId, vtkIdType numCells);

  // Description:
  // Methods for adding points to the collection
  void SetupPointPropertyForReading(const vtkIdType& numTuples, const vtkIdType& numComps,
    const char* name, const bool& isIdType, const bool& isProperty, const bool& isGeometryPoints,
    const bool& isRoadPoints);
  template <typename T>
  void FillPointProperty(const vtkIdType& numTuples, const vtkIdType& numComps,
    vtkLSDynaPart** parts, vtkIdType numParts);

private:
  vtkLSDynaPartCollection(const vtkLSDynaPartCollection&) = delete;
  void operator=(const vtkLSDynaPartCollection&) = delete;

  LSDynaMetaData* MetaData;

  class LSDynaPartStorage;
  LSDynaPartStorage* Storage;
};

VTK_ABI_NAMESPACE_END
#endif // vtkLSDynaPartCollection_h
