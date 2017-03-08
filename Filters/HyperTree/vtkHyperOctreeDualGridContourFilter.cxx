/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctreeDualGridContourFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperOctreeDualGridContourFilter.h"
#include "vtkMarchingCubesTriangleCases.h"

#include "vtkHyperOctree.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkClipVolume.h"
#include "vtkExecutive.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkImageData.h"
#include "vtkImplicitFunction.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkHyperOctreeCursor.h"
#include "vtkVoxel.h"
#include "vtkPixel.h"
#include "vtkLine.h"
#include "vtkTetra.h"
#include "vtkPolygon.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include <cmath>
#include <cassert>
#include <set>
#include "vtkBitArray.h"
#include "vtkTimerLog.h"
#include "vtkIncrementalPointLocator.h"



//----------------------------------------------------------------------------
void vtkHyperOctreeDualGridContourFilter::PrintSelf(ostream& os,
                                                          vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  this->ContourValues->PrintSelf(os,indent.GetNextIndent());

  if ( this->Locator )
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }
}


class vtkHyperOctreeIdSet // Pimpl idiom
{
public:
  std::set<vtkIdType> Set;
};

vtkStandardNewMacro(vtkHyperOctreeDualGridContourFilter);

//----------------------------------------------------------------------------
// Construct with user-specified implicit function; value
// set to 0.0; and generate cut scalars turned off.
vtkHyperOctreeDualGridContourFilter::vtkHyperOctreeDualGridContourFilter()
{
  this->ContourValues = vtkContourValues::New();

  this->Locator = NULL;

  this->SetNumberOfOutputPorts(1);

  // by default process active cell scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);

  this->Input=0;
  this->Output=0;

  this->NewPolys=0;

  this->InPD=0;
  this->OutPD=0;

  this->InScalars=0;

  // Create the table necessary to move the neighborhhood through the tree.
  this->GenerateTraversalTable();
}

//----------------------------------------------------------------------------
vtkHyperOctreeDualGridContourFilter::~vtkHyperOctreeDualGridContourFilter()
{
  this->ContourValues->Delete();
  if ( this->Locator )
  {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
  }
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If Cut function is modified,
// then this object is modified as well.
vtkMTimeType vtkHyperOctreeDualGridContourFilter::GetMTime()
{
  vtkMTimeType mTime=this->Superclass::GetMTime();
  vtkMTimeType contourValuesMTime=this->ContourValues->GetMTime();
  vtkMTimeType time;

  mTime = ( contourValuesMTime > mTime ? contourValuesMTime : mTime );

  if ( this->Locator != NULL )
  {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
  }

  return mTime;
}

//----------------------------------------------------------------------------
// This table is used to move a 3x3x3 neighborhood of cursors through the tree.
void vtkHyperOctreeDualGridContourFilter::GenerateTraversalTable()
{
  int xChild, yChild, zChild;
  int xCursor, yCursor, zCursor;
  int xNeighbor, yNeighbor, zNeighbor;
  int xNewCursor, yNewCursor, zNewCursor;
  int xNewChild, yNewChild, zNewChild;
  int cursor, child, newCursor, newChild;

  for (zChild = 0; zChild < 2; ++zChild)
  {
    for (yChild = 0; yChild < 2; ++yChild)
    {
      for (xChild = 0; xChild < 2; ++xChild)
      {
        for (zCursor = 0; zCursor < 2; ++zCursor)
        {
          for (yCursor = 0; yCursor < 2; ++yCursor)
          {
            for (xCursor = 0; xCursor < 2; ++xCursor)
            {
              // Compute the x, y, z index into the
              // 4x4x4 neighborhood of children.
              xNeighbor = xCursor + xChild;
              yNeighbor = yCursor + yChild;
              zNeighbor = zCursor + zChild;
              // Separate neighbor index into Cursor/Child index.
              xNewCursor = xNeighbor / 2;
              yNewCursor = yNeighbor / 2;
              zNewCursor = zNeighbor / 2;
              xNewChild = xNeighbor - xNewCursor*2;
              yNewChild = yNeighbor - yNewCursor*2;
              zNewChild = zNeighbor - zNewCursor*2;
              // Cursor and traversal child are for index into table.
              cursor = xCursor + 2*yCursor + 4*zCursor;
              child = xChild + 2*yChild + 4*zChild;
              // New cursor and new child are for the value of the table.
              newCursor = xNewCursor + 2*yNewCursor + 4*zNewCursor;
              newChild = xNewChild + 2*yNewChild + 4*zNewChild;
              this->NeighborhoodTraversalTable[8*child + cursor]
                      = newChild+ 8*newCursor;
            }
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
// The purpose of traversing the neighborhood / cells is to visit
// every point and have the cells connected to that point.
void vtkHyperOctreeDualGridContourFilter::TraverseNeighborhoodRecursively(
  vtkHyperOctreeLightWeightCursor* neighborhood,
  unsigned short *xyzIds)
{
  int divide = 0;
  unsigned char childrenToTraverse[8];
  memset(childrenToTraverse,0,8);

  if ( ! neighborhood[0].GetIsLeaf())
  { // Main cursor is a node.  Traverse all children.
    divide = 1;
    childrenToTraverse[0] = childrenToTraverse[1]
       = childrenToTraverse[2] = childrenToTraverse[3]
       = childrenToTraverse[4] = childrenToTraverse[5]
       = childrenToTraverse[6] = childrenToTraverse[7] = 1;
  }
  else
  {
    if (! neighborhood[1].GetIsLeaf() )
    { // x face
      divide = 1;
      childrenToTraverse[1] = childrenToTraverse[3]
         = childrenToTraverse[5] = childrenToTraverse[7] = 1;
    }
    if (! neighborhood[2].GetIsLeaf() )
    { // y face
      divide = 1;
      childrenToTraverse[2] = childrenToTraverse[3]
         = childrenToTraverse[6] = childrenToTraverse[7] = 1;
    }
    if (! neighborhood[4].GetIsLeaf() )
    { // z face
      divide = 1;
      childrenToTraverse[4] = childrenToTraverse[5]
         = childrenToTraverse[6] = childrenToTraverse[7] = 1;
    }
    if (! neighborhood[3].GetIsLeaf() )
    { // xy edge
      divide = 1;
      childrenToTraverse[3] = childrenToTraverse[7] = 1;
    }
    if (! neighborhood[5].GetIsLeaf() )
    { // xz edge
      divide = 1;
      childrenToTraverse[5] = childrenToTraverse[7] = 1;
    }
    if (! neighborhood[6].GetIsLeaf() )
    { // xz edge
      divide = 1;
      childrenToTraverse[6] = childrenToTraverse[7] = 1;
    }
    if (! neighborhood[7].GetIsLeaf() )
    { // xyz corner
      divide = 1;
      childrenToTraverse[7] = 1;
    }
  }

  if (divide)
  {
    int child;
    int neighbor;
    unsigned char tChild, tParent;
    unsigned char* traversalTable = this->NeighborhoodTraversalTable;
    vtkHyperOctreeLightWeightCursor newNeighborhood[8];
    // Storing 4 per neighbor for efficiency.
    // This might also be useful for 4d trees :)
    unsigned short newXYZIds[32];
    for (child = 0; child < 8; ++child)
    {
      if (childrenToTraverse[child])
      {
        unsigned short *inId;
        unsigned short *outId = newXYZIds;
        // Move each neighbor down to a child.
        for (neighbor = 0; neighbor < 8; ++neighbor)
        {
          tChild = (*traversalTable) & 7;
          tParent = ((*traversalTable) & 248)>>3;
          inId = xyzIds+(tParent<<2); // Faster to multiply by 4 than 3.
          if (neighborhood[tParent].GetIsLeaf())
          { // Parent is a leaf or this is an empty node.
            // We can't traverse anymore.
            // equal operator should work for this class.
            newNeighborhood[neighbor] = neighborhood[tParent];
            *outId++ = *inId++;
            *outId++ = *inId++;
            *outId++ = *inId++;
            // We need an extra increment to skip over unused 4th id.
            ++outId;
          }
          else
          { // Move to child.
            // equal operator should work for this class.
            newNeighborhood[neighbor] = neighborhood[tParent];
            newNeighborhood[neighbor].ToChild(tChild);
            // Multiply parent index by two for new level.
            // Increment by 1 if child requires.
            *outId++ = (*inId++ << 1) | (tChild&1);
            *outId++ = (*inId++ << 1) | ((tChild>>1)&1);
            *outId++ = (*inId++ << 1) | ((tChild>>2)&1);
            // We need an extra increment to skip over unused 4th id.
            ++outId;
          }
          ++traversalTable;
        }
        this->TraverseNeighborhoodRecursively(newNeighborhood, newXYZIds);
      }
      else
      {
        traversalTable += 8;
      }
    }
    return;
  }

  // All neighbors must be leaves.

  // If we are not on the border, create the cell
  // associated with the center point of the neighborhood.
  this->EvaluatePoint(neighborhood, xyzIds);
}

//----------------------------------------------------------------------------
// Contour the cell associated with the center point.
// if it has not already been contoured.
void vtkHyperOctreeDualGridContourFilter::EvaluatePoint(
  vtkHyperOctreeLightWeightCursor* neighborhood,
  unsigned short* xyzIds)
{
  // If any neighbor is NULL, then we are on the border.
  // Do nothing if we are on a border.
  // We know that neighbor 0 is never NULL.
  if (!neighborhood[1].GetTree() ||
      !neighborhood[2].GetTree() || !neighborhood[3].GetTree() ||
      !neighborhood[4].GetTree() || !neighborhood[5].GetTree() ||
      !neighborhood[6].GetTree() || !neighborhood[7].GetTree())
  {
    return;
  }

  static int edges[12][2] = { {0,1}, {1,2}, {2,3}, {0,3},
                              {4,5}, {5,6}, {6,7}, {4,7},
                              {0,4}, {1,5}, {3,7}, {2,6}};

  static int CASE_MASK[8] = {1,2,4,8,16,32,64,128};
  int vertMap[8];
  // !!!! Notice the translation from Voxel ids to Hex ids.
  vertMap[0] = neighborhood[0].GetLeafIndex();
  vertMap[1] = neighborhood[1].GetLeafIndex();
  vertMap[2] = neighborhood[3].GetLeafIndex();
  vertMap[3] = neighborhood[2].GetLeafIndex();
  vertMap[4] = neighborhood[4].GetLeafIndex();
  vertMap[5] = neighborhood[5].GetLeafIndex();
  vertMap[6] = neighborhood[7].GetLeafIndex();
  vertMap[7] = neighborhood[6].GetLeafIndex();
  // We need a map to permute the point ids from voxel to hex.
  // Note: Permutation is its own inverse.  Makes life easy.
  static int HEX_VOX_PERMUTATION[8] = {0,1,3,2,4,5,7,6};

  double points[8][3];
  double scalars[8];
  double levelDim;
  for (int iter = 0; iter < 8; ++iter)
  {
    // Note: we have to extent points on boundary of tree !!!
    scalars[iter] = this->InScalars->GetComponent(vertMap[iter],0);
    levelDim = static_cast<double>(1<<neighborhood[iter].GetLevel());
    points[HEX_VOX_PERMUTATION[iter]][0]
       = this->Origin[0] +
      (static_cast<double>(*xyzIds++)+0.5)*(this->Size[0])/levelDim;
    points[HEX_VOX_PERMUTATION[iter]][1]
       = this->Origin[1] +
      (static_cast<double>(*xyzIds++)+0.5)*(this->Size[1])/levelDim;
    points[HEX_VOX_PERMUTATION[iter]][2]
       = this->Origin[2] +
      (static_cast<double>(*xyzIds++)+0.5)*(this->Size[2])/levelDim;
    // We need to skip over unused 4th id.
    ++xyzIds;
  }

  int numContours=this->ContourValues->GetNumberOfContours();
  for (int iter = 0; iter < numContours; ++iter)
  {
    double value = this->ContourValues->GetValue(iter);

    // The cell contour method had two problems:
    // You could not switch cell and point data easily,
    // and you had to copy the scalars and points.
    //this->Voxel->Contour(value, this->VoxelScalars, this->Locator,
    //                     this->NewVerts,this->NewLines,this->NewPolys,
    //                     this->InPD, this->OutPD, this->InPD,
    //                     this->InCellCount, this->OutCD);
    // Contour the voxel our self.
    // Some voxels will be degenerated with points shared between corners.
    // Appropriate faces will always line up.
    vtkMarchingCubesTriangleCases *triCase;
    EDGE_LIST  *edge;
    int i, j, index, *vert;
    vtkIdType pts[3];
    double t, x[3];
    double *x1;
    double *x2;

    // Build the case table
    for ( i=0, index = 0; i < 8; i++)
    {
      if (scalars[i] >= value)
      {
        index |= CASE_MASK[i];
      }
    }

    triCase = vtkMarchingCubesTriangleCases::GetCases() + index;
    edge = triCase->edges;

    for ( ; edge[0] > -1; edge += 3 )
    {
      for (i=0; i<3; i++) // insert triangle
      {
        vert = edges[edge[i]];
        t = (value - scalars[vert[0]]) / (scalars[vert[1]] - scalars[vert[0]]);
        x1 = (points[vert[0]]);
        x2 = (points[vert[1]]);
        for (j=0; j<3; j++)
        {
          x[j] = x1[j] + t * (x2[j] - x1[j]);
        }
        if ( this->Locator->InsertUniquePoint(x, pts[i]) )
        {
          int p1 = vertMap[vert[0]];
          int p2 = vertMap[vert[1]];
          this->OutPD->InterpolateEdge(this->InPD,pts[i],p1,p2,t);
        }
      }
      // check for degenerate triangle
      if ( pts[0] != pts[1] &&
           pts[0] != pts[2] &&
           pts[1] != pts[2] )
      {
        this->NewPolys->InsertNextCell(3,pts);
        // We have no point data in the octree that would convert to cell data.
        //outCd->CopyData(inCd,cellId,newCellId);
      }
    }
  }

  // We have passed all the tests, generate the dual cell.
  /*
  vtkIdType ptIds[2];
  ptIds[0] = neighborhood[c000].GetLeafIndex();
  ptIds[1] = neighborhood[c001].GetLeafIndex();
  lines->InsertNextCell(2, ptIds);
  ptIds[0] = neighborhood[c010].GetLeafIndex();
  ptIds[1] = neighborhood[c011].GetLeafIndex();
  lines->InsertNextCell(2, ptIds);
  ptIds[0] = neighborhood[c100].GetLeafIndex();
  ptIds[1] = neighborhood[c101].GetLeafIndex();
  lines->InsertNextCell(2, ptIds);
  ptIds[0] = neighborhood[c110].GetLeafIndex();
  ptIds[1] = neighborhood[c111].GetLeafIndex();
  lines->InsertNextCell(2, ptIds);

  ptIds[0] = neighborhood[c000].GetLeafIndex();
  ptIds[1] = neighborhood[c010].GetLeafIndex();
  lines->InsertNextCell(2, ptIds);
  ptIds[0] = neighborhood[c001].GetLeafIndex();
  ptIds[1] = neighborhood[c011].GetLeafIndex();
  lines->InsertNextCell(2, ptIds);
  ptIds[0] = neighborhood[c100].GetLeafIndex();
  ptIds[1] = neighborhood[c110].GetLeafIndex();
  lines->InsertNextCell(2, ptIds);
  ptIds[0] = neighborhood[c101].GetLeafIndex();
  ptIds[1] = neighborhood[c111].GetLeafIndex();
  lines->InsertNextCell(2, ptIds);

  ptIds[0] = neighborhood[c000].GetLeafIndex();
  ptIds[1] = neighborhood[c100].GetLeafIndex();
  lines->InsertNextCell(2, ptIds);
  ptIds[0] = neighborhood[c001].GetLeafIndex();
  ptIds[1] = neighborhood[c101].GetLeafIndex();
  lines->InsertNextCell(2, ptIds);
  ptIds[0] = neighborhood[c010].GetLeafIndex();
  ptIds[1] = neighborhood[c110].GetLeafIndex();
  lines->InsertNextCell(2, ptIds);
  ptIds[0] = neighborhood[c011].GetLeafIndex();
  ptIds[1] = neighborhood[c111].GetLeafIndex();
  lines->InsertNextCell(2, ptIds);
  */
}


//----------------------------------------------------------------------------
//
// Cut through data generating surface.
//
int vtkHyperOctreeDualGridContourFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input
  this->Input = vtkHyperOctree::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  if(this->Input->GetNumberOfLevels()==1)
  {
    // just the root. There is absolutely no chance
    // to get an isosurface here.
    this->Input=0;
    return 1;
  }

  if (this->Input->GetDimension() != 3)
  {
    vtkErrorMacro("This class only handles 3d Octree's");
    return 1;
  }

  this->InScalars=this->GetInputArrayToProcess(0,inputVector);
  if(this->InScalars==0)
  {
    vtkDebugMacro(<<"No data to contour");
    this->Input=0;
    return 1;
  }

  int numContours=this->ContourValues->GetNumberOfContours();
  if(numContours==0)
  {
    vtkDebugMacro(<<"No contour");
    this->Input=0;
    return 1;
  }

  double *values=this->ContourValues->GetValues();

  // If all the contour values are out of the range of the input scalar
  // there is no chance to get a contour, just exit.

  double range[2];
  this->InScalars->GetRange(range);
  int i=0;
  int allOut=1;
  while(allOut && i<numContours)
  {
    allOut=(values[i]<range[0]) || (values[i]>range[1]);
    ++i;
  }
  if(allOut)
  {
    // empty output
    this->Input=0;
    return 1;
  }

  this->Output=vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  this->Input->GetOrigin(this->Origin);
  this->Input->GetSize(this->Size);

  // Assumes that the DataSet API returns dual.
  vtkIdType numLeaves = this->Input->GetNumberOfPoints();
  //cout << numLeaves << " leaves\n";

  vtkIdType estimatedSize = numLeaves / 2;

  vtkPoints *newPoints = vtkPoints::New();
  newPoints->Allocate(estimatedSize,estimatedSize/2);

  this->NewPolys = vtkCellArray::New();
  this->NewPolys->Allocate(estimatedSize,estimatedSize/2);

  // locator used to merge potentially duplicate points
  if ( this->Locator == NULL )
  {
    this->CreateDefaultLocator();
  }

  this->Locator->InitPointInsertion (newPoints, this->Input->GetBounds());

  this->InPD=this->Input->GetLeafData();
  this->OutPD=this->Output->GetPointData();
  this->OutPD->CopyAllocate(this->InPD,estimatedSize,estimatedSize/2);

  // Create an array of cursors that occupy 1 2x2x2 neighborhhod.  This
  // will traverse the tree as one.
  vtkHyperOctreeLightWeightCursor neighborhood[8];
  neighborhood[0].Initialize(this->Input);
  // Index of node in uniform grid (x,y,z) for each neighbor.
  // Storing 4 indexes per neighbor for efficiency.
  // Could also be useful for 4d trees :)
  unsigned short xyzId[32];
  memset(xyzId,0,32*sizeof(unsigned short));
  this->TraverseNeighborhoodRecursively(neighborhood, xyzId);

  this->Output->SetPolys(this->NewPolys);
  this->NewPolys->Delete();
  this->NewPolys = 0;
  // Points were added by the locator.
  this->Output->SetPoints(newPoints);
  newPoints->Delete();
  newPoints = 0;

  return 1;
}



//----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By default,
// an instance of vtkMergePoints is used.
void vtkHyperOctreeDualGridContourFilter::SetLocator(vtkIncrementalPointLocator *locator)
{
  if ( this->Locator == locator)
  {
    return;
  }

  if ( this->Locator )
  {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
  }

  if ( locator )
  {
    locator->Register(this);
  }

  this->Locator = locator;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkHyperOctreeDualGridContourFilter::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
  {
    this->Locator = vtkMergePoints::New();
    this->Locator->Register(this);
    this->Locator->Delete();
  }
}

//----------------------------------------------------------------------------
int vtkHyperOctreeDualGridContourFilter::RequestUpdateExtent(
  vtkInformation *,
  vtkInformationVector **inputVector,
  vtkInformationVector *)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperOctreeDualGridContourFilter::FillInputPortInformation(int,
                                                          vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperOctree");
  return 1;
}
