/*=========================================================================

  Program:   Visualization Library
  Module:    RibbonF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlRibbonFilter - create oriented ribbons from lines defined in polygonal dataset
// .SECTION Description
// vlRibbonFilter is a filter to create oriented ribbons from lines defined
// in polygonal dataset. The orientation of the ribbon is perpendicular to the 
// line normals (if any). If no line normals are defined, then normals are 
// computed. An offset angle can be specified to rotate the ribbon with respect
// to the normal. This is useful to construct a tube from ribbons.

#ifndef __vlRibbonFilter_h
#define __vlRibbonFilter_h

#include "P2PF.hh"

class vlRibbonFilter : public vlPolyToPolyFilter 
{
public:
  vlRibbonFilter();
  ~vlRibbonFilter() {};
  char *GetClassName() {return "vlRibbonFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Set the radius (offset distance) of the ribbon from the line.
  vlSetClampMacro(Radius,float,0,LARGE_FLOAT);
  vlGetMacro(Radius,float);

  // Description:
  // Set the offset angle of the ribbon from the line normal.
  vlSetClampMacro(Angle,float,0,360);
  vlGetMacro(Angle,float);

protected:
  void Execute();
  float Radius;
  float Angle;
};

#endif


