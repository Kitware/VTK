/*=========================================================================

  Program:   Visualization Library
  Module:    PtPicker.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlPointPicker - select a point by shooting a ray into graphics window
// .SECTION Description
// vlPointPicker is used to select a point by shooting a ray into graphics
// window and intersecting with actor's defining geometry - specifically 
// its points. Beside returning coordinates, actor and mapper, vlPointPicker
// returns the id of the closest point within the tolerance along the pick ray.
// .SECTION See Also
// For quick picking, see vlPicker. To uniquely pick actors, see vlCellPicker.

#ifndef __vlPointPicker_h
#define __vlPointPicker_h

#include "Picker.hh"

class vlPointPicker : public vlPicker
{
public:
  vlPointPicker();
  ~vlPointPicker() {};
  char *GetClassName() {return "vlPointPicker";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Get the id of the picked point. If PointId = -1, nothing was picked.
  vlGetMacro(PointId,int);

protected:
  int PointId; //picked point

  void IntersectWithLine(float p1[3], float p2[3], float tol, 
                         vlActor *a, vlMapper *m);
  void Initialize();

};

#endif


