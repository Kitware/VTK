/*=========================================================================

  Program:   Visualization Library
  Module:    CellArr.cc
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
#include "CellArr.hh"

vlCellArray::vlCellArray (const vlCellArray& ca)
{
  this->NumberOfCells = ca.NumberOfCells;
  this->Location = 0;
  this->Ia = ca.Ia;
}
