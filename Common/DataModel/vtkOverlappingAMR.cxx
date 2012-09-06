/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUniformGridAMR.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOverlappingAMR.h"
#include "vtkUniformGridAMRDataIterator.h"
#include "vtkObjectFactory.h"
#include "vtkAMRInformation.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUniformGrid.h"
#include "vtkInformationIdTypeKey.h"
#include <vector>

vtkStandardNewMacro(vtkOverlappingAMR);

vtkInformationKeyMacro(vtkOverlappingAMR,NUMBER_OF_BLANKED_POINTS,IdType);

//----------------------------------------------------------------------------
vtkOverlappingAMR::vtkOverlappingAMR()
{
  this->PadCellVisibility = false;
}

//----------------------------------------------------------------------------
vtkOverlappingAMR::~vtkOverlappingAMR()
{
}

//----------------------------------------------------------------------------
void vtkOverlappingAMR::Initialize(int numLevels, int * blocksPerLevel,  double origin[3],  int gridDescription)
{
  Superclass::Initialize();
  this->AMRInfo->Initialize(numLevels,blocksPerLevel,origin,gridDescription);
}

//----------------------------------------------------------------------------
void vtkOverlappingAMR::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << "PadCellVisibility: ";
  if( this->PadCellVisibility )
    {
    os << "ON(";
    }
  else
    {
    os << "OFF(";
    }
  os << this->PadCellVisibility << ")\n";
}

//----------------------------------------------------------------------------
vtkCompositeDataIterator* vtkOverlappingAMR::NewIterator()
{
  vtkUniformGridAMRDataIterator* iter =  vtkUniformGridAMRDataIterator::New();
  iter->SetDataSet( this );
  return iter;
}

//----------------------------------------------------------------------------
void vtkOverlappingAMR::SetRefinementRatio(unsigned int level,
                                                   int ratio)
{
  this->AMRInfo->SetRefinementRatio(level,ratio);
}

//----------------------------------------------------------------------------
int vtkOverlappingAMR::GetRefinementRatio(unsigned int level)
{
  if(!AMRInfo->HasRefinementRatio())
    {
    AMRInfo->GenerateRefinementRatio();
    }
  return this->AMRInfo->GetRefinementRatio(level);
}


//----------------------------------------------------------------------------
int vtkOverlappingAMR::GetRefinementRatio(vtkCompositeDataIterator* iter)
{
  vtkUniformGridAMRDataIterator *amrIter =
      vtkUniformGridAMRDataIterator::SafeDownCast( iter );

  unsigned int level = amrIter->GetCurrentLevel();
  return this->AMRInfo->GetRefinementRatio(level);
}

//----------------------------------------------------------------------------
void vtkOverlappingAMR::GenerateParentChildInformation()
{
  this->AMRInfo->GenerateParentChildInformation();
}
//----------------------------------------------------------------------------
bool vtkOverlappingAMR::
HasChildrenInformation()
{
  return AMRInfo->HasChildrenInformation();
}

//----------------------------------------------------------------------------
unsigned int *vtkOverlappingAMR::
GetParents(unsigned int level, unsigned int index, unsigned int& num)
{
  return this->AMRInfo->GetParents(level,index,num);
}

//------------------------------------------------------------------------------
unsigned int *vtkOverlappingAMR::
GetChildren(unsigned int level, unsigned int index, unsigned int& num)
{
  return this->AMRInfo->GetChildren(level,index,num);
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR::
PrintParentChildInfo(unsigned int level, unsigned int index)
{
  this->AMRInfo->PrintParentChildInfo(level,index);
}

//------------------------------------------------------------------------------
const vtkAMRBox& vtkOverlappingAMR::GetAMRBox(unsigned int level, unsigned int id)
{
  const vtkAMRBox& box = this->AMRInfo->GetAMRBox(level,id);
  if(box.IsInvalid())
    {
    vtkErrorMacro("Invalid AMR box");
    }
  return box;
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR::SetAMRBox(unsigned int level, unsigned int id, double* min, int* dimensions, double* h)
{
  this->AMRInfo->SetAMRBox(level,id,min,dimensions,h);
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR::GetGridSpacing(unsigned int level, double spacing[3])
{
  return this->AMRInfo->GetSpacing(level,spacing);
}

//----------------------------------------------------------------------------
void vtkOverlappingAMR::GetBounds(unsigned int level, unsigned int id, double* bb)
{
  this->AMRInfo->GetBounds(level, id, bb);
}

//----------------------------------------------------------------------------
double* vtkOverlappingAMR::GetOrigin()
{
  return this->AMRInfo->GetOrigin();
}
