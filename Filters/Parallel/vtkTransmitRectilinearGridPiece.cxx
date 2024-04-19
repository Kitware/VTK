// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTransmitRectilinearGridPiece.h"

#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTransmitRectilinearGridPiece);

//------------------------------------------------------------------------------
vtkTransmitRectilinearGridPiece::vtkTransmitRectilinearGridPiece() = default;

//------------------------------------------------------------------------------
vtkTransmitRectilinearGridPiece::~vtkTransmitRectilinearGridPiece() = default;

//------------------------------------------------------------------------------
void vtkTransmitRectilinearGridPiece::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
