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

namespace
{
  void BlankGridsAtLevel(vtkOverlappingAMR* amr, int levelIdx, std::vector<std::vector<unsigned int> >& children)
  {
    unsigned int numDataSets = amr->GetNumberOfDataSets(levelIdx);
    int N;

    for( unsigned int dataSetIdx=0; dataSetIdx<numDataSets; dataSetIdx++)
      {
      const vtkAMRBox& box = amr->GetAMRBox(levelIdx, dataSetIdx);
      vtkUniformGrid* grid = amr->GetDataSet(levelIdx, dataSetIdx);
      if (grid == NULL )
        {
        continue;
        }
      N = grid->GetNumberOfCells();

      vtkUnsignedCharArray* vis = vtkUnsignedCharArray::New();
      vis->SetName("visibility");
      vis->SetNumberOfTuples( N );
      vis->FillComponent(0,static_cast<char>(1));
      grid->SetCellVisibilityArray(vis);
      vis->Delete();

      if (children.size() <= dataSetIdx)
        continue;

      std::vector<unsigned int>& dsChildren = children[dataSetIdx];
      std::vector<unsigned int>::iterator iter;

      // For each higher res box fill in the cells that
      // it covers
      for (iter=dsChildren.begin(); iter!=dsChildren.end(); iter++)
        {
        vtkAMRBox ibox;;
        if (amr->GetAMRInfo()->GetCoarsenedAMRBox(levelIdx+1, *iter, ibox))
          {
          const int *loCorner=ibox.GetLoCorner();
          int hi[3];
          ibox.GetValidHiCorner(hi);
          for( int iz=loCorner[2]; iz<=hi[2]; iz++ )
            {
            for( int iy=loCorner[1]; iy<=hi[1]; iy++ )
              {
              for( int ix=loCorner[0]; ix<=hi[0]; ix++ )
                {
                vtkIdType id =  vtkAMRBox::GetCellLinearIndex(box,ix, iy, iz, grid->GetDimensions());
                vis->SetValue(id, 0);
                } // END for x
              } // END for y
            } // END for z
          }
        } // Processing all higher boxes for a specific coarse grid
      }
  }
}

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
void vtkOverlappingAMR::GenerateVisibilityArrays()
{
  if(!this->AMRInfo->HasRefinementRatio())
    {
    this->AMRInfo->GenerateRefinementRatio();
    }
  if(!this->AMRInfo->IsValid())
    {
    vtkErrorMacro("Invalid vtkAMRInformation object. Failed to generate visibility arrays");
    return;
    }
  unsigned int numLevels = this->GetNumberOfLevels();
  if(!this->AMRInfo->HasChildrenInformation())
    {
    this->AMRInfo->GenerateParentChildInformation();
    }
  for(unsigned int i=0; i<numLevels; i++)
    {
    BlankGridsAtLevel(this, i, this->AMRInfo->GetChildrenAtLevel(i));
    }
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
