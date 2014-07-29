/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXdmf3ArrayKeeper.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkXdmf3ArrayKeeper.h"

#include "XdmfArray.hpp"

//------------------------------------------------------------------------------
vtkXdmf3ArrayKeeper::vtkXdmf3ArrayKeeper()
{
  generation = 0;
}

//------------------------------------------------------------------------------
vtkXdmf3ArrayKeeper::~vtkXdmf3ArrayKeeper()
{
  this->Release(true);
}

//------------------------------------------------------------------------------
void vtkXdmf3ArrayKeeper::BumpGeneration()
{
  this->generation++;
}

//------------------------------------------------------------------------------
void vtkXdmf3ArrayKeeper::Insert(XdmfArray *val)
{
  this->operator[](val) = this->generation;
}

//------------------------------------------------------------------------------
void vtkXdmf3ArrayKeeper::Release(bool force)
{
  vtkXdmf3ArrayKeeper::iterator it = this->begin();
  //int cnt = 0;
  //int total = 0;
  while (it != this->end())
    {
    //total++;
    vtkXdmf3ArrayKeeper::iterator current = it++;
    if (force || (current->second != this->generation))
      {
      XdmfArray* atCurrent = current->first;
      atCurrent->release();
      this->erase(current);
      //cnt++;
      }
    }
  //cerr << "released " << cnt << "/" << total << " arrays" << endl;
}
