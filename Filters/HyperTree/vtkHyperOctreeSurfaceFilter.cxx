/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctreeSurfaceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperOctreeSurfaceFilter.h"

#include "vtkHyperOctree.h"
#include "vtkHyperOctreeCursor.h"
#include "vtkObjectFactory.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkCellArray.h"
#include <assert.h>
#include "vtkMergePoints.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkIncrementalPointLocator.h"

vtkStandardNewMacro(vtkHyperOctreeSurfaceFilter);

// merging: locator
// no merging: insertnextpoint

//----------------------------------------------------------------------------
vtkHyperOctreeSurfaceFilter::vtkHyperOctreeSurfaceFilter()
{
  this->Merging=0;
  this->Locator=0;

  this->OutPts=0;
  this->InputCD=0;
  this->OutputCD=0;
  this->Cursor=0;

  this->PassThroughCellIds = 0;
  this->OriginalCellIds = NULL;
}

//----------------------------------------------------------------------------
vtkHyperOctreeSurfaceFilter::~vtkHyperOctreeSurfaceFilter()
{
  if(this->Locator!=0)
    {
    this->Locator->UnRegister(this);
    this->Locator=0;
    }
  if (this->OriginalCellIds != NULL)
    {
    this->OriginalCellIds->Delete();
    this->OriginalCellIds = NULL;
    }
}

//----------------------------------------------------------------------------
int vtkHyperOctreeSurfaceFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkHyperOctree *input = vtkHyperOctree::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  this->OutPts=vtkPoints::New();

  if(this->Merging)
    {
    if(this->Locator==0)
      {
      this->CreateDefaultLocator();
      }
    this->Locator->InitPointInsertion (this->OutPts, input->GetBounds());
    }

  vtkIdType numCells=input->GetNumberOfLeaves();
  this->InputCD = input->GetLeafData();
  this->OutputCD = output->GetCellData();
  this->OutputCD->CopyAllocate(this->InputCD,numCells,numCells/2);

  this->Cursor=input->NewCellCursor();
  this->Cursor->ToRoot();

  double bounds[6];
  input->GetBounds(bounds);
  int dim=input->GetDimension();
  assert("check: valid_dim" && dim>=1 && dim<=3);


  if (this->PassThroughCellIds)
    {
    this->OriginalCellIds = vtkIdTypeArray::New();
    this->OriginalCellIds->SetName("vtkOriginalCellIds");
    this->OriginalCellIds->SetNumberOfComponents(1);
    this->OutputCD->AddArray(this->OriginalCellIds);
    }

  double pt[3];

  vtkIdType ptIds[8];
  int idx;
  int z;
  int y;
  int x;

  switch(dim)
    {
    case 3:
      idx=0;
      z=0;
      while(z<2)
        {
        y=0;
        while(y<2)
          {
          x=0;
          while(x<2)
            {
            pt[0]=bounds[x];
            pt[1]=bounds[2+y];
            pt[2]=bounds[4+z];
            ptIds[idx]=this->OutPts->InsertNextPoint(pt);
            ++x;
            ++idx;
            }
          ++y;
          }
        ++z;
        }

      int onFace[6];
      onFace[0]=1; // -x
      onFace[1]=1; // +x
      onFace[2]=1; // -y
      onFace[3]=1; // +y
      onFace[4]=1; // -z;
      onFace[5]=1; // +z;
      this->OutCells=vtkCellArray::New();
      this->GenerateFaces(bounds,ptIds,onFace);

      output->SetPolys(this->OutCells);
      this->OutCells->UnRegister(this);
      this->OutCells=0;
      break;
    case 2:
      pt[2]=0;

      idx=0;
      y=0;
      while(y<2)
        {
        x=0;
        while(x<2)
          {
          pt[0]=bounds[x];
          pt[1]=bounds[2+y];
          ptIds[idx]=this->OutPts->InsertNextPoint(pt);
          ++x;
          ++idx;
          }
          ++y;
        }
      this->OutCells=vtkCellArray::New();
      this->GenerateQuads(bounds,ptIds);
      output->SetPolys(this->OutCells);
      this->OutCells->UnRegister(this);
      this->OutCells=0;
      break;
    case 1:
      // left point
      pt[0]=bounds[0];
      pt[1]=0;
      pt[2]=0;
      this->OutPts->InsertNextPoint(pt);
      // right point
      pt[0]=bounds[1];
      this->OutPts->InsertNextPoint(pt);
      ptIds[0]=0;
      ptIds[1]=1;
      this->OutCells=vtkCellArray::New();
      this->GenerateLines(bounds,ptIds);
      output->SetLines(this->OutCells);
      this->OutCells->UnRegister(this);
      this->OutCells=0;
      break;
    default:
      assert("check: impossible case" && 0);
    }
  output->SetPoints(this->OutPts);
  this->OutPts->Delete();
  this->OutPts=0;

  this->InputCD=0;
  this->OutputCD=0;
  this->Cursor->UnRegister(this);

  if (this->OriginalCellIds != NULL)
    {
    this->OriginalCellIds->Delete();
    this->OriginalCellIds = NULL;
    }

  output->Squeeze();

  return 1;
}

//-----------------------------------------------------------------------------
void vtkHyperOctreeSurfaceFilter::GenerateLines(double bounds[2],
                                                vtkIdType ptIds[2])
{
  if(this->Cursor->CurrentIsLeaf())
    {
    // generate line (points, cell and point data from cell data)
    // from bounds
    vtkIdType inId=this->Cursor->GetLeafId();
    vtkIdType outId = this->OutCells->InsertNextCell(2);
    this->OutCells->InsertCellPoint(ptIds[0]);
    this->OutCells->InsertCellPoint(ptIds[1]);
    // Copy cell data.
    this->OutputCD->CopyData(this->InputCD,inId,outId);
    this->RecordOrigCellId(outId, inId);
    }
  else
    {
    double mid=(bounds[0]+bounds[1])*0.5;
    double subBounds[2];
    vtkIdType subPtIds[2];
    double pt[3];
    vtkIdType ptId;

    pt[0]=mid;
    pt[1]=0;
    pt[2]=0;
    ptId=this->OutPts->InsertNextPoint(pt);
    // no point data to copy. Octree does not handle point data.

    this->Cursor->ToChild(VTK_BINARY_TREE_CHILD_LEFT);
    subBounds[0]=bounds[0];
    subBounds[1]=mid;
    subPtIds[0]=ptIds[0];
    subPtIds[1]=ptId;
    this->GenerateLines(subBounds,subPtIds);
    this->Cursor->ToParent();

    this->Cursor->ToChild(VTK_BINARY_TREE_CHILD_RIGHT);
    subBounds[0]=mid;
    subBounds[1]=bounds[1];
    subPtIds[0]=ptId;
    subPtIds[1]=ptIds[1];
    this->GenerateLines(subBounds,subPtIds);
    this->Cursor->ToParent();
    }
}

//-----------------------------------------------------------------------------
void vtkHyperOctreeSurfaceFilter::GenerateQuads(double bounds[4],
                                                vtkIdType ptIds[4])
{
  if(this->Cursor->CurrentIsLeaf())
    {
    // generate quad (points, cell and point data from cell data)
    // from bounds
    vtkIdType inId=this->Cursor->GetLeafId();
    vtkIdType outId = this->OutCells->InsertNextCell(4);
    this->OutCells->InsertCellPoint(ptIds[0]);
    this->OutCells->InsertCellPoint(ptIds[1]);
    this->OutCells->InsertCellPoint(ptIds[3]);
    this->OutCells->InsertCellPoint(ptIds[2]);
    // Copy cell data.
    this->OutputCD->CopyData(this->InputCD,inId,outId);
    this->RecordOrigCellId(outId, inId);
    }
  else
    {
    // TO RE-WRITE
    double midX=(bounds[0]+bounds[1])*0.5;
    double midY=(bounds[2]+bounds[3])*0.5;
    double subBounds[4];
    vtkIdType subPtIds[4];
    double pt[3];
    vtkIdType newPtIds[5]; // center of the quad + middle of each edge

    // south
    pt[0]=midX;
    pt[1]=bounds[2];
    pt[2]=0;
    newPtIds[0]=this->OutPts->InsertNextPoint(pt);

    // west
    pt[0]=bounds[0];
    pt[1]=midY;
    newPtIds[1]=this->OutPts->InsertNextPoint(pt);

    // center
    pt[0]=midX;
    pt[1]=midY;
    newPtIds[2]=this->OutPts->InsertNextPoint(pt);

    // east
    pt[0]=bounds[1];
    pt[1]=midY;
    newPtIds[3]=this->OutPts->InsertNextPoint(pt);

    // north
    pt[0]=midX;
    pt[1]=bounds[3];
    newPtIds[4]=this->OutPts->InsertNextPoint(pt);


    // no point data to copy. Octree does not handle point data yet.

    this->Cursor->ToChild(VTK_QUADTREE_CHILD_SW);
    subBounds[0]=bounds[0];
    subBounds[1]=midX;
    subBounds[2]=bounds[2];
    subBounds[3]=midY;

    subPtIds[0]=ptIds[0];
    subPtIds[1]=newPtIds[0];
    subPtIds[2]=newPtIds[1];
    subPtIds[3]=newPtIds[2];
    this->GenerateQuads(subBounds,subPtIds);
    this->Cursor->ToParent();

    this->Cursor->ToChild(VTK_QUADTREE_CHILD_SE);
    subBounds[0]=midX;
    subBounds[1]=bounds[1];
    subBounds[2]=bounds[2];
    subBounds[3]=midY;

    subPtIds[0]=newPtIds[0];
    subPtIds[1]=ptIds[1];
    subPtIds[2]=newPtIds[2];
    subPtIds[3]=newPtIds[3];
    this->GenerateQuads(subBounds,subPtIds);
    this->Cursor->ToParent();

    this->Cursor->ToChild(VTK_QUADTREE_CHILD_NW);
    subBounds[0]=bounds[0];
    subBounds[1]=midX;
    subBounds[2]=midY;
    subBounds[3]=bounds[3];

    subPtIds[0]=newPtIds[1];
    subPtIds[1]=newPtIds[2];
    subPtIds[2]=ptIds[2];
    subPtIds[3]=newPtIds[4];
    this->GenerateQuads(subBounds,subPtIds);
    this->Cursor->ToParent();

    this->Cursor->ToChild(VTK_QUADTREE_CHILD_NE);
    subBounds[0]=midX;
    subBounds[1]=bounds[1];
    subBounds[2]=midY;
    subBounds[3]=bounds[3];

    subPtIds[0]=newPtIds[2];
    subPtIds[1]=newPtIds[3];
    subPtIds[2]=newPtIds[4];
    subPtIds[3]=ptIds[3];
    this->GenerateQuads(subBounds,subPtIds);
    this->Cursor->ToParent();
    }
}


// For each face, give the sequence of for point id, counterclockwise.
// [face][ptId]
int Quads[6][4]={{0,4,6,2}, // -x
                 {3,7,5,1}, // +x
                 {0,1,5,4}, // -y
                 {2,6,7,3}, // +y
                 {0,2,3,1}, // -z
                 {4,5,7,6}}; // +z

//-----------------------------------------------------------------------------
void vtkHyperOctreeSurfaceFilter::GenerateFaces(double bounds[6],
                                                vtkIdType ptIds[8],
                                                int onFace[6])
{
  if(this->Cursor->CurrentIsLeaf())
    {
    int face=0;
    vtkIdType inId=this->Cursor->GetLeafId();
    while(face<6)
      {
      if(onFace[face])
        {
        vtkIdType outId = this->OutCells->InsertNextCell(4);
        int idx=0;
        while(idx<4)
          {
          this->OutCells->InsertCellPoint(ptIds[Quads[face][idx]]);
          ++idx;
          }
        // Copy cell data.
        this->OutputCD->CopyData(this->InputCD,inId,outId);
        this->RecordOrigCellId(outId, inId);
        }
      ++face;
      }
    }
  else
    {
    // generate mid points on the faces that are on the octree boundary.
    double pt[3];

    // [x,y,z][min,mid,max]
    double allBounds[3][3];
    int coord=0;
    while(coord<3)
      {
      int coord2=coord<<1; // *2
      allBounds[coord][0]=bounds[coord2];
      allBounds[coord][1]=(bounds[coord2]+bounds[coord2+1])*0.5;
      allBounds[coord][2]=bounds[coord2+1];
      coord++;
      }

    // Create the new mid-points (on those which are edges or faces
    // of the octree)

    // [i][j][k]
    // [x][y][z] in [0,2]
    vtkIdType allPtIds[3][3][3];

    // 0 mid points: original point
    // 1 mid point: center of a edge
    // 2 mid point: center of a face
    // 3 mid point: internal point, not used
    int midpoints=0;
    int originalpt=0;

    double xStep=(bounds[1]-bounds[0])*0.5;
    double yStep=(bounds[3]-bounds[2])*0.5;
    double zStep=(bounds[5]-bounds[4])*0.5;

    int onface; // boolean
    int zi=0;
    pt[2]=bounds[4];
    while(zi<3)
      {
      int yi=0;
      pt[1]=bounds[2];
      while(yi<3)
        {
        int xi=0;
        pt[0]=bounds[0];
        while(xi<3)
          {
          switch(midpoints)
            {
            case 0:
              allPtIds[xi][yi][zi]=ptIds[originalpt];
              ++originalpt;
              break;
            case 1: // edge center
//              pt[0]=allBounds[0][xi];
//              pt[1]=allBounds[1][yi];
//              pt[2]=allBounds[2][zi];
              onface=0;
              if(xi!=1)
                {
                if(xi==0)
                  {
                  onface=onface||onFace[0];
                  }
                else
                  {
                  onface=onface||onFace[1];
                  }
                }
              if(yi!=1)
                {
                if(yi==0)
                  {
                  onface=onface||onFace[2];
                  }
                else
                  {
                  onface=onface||onFace[3];
                  }
                }
              if(zi!=1)
                {
                if(zi==0)
                  {
                  onface=onface||onFace[4];
                  }
                else
                  {
                  onface=onface||onFace[5];
                  }
                }
              if(onface)
                {
                allPtIds[xi][yi][zi]=this->OutPts->InsertNextPoint(pt);
                }
              break;
            case 2: // face center
              if(xi!=1)
                {
                if(xi==0)
                  {
                  onface=onFace[0];
                  }
                else
                  {
                  onface=onFace[1];
                  }
                }
              else
                {
                if(yi!=1)
                  {
                  if(yi==0)
                    {
                    onface=onFace[2];
                    }
                  else
                    {
                    onface=onFace[3];
                    }
                  }
                else
                  {
                  if(zi==0)
                    {
                    onface=onFace[4];
                    }
                  else
                    {
                    assert("check: not_one" && zi==2);
                    onface=onFace[5];
                    }
                  }
                }
                if(onface)
                  {
                  allPtIds[xi][yi][zi]=this->OutPts->InsertNextPoint(pt);
                  }
              break;
            default: // 3: internal point, not used, not initialized
              break;
            }
          ++xi;
          if(xi==1)
            {
            ++midpoints;
            }
          else
            {
            if(xi==2)
              {
              --midpoints;
              }
            }
          pt[0]+=xStep;
          }
        ++yi;
        if(yi==1)
          {
          ++midpoints;
          }
        else
          {
          if(yi==2)
            {
            --midpoints;
            }
          }
        pt[1]+=yStep;
        }
      ++zi;
      if(zi==1)
        {
        ++midpoints;
        }
      else
        {
        if(zi==2)
          {
          --midpoints;
          }
        }
      pt[2]+=zStep;
      }

    // traverse children (zi|yi|xi) that are on the octree boundary
    zi=0;
    int zchild=0;
    while(zi<2)
      {
      int yi=0;
      int ychild=zchild;
      while(yi<2)
        {
        int xi=0;
        int child=ychild;
        while(xi<2)
          {
          // find if the current child is on the octree boundary
          // and compute the flag for the subfaces.
          int subOnFace[6];
          int onOctreeBoundary=0;

          int face=0;
          int max=0;
          int mask=1;

          while(face<6)
            {
            if(max)
              {
              subOnFace[face]=onFace[face]&&(child&mask); // Face[child][0];
              mask<<=1;
              }
            else
              {
              subOnFace[face]=onFace[face]&&!(child&mask); // !Face[child][0];
              }
            onOctreeBoundary=onOctreeBoundary||subOnFace[face];
            ++face;
            max^=1;
            }


          // skip the interior children.
          if(onOctreeBoundary)
            {
            // Compute the bounds
            double newBounds[6];
            newBounds[0]=allBounds[0][xi];
            newBounds[1]=allBounds[0][xi+1];
            newBounds[2]=allBounds[1][yi];
            newBounds[3]=allBounds[1][yi+1];
            newBounds[4]=allBounds[2][zi];
            newBounds[5]=allBounds[2][zi+1];

            // Set the point ids
            vtkIdType subPtIds[8];
            int pti=0;
            int z=zi;
            while(z<(zi+2))
              {
              int y=yi;
              while(y<(yi+2))
                {
                int x=xi;
                while(x<(xi+2))
                  {
                  subPtIds[pti]=allPtIds[x][y][z];
                  ++pti;
                  ++x;
                  }
                ++y;
                }
              ++z;
              }

            this->Cursor->ToChild(child);
            this->GenerateFaces(newBounds,subPtIds,subOnFace);
            this->Cursor->ToParent();
            }

          ++child;
          ++xi;
          }
        ++yi;
        ychild+=2;
        }
      ++zi;
      zchild+=4;
      }
    }
}

//-----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By
// default an instance of vtkMergePoints is used.
void vtkHyperOctreeSurfaceFilter::SetLocator(vtkIncrementalPointLocator *locator)
{
  if(this->Locator!=locator)
    {
    if(this->Locator!=0)
      {
      this->Locator->UnRegister(this);
      }
    this->Locator=locator;

    if(this->Locator!=0)
      {
      this->Locator->Register(this);
      }
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkHyperOctreeSurfaceFilter::CreateDefaultLocator()
{
  if(this->Locator==0)
    {
    this->Locator=vtkMergePoints::New();
    }
}

//-----------------------------------------------------------------------------
int vtkHyperOctreeSurfaceFilter::FillInputPortInformation(int,
                                                          vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperOctree");
  return 1;
}

//-----------------------------------------------------------------------------
void vtkHyperOctreeSurfaceFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Merging: " << (this->Merging ? "On\n" : "Off\n");
  if ( this->Locator )
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else
    {
    os << indent << "Locator: (none)\n";
    }
  os << indent << "PassThroughCellIds: " << (this->PassThroughCellIds ? "On\n" : "Off\n");
}

//-----------------------------------------------------------------------------
unsigned long int vtkHyperOctreeSurfaceFilter::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long time;

  if(this->Locator!=0)
    {
    time=this->Locator->GetMTime();
    if(time>mTime)
      {
      mTime=time;
      }
    }
  return mTime;
}

//----------------------------------------------------------------------------
void vtkHyperOctreeSurfaceFilter::RecordOrigCellId(vtkIdType destIndex,
                                                   vtkIdType originalId)
{
  if (this->OriginalCellIds != NULL)
    {
    this->OriginalCellIds->InsertValue(destIndex, originalId);
    }
}
