/*=========================================================================

  Program:   Visualization Toolkit
  Module:    MaskPts.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkMaskPoints - selectively filter points
// .SECTION Description
// vtkMaskPoints is a filter that passes through points and point attributes 
// from input dataset. (Other geometry is not passed through). It is 
// possible to mask every nth point, and to specify an initial offset
// to begin masking from.

#ifndef __vtkMaskPoints_h
#define __vtkMaskPoints_h

#include "DS2PolyF.hh"

class vtkMaskPoints : public vtkDataSetToPolyFilter
{
public:
  vtkMaskPoints():OnRatio(2),Offset(0),RandomMode(0) {};
  ~vtkMaskPoints() {};
  char *GetClassName() {return "vtkMaskPoints";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Turn on every nth point.
  vtkSetClampMacro(OnRatio,int,1,LARGE_INTEGER);
  vtkGetMacro(OnRatio,int);

  // Description:
  // Start with this point.
  vtkSetClampMacro(Offset,int,0,LARGE_INTEGER);
  vtkGetMacro(Offset,int);

  // Description:
  // Special flag causes randomization of point selection. If this mode is on,
  // statitically every nth point (i.e., OnRatio) will be displayed.
  vtkSetMacro(RandomMode,int);
  vtkGetMacro(RandomMode,int);
  vtkBooleanMacro(RandomMode,int);

protected:
  void Execute();

  int OnRatio;     // every OnRatio point is on; all others are off.
  int Offset;      // offset (or starting point id)
  int RandomMode;  // turn on/off randomization
};

#endif


