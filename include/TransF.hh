/*=========================================================================

  Program:   Visualization Library
  Module:    TransF.hh
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
// Class to transform points
//
#ifndef __vlTransformFilter_h
#define __vlTransformFilter_h

#include "PtS2PtSF.hh"
#include "Trans.hh"

class vlTransformFilter : public vlPointSetToPointSetFilter
{
public:
  vlTransformFilter() : Transform(NULL) {};
  ~vlTransformFilter() {};
  char *GetClassName() {return "vlTransformFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  unsigned long int GetMTime();

  vlSetObjectMacro(Transform,vlTransform);
  vlGetObjectMacro(Transform,vlTransform);

protected:
  void Execute();
  vlTransform *Transform;
};

#endif


