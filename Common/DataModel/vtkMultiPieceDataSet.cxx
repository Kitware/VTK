// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMultiPieceDataSet.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkMultiPieceDataSet);
//------------------------------------------------------------------------------
vtkMultiPieceDataSet::vtkMultiPieceDataSet() = default;

//------------------------------------------------------------------------------
vtkMultiPieceDataSet::~vtkMultiPieceDataSet() = default;

//------------------------------------------------------------------------------
vtkMultiPieceDataSet* vtkMultiPieceDataSet::GetData(vtkInformation* info)
{
  return info ? vtkMultiPieceDataSet::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//------------------------------------------------------------------------------
vtkMultiPieceDataSet* vtkMultiPieceDataSet::GetData(vtkInformationVector* v, int i)
{
  return vtkMultiPieceDataSet::GetData(v->GetInformationObject(i));
}

//------------------------------------------------------------------------------
void vtkMultiPieceDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
