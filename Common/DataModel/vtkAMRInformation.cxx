/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRInformation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAMRInformation.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedIntArray.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkStructuredGrid.h"
#include "vtkBoundingBox.h"
#include "vtkAMRBox.h"
#include "vtkDoubleArray.h"
#include <cassert>
#include <set>

vtkStandardNewMacro(vtkAMRInformation);

#define ReturnFalseIfMacro(b,msg) { if(b) { std::cerr<<msg<<std::endl; return false;}}

namespace
{
  inline bool Inside(double q[3], double gbounds[6])
  {
    if ((q[0] < gbounds[0]) || (q[0] > gbounds[1]) ||
        (q[1] < gbounds[2]) || (q[1] > gbounds[3]) ||
        (q[2] < gbounds[4]) || (q[2] > gbounds[5]))
      {
      return false;
      }
    else
      {
      return true;
      }
  }

// Utility class used to store bin properties
// and contents
  class DataSetBinner
  {
    std::vector<std::vector<unsigned int> > Bins;
    unsigned int NBins[3];
    unsigned int LoCorner[3];
    // Binsize in "extent coordinates"
    unsigned int BinSize[3];
    size_t TotalNumBins;

  public:

    // Create a set of bins given:
    // - number of bins in x, y, z
    // - lower extent of the binned space
    // - the size of bins in "extent coordinates"
    DataSetBinner(unsigned int nbins[3],
                  unsigned int locorner[3],
                  unsigned int binsize[3])
    {
      memcpy(this->NBins, nbins, 3*sizeof(unsigned int));
      memcpy(this->LoCorner, locorner, 3*sizeof(unsigned int));
      memcpy(this->BinSize, binsize, 3*sizeof(unsigned int));
      this->TotalNumBins = nbins[0]*nbins[1]*nbins[2];
      this->Bins.resize(this->TotalNumBins);
      for (size_t i=0; i<this->TotalNumBins; i++)
        {
        this->Bins[i].reserve(5);
        }
    }

    // Note that this does not check if the bin already
    // contains the blockId. This works fine for what this
    // class is used for.
    void AddToBin(unsigned int binIndex[3], int blockId)
    {
      size_t idx = binIndex[2] +
        binIndex[1]*this->NBins[2] +
        binIndex[0]*this->NBins[2]*this->NBins[1];
      std::vector<unsigned int>& bin = this->Bins[idx];
      bin.push_back(blockId);
    }

    const std::vector<unsigned int>& GetBin(unsigned int binIndex[3]) const
    {
      size_t idx = binIndex[2] +
        binIndex[1]*this->NBins[2] +
        binIndex[0]*this->NBins[2]*this->NBins[1];
      return this->Bins[idx];
    }

    // Given an input AMR box, return all boxes in the bins that intersect it
    void GetBoxesInIntersectingBins(const vtkAMRBox& box, std::set<unsigned int>& boxes)
    {
      boxes.clear();

      unsigned int minbin[3];
      unsigned int maxbin[3];

      const int* loCorner = box.GetLoCorner();
      int hiCorner[3];
      box.GetValidHiCorner(hiCorner);

      for (int j=0; j<3; j++)
        {
        minbin[j] = (loCorner[j] - this->LoCorner[j]) / this->BinSize[j];
        maxbin[j] = (hiCorner[j] - this->LoCorner[j]) / this->BinSize[j];
        }

      unsigned int idx[3];
      for (idx[0]=minbin[0]; idx[0]<=maxbin[0]; idx[0]++)
        for (idx[1]=minbin[1]; idx[1]<=maxbin[1]; idx[1]++)
          for (idx[2]=minbin[2]; idx[2]<=maxbin[2]; idx[2]++)
            {
            const std::vector<unsigned int>& bin =
              this->GetBin(idx);
            std::vector<unsigned int>::const_iterator iter;
            for(iter=bin.begin(); iter!=bin.end(); iter++)
              {
              boxes.insert(*iter);
              }
            }
    }


  };
};

//----------------------------------------------------------------------------

vtkAMRInformation::vtkAMRInformation(): NumBlocks(1,0)
{
  this->Refinement = vtkSmartPointer<vtkIntArray>::New();
  this->SourceIndex = NULL;
  this->GridDescription = -1;

  this->Origin[0] = this->Origin[1] = this->Origin[2] = DBL_MAX;
  this->Spacing =NULL;
  this->BlockLevel = NULL;

  this->Bounds[0] = VTK_DOUBLE_MAX;
  this->Bounds[1] = VTK_DOUBLE_MIN;
  this->Bounds[2] = VTK_DOUBLE_MAX;
  this->Bounds[3] = VTK_DOUBLE_MIN;
  this->Bounds[4] = VTK_DOUBLE_MAX;
  this->Bounds[5] = VTK_DOUBLE_MIN;
}

vtkAMRInformation::~vtkAMRInformation()
{
}

void vtkAMRInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Grid description: " <<  this->GetGridDescription()<< "\n";
  os << indent << "Global origin: ("
     << this->GetOrigin()[0] <<", "
     << this->GetOrigin()[1] <<", "
     << this->GetOrigin()[2] <<")\n ";

  os<< indent << "Number of blocks per level: ";
  for(unsigned int i=1; i<this->NumBlocks.size();i++)
    {
    os<<indent<<this->NumBlocks[i]-this->NumBlocks[i-1]<<" ";
    }
  os<<"\n";

  os<<indent<<"Refinemnt Ratio: ";
  if(this->HasRefinementRatio())
    {
    for(unsigned int i=0; i<this->GetNumberOfLevels();i++)
      {
      os<<this->GetRefinementRatio(i)<<" ";
      }
    os<<"\n";
    }
  else
    {
    os<<"None\n";
    }
  for (unsigned int levelIdx=0; levelIdx<this->GetNumberOfLevels(); levelIdx++)
    {
    unsigned int numDataSets = this->GetNumberOfDataSets( levelIdx );
    os<<indent<<"level "<<levelIdx<<"-------------------------"<<endl;
    for( unsigned int dataIdx=0; dataIdx < numDataSets; ++dataIdx )
      {
      const vtkAMRBox& box = this->GetAMRBox(levelIdx,dataIdx);
      os<<indent;
      os<<"["<<box.GetLoCorner()[0]<<", "<<box.GetHiCorner()[0]<<"]"
        <<"["<<box.GetLoCorner()[1]<<", "<<box.GetHiCorner()[1]<<"]"
        <<"["<<box.GetLoCorner()[2]<<", "<<box.GetHiCorner()[2]<<"]"<<endl;
      }
    }
  if(this->HasChildrenInformation())
    {
    os<<indent<<"Parent Child information: \n";
    for (unsigned int levelIdx=0; levelIdx<this->GetNumberOfLevels(); levelIdx++)
      {
      unsigned int numDataSets = this->GetNumberOfDataSets( levelIdx );
      for( unsigned int dataIdx=0; dataIdx < numDataSets; ++dataIdx )
        {
        this->PrintParentChildInfo(levelIdx,dataIdx);
        }
      }
    }
  os<<"\n";
}

bool vtkAMRInformation::Audit()
{
  int emptyDimension(-1);
  switch (this->GridDescription)
    {
    case VTK_YZ_PLANE: emptyDimension = 0; break;
    case VTK_XZ_PLANE: emptyDimension = 1; break;
    case VTK_XY_PLANE: emptyDimension = 2; break;
    }

  //Check origin
  for(int d = 0; d<3; d++)
    {
    if(d!=emptyDimension)
      {
      if(this->Origin[d]!=this->Bounds[2*d])
        {
        vtkErrorMacro("Bound min does not match origin at dimension "<<d<<": "<<this->Origin[d]<<" != "<< this->Bounds[2*d]);
        }
      }
    }

  //check refinement levels
  if(this->HasRefinementRatio() && static_cast<unsigned int>(this->Refinement->GetNumberOfTuples())!=this->GetNumberOfLevels())
    {
    vtkErrorMacro("Refinement levels wrong "<< this->Refinement->GetNumberOfTuples());
    }

  //check spacing
  for(unsigned int i=0; i<this->GetNumberOfLevels(); i++)
    {
    double h[3];
    this->GetSpacing(i,h);
    for(int d=0; d<3; d++)
      {
      if(h[d]<0)
        {
        vtkErrorMacro("Invalid spacing at level "<<i<<endl);
        }
      }
    }

  //check amr boxes
  for(unsigned int i=0; i<this->Boxes.size();i++)
    {
    const vtkAMRBox& box = this->Boxes[i];
    if(box.IsInvalid())
      {
      vtkErrorMacro("Invalid AMR Box");
      }
    bool valid(true);
    switch (this->GridDescription)
      {
      case VTK_YZ_PLANE: valid = box.EmptyDimension(0); break;
      case VTK_XZ_PLANE: valid = box.EmptyDimension(1); break;
      case VTK_XY_PLANE: valid = box.EmptyDimension(2); break;
      }
    if(!valid)
      {
      vtkErrorMacro("Invalid AMRBox. Wrong dimension");
      }
    }

  return true;
}

void vtkAMRInformation::Initialize(int numLevels,  const int* blocksPerLevel)
{
  if(numLevels<0)
    {
    vtkErrorMacro("Number of levels must be at least 0: "<<numLevels);
    return;
    }
  // allocate boxes
  this->NumBlocks.resize(numLevels+1,0);
  for(unsigned int i=0; i<static_cast<unsigned int>(numLevels);i++)
    {
    this->NumBlocks[i+1] = this->NumBlocks[i] + blocksPerLevel[i];
    }

  int numBlocks = this->NumBlocks.back();
  this->AllocateBoxes(numBlocks);
  this->Spacing = vtkSmartPointer<vtkDoubleArray>::New();
  this->Spacing->SetNumberOfTuples(3*numLevels);
  this->Spacing->SetNumberOfComponents(3);
  for(int i=0; i<numLevels; i++)
    {
    double h[3]={-1,-1,-1};
    this->Spacing->SetTuple(i,h);
    }
}

unsigned int vtkAMRInformation::GetNumberOfDataSets(unsigned int level) const
{
  if( level>= this->GetNumberOfLevels())
    {
    cerr<<"WARNING: No data set at this level"<<endl;
    return 0;
    }
  return this->NumBlocks[level+1]-this->NumBlocks[level];
}

void vtkAMRInformation::AllocateBoxes(unsigned int n)
{
  this->Boxes.clear();
  for(unsigned int i=0; i<n;i++)
    {
    vtkAMRBox box;
    this->Boxes.push_back(box);
    }

  for(unsigned int i=0; i<n;i++)
    {
    this->Boxes[i].Invalidate();
    }

}

void vtkAMRInformation::SetAMRBox(unsigned int level, unsigned int id, const vtkAMRBox& box)
{
  unsigned int index = this->GetIndex(level,id);
  this->Boxes[index] = box;
  if(this->HasSpacing(level)) //has valid spacing
    {
    this->UpdateBounds(level,id);
    }
}

int vtkAMRInformation::GetAMRBlockSourceIndex(int index)
{
  return this->SourceIndex->GetValue(index);
}

void vtkAMRInformation::SetAMRBlockSourceIndex(int index, int sourceId)
 {
  if(!this->SourceIndex)
    {
    this->SourceIndex = vtkSmartPointer<vtkIntArray>::New();
    this->SourceIndex->SetNumberOfValues(this->GetTotalNumberOfBlocks());
    }
  if(index>=this->SourceIndex->GetNumberOfTuples())
    {
    vtkErrorMacro("Invalid index");
    return;
    }
  SourceIndex->SetValue(index,sourceId);
}

void vtkAMRInformation::ComputeIndexPair(unsigned int index, unsigned int& level, unsigned int& id)
{
  this->GenerateBlockLevel();
  level = this->BlockLevel->GetValue(static_cast<vtkIdType>(index));
  id = index - this->NumBlocks[level] ;
}

void vtkAMRInformation::GetOrigin( double o[3] )
{
  for( int i=0; i < 3; ++i )
    {
    o[i] = this->Origin[i];
    }
}
double* vtkAMRInformation::GetOrigin()
{
  if(!this->HasValidOrigin())
    {
    vtkErrorMacro("Invalid Origin");
    }
  return this->Origin;
}

void vtkAMRInformation::SetOrigin(const double* origin)
{
  for(int d=0; d<3; d++)
    {
    this->Origin[d] = origin[d];
    }
}

int vtkAMRInformation::GetRefinementRatio(unsigned int level) const
{
  return this->Refinement->GetValue(level);
}

void vtkAMRInformation::SetRefinementRatio(unsigned int level, int refRatio)
{
  if(!HasRefinementRatio())
    {
    this->Refinement->SetNumberOfTuples(this->GetNumberOfLevels());
    }
  this->Refinement->SetValue(level,refRatio);
}

bool vtkAMRInformation::HasRefinementRatio()
{
  return this->Refinement && static_cast<unsigned int>(this->Refinement->GetNumberOfTuples())==this->GetNumberOfLevels();
}

void vtkAMRInformation::GenerateRefinementRatio()
{
  this->Refinement->SetNumberOfTuples(this->GetNumberOfLevels());

  // sanity check
  int numLevels = this->GetNumberOfLevels();

  if( numLevels < 1 )
    {
    // Dataset is empty!
    return;
    }

  if( numLevels == 1)
    {
    // No refinement, data-set has only a single level.
    // The refinement ratio is set to 2 to satisfy the
    // vtkOverlappingAMR requirement.
    this->Refinement->SetValue(0,2);
    return;
    }

  for( int level=0; level < numLevels-1; ++level )
    {
    int childLevel = level+1;

    if(this->GetNumberOfDataSets(childLevel)<1 || this->GetNumberOfDataSets(level)<1 )
      {
      continue;
      }

    unsigned int id =0;
    for(; id< this->GetNumberOfDataSets(level);id++)
      {
      if(!this->GetAMRBox(level,id).IsInvalid())
        {
        break;
        }
      }

    double childSpacing[3];
    this->GetSpacing(childLevel, childSpacing);

    double currentSpacing[3];
    this->GetSpacing(level, currentSpacing );

    // Note current implementation assumes uniform spacing. The
    // refinement ratio is the same in each dimension i,j,k.
    int nonEmptyDimension = 0;
    switch(this->GridDescription)
      {
      case VTK_XY_PLANE: nonEmptyDimension = 0;break;
      case VTK_YZ_PLANE: nonEmptyDimension = 1;break;
      case VTK_XZ_PLANE: nonEmptyDimension = 2;break;
      }
    int ratio = vtkMath::Round(currentSpacing[nonEmptyDimension]/childSpacing[nonEmptyDimension]);

    // Set the ratio at the last level, i.e., level numLevels-1, to be the
    // same as the ratio at the previous level,since the highest level
    // doesn't really have a refinement ratio.
    if( level==numLevels-2 )
      {
      this->Refinement->SetValue(level+1,ratio);
      }
    this->Refinement->SetValue(level,ratio);
    } // END for all hi-res levels
}

bool vtkAMRInformation::HasChildrenInformation()
{
  return !this->AllChildren.empty();
}


unsigned int *vtkAMRInformation::GetParents(unsigned int level, unsigned int index, unsigned int& num)
{
  if(level>=this->AllParents.size() ||
     index>=this->AllParents[level].size() ||
     this->AllParents[level][index].empty())
    {
    num = 0;
    return NULL;
    }

  num = static_cast<unsigned int>(this->AllParents[level][index].size());

  return  &this->AllParents[level][index][0];

}

unsigned int *vtkAMRInformation::
GetChildren(unsigned int level, unsigned int index, unsigned int& size)
{
  if(level>=this->AllChildren.size() ||
     index>=this->AllChildren[level].size() ||
     this->AllChildren[level][index].empty())
    {
    size = 0;
    return NULL;
    }

  size = static_cast<unsigned int>(this->AllChildren[level][index].size());

  return  &this->AllChildren[level][index][0];
}

void vtkAMRInformation::
PrintParentChildInfo(unsigned int level, unsigned int index)
{
  unsigned int *ptr, i, numParents;
  std::cerr << "Parent Child Info for block " << index
            << " of Level: " << level << endl;
  ptr = this->GetParents(level, index, numParents);
  std::cerr << "  Parents: ";
  for (i = 0; i < numParents; i++)
    {
    std::cerr << ptr[i] << " ";
    }
  std::cerr << endl;
  std::cerr << "  Children: ";
  unsigned int numChildren;
  ptr = this->GetChildren(level, index,numChildren);
  for (i = 0; i <numChildren; i++)
    {
    std::cerr << ptr[i] << " ";
    }
  std::cerr << endl;
}

void vtkAMRInformation::GenerateParentChildInformation()
{
  if(!this->HasRefinementRatio())
    {
    this->GenerateRefinementRatio();
    }
  AllChildren.resize(this->GetNumberOfLevels());
  AllParents.resize(this->GetNumberOfLevels());

  unsigned int numLevels = this->GetNumberOfLevels();
  for(unsigned int i=1; i<numLevels; i++)
    {
    this->CalculateParentChildRelationShip(i, AllChildren[i-1], AllParents[i]);
    }
}

bool vtkAMRInformation::HasValidOrigin()
{
  return this->Origin[0]!=DBL_MAX && this->Origin[1]!=DBL_MAX && this->Origin[2]!=DBL_MAX;
}

bool vtkAMRInformation::HasValidBounds()
{
  return this->Bounds[0]!=DBL_MAX && this->Bounds[1]!=DBL_MAX && this->Bounds[2]!=DBL_MAX;
}

void vtkAMRInformation::SetGridDescription(int description)
{
  if(this->GridDescription>=0 && description!=this->GridDescription)
    {
    vtkErrorMacro("Inconsistent types of vtkUniformGrid");
    }
  this->GridDescription = description;
}

void vtkAMRInformation::SetSpacing(unsigned int level, const double* h)
{
  double* spacing = this->Spacing->GetTuple(level);
  for(unsigned int i=0; i<3; i++)
    {
    if(spacing[i]>0 && spacing[i]!=h[i])
      {
      vtkWarningMacro("Inconsistent spacing: "<<spacing[i]<<" != "<<h[i]);
      }
    }
  this->Spacing->SetTuple(level, h);
}

void vtkAMRInformation::GenerateBlockLevel()
{
  if(this->BlockLevel)
    {
    return;
    }
  this->BlockLevel = vtkSmartPointer<vtkUnsignedIntArray>::New();

  this->BlockLevel->SetNumberOfValues( static_cast<vtkIdType>(this->GetTotalNumberOfBlocks()));

  assert(this->NumBlocks.size()==this->GetNumberOfLevels()+1);

  vtkIdType index(0);
  for(size_t level = 0; level< this->NumBlocks.size()-1; level++)
    {
    unsigned int begin = this->NumBlocks[level];
    unsigned int end = this->NumBlocks[level+1];
    for(unsigned int id=begin; id!=end; id++)
      {
      this->BlockLevel->SetValue(index++,static_cast<unsigned int>(level));
      }
    }
}

void vtkAMRInformation::GetBounds(unsigned int level, unsigned int id, double* bb)
{
  const vtkAMRBox& box = this->Boxes[this->GetIndex(level,id)];
  vtkAMRBox::GetBounds(box, this->Origin, this->Spacing->GetTuple(level), bb);
}

const vtkAMRBox& vtkAMRInformation::GetAMRBox(unsigned int level, unsigned int id) const
{
  return this->Boxes[this->GetIndex(level,id)];
}

void vtkAMRInformation::GetSpacing(unsigned int level, double spacing[3])
{
  return this->Spacing->GetTuple(level,spacing);
}

void vtkAMRInformation::CalculateParentChildRelationShip(
    unsigned int level,
    std::vector<std::vector<unsigned int> >& children,
    std::vector<std::vector<unsigned int> >& parents
  )
{
  if (level == 0 || level > this->GetNumberOfLevels())
    {
    return;
    }

  // 1. Find the bounds of all boxes at level n-1
  // 2. Find the average block size
  int extents[6] = { VTK_INT_MAX, -VTK_INT_MAX,
                     VTK_INT_MAX, -VTK_INT_MAX,
                     VTK_INT_MAX, -VTK_INT_MAX};
  float totalsize[3] = {0, 0, 0};
  unsigned int numParentDataSets = this->GetNumberOfDataSets(level - 1);
  int refinementRatio = this->GetRefinementRatio(level - 1);
  for (unsigned int id=0; id<numParentDataSets; id++)
    {
    vtkAMRBox box = this->GetAMRBox(level - 1, id);
    if (!box.IsInvalid())
      {
      box.Refine(refinementRatio);
      const int* loCorner = box.GetLoCorner();
      int hiCorner[3];
      box.GetValidHiCorner(hiCorner);
      for (int i = 0; i<3; i++)
        {
        if (loCorner[i] < extents[2*i])
          {
          extents[2*i] = loCorner[i];
          }
        if (hiCorner[i] > extents[2*i+1])
          {
          extents[2*i+1] = hiCorner[i];
          }
        totalsize[i] += (hiCorner[i] - loCorner[i] + 1);
        }
      }
    }

  // Calculate number of bins and binsize. Note that bins
  // are cell aligned and we use AMRBox indices to represent
  // them
  unsigned int nbins[3];
  unsigned int binsize[3];
  for (int i=0; i<3; i++)
    {
    binsize[i] = vtkMath::Round(totalsize[i] / numParentDataSets);
    nbins[i] = (extents[2*i+1] - extents[2*i]) / binsize[i] + 1;
    }

  double origin[3];
  double spacing[3];

  this->GetOrigin(origin);
  this->GetSpacing(0,spacing);
  for (unsigned int i=0; i<level; i++)
    {
    for (int j=0; j<3; j++)
      spacing[j] /= this->GetRefinementRatio(i);
    }

  unsigned int loExtent[3];
  loExtent[0] = extents[0];
  loExtent[1] = extents[2];
  loExtent[2] = extents[4];
  DataSetBinner binner(nbins, loExtent, binsize);
  // for(int i=0; i<3; i++)
  //   {
  //   cout<<nbins[i]<<" "<<loExtent[i]<<" "<<binsize[i]<<endl;
  //   }

  // Bin the blocks
  for (unsigned int i=0; i<numParentDataSets; i++)
    {
    vtkAMRBox box = this->GetAMRBox(level - 1, i);
    if (!box.IsInvalid())
      {
      unsigned int minbin[3];
      unsigned int maxbin[3];

      box.Refine(refinementRatio);

      const int* loCorner = box.GetLoCorner();
      int hiCorner[3];
      box.GetValidHiCorner(hiCorner);

      for (int j=0; j<3; j++)
        {
        minbin[j] = (loCorner[j] - extents[2*j]) / binsize[j];
        maxbin[j] = (hiCorner[j] - extents[2*j]) / binsize[j];
        }

      unsigned int idx[3];
      for (idx[0]=minbin[0]; idx[0]<=maxbin[0]; idx[0]++)
        {
        for (idx[1]=minbin[1]; idx[1]<=maxbin[1]; idx[1]++)
          {
          for (idx[2]=minbin[2]; idx[2]<=maxbin[2]; idx[2]++)
            {
            binner.AddToBin(idx, i);
            }
          }
        }
      }
    }

  // Write bins for debugging
  // WriteBins(origin, spacing, extents, binsize, nbins, binner);

  // Actually find parent-children relationship
  // between blocks in level and level-1
  children.resize(this->GetNumberOfDataSets(level-1));
  parents.resize(this->GetNumberOfDataSets(level));

  unsigned int numDataSets = this->GetNumberOfDataSets(level);
  for (unsigned int i=0; i<numDataSets; i++)
    {
    const vtkAMRBox& box = this->GetAMRBox(level, i);
    if (!box.IsInvalid())
      {
      std::set<unsigned int> boxes;
      binner.GetBoxesInIntersectingBins(box, boxes);
      std::set<unsigned int>::iterator iter;
      for (iter=boxes.begin(); iter!=boxes.end(); iter++)
        {
        vtkAMRBox potentialParent = this->GetAMRBox(level - 1, *iter);
        if (!potentialParent.IsInvalid())
          {
          potentialParent.Refine(refinementRatio);
          if (box.DoesIntersect(potentialParent))
            {
            children[*iter].push_back(i);
            parents[i].push_back(*iter);
            }
          }
        }
      }
    }
}

bool vtkAMRInformation::FindCell(double q[3],unsigned int level, unsigned int id,int &cellIdx)
{
  double h[3];
  this->GetSpacing(level,h);

  const vtkAMRBox& box = this->GetAMRBox(level,id);
  double gbounds[6];
  this->GetBounds(level,id,gbounds);
  if ((q[0] < gbounds[0]) || (q[0] > gbounds[1]) ||
      (q[1] < gbounds[2]) || (q[1] > gbounds[3]) ||
      (q[2] < gbounds[4]) || (q[2] > gbounds[5]))
    {
    return false;
    }
  int ijk[3];
  double pcoords[3];
  int status = vtkAMRBox::ComputeStructuredCoordinates(box, this->Origin, h, q, ijk, pcoords );
  if( status == 1 )
    {
    int dims[3];
    box.GetNumberOfNodes(dims);
    cellIdx=vtkStructuredData::ComputeCellId(dims,ijk);
    return true;
    }
  return false;
}

bool vtkAMRInformation::GetCoarsenedAMRBox(unsigned int level, unsigned int id, vtkAMRBox& box) const
{
  box = this->GetAMRBox(level,id);
  ReturnFalseIfMacro(box.IsInvalid(),"Invalid AMR box.")
  ReturnFalseIfMacro(level==0, "Cannot get AMR box at level 0.")

  int refinementRatio = this->GetRefinementRatio(level-1);
  box.Coarsen(refinementRatio);
  return true;
}

bool vtkAMRInformation::operator==(const vtkAMRInformation& other)
{
  if(this->GridDescription!=other.GridDescription)
    {
    return false;
    }
  if(this->NumBlocks.size()!=other.NumBlocks.size())
    {
    return false;
    }
  for(int i=0; i<3; i++)
    {
    if(this->Origin[i]!=other.Origin[i])
      {
      return false;
      }
    }
  for(size_t i=0; i<this->NumBlocks.size();i++)
    {
    if(this->NumBlocks[i]!=other.NumBlocks[i])
      {
      return false;
      }
    }

  for(size_t i=0; i<this->Boxes.size(); i++)
    {
    if(this->Boxes[i]!=other.Boxes[i])
      {
      return false;
      }
    }
  if(this->SourceIndex && other.SourceIndex)
    {
    for(vtkIdType i=0; i<this->SourceIndex->GetNumberOfTuples();i++)
      {
      if(this->SourceIndex->GetValue(i)!=other.SourceIndex->GetValue(i))
        {
        return false;
        }
      }
    }

  if(this->Spacing->GetNumberOfTuples()!=other.Spacing->GetNumberOfTuples())
    {
    return false;
    }
  for(vtkIdType i=0; i<this->Spacing->GetNumberOfTuples();i++)
    {
    if(this->Spacing->GetValue(i)!=other.Spacing->GetValue(i))
      {
      return false;
      }
    }
  return true;
}

bool vtkAMRInformation::GetOrigin(unsigned int level, unsigned int id, double* origin)
{
  const vtkAMRBox& box = this->Boxes[this->GetIndex(level,id)];
  vtkAMRBox::GetBoxOrigin(box, this->Origin, this->Spacing->GetTuple(level), origin);
  return true;
}

void vtkAMRInformation::UpdateBounds(const int level, const int id)
{
  double bb[6];
  vtkAMRBox::GetBounds(this->GetAMRBox(level,id), this->Origin, this->Spacing->GetTuple(level), bb);
  for( int i=0; i < 3; ++i )
    {
    if( bb[i*2] < this->Bounds[i*2] )
      {
      this->Bounds[i*2] = bb[i*2];
      }
    if( bb[i*2+1] > this->Bounds[i*2+1])
      {
      this->Bounds[i*2+1] = bb[i*2+1];
      }
    } // END for each dimension
}

void vtkAMRInformation::DeepCopy(vtkAMRInformation *other)
{
  this->GridDescription = other->GridDescription;
  memcpy(this->Origin, other->Origin, sizeof(double)*3);
  this->Boxes = other->Boxes;
  this->NumBlocks = other->NumBlocks;
  if(other->SourceIndex)
    {
    this->SourceIndex = vtkSmartPointer<vtkIntArray>::New();
    this->SourceIndex->DeepCopy(other->SourceIndex);
    }
  if(other->Spacing)
    {
    this->Spacing = vtkSmartPointer<vtkDoubleArray>::New();
    this->Spacing->DeepCopy(other->Spacing);
    }
  memcpy(this->Bounds, other->Bounds, sizeof(double)*6);

}

bool vtkAMRInformation::HasSpacing(unsigned int level)
{
  return   this->Spacing->GetTuple(level)[0] >=0
        || this->Spacing->GetTuple(level)[1] >=0
        || this->Spacing->GetTuple(level)[2] >=0;
}

const double* vtkAMRInformation::GetBounds()
{
  if(!HasValidBounds())
    {
    for(unsigned int i=0; i<this->GetNumberOfLevels();i++)
      {
      for(unsigned int j=0; j<this->GetNumberOfDataSets(i);j++)
        {
        this->UpdateBounds(i,j);
        }
      }
    }
  return this->Bounds;
}

bool vtkAMRInformation::FindGrid(double q[3], unsigned int& level, unsigned int& gridId)
{
  if (!this->HasChildrenInformation())
    {
    this->GenerateParentChildInformation();
    }

  if (!this->FindGrid(q, 0,gridId))
    {
    return false;
    }

  unsigned int maxLevels = this->GetNumberOfLevels();
  for(level=0; level<maxLevels;level++)
    {
    unsigned int n;
    unsigned int *children = this->GetChildren(level, gridId,n);
    if (children == NULL)
      {
      break;
      }
    unsigned int i;
    for (i = 0; i < n; i++)
      {
      double bb[6];
      this->GetBounds(level+1, children[i],bb);
      if(Inside(q,bb))
        {
        gridId = children[i];
        break;
        }
      }
    if(i>=n)
      {
      break;
      }
    }
  return true;
}

bool vtkAMRInformation::FindGrid(double q[3], int level, unsigned int& gridId)
{
  for(unsigned int i = 0; i < this->GetNumberOfDataSets(level); i++ )
    {
    double gbounds[6];
    this->GetBounds(level,i,gbounds);
    bool inside = Inside(q,gbounds);
    if(inside)
      {
      gridId = i;
      return true;
      }
    }
  return false;
}
