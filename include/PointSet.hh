/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PointSet.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkPointSet - abstract class for specifying dataset behavior
// .SECTION Description
// vtkPointSet is an abstract class that specifies the interface for 
// datasets that explicitly use "point" arrays to represent geometry.
// For example, vtkPolyData and vtkUnstructuredGrid require point arrays
// to specify point position, while vtkStructuredPoints generates point
// positions implicitly.

#ifndef __vtkPointSet_h
#define __vtkPointSet_h

#include "DataSet.hh"
#include "Locator.hh"

class vtkPointSet : public vtkDataSet
{
public:
  vtkPointSet();
  ~vtkPointSet();
  vtkPointSet(const vtkPointSet& ps);
  char *GetClassName() {return "vtkPointSet";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // dataset interface
  void Initialize();
  int GetNumberOfPoints();
  float *GetPoint(int ptId) {return this->Points->GetPoint(ptId);};
  void GetPoint(int ptId, float x[3]) {this->Points->GetPoint(ptId,x);};
  int FindCell(float x[3], vtkCell *cell, float tol2, int& subId, 
               float pcoords[3], float weights[MAX_CELL_SIZE]);

  unsigned long int GetMTime();

  // compute bounds of data
  void ComputeBounds();
  
  // reclaim memory
  void Squeeze();

  // Description:
  // Specify point array to define point coordinates.
  vtkSetRefCountedObjectMacro(Points,vtkPoints);
  vtkGetObjectMacro(Points,vtkPoints);

protected:
  vtkPoints *Points;
  vtkLocator *Locator;
};

inline int vtkPointSet::GetNumberOfPoints()
{
  if (this->Points) return this->Points->GetNumberOfPoints();
  else return 0;
}


#endif


