/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationIterator.h"

#include "vtkInformation.h"
#include "vtkInformationInternals.h"
#include "vtkInformationKey.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkInformationIterator, "1.3");
vtkStandardNewMacro(vtkInformationIterator);

vtkCxxSetObjectMacro(vtkInformationIterator, Information, vtkInformation);

//----------------------------------------------------------------------------
vtkInformationIterator::vtkInformationIterator()
{
  this->Information = 0;
  this->Index = 0;
}

//----------------------------------------------------------------------------
vtkInformationIterator::~vtkInformationIterator()
{
  if (this->Information)
    {
    this->Information->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkInformationIterator::GoToFirstItem()
{
  if (!this->Information)
    {
    vtkErrorMacro("No information has been set.");
    return;
    }
  this->Index = 0;
  vtkInformationKey** keys = this->Information->Internal->Keys;
  unsigned short tableSize = this->Information->Internal->TableSize;
  if (!keys)
    {
    return;
    }

  while(!keys[this->Index] && this->Index < tableSize)
    {
    this->Index++;
    }
}

//----------------------------------------------------------------------------
void vtkInformationIterator::GoToNextItem()
{
  if (!this->Information)
    {
    vtkErrorMacro("No information has been set.");
    return;
    }

  vtkInformationKey** keys = this->Information->Internal->Keys;
  unsigned short tableSize = this->Information->Internal->TableSize;

  this->Index++;
  while(this->Index < tableSize && !keys[this->Index])
    {
    this->Index++;
    }
}

//----------------------------------------------------------------------------
int vtkInformationIterator::IsDoneWithTraversal()
{
  if (!this->Information)
    {
    vtkErrorMacro("No information has been set.");
    return 1;
    }

  if (this->Index >= this->Information->Internal->TableSize)
    {
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformationIterator::GetCurrentKey()
{
  if (this->IsDoneWithTraversal())
    {
    return 0;
    }

  vtkInformationKey** keys = this->Information->Internal->Keys;
  return keys[this->Index];
}

//----------------------------------------------------------------------------
void vtkInformationIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Information: ";
  if (this->Information)
    {
    os << endl;
    this->Information->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}

