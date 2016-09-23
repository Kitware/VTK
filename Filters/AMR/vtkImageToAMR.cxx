/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToAMR.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageToAMR.h"

#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkSmartPointer.h"
#include "vtkTuple.h"
#include "vtkAMRBox.h"
#include "vtkUniformGrid.h"
#include "vtkNew.h"
#include "vtkAMRUtilities.h"

#include <algorithm>
#include <vector>
#include "vtkAMRInformation.h"
namespace
{
  //Split one box to eight
  int SplitXYZ(vtkAMRBox inBox, int refinementRatio, std::vector<vtkAMRBox>& out)
  {
    inBox.Refine(refinementRatio);
    const int* lo = inBox.GetLoCorner();
    const int* hi = inBox.GetHiCorner();

    //the cartesian product A[0][0..n[0]] X A[1][0..n[1] X A[2][0..n[2]] is the refined grid
    int A[3][3], n[3];
    for(int d=0; d<3; d++)
    {
      A[d][0] = lo[d]-1;
      A[d][2] = hi[d];
      if(inBox.EmptyDimension(d))
      {
        n[d]=1;
        A[d][1] = hi[d];
      }
      else
      {
        n[d]=2;
        A[d][1] = (lo[d]+hi[d])/2;
      }
    }

    //create the refined boxes and push them to the output stack
    int numOut(0);
    for(int i=0; i<n[0]; i++)
    {
      for(int j=0; j<n[1]; j++)
      {
        for(int k=0; k<n[2]; k++)
        {
          vtkAMRBox box;
          box.SetDimensions( A[0][i]+1,  A[1][j]+1,  A[2][k]+1,
                             A[0][i+1],  A[1][j+1],  A[2][k+1]);
          out.push_back(box);
          numOut++;
        }
      }
    }
    return numOut;
  }

  int ComputeTreeHeight(int maxNumNodes, int degree)
  {
    if(maxNumNodes<=0)
    {
      return 0;
    }
    //could have used a formula, but this is more clear
    int height = 1;
    int numNodes= 1;
    while(numNodes<=maxNumNodes)
    {
      numNodes = numNodes + degree*numNodes;
      height++;
    }
    height--;
    return height;
  }


  //split the blocks into a tree that starts out as a single stem
  //than turn a full tree. This shape is designed so that numLevels and maxNumBlocks
  //constraint can be satisifed
  void Split(const vtkAMRBox& rootBox, int numLevels, int refinementRatio, int maxNumBlocks,
             std::vector<std::vector<vtkAMRBox> >& out)
  {
    out.clear();
    out.resize(1);
    out.back().push_back(rootBox);
    maxNumBlocks--;

    int treeDegree = rootBox.ComputeDimension()*2;
    int numTreeLevels= std::min(numLevels,ComputeTreeHeight(maxNumBlocks-(numLevels-1), treeDegree))-1; //minus one because root already has one
    int level=1;
    for(; level<numLevels-numTreeLevels; level++)
    {
      out.push_back(std::vector<vtkAMRBox>());
      const std::vector<vtkAMRBox>& parentBoxes = out[level-1];
      std::vector<vtkAMRBox>& childBoxes = out[level];
      vtkAMRBox child = parentBoxes.back();
      child.Refine(refinementRatio);
      childBoxes.push_back(child);
    }

    for(; level<numLevels;level++)
    {
      out.push_back(std::vector<vtkAMRBox>());
      const std::vector<vtkAMRBox>& parentBoxes = out[level-1];
      std::vector<vtkAMRBox>& childBoxes = out[level];
      for(size_t i = 0;i<parentBoxes.size();i++)
      {
        const vtkAMRBox& parent = parentBoxes[i];
        SplitXYZ(parent,refinementRatio,childBoxes);
      }
    }
  };

  //create a grid by sampling from input using the indices in box
  vtkUniformGrid* ConstructGrid(vtkImageData *input, const vtkAMRBox& box, int coarsenRatio, double* origin, double* spacing)
  {
    int numPoints[3];
    box.GetNumberOfNodes(numPoints);

    vtkUniformGrid* grid = vtkUniformGrid::New();
    grid->Initialize();
    grid->SetDimensions(numPoints);
    grid->SetSpacing(spacing);
    grid->SetOrigin(origin);

    vtkPointData  *inPD=input->GetPointData(), *outPD = grid->GetPointData();
    vtkCellData   *inCD=input->GetCellData(), *outCD = grid->GetCellData();

    outPD->CopyAllocate(inPD, grid->GetNumberOfPoints());
    outCD->CopyAllocate(inCD, grid->GetNumberOfCells());

    vtkAMRBox box0(box);
    box0.Refine(coarsenRatio); //refine it to the image data level

    int extents[6]; input->GetExtent(extents);
    int imLo[3] = {extents[0],extents[2], extents[4]};
    const int *lo=box.GetLoCorner();

    for( int iz=0; iz<numPoints[2]; iz++ )
    {
      for( int iy=0; iy<numPoints[1]; iy++ )
      {
        for( int ix=0; ix<numPoints[0]; ix++ )
        {
          int ijkDst[3] = {ix,iy,iz};
          vtkIdType idDst =  grid->ComputePointId(ijkDst);

          int ijkSrc[3] = {(lo[0]+ix)*coarsenRatio + imLo[0],
                           (lo[1]+iy)*coarsenRatio + imLo[1],
                           (lo[2]+iz)*coarsenRatio + imLo[2]};

          vtkIdType idSrc = input->ComputePointId(ijkSrc);

          outPD->CopyData(inPD,idSrc, idDst);
        }
      }
    }

    int numCells[3];
    for(int d=0; d<3; d++)
    {
      numCells[d] = std::max(numPoints[d]-1,1);
    }

    for( int iz=0; iz<numCells[2]; iz++ )
    {
      for( int iy=0; iy<numCells[1]; iy++ )
      {
        for( int ix=0; ix<numCells[0]; ix++ )
        {
          int ijkDst[3] = {ix,iy,iz};
          vtkIdType idDst =  grid->ComputeCellId(ijkDst);

          int ijkSrc[3] = {(lo[0]+ix)*coarsenRatio + imLo[0],
                           (lo[1]+iy)*coarsenRatio + imLo[1],
                           (lo[2]+iz)*coarsenRatio + imLo[2]};

          vtkIdType idSrc = input->ComputeCellId(ijkSrc);

          outCD->CopyData(inCD,idSrc, idDst);
        }
      }
    }

    return grid;
  }

};


vtkStandardNewMacro(vtkImageToAMR);
//----------------------------------------------------------------------------
vtkImageToAMR::vtkImageToAMR()
{
  this->NumberOfLevels = 2;
  this->RefinementRatio = 2;
  this->MaximumNumberOfBlocks = 100;
}

//----------------------------------------------------------------------------
vtkImageToAMR::~vtkImageToAMR()
{
}

//----------------------------------------------------------------------------
int vtkImageToAMR::FillInputPortInformation(int , vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageToAMR::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  vtkImageData* input = vtkImageData::GetData(inputVector[0], 0);
  vtkOverlappingAMR* amr = vtkOverlappingAMR::GetData(outputVector);

  if(input->GetDataDimension()<2)
  {
    vtkErrorMacro("Image dimension must be at least two.");
    return 0;
  }


  int whole_extent[6];
  inInfo->Get(vtkCompositeDataPipeline::WHOLE_EXTENT(), whole_extent);

  int dims[3] = { whole_extent[1] - whole_extent[0] + 1,
                  whole_extent[3] - whole_extent[2] + 1,
                  whole_extent[5] - whole_extent[4] + 1 };

  double inputBounds[6];
  input->GetBounds(inputBounds);

  double inputOrigin[3]= {inputBounds[0],inputBounds[2],inputBounds[4]};

  double inputSpacing[3];
  input->GetSpacing(inputSpacing);

  int gridDescription = vtkStructuredData::GetDataDescription(dims);

  //check whether the parameters are valid
  //and compute the base image resolution
  int dims0[3];
  double spacing0[3];
  for(int d=0; d<3; d++)
  {
    if(dims[d]<=1)
    {
      if(dims[d]==0)
      {
        vtkWarningMacro("Zero dimension? Really?");
      }
      dims0[d] = 1;
      spacing0[d] = 1.0;
    }
    else
    {
      int r = (int)(pow(static_cast<double>(this->RefinementRatio),this->NumberOfLevels-1));
      if((dims[d]-1)%r!=0)
      {
        vtkErrorMacro("Image cannot be refined");
        return 0;
      }
      dims0[d] = 1+(dims[d]-1)/r;
      spacing0[d] = r*inputSpacing[d];
    }
  }

  vtkAMRBox rootBox(inputOrigin, dims0, spacing0, inputOrigin, gridDescription);

  std::vector<std::vector<vtkAMRBox> > amrBoxes;
  Split(rootBox,this->NumberOfLevels, this->RefinementRatio, this->MaximumNumberOfBlocks, amrBoxes);

  std::vector<int> blocksPerLevel;
  for(size_t i=0; i<amrBoxes.size();i++)
  {
    blocksPerLevel.push_back(static_cast<int>(amrBoxes[i].size()));
  }

  unsigned int numLevels = static_cast<unsigned int>(blocksPerLevel.size());

  amr->Initialize(static_cast<int>(numLevels), &blocksPerLevel[0]);
  amr->SetOrigin(inputOrigin);
  amr->SetGridDescription(gridDescription);

  double spacingi[3] = {spacing0[0],spacing0[1],spacing0[2]};
  for(unsigned int i=0; i<numLevels; i++)
  {
    amr->SetSpacing(i,spacingi);
    for(int d=0;d<3;d++)
    {
      spacingi[d]/=this->RefinementRatio;
    }
  }

  for(unsigned int level = 0; level<numLevels; level++)
  {
    const std::vector<vtkAMRBox>& boxes = amrBoxes[level];
    for(size_t i=0; i<boxes.size();i++)
    {
      amr->SetAMRBox(level,static_cast<unsigned int>(i), boxes[i]);
    }
  }

  for(unsigned int level = 0; level< numLevels; level++)
  {
    double spacing[3];
    amr->GetSpacing(level, spacing);
    int coarsenRatio = (int)pow( static_cast<double>(this->RefinementRatio), static_cast<int>(numLevels- 1 - level));//againt the finest level
    for(size_t i=0; i<amr->GetNumberOfDataSets(level);i++)
    {
      const vtkAMRBox& box = amr->GetAMRBox(level,static_cast<unsigned int>(i));
      double origin[3];
      vtkAMRBox::GetBoxOrigin(box,inputOrigin,spacing,origin);
      vtkUniformGrid* grid = ConstructGrid(input,box,coarsenRatio,origin,spacing);
      amr->SetDataSet(level,static_cast<unsigned int>(i), grid);
      grid->Delete();
    }
  }

  vtkAMRUtilities::BlankCells(amr);
  return 1;
}

//----------------------------------------------------------------------------
void vtkImageToAMR::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfLevels: " << this->NumberOfLevels << endl;
  os << indent << "RefinementRatio: " << this->RefinementRatio << endl;
}
