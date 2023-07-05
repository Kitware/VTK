// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTransmitStructuredGridPiece.h"

#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTransmitStructuredGridPiece);

//------------------------------------------------------------------------------
vtkTransmitStructuredGridPiece::vtkTransmitStructuredGridPiece() = default;

//------------------------------------------------------------------------------
vtkTransmitStructuredGridPiece::~vtkTransmitStructuredGridPiece() = default;

//------------------------------------------------------------------------------
void vtkTransmitStructuredGridPiece::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
