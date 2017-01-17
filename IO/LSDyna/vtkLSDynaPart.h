/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLSDynaPart.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#ifndef vtkLSDynaPart_h
#define vtkLSDynaPart_h

#include "vtkIOLSDynaModule.h" // For export macro
#include "vtkObject.h"
#include "LSDynaMetaData.h" //needed for lsdyna types
#include "vtkStdString.h" //needed for string

class vtkUnstructuredGrid;
class vtkPoints;

class VTKIOLSDYNA_EXPORT vtkLSDynaPart: public vtkObject
{
public:
  static vtkLSDynaPart *New();

  vtkTypeMacro(vtkLSDynaPart,vtkObject);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  //Description: Set the type of the part
  void SetPartType(int type);

  //Description: Returns the type of the part
  LSDynaMetaData::LSDYNA_TYPES PartType() const { return Type; }

  //Description: Returns if the type of the part is considered valid
  bool hasValidType() const;

  vtkIdType GetUserMaterialId() const { return UserMaterialId; }
  vtkIdType GetPartId() const { return PartId; }
  bool HasCells() const;

  //Setup the part with some basic information about what it holds
  void InitPart(vtkStdString name,
                const vtkIdType& partId,
                const vtkIdType& userMaterialId,
                const vtkIdType& numGlobalPoints,
                const int& sizeOfWord);

  //Reserves the needed space in memory for this part
  //that way we never over allocate memory
  void AllocateCellMemory(const vtkIdType& numCells, const vtkIdType& cellLen);

  //Add a cell to the part
  void AddCell(const int& cellType, const vtkIdType& npts, vtkIdType conn[8]);

  //Description:
  //Setups the part cell topology so that we can cache information
  //between timesteps.
  void BuildToplogy();

  //Description:
  //Returns if the toplogy for this part has been constructed
  bool IsTopologyBuilt() const { return TopologyBuilt; }

  //Description:
  //Constructs the grid for this part and returns it.
  vtkUnstructuredGrid* GenerateGrid();

  //Description:
  //allows the part to store dead cells
  void EnableDeadCells(const int& deadCellsAsGhostArray);

  //Description:
  //removes the dead cells array if it exists from the grid
  void DisableDeadCells();

  //Description:
  //We set cells as dead to make them not show up during rendering
  void SetCellsDeadState(unsigned char *dead, const vtkIdType &size);

  //Description:
  //allows the part to store user cell ids
  void EnableCellUserIds();

  //Description:
  //Set the user ids for the cells of this grid
  void SetNextCellUserIds(const vtkIdType& value);


  //Description:
  //Called to init point filling for a property
  //is also able to set the point position of the grid too as that
  //is stored as a point property
  void AddPointProperty(const char* name, const vtkIdType& numComps,
    const bool& isIdTypeProperty, const bool &isProperty,
    const bool& isGeometryPoints);

  //Description:
  //Given a chunk of point property memory copy it to the correct
  //property on the part
  void ReadPointBasedProperty(float *data,
                              const vtkIdType& numTuples,
                              const vtkIdType& numComps,
                              const vtkIdType& currentGlobalPointIndex);

  void ReadPointBasedProperty(double *data,
                              const vtkIdType& numTuples,
                              const vtkIdType& numComps,
                              const vtkIdType& currentGlobalPointIndex);

  //Description:
  //Adds a property to the part
  void AddCellProperty(const char* name, const int& offset, const int& numComps);

  //Description:
  //Given the raw data converts it to be the properties for this part
  //The cell properties are woven together as a block for each cell
  void ReadCellProperties(float *cellProperties, const vtkIdType& numCells,
                          const vtkIdType &numPropertiesInCell);
  void ReadCellProperties(double *cellsProperties, const vtkIdType& numCells,
                          const vtkIdType &numPropertiesInCell);

  //Description:
  //Get the id of the lowest global point this part needs
  //Note: Presumes topology has been built already
  vtkIdType GetMinGlobalPointId() const;

  //Description:
  //Get the id of the largest global point this part needs
  //Note: Presumes topology has been built already
  vtkIdType GetMaxGlobalPointId() const;

protected:
  vtkLSDynaPart();
  ~vtkLSDynaPart() VTK_OVERRIDE;

  vtkUnstructuredGrid* RemoveDeletedCells();

  void BuildUniquePoints();
  void BuildCells();

  void GetPropertyData(const char* name, const vtkIdType &numComps,
  const bool &isIdTypeArray, const bool& isProperty, const bool& isGeometry);

  template<typename T>
  void AddPointInformation(T *buffer,T *pointData,
                           const vtkIdType& numTuples,
                           const vtkIdType& numComps,
                           const vtkIdType& currentGlobalPointIndex);

  //basic info about the part
  LSDynaMetaData::LSDYNA_TYPES Type;
  vtkStdString Name;
  vtkIdType UserMaterialId;
  vtkIdType PartId;

  vtkIdType NumberOfCells;
  vtkIdType NumberOfPoints;
  vtkIdType NumberOfGlobalPoints;

  bool DeadCellsAsGhostArray;
  bool HasDeadCells;

  bool TopologyBuilt;
  bool DoubleBased;

  vtkUnstructuredGrid* Grid;
  vtkUnstructuredGrid* ThresholdGrid;

  vtkPoints* Points;

  class InternalCells;
  InternalCells *Cells;

  class InternalCellProperties;
  InternalCellProperties *CellProperties;

  class InternalPointsUsed;
  class DensePointsUsed;
  class SparsePointsUsed;
  InternalPointsUsed *GlobalPointsUsed;

  //used when reading properties
  class InternalCurrentPointInfo;
  InternalCurrentPointInfo *CurrentPointPropInfo;

private:
  vtkLSDynaPart( const vtkLSDynaPart& ) VTK_DELETE_FUNCTION;
  void operator = ( const vtkLSDynaPart& ) VTK_DELETE_FUNCTION;
};

#endif // VTKLSDYNAPART
