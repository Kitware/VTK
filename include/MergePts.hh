/*=========================================================================

  Program:   Visualization Library
  Module:    MergePts.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlMergePoints - merge exactly coincident points
// .SECTION Description
// vlMergePoints is a locator object to quickly locate points in 3-D.
// The primary difference between vlMergePoints and its superclass
// vlLocator is that vlMergePoints merges precisely coincident points
// and is therefor much faster.

#ifndef __vlMergePoints_h
#define __vlMergePoints_h

#include "Locator.hh"

class vlMergePoints : public vlLocator
{
public:
  vlMergePoints() {};
  ~vlMergePoints() {};
  char *GetClassName() {return "vlMergePoints";};

  virtual int *MergePoints();
  int InsertPoint(float x[3]);
};

#endif


