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
#include "vtkOverlappingAMR2.h"
#include "vtkUniformGridAMRDataIterator2.h"
#include "vtkObjectFactory.h"
#include "vtkAMRInformation.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUniformGrid.h"
#include "vtkInformationIdTypeKey.h"
#include "vtkCellData.h"
#include <vector>

vtkStandardNewMacro(vtkOverlappingAMR2);

vtkInformationKeyMacro(vtkOverlappingAMR2,NUMBER_OF_BLANKED_POINTS,IdType);

//----------------------------------------------------------------------------
vtkOverlappingAMR2::vtkOverlappingAMR2()
{
}

//----------------------------------------------------------------------------
vtkOverlappingAMR2::~vtkOverlappingAMR2()
{
}

//----------------------------------------------------------------------------
void vtkOverlappingAMR2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  if(this->AMRInfo)
    {
    this->AMRInfo->PrintSelf(os,indent);
    }
}

//----------------------------------------------------------------------------
vtkCompositeDataIterator* vtkOverlappingAMR2::NewIterator()
{
  vtkUniformGridAMRDataIterator2* iter =  vtkUniformGridAMRDataIterator2::New();
  iter->SetDataSet( this );
  return iter;
}

//----------------------------------------------------------------------------
void vtkOverlappingAMR2::SetRefinementRatio(unsigned int level,
                                                   int ratio)
{
  this->AMRInfo->SetRefinementRatio(level,ratio);
}

//----------------------------------------------------------------------------
int vtkOverlappingAMR2::GetRefinementRatio(unsigned int level)
{
  if(!AMRInfo->HasRefinementRatio())
    {
    AMRInfo->GenerateRefinementRatio();
    }
  return this->AMRInfo->GetRefinementRatio(level);
}


//----------------------------------------------------------------------------
int vtkOverlappingAMR2::GetRefinementRatio(vtkCompositeDataIterator* iter)
{
  vtkUniformGridAMRDataIterator2 *amrIter =
      vtkUniformGridAMRDataIterator2::SafeDownCast( iter );

  unsigned int level = amrIter->GetCurrentLevel();
  return this->AMRInfo->GetRefinementRatio(level);
}

//----------------------------------------------------------------------------
void vtkOverlappingAMR2::GenerateParentChildInformation()
{
  this->AMRInfo->GenerateParentChildInformation();
}

//----------------------------------------------------------------------------
bool vtkOverlappingAMR2::
HasChildrenInformation()
{
  return AMRInfo->HasChildrenInformation();
}

//----------------------------------------------------------------------------
unsigned int *vtkOverlappingAMR2::
GetParents(unsigned int level, unsigned int index, unsigned int& num)
{
  return this->AMRInfo->GetParents(level,index,num);
}

//------------------------------------------------------------------------------
unsigned int *vtkOverlappingAMR2::
GetChildren(unsigned int level, unsigned int index, unsigned int& num)
{
  return this->AMRInfo->GetChildren(level,index,num);
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR2::
PrintParentChildInfo(unsigned int level, unsigned int index)
{
  this->AMRInfo->PrintParentChildInfo(level,index);
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR2::SetAMRBox(unsigned int level, unsigned int id, const vtkAMRBox& box)
{
  this->AMRInfo->SetAMRBox(level,id,box);
}

//------------------------------------------------------------------------------
const vtkAMRBox& vtkOverlappingAMR2::GetAMRBox(unsigned int level, unsigned int id)
{
  const vtkAMRBox& box = this->AMRInfo->GetAMRBox(level,id);
  if(box.IsInvalid())
    {
    vtkErrorMacro("Invalid AMR box");
    }
  return box;
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR2::SetSpacing(unsigned int level,const double spacing[3])
{
  this->AMRInfo->SetSpacing(level,spacing);
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR2::GetSpacing(unsigned int level, double spacing[3])
{
  return this->AMRInfo->GetSpacing(level,spacing);
}

//----------------------------------------------------------------------------
void vtkOverlappingAMR2::GetBounds(unsigned int level, unsigned int id, double* bb)
{
  this->AMRInfo->GetBounds(level, id, bb);
}

//----------------------------------------------------------------------------
void vtkOverlappingAMR2::GetOrigin(unsigned int level, unsigned int id, double origin[3])
{
  double bb[6];
  this->GetBounds(level,id,bb);
  origin[0] = bb[0];
  origin[1] = bb[2];
  origin[2] = bb[4];
}


//----------------------------------------------------------------------------
void vtkOverlappingAMR2::SetOrigin(const double* origin)
{
  return this->AMRInfo->SetOrigin(origin);
}

//----------------------------------------------------------------------------
double* vtkOverlappingAMR2::GetOrigin()
{
  return this->AMRInfo->GetOrigin();
}

//----------------------------------------------------------------------------
void vtkOverlappingAMR2::SetAMRBlockSourceIndex(unsigned int level, unsigned int id, int sourceId)
{
  unsigned int index = this->AMRInfo->GetIndex(level,id);
  this->AMRInfo->SetAMRBlockSourceIndex(index, sourceId);
}

//----------------------------------------------------------------------------
int vtkOverlappingAMR2::GetAMRBlockSourceIndex(unsigned int level, unsigned int id)
{
  unsigned int index = this->AMRInfo->GetIndex(level,id);
  return this->AMRInfo->GetAMRBlockSourceIndex(index);
}

//----------------------------------------------------------------------------
void vtkOverlappingAMR2::Audit()
{
  this->AMRInfo->Audit();

  int emptyDimension(-1);
  switch (this->GetGridDescription())
    {
    case VTK_YZ_PLANE: emptyDimension = 0; break;
    case VTK_XZ_PLANE: emptyDimension = 1; break;
    case VTK_XY_PLANE: emptyDimension = 2; break;
    }

  vtkSmartPointer<vtkUniformGridAMRDataIterator2> iter;
  iter.TakeReference(vtkUniformGridAMRDataIterator2::SafeDownCast(this->NewIterator()));
  iter->SetSkipEmptyNodes(1);
  for(iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkUniformGrid* grid = vtkUniformGrid::SafeDownCast(iter->GetCurrentDataObject());
    bool hasGhost  = grid->GetCellData()->GetArray("vtkGhostLevels")!=NULL;

    unsigned int level = iter->GetCurrentLevel();
    unsigned int id = iter->GetCurrentIndex();
    const vtkAMRBox& box = this->AMRInfo->GetAMRBox(level,id);
    int dims[3];
    box.GetNumberOfNodes(dims);

    double spacing[3];
    this->GetSpacing(level,spacing);

    double origin[3];
    this->GetOrigin(level,id,origin);

    for(int d = 0; d<3; d++)
      {
      if(d==emptyDimension)
        {
        if(grid->GetSpacing()[d]!=spacing[d])
          {
          vtkErrorMacro("The grid spacing does not match AMRInfo at ("<<level<<", "<<id<<")");
          }
        if(!hasGhost && grid->GetOrigin()[d]!=origin[d])
          {
          vtkErrorMacro("The grid origin does not match AMRInfo at ("<<level<<", "<<id<<")");
          }
        if(!hasGhost && grid->GetDimensions()[d]!=dims[d])
          {
          vtkErrorMacro("The grid dimensions does not match AMRInfo at ("<<level<<", "<<id<<")");
          }
        }
      }
    }
}


bool vtkOverlappingAMR2::FindGrid(double q[3], unsigned int& level, unsigned int& gridId)
{
  return this->AMRInfo->FindGrid(q,level,gridId);
}
