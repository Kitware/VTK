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

  void SetMetaData(LSDynaMetaData *metaData);
  void FinalizeTopology();
  void Finalize(vtkPoints *commonPoints,const int& removeDeletedCells);



  //Description: Insert a cell of a given type and material index to the 
  //collection.
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

  //Description:
  //Adds a property for all parts of a certain type
  void AddProperty(const LSDynaMetaData::LSDYNA_TYPES& type, const char* name,
                    const int& offset, const int& numComps);

  //Description:
  //Actually reads from the file the needed properties for all parts
  //of a given type of lsdyna cell
  void ReadProperties(const LSDynaMetaData::LSDYNA_TYPES& type,
                      const int& numTuples);
protected:
  vtkLSDynaPartCollection();
  ~vtkLSDynaPartCollection();

  //Description:
  //Clears all storage information
  void ResetTimeStepInfo();

  //Builds up the basic meta information needed for topology storage
  void BuildPartInfo();

  //Description:
  //builds the unstructured grid which represents the part
  //will clear out all the temporary storage for the part when this happens
  //the point set passed in is all the world points, we will construct a subset

  void ConstructGridCellsWithoutDeadCells(LSDynaPart *part);
  void ConstructGridCells(LSDynaPart *part);
  void ConstructGridPoints(LSDynaPart *part, vtkPoints *commonPoints);

  template<typename T>
  void FillPropertyArray(T *buffer,const LSDynaMetaData::LSDYNA_TYPES& type, const vtkIdType& numCells, const int& numTuples);

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
