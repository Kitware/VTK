/*=========================================================================

  Program:   Visualization Library
  Module:    CellType.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlCellType - define types of cells
// .SECTION Description
// vlCellType defines the allowable cell types in the visualization 
// library (vl). In vl, datasets consist of collections of cells. 
// Different datasets consist of different cell types. The cells may be 
// explicitly represented (as in vlPolyData), or may be implicit to the
// data type (vlStructuredPoints).

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


