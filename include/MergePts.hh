/*=========================================================================

  Program:   Visualization Toolkit
  Module:    MergePts.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkMergePoints - merge exactly coincident points
// .SECTION Description
// vtkMergePoints is a locator object to quickly locate points in 3-D.
// The primary difference between vtkMergePoints and its superclass
// vtkLocator is that vtkMergePoints merges precisely coincident points
// and is therefor much faster.

#ifndef __vtkMergePoints_h
#define __vtkMergePoints_h

#include "Locator.hh"

class vtkMergePoints : public vtkLocator
{
public:
  vtkMergePoints() {};
  ~vtkMergePoints() {};
  char *GetClassName() {return "vtkMergePoints";};

  virtual int *MergePoints();
  int InsertPoint(float x[3]);
};

#endif


