/*=========================================================================

  Program:   Visualization Library
  Module:    CellType.hh
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
// Class to localize mapping of cell types to object.  Adding new cell type 
// means adding new & unique #define and then implementing the object.
//
#ifndef __vlCellTypes_h
#define __vlCellTypes_h

class vlCell;

#define vlPOINT 0
#define vlPOLY_POINTS 1
#define vlLINE 2
#define vlPOLY_LINE 3
#define vlTRIANGLE 4
#define vlTRIANGLE_STRIP 5
#define vlPOLYGON 6
#define vlRECTANGLE 7
#define vlQUAD 8
#define vlTETRA 9
#define vlBRICK 10
#define vlHEXAHEDRON 11

class vlCellTypes
{
public:
  vlCell *MapType(int type);
};

#endif


