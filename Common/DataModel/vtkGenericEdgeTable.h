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
/**
 * @class   vtkGenericEdgeTable
 * @brief   keep track of edges (defined by pair of integer id's)
 *
 * vtkGenericEdgeTable is used to indicate the existence of and hold
 * information about edges. Similar to vtkEdgeTable, this class is
 * more sophisticated in that it uses reference counting to keep track
 * of when information about an edge should be deleted.
 *
 * vtkGenericEdgeTable is a helper class used in the adaptor framework.  It
 * is used during the tessellation process to hold information about the
 * error metric on each edge. This avoids recomputing the error metric each
 * time the same edge is visited.
*/

#ifndef vtkGenericEdgeTable_h
#define vtkGenericEdgeTable_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

class vtkEdgeTableEdge;
class vtkEdgeTablePoints;

class VTKCOMMONDATAMODEL_EXPORT vtkGenericEdgeTable : public vtkObject
{
public:
  /**
   * Instantiate an empty edge table.
   */
  static vtkGenericEdgeTable *New();

  //@{
  /**
   * Standard VTK type and print macros.
   */
  vtkTypeMacro(vtkGenericEdgeTable,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * Split the edge with the indicated point id.
   */
  void InsertEdge(vtkIdType e1, vtkIdType e2, vtkIdType cellId,
                  int ref, vtkIdType &ptId );

  /**
   * Insert an edge but do not split it.
   */
  void InsertEdge(vtkIdType e1, vtkIdType e2, vtkIdType cellId, int ref = 1 );

  /**
   * Method to remove an edge from the table. The method returns the
   * current reference count.
   */
  int RemoveEdge(vtkIdType e1, vtkIdType e2);

  /**
   * Method to determine whether an edge is in the table (0 or 1), or not (-1).
   * It returns whether the edge was split (1) or not (0),
   * and the point id exists.
   */
  int CheckEdge(vtkIdType e1, vtkIdType e2, vtkIdType &ptId);

  /**
   * Method that increments the referencecount and returns it.
   */
  int IncrementEdgeReferenceCount(vtkIdType e1, vtkIdType e2,
                                  vtkIdType cellId);

  /**
   * Return the edge reference count.
   */
  int CheckEdgeReferenceCount(vtkIdType e1, vtkIdType e2);

  /**
   * To specify the starting point id. It will initialize LastPointId
   * This is very sensitive the start point should be cautiously chosen
   */
  void Initialize(vtkIdType start);

  /**
   * Return the total number of components for the point-centered attributes.
   * \post positive_result: result>0
   */
  int GetNumberOfComponents();

  /**
   * Set the total number of components for the point-centered attributes.
   * \pre positive_count: count>0
   */
  void SetNumberOfComponents(int count);

  /**
   * Check if a point is already in the point table.
   */
  int CheckPoint(vtkIdType ptId);

  /**
   * Check for the existence of a point and return its coordinate value.
   * \pre scalar_size: sizeof(scalar)==this->GetNumberOfComponents()
   */
  int CheckPoint(vtkIdType ptId, double point[3], double *scalar);

  //@{
  /**
   * Insert point associated with an edge.
   */
  void InsertPoint(vtkIdType ptId, double point[3]);
  // \pre: sizeof(s)==GetNumberOfComponents()
  void InsertPointAndScalar(vtkIdType ptId, double pt[3], double *s);
  //@}

  /**
   * Remove a point from the point table.
   */
  void RemovePoint(vtkIdType ptId);

  /**
   * Increment the reference count for the indicated point.
   */
  void IncrementPointReferenceCount(vtkIdType ptId );

  //@{
  /**
   * For debugging purposes. It is particularly useful to dump the table
   * and check that nothing is left after a complete iteration. LoadFactor
   * should ideally be very low to be able to have a constant time access
   */
  void DumpTable();
  void LoadFactor();
  //@}

class PointEntry
{
public:
  vtkIdType PointId;
  double Coord[3];
  double *Scalar;  // point data: all point-centered attributes at this point
  int numberOfComponents;

  int Reference;  //signed char

  /**
   * Constructor with a scalar field of `size' doubles.
   * \pre positive_number_of_components: size>0
   */
  PointEntry(int size);

  ~PointEntry()
  {
      delete[] this->Scalar;
  }

  PointEntry(const PointEntry &other)
  {
    this->PointId  = other.PointId;

    memcpy(this->Coord,other.Coord,sizeof(double)*3);

    int c = other.numberOfComponents;
    this->numberOfComponents = c;
    this->Scalar = new double[c];
    memcpy(this->Scalar, other.Scalar, sizeof(double)*c);
    this->Reference = other.Reference;
  }

  PointEntry& operator=(const PointEntry &other)
  {
    if(this != &other)
    {
      this->PointId  = other.PointId;

      memcpy(this->Coord, other.Coord, sizeof(double)*3);

      int c = other.numberOfComponents;

      if(this->numberOfComponents!=c)
      {
        delete[] this->Scalar;
        this->Scalar = new double[c];
        this->numberOfComponents = c;
      }
      memcpy(this->Scalar, other.Scalar, sizeof(double)*c);
      this->Reference = other.Reference;
    }
    return *this;
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
  vtkIdType CellId; //CellId the edge refer to at a step in tessellation

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

  EdgeEntry& operator=(const EdgeEntry& entry)
  {
    if(this == &entry)
    {
      return *this;
    }
    this->E1 = entry.E1;
    this->E2 = entry.E2;
    this->Reference = entry.Reference;
    this->ToSplit = entry.ToSplit;
    this->PtId = entry.PtId;
    this->CellId = entry.CellId;
    return *this;
  }
};

protected:
  vtkGenericEdgeTable();
  ~vtkGenericEdgeTable() override;

  /**
   * Split the edge with the indicated point id.
   */
  void InsertEdge(vtkIdType e1, vtkIdType e2, vtkIdType cellId,
                  int ref, int toSplit, vtkIdType &ptId );

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
  vtkIdType LastPointId;

  vtkIdType NumberOfComponents;

private:
  vtkGenericEdgeTable(const vtkGenericEdgeTable&) = delete;
  void operator=(const vtkGenericEdgeTable&) = delete;

};

#endif

