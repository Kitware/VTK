/*=========================================================================

  Program:   Visualization Library
  Module:    CellPick.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlCellPicker - select a cell by shooting a ray into graphics window
// .SECTION Description
// vlCellPicker is used to select a cell by shooting a ray into graphics
// window and intersecting with actor's defining geometry - specifically 
// its cells. Beside returning coordinates, actor and mapper, vlCellPicker
// returns the id of the closest cell within the tolerance along the pick
// ray, and the dataset that was picked.
// .SECTION See Also
// For quick picking, see vlPicker. To pick points, see vlPointPicker.

#ifndef __vlCellPicker_h
#define __vlCellPicker_h

#include "Picker.hh"

class vlCellPicker : public vlPicker
{
public:
  vlCellPicker();
  ~vlCellPicker() {};
  char *GetClassName() {return "vlCellPicker";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Get the id of the picked cell. If CellId = -1, nothing was picked.
  vlGetMacro(CellId,int);

  // Description:
  // Get the subId of the picked cell. If SubId = -1, nothing was picked.
  vlGetMacro(SubId,int);

  // Description:
  // Get the parametric coordinates of the picked cell. Only valid is 
  // pick was made.
  vlGetVectorMacro(PCoords,float,3);

protected:
  int CellId; // picked cell
  int SubId; // picked cell subId
  float PCoords[3]; // picked cell parametric coordinates

  void IntersectWithLine(float p1[3], float p2[3], float tol, 
                         vlActor *a, vlMapper *m);
  void Initialize();
};

#endif


