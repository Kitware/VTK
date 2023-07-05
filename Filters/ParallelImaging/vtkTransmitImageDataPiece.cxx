// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTransmitImageDataPiece.h"

#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTransmitImageDataPiece);

//------------------------------------------------------------------------------
vtkTransmitImageDataPiece::vtkTransmitImageDataPiece() = default;

//------------------------------------------------------------------------------
vtkTransmitImageDataPiece::~vtkTransmitImageDataPiece() = default;

//------------------------------------------------------------------------------
void vtkTransmitImageDataPiece::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
