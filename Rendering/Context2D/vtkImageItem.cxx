// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkImageItem.h"

#include "vtkContext2D.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImageItem);

//------------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkImageItem, Image, vtkImageData);

//------------------------------------------------------------------------------
vtkImageItem::vtkImageItem()
{
  this->Position[0] = this->Position[1] = 0;
  this->Image = nullptr;
}

//------------------------------------------------------------------------------
vtkImageItem::~vtkImageItem()
{
  this->SetImage(nullptr);
}

//------------------------------------------------------------------------------
bool vtkImageItem::Paint(vtkContext2D* painter)
{
  if (this->Image)
  {
    int dims[3];
    this->Image->GetDimensions(dims);
    if (dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
    {
      return true;
    }

    // Draw our image in the bottom left corner of the item
    painter->DrawImage(this->Position[0], this->Position[1], this->Image);
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkImageItem::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
