/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalBoxDataSet.h"

#include "vtkAMRBox.h"
#include "vtkHierarchicalBoxDataIterator.h"
#include "vtkInformation.h"
#include "vtkInformationIdTypeKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationKey.h"
#include "vtkInformationVector.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"

#include <vtkstd/vector>
#include <assert.h>

vtkStandardNewMacro(vtkHierarchicalBoxDataSet);
vtkInformationKeyRestrictedMacro(vtkHierarchicalBoxDataSet,BOX,IntegerVector, 6);
vtkInformationKeyMacro(vtkHierarchicalBoxDataSet,NUMBER_OF_BLANKED_POINTS,IdType);
vtkInformationKeyMacro(vtkHierarchicalBoxDataSet,REFINEMENT_RATIO,Integer);
vtkInformationKeyMacro(vtkHierarchicalBoxDataSet,BOX_DIMENSIONALITY,Integer);

typedef vtkstd::vector<vtkAMRBox> vtkAMRBoxList;
//----------------------------------------------------------------------------
vtkHierarchicalBoxDataSet::vtkHierarchicalBoxDataSet()
{
  this->ScalarRange[0]=VTK_DOUBLE_MAX;
  this->ScalarRange[1]=VTK_DOUBLE_MIN;
}

//----------------------------------------------------------------------------
vtkHierarchicalBoxDataSet::~vtkHierarchicalBoxDataSet()
{
}

//----------------------------------------------------------------------------
vtkCompositeDataIterator* vtkHierarchicalBoxDataSet::NewIterator()
{
  vtkHierarchicalBoxDataIterator* iter = vtkHierarchicalBoxDataIterator::New();
  iter->SetDataSet(this);
  return iter;
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::SetNumberOfLevels(unsigned int numLevels)
{
  this->Superclass::SetNumberOfChildren(numLevels);

  // Initialize each level with a vtkMultiPieceDataSet. 
  // vtkMultiPieceDataSet is an overkill here, since the datasets with in a
  // level cannot be composite datasets themselves. 
  // This will make is possible for the user to set information with each level
  // (in future).
  for (unsigned int cc=0; cc < numLevels; cc++)
    {
    if (!this->Superclass::GetChild(cc))
      {
      vtkMultiPieceDataSet* mds = vtkMultiPieceDataSet::New();
      this->Superclass::SetChild(cc, mds);
      mds->Delete();
      }
    }
}

//----------------------------------------------------------------------------
unsigned int vtkHierarchicalBoxDataSet::GetNumberOfLevels()
{
  return this->Superclass::GetNumberOfChildren();
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::SetNumberOfDataSets(unsigned int level, 
  unsigned int numDS)
{
  if (level >= this->GetNumberOfLevels())
    {
    this->SetNumberOfLevels(level+1);
    }
  vtkMultiPieceDataSet* levelDS = vtkMultiPieceDataSet::SafeDownCast(
    this->Superclass::GetChild(level));
  if (levelDS)
    {
    levelDS->SetNumberOfPieces(numDS);
    }
}

//----------------------------------------------------------------------------
unsigned int vtkHierarchicalBoxDataSet::GetNumberOfDataSets(unsigned int level)
{
  vtkMultiPieceDataSet* levelDS = vtkMultiPieceDataSet::SafeDownCast(
    this->Superclass::GetChild(level));
  if (levelDS)
    {
    return levelDS->GetNumberOfPieces();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::SetDataSet(
  unsigned int level, unsigned int id,
  int LoCorner[3], int HiCorner[3], vtkUniformGrid* dataSet)
{
  vtkAMRBox box(3, LoCorner, HiCorner);
  this->SetDataSet(level, id, box, dataSet);
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::SetDataSet(
  unsigned int level, unsigned int id, vtkAMRBox& box, vtkUniformGrid* dataSet)
{
  if (level >= this->GetNumberOfLevels())
    {
    this->SetNumberOfLevels(level+1);
    }
  vtkMultiPieceDataSet* levelDS = vtkMultiPieceDataSet::SafeDownCast(
    this->Superclass::GetChild(level));
  if (levelDS)
    {
    levelDS->SetPiece(id, dataSet);
    vtkInformation* info = levelDS->GetMetaData(id);
    if (info)
      {
      const int *loCorner=box.GetLoCorner();
      const int *hiCorner=box.GetHiCorner();
      info->Set(BOX_DIMENSIONALITY(), box.GetDimensionality());
      info->Set(BOX(),
        loCorner[0], loCorner[1], loCorner[2],
        hiCorner[0], hiCorner[1], hiCorner[2]);
      }
    }
}

//----------------------------------------------------------------------------
vtkUniformGrid* vtkHierarchicalBoxDataSet::GetDataSet(unsigned int level,
                                                      unsigned int id,
                                                      vtkAMRBox& box)
{
  if (this->GetNumberOfLevels() <= level ||
    this->GetNumberOfDataSets(level) <= id)
    {
    return 0;
    }

  vtkMultiPieceDataSet* levelDS = vtkMultiPieceDataSet::SafeDownCast(
    this->Superclass::GetChild(level));
  if (levelDS)
    {
    vtkUniformGrid* ds = vtkUniformGrid::SafeDownCast(levelDS->GetPiece(id));
    vtkInformation* info = levelDS->GetMetaData(id);
    if (info)
      {
      int dimensionality = info->Has(BOX_DIMENSIONALITY())?
        info->Get(BOX_DIMENSIONALITY()) : 3;
      box.SetDimensionality(dimensionality);

      int* boxVec = info->Get(BOX());
      if (boxVec)
        {
        box.SetDimensions(boxVec,boxVec+3);
        }
      }
    return ds;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::SetRefinementRatio(unsigned int level,
                                                   int ratio)
{
  assert("pre: valid_ratio" && ratio>=2);
  if (level >= this->GetNumberOfLevels())
    {
    this->SetNumberOfLevels(level+1);
    }

  vtkInformation* info = this->Superclass::GetChildMetaData(level);
  info->Set(REFINEMENT_RATIO(), ratio);
}

//----------------------------------------------------------------------------
int vtkHierarchicalBoxDataSet::GetRefinementRatio(unsigned int level)
{
  if (!this->Superclass::HasChildMetaData(level))
    {
    return 0;
    }

  vtkInformation* info = this->Superclass::GetChildMetaData(level);
  if (!info)
    {
    return 0;
    }
  return info->Has(REFINEMENT_RATIO())? info->Get(REFINEMENT_RATIO()): 0;
}

//----------------------------------------------------------------------------
int vtkHierarchicalBoxDataSet::GetRefinementRatio(vtkCompositeDataIterator* iter)
{
  if (!this->HasMetaData(iter))
    {
    return 0;
    }
  vtkInformation* info = this->GetMetaData(iter);
  if (!info)
    {
    return 0;
    }
  return info->Has(REFINEMENT_RATIO())? info->Get(REFINEMENT_RATIO()): 0;
}

//----------------------------------------------------------------------------
vtkInformation* vtkHierarchicalBoxDataSet::GetMetaData(unsigned int level,
  unsigned int index)
{
  vtkMultiPieceDataSet* levelMDS = vtkMultiPieceDataSet::SafeDownCast(
    this->GetChild(level));
  if (levelMDS)
    {
    return levelMDS->GetMetaData(index);
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkHierarchicalBoxDataSet::HasMetaData(unsigned int level,
  unsigned int index)
{
  vtkMultiPieceDataSet* levelMDS = vtkMultiPieceDataSet::SafeDownCast(
    this->GetChild(level));
  if (levelMDS)
    {
    return levelMDS->HasMetaData(index);
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkHierarchicalBoxDataSetIsInBoxes(vtkAMRBoxList& boxes,
                                       int i, int j, int k)
{
  vtkAMRBoxList::iterator it=boxes.begin();
  vtkAMRBoxList::iterator end=boxes.end();
  for(; it!=end; ++it)
    {
    if (it->Contains(i, j, k))
      {
      return 1;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::GenerateVisibilityArrays()
{
  unsigned int numLevels = this->GetNumberOfLevels();

  for (unsigned int levelIdx=0; levelIdx<numLevels; levelIdx++)
    {
    // Copy boxes of higher level and coarsen to this level
    vtkAMRBoxList boxes;
    unsigned int numDataSets = this->GetNumberOfDataSets(levelIdx+1);
    unsigned int dataSetIdx;
    if (levelIdx < numLevels - 1)
      {
      for (dataSetIdx=0; dataSetIdx<numDataSets; dataSetIdx++)
        {
        if (!this->HasMetaData(levelIdx+1, dataSetIdx) ||
          !this->HasLevelMetaData(levelIdx))
          {
          continue;
          }
        vtkInformation* info = this->GetMetaData(
            levelIdx+1,dataSetIdx);
        int* boxVec = info->Get(BOX());
        int dimensionality = info->Has(BOX_DIMENSIONALITY())?
          info->Get(BOX_DIMENSIONALITY()) : 3;
        vtkAMRBox coarsebox(dimensionality,boxVec,boxVec+3);
        int refinementRatio = this->GetRefinementRatio(levelIdx);
        if (refinementRatio == 0)
          {
          continue;
          }
        coarsebox.Coarsen(refinementRatio);
        boxes.push_back(coarsebox);
        }
      }

    numDataSets = this->GetNumberOfDataSets(levelIdx);
    for (dataSetIdx=0; dataSetIdx<numDataSets; dataSetIdx++)
      {
      vtkAMRBox box;
      vtkUniformGrid* grid = this->GetDataSet(levelIdx, dataSetIdx, box);
      if (grid && !box.Empty())
        {
        int cellDims[3];
        box.GetNumberOfCells(cellDims);
        vtkUnsignedCharArray* vis = vtkUnsignedCharArray::New();
        vtkIdType numCells = box.GetNumberOfCells();
        vis->SetNumberOfTuples(numCells);
        vis->FillComponent(0,static_cast<char>(1));
        vtkIdType numBlankedPts = 0;
        if (!boxes.empty()) 
          {
          const int *loCorner=box.GetLoCorner();
          const int *hiCorner=box.GetHiCorner();
          for (int iz=loCorner[2]; iz<=hiCorner[2]; iz++)
            {
            for (int iy=loCorner[1]; iy<=hiCorner[1]; iy++)
              {
              for (int ix=loCorner[0]; ix<=hiCorner[0]; ix++)
                {
                // Blank if cell is covered by a box of higher level
                if (vtkHierarchicalBoxDataSetIsInBoxes(boxes, ix, iy, iz))
                  {
                  vtkIdType id =
                    (iz-loCorner[2])*cellDims[0]*cellDims[1] +
                    (iy-loCorner[1])*cellDims[0] +
                    (ix-loCorner[0]);
                  vis->SetValue(id, 0);
                  numBlankedPts++;
                  }
                }
              }
            }
          }
        grid->SetCellVisibilityArray(vis);
        vis->Delete();
        if (this->HasMetaData(levelIdx, dataSetIdx))
          {
          vtkInformation* infotmp =
            this->GetMetaData(levelIdx,dataSetIdx);
          infotmp->Set(NUMBER_OF_BLANKED_POINTS(), numBlankedPts);
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
vtkAMRBox vtkHierarchicalBoxDataSet::GetAMRBox(vtkCompositeDataIterator* iter)
{
  vtkAMRBox box;
  if (this->HasMetaData(iter))
    {
    vtkInformation* info = this->GetMetaData(iter);
    int dimensionality = info->Has(BOX_DIMENSIONALITY())?
      info->Get(BOX_DIMENSIONALITY()) : 3;
    box.SetDimensionality(dimensionality);
    int* boxVec = info->Get(BOX());
    if (boxVec)
      {
      box.SetDimensions(boxVec,boxVec+3);
      }
    }
  return box;
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  /*
  unsigned int numLevels = this->GetNumberOfLevels();
  os << indent << "Number of levels: " <<  numLevels << endl;
  for (unsigned int i=0; i<numLevels; i++)
    {
    unsigned int numDataSets = this->GetNumberOfDataSets(i);
    os << indent << "Level " << i << " number of datasets: " << numDataSets
       << endl;
    for (unsigned j=0; j<numDataSets; j++)
      {
      os << indent << "DataSet(" << i << "," << j << "):";
      vtkDataObject* dobj = this->GetDataSet(i, j);
      if (dobj)
        {
        os << endl;
        dobj->PrintSelf(os, indent.GetNextIndent());
        }
      else
        {
        os << "(none)" << endl;
        }
      }
    }
    */
}

//----------------------------------------------------------------------------
unsigned int vtkHierarchicalBoxDataSet::GetFlatIndex(unsigned int level, 
  unsigned int index)
{
  if (level > this->GetNumberOfLevels() || index > this->GetNumberOfDataSets(level))
    {
    // invalid level, index.
    vtkErrorMacro("Invalid level (" << level << ") or index (" << index << ")");
    return 0;
    }

  unsigned int findex=0;
  for (unsigned int l=0; l < level; l++)
    {
    findex += 1;
    findex += this->GetNumberOfDataSets(l);
    }
  findex += 1;
  findex += (index + 1);
  return findex;
}

//----------------------------------------------------------------------------
vtkHierarchicalBoxDataSet* vtkHierarchicalBoxDataSet::GetData(
  vtkInformation* info)
{
  return
    info?vtkHierarchicalBoxDataSet::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkHierarchicalBoxDataSet* vtkHierarchicalBoxDataSet::GetData(
  vtkInformationVector* v, int i)
{
  return vtkHierarchicalBoxDataSet::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
// Description:
// Copy the cached scalar range into range.
void vtkHierarchicalBoxDataSet::GetScalarRange(double range[2])
{
  this->ComputeScalarRange();
  range[0]=this->ScalarRange[0];
  range[1]=this->ScalarRange[1];
}
  
//----------------------------------------------------------------------------
// Description:
// Return the cached range.
double *vtkHierarchicalBoxDataSet::GetScalarRange()
{
  this->ComputeScalarRange();
  return this->ScalarRange;
}

//----------------------------------------------------------------------------
// Description:
// Compute the range of the scalars and cache it into ScalarRange
// only if the cache became invalid (ScalarRangeComputeTime).
void vtkHierarchicalBoxDataSet::ComputeScalarRange()
{
  if ( this->GetMTime() > this->ScalarRangeComputeTime )
    {
    double dataSetRange[2];
    this->ScalarRange[0]=VTK_DOUBLE_MAX;
    this->ScalarRange[1]=VTK_DOUBLE_MIN;
    unsigned int level=0;
    unsigned int levels=this->GetNumberOfLevels();
    vtkAMRBox temp;
    while(level<levels)
      {
      unsigned int dataset=0;
      unsigned int datasets=this->GetNumberOfDataSets(level);
      while(dataset<datasets)
        {
        vtkUniformGrid *ug = 
          static_cast<vtkUniformGrid*>(this->GetDataSet(level, dataset, temp));
        ug->GetScalarRange(dataSetRange);
        if(dataSetRange[0]<this->ScalarRange[0])
          {
          this->ScalarRange[0]=dataSetRange[0];
          }
        if(dataSetRange[1]>this->ScalarRange[1])
          {
          this->ScalarRange[1]=dataSetRange[1];
          }
        ++dataset;
        }
      ++level;
      }
    this->ScalarRangeComputeTime.Modified();
    }
}
