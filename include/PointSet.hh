/*=========================================================================

  Program:   Visualization Library
  Module:    PointSet.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Abstract class for specifying dataset behaviour
//
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
  int GetNumberOfPoints()
  {if (this->Points) return this->Points->GetNumberOfPoints();
  else return 0;};

  float *GetPoint(int ptId) {return this->Points->GetPoint(ptId);};

  // Locate cell point is in based on global coordiate and tolerance.
  // Returns cellId >= 0 if inside, < 0 otherwise.
  int FindCell(float x[3], float dist2);

  // some data sets are composite objects and need to check each part for MTime
  unsigned long int GetMTime();

  // compute bounds of data
  void ComputeBounds();
  
  vlSetObjectMacro(Points,vlPoints);
  vlGetObjectMacro(Points,vlPoints);

protected:
  vlPoints *Points;
  vlLocator *Locator;
};

#endif


