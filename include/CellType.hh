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

#define vlNULL_ELEMENT 0
#define vlVERTEX 1
#define vlPOLY_VERTEX 2
#define vlLINE 3
#define vlPOLY_LINE 4
#define vlTRIANGLE 5
#define vlTRIANGLE_STRIP 6
#define vlPOLYGON 7
#define vlPIXEL 8
#define vlQUAD 9
#define vlTETRA 10
#define vlVOXEL 11
#define vlHEXAHEDRON 12

class vlCellTypes
{
public:
  vlCell *MapType(int type);
};

#endif


