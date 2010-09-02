/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkImageItem.h"

#include "vtkContext2D.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkImageItem);

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkImageItem, Image, vtkImageData);

//-----------------------------------------------------------------------------
vtkImageItem::vtkImageItem()
{
  this->Position[0] = this->Position[1] = 0;
  this->Image = NULL;
}

//-----------------------------------------------------------------------------
vtkImageItem::~vtkImageItem()
{
  this->SetImage(NULL);
}

//-----------------------------------------------------------------------------
bool vtkImageItem::Paint(vtkContext2D *painter)
{
  if (this->Image)
    {
    // Draw our image in the bottom left corner of the item
    painter->DrawImage(this->Position[0], this->Position[1], this->Image);
    }
  return true;
}

//-----------------------------------------------------------------------------
void vtkImageItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
