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
#include "LSDynaExport.h"
#include "LSDynaMetaData.h"

#include "vtkObject.h"
class vtkUnstructuredGrid;

class VTK_HYBRID_EXPORT vtkLSDynaPartCollection: public vtkObject
{
public:
  static vtkLSDynaPartCollection *New();

  vtkTypeMacro(vtkLSDynaPartCollection,vtkObject);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  void SetMetaData(LSDynaMetaData *metaData);

  void InsertCell(const int& partType, const vtkIdType& cellIndex, const vtkIdType& matIdx,
                      const int& cellType, const vtkIdType& npts, vtkIdType conn[8]);

  bool IsActivePart(const int& id) const;

  //Description:
  //Given the part type (ie solid, road surface, etc) and the cell index
  //return the part grid id. This is useful when you have properties and
  //you need to relate a property to a part grid
  //Note this method does not out of bounds checks!
  int PartIdForCellIndex(const int& partType,const int& index) const;

  //Description:
  //Given a part
  vtkUnstructuredGrid* GetGridForPart(const int& index) const;

  int GetNumberOfParts() const;

  void Reset();
protected:
  vtkLSDynaPartCollection();
  ~vtkLSDynaPartCollection();

  void BuildPartInfo();

private:
  vtkLSDynaPartCollection( const vtkLSDynaPartCollection& ); // Not implemented.
  void operator = ( const vtkLSDynaPartCollection& ); // Not implemented.

  LSDynaMetaData *MetaData;

  class LSDynaPartVector;
  LSDynaPartVector* Parts;
};

#endif // LSDYNAPARTS_H
