/*=========================================================================

  Program:   Visualization Library
  Module:    IdList.cc
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
#include "IdList.hh"
#include "Cell.hh"

void vlIdList::DeleteId(int cellId)
{
  static vlIdList tempList(MAX_CELL_SIZE);
  int i, id;

  tempList.Reset();
  for (i=0; i < this->GetNumberOfIds(); i++)
    {
    if ( (id = this->GetId(i)) != cellId ) tempList.InsertNextId(id);
    }

  this->Reset();
  for (i=0; i < tempList.GetNumberOfIds(); i++)
    {
    this->InsertNextId(tempList.GetId(i));
    }
}


void vlIdList::IntersectWith(vlIdList *otherIds)
{
  int id;

  for ( int i=0; i < this->GetNumberOfIds(); i++ )
    {
    id =  this->GetId(i);
    if ( ! otherIds->IsId(id) ) this->DeleteId(id);
    }
}
