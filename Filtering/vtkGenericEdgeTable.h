/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericEdgeTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericEdgeTable - keep track of edges (defined by pair of integer id's)
// .SECTION Description
// vtkGenericEdgeTable is used to indicate the existance of and hold
// information about edges. Similar to vtkEdgeTable, this class is
// more sophisticated in that it uses reference counting to keep track
// of when information about an edge should be deleted.
//
// vtkGenericEdgeTable is a helper class used in the adaptor framework.  It
// is used during the tessellation process to hold information about the
// error metric on each edge. This avoids recomputing the error metric each
// time the same edge is visited.

#ifndef __vtkGenericEdgeTable_h
#define __vtkGenericEdgeTable_h

#include "vtkObject.h"

class vtkEdgeTableEdge;
class vtkEdgeTablePoints;

class VTK_FILTERING_EXPORT vtkGenericEdgeTable : public vtkObject
{
public:
  // Description:
  // Instantiate an empty edge table.
  static vtkGenericEdgeTable *New();
  
  // Description:
  // Standard VTK type and print macros.
  vtkTypeRevisionMacro(vtkGenericEdgeTable,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Split the edge with the indicated point id.
  void InsertEdge(vtkIdType e1, vtkIdType e2, vtkIdType cellId, 
                  int ref, vtkIdType &ptId );

  // Description:
  // Split the edge with the indicated point id.
  void InsertEdge(vtkIdType e1, vtkIdType e2, vtkIdType cellId, 
                  int ref, int toSplit, vtkIdType &ptId );

  // Description:
  // Insert an edge but do not split it.
  void InsertEdge(vtkIdType e1, vtkIdType e2, vtkIdType cellId, int ref = 1 );

  // Description:
  // Method to remove an edge from the table. The method returns the
  // current reference count.
  int RemoveEdge(vtkIdType e1, vtkIdType e2);

  // Description:
  // Method to determine whether an edge is in the table.
  // It returns if the edge was split, and the point id exists.
  int CheckEdge(vtkIdType e1, vtkIdType e2, vtkIdType &ptId);
  
  // Description:
  // Method that increments the referencecount and returns it.
  int IncrementEdgeReferenceCount(vtkIdType e1, vtkIdType e2, 
                                  vtkIdType cellId);
  
  // Description:
  // Return the edge reference count.
  int CheckEdgeReferenceCount(vtkIdType e1, vtkIdType e2);

  // Description:
  // To specify the starting point id.
  void Initialize(vtkIdType start);

  // Description:
  // Return the last point id inserted.
  vtkIdType GetLastPointId();
  
  // Description:
  // Increment the last point id.
  void IncrementLastPointId();
  
  // Description:
  // Check if a point is already in the point table.
  int CheckPoint(vtkIdType ptId);

  // Description:
  // Check for the existence of a point and return its coordinate value.
  int CheckPoint(vtkIdType ptId, double point[3], double scalar[3]);

  // Description:
  // Insert point associated with an edge.
  void InsertPoint(vtkIdType ptId, double point[3]);
  void InsertPointAndScalar(vtkIdType ptId, double pt[3], double s[3]);

  // Description:
  // Remove a point from the point table.
  void RemovePoint(vtkIdType ptId);
  
  // Description:
  // Increment the reference count for the indicated point.
  void IncrementPointReferenceCount(vtkIdType ptId );

//BTX
class PointEntry
{
public:
  vtkIdType PointId;
  double Coord[3];
  double Scalar[3];  //FIXME
  int Reference;  //signed char

  PointEntry() 
    { 
    this->Reference = -10;

    this->Coord[0]  = -100;
    this->Coord[1]  = -100;
    this->Coord[2]  = -100;
    this->Scalar[0]  = -200;
    this->Scalar[1]  = -200;
    this->Scalar[2]  = -200;
    }
  ~PointEntry() {}

  PointEntry(const PointEntry& copy)
    {
    this->PointId  = copy.PointId;

    this->Coord[0]  = copy.Coord[0];
    this->Coord[1]  = copy.Coord[1];
    this->Coord[2]  = copy.Coord[2];

    this->Scalar[0] = copy.Scalar[0];
    this->Scalar[1] = copy.Scalar[1];
    this->Scalar[2] = copy.Scalar[2];

    this->Reference = copy.Reference;
    }

  void operator=(const PointEntry& entry) 
    {
    if(this == &entry)
      {
      return;
      }
    this->PointId  = entry.PointId;

    this->Coord[0] = entry.Coord[0];
    this->Coord[1] = entry.Coord[1];
    this->Coord[2] = entry.Coord[2];

    this->Scalar[0] = entry.Scalar[0];
    this->Scalar[1] = entry.Scalar[1];
    this->Scalar[2] = entry.Scalar[2];

    this->Reference = entry.Reference;
    }
};

class EdgeEntry
{
public:
  vtkIdType E1;
  vtkIdType E2;

  int Reference;  //signed char
  int ToSplit;  //signed char
  vtkIdType PtId;
  vtkIdType CellId; //CellId the edge refer to at a step in tesselation
  
  EdgeEntry() 
    { 
    this->Reference = 0; 
    this->CellId = -1;
    }
  ~EdgeEntry() {}

  EdgeEntry(const EdgeEntry& copy)
    {
    this->E1 = copy.E1;
    this->E2 = copy.E2;

    this->Reference = copy.Reference;
    this->ToSplit = copy.ToSplit;
    this->PtId = copy.PtId;
    this->CellId = copy.CellId;
    }
  
  void operator=(const EdgeEntry& entry) 
    {
    if(this == &entry)
      {
      return;
      }
    this->E1 = entry.E1;
    this->E2 = entry.E2;
    this->Reference = entry.Reference;
    this->ToSplit = entry.ToSplit;
    this->PtId = entry.PtId;
    this->CellId = entry.CellId;
    }
};
//ETX

protected:
  vtkGenericEdgeTable();
  ~vtkGenericEdgeTable();

  // Description:
  // For debuggin purposes.
  void DumpTable();
  void LoadFactor();
  
  //Hash table that contiain entry based on edges:
  vtkEdgeTableEdge   *EdgeTable;

  //At end of process we should be able to retrieve points coord based on pointid
  vtkEdgeTablePoints *HashPoints;

  // Main hash functions
  //For edge table:
  vtkIdType HashFunction(vtkIdType e1, vtkIdType e2);

  //For point table:
  vtkIdType HashFunction(vtkIdType ptId);

  // Keep track of the last point id we inserted, increment it each time:
  vtkIdType LastPointId;  //static
//  vtkGetMacro(LastPointId, vtkIdType);
  //Use only once !
//  vtkSetMacro(LastPointId, vtkIdType);

private:
  vtkGenericEdgeTable(const vtkGenericEdgeTable&);  // Not implemented.
  void operator=(const vtkGenericEdgeTable&);  // Not implemented.

};

#endif

