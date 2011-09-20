/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLSDynaReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#ifndef __vtkLSDynaPartCollection_h
#define __vtkLSDynaPartCollection_h
#include "LSDynaMetaData.h"

#include "vtkObject.h"

class vtkDataArray;
class vtkUnstructuredGrid;
class vtkPoints;
class vtkIntArray;

class VTK_IO_EXPORT vtkLSDynaPartCollection: public vtkObject
{
public:
  struct LSDynaPart;
  static vtkLSDynaPartCollection *New();

  vtkTypeMacro(vtkLSDynaPartCollection,vtkObject);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  //Description:
  //Pass in the metadata to setup this collection.
  //The optional min and max cell Id are used when in parallel to load balance the nodes.
  //Meaning the collection will only store subsections of parts that fall within
  //the range of the min and max
  //Note: min is included, and max is excluded from the valid range of cells.
  void InitCollection(LSDynaMetaData *metaData,
    vtkIdType* mins=NULL, vtkIdType* maxs=NULL);


  //Description:
  //For a given part type returns the number of cells to read and the number
  //of cells to skip first to not read
  void GetPartReadInfo(const int& partType, vtkIdType& numberOfCells,
    vtkIdType& numCellsToSkip,vtkIdType& numCellsToSkipEnd) const;

  //Description:
  //Finalizes the cell topology by mapping the cells point indexes
  //to a relative number based on the cells this collection is storing
  void FinalizeTopology();

  //Description:
  //Given a point set and having the dead flags set for a time step
  //this will construct that valid unstructured grid for each part
  //that this collection has
  void Finalize(vtkPoints *commonPoints,const int& removeDeletedCells);

  //Description: Insert a cell of a given type and material index to the 
  //collection.
  //NOTE: the cellIndex is relative to the collection. So in parallel
  //the cellIndex will be from 0 to MaxId-MinId
  void InsertCell(const int& partType, const vtkIdType& cellIndex,
                  const vtkIdType& matIdx, const int& cellType,
                  const vtkIdType& npts, vtkIdType conn[8]);

  //Description:
  //Set for each part type what cells are deleted/dead
  void SetCellDeadFlags(const int& partType, vtkIntArray *death);

  bool IsActivePart(const int& id) const;

  //Description:
  //Given a part will return the unstructured grid for the part.
  //Note: You must call finalize before using this method
  vtkUnstructuredGrid* GetGridForPart(const int& index) const;

  int GetNumberOfParts() const;

  //Description:
  //Adds a complete point data array to the storage.
  //This array will be split up to be the subset needed for each part
  //once the collection is finalized.
  void AddPointArray(vtkDataArray* data);
  int GetNumberOfPointArrays() const;
  vtkDataArray* GetPointArray(const int& index) const;

  //Description:
  //Adds a property for all parts of a certain type
  void AddProperty(const LSDynaMetaData::LSDYNA_TYPES& type, const char* name,
                    const int& offset, const int& numComps);

  void FillCellProperties(float *buffer,const LSDynaMetaData::LSDYNA_TYPES& type,
    const vtkIdType& numCells, const int& numTuples);
  void FillCellProperties(double *buffer,const LSDynaMetaData::LSDYNA_TYPES& type,
    const vtkIdType& numCells, const int& numTuples);

protected:
  vtkLSDynaPartCollection();
  ~vtkLSDynaPartCollection();

  vtkIdType* MinIds;
  vtkIdType* MaxIds;

  //Description:
  //Clears all storage information
  void ResetTimeStepInfo();

  //Builds up the basic meta information needed for topology storage
  void BuildPartInfo(vtkIdType* mins, vtkIdType* maxs);

  //Description:
  //builds the unstructured grid which represents the part
  //will clear out all the temporary storage for the part when this happens
  //the point set passed in is all the world points, we will construct a subset
  void ConstructGridCellsWithoutDeadCells(LSDynaPart *part);
  void ConstructGridCells(LSDynaPart *part);
  void ConstructGridPoints(LSDynaPart *part, vtkPoints *commonPoints);

  //Description:
  //Breaks down the buffer of cell properties to the cell properties we
  //are interested in. This will remove all properties that aren't active or
  //for parts we are not loading
  template<typename T>
  void FillCellArray(T *buffer,const LSDynaMetaData::LSDYNA_TYPES& type,
    const vtkIdType& numCells, const int& numTuples);

private:
  vtkLSDynaPartCollection( const vtkLSDynaPartCollection& ); // Not implemented.
  void operator = ( const vtkLSDynaPartCollection& ); // Not implemented.

  //holds if the class has been finalized
  bool Finalized;

  LSDynaMetaData *MetaData;
  
  class LSDynaPartStorage;
  LSDynaPartStorage* Storage;
};



#endif // LSDYNAPARTS_H
