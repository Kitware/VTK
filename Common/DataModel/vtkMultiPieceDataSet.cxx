/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiPieceDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiPieceDataSet.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkMultiPieceDataSet);
//----------------------------------------------------------------------------
vtkMultiPieceDataSet::vtkMultiPieceDataSet() = default;

//----------------------------------------------------------------------------
vtkMultiPieceDataSet::~vtkMultiPieceDataSet() = default;

//----------------------------------------------------------------------------
vtkMultiPieceDataSet* vtkMultiPieceDataSet::GetData(vtkInformation* info)
{
  return info ? vtkMultiPieceDataSet::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//----------------------------------------------------------------------------
vtkMultiPieceDataSet* vtkMultiPieceDataSet::GetData(vtkInformationVector* v, int i)
{
  return vtkMultiPieceDataSet::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
void vtkMultiPieceDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
