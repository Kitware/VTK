/*=========================================================================

  Program:   Visualization Library
  Module:    PointSet.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlPointSet - abstract class for specifying dataset behavior
// .SECTION Description
// vlPointSet is an abstract class that specifies the interface for 
// datasets that explicitly use "point" arrays to represent geometry.
// For example, vlPolyData and vlUnstructuredGrid require point arrays
// to specify point position, while vlStructuredPoints generates point
// positions implicitly.

#ifndef __vlPointSet_h
#define __vlPointSet_h

#include "DataSet.hh"
#include "Locator.hh"

class vlPointSet : virtual public vlDataSet
{
public:
  vlPointSet();
  vlPointSet(const vlPointSet& ps);
  char *GetClassName() {return "vlPointSet";};
  void PrintSelf(ostream& os, vlIndent indent);

  // dataset interface
  void Initialize();
  int GetNumberOfPoints();
  float *GetPoint(int ptId) {return this->Points->GetPoint(ptId);};
  int FindCell(float x[3], vlCell *cell, float tol2, int& subId, float pcoords[3]);

  unsigned long int GetMTime();

  // compute bounds of data
  void ComputeBounds();
  
  // reclaim memory
  void Squeeze();

  // Description:
  // Specify point array to define point coordinates.
  vlSetObjectMacro(Points,vlPoints);
  vlGetObjectMacro(Points,vlPoints);

protected:
  vlPoints *Points;
  vlLocator *Locator;
};

inline int vlPointSet::GetNumberOfPoints()
{
  if (this->Points) return this->Points->GetNumberOfPoints();
  else return 0;
}


#endif


