/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctreeCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperOctreeCutter.h"

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
#include "vtkOrderedTriangulator.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include <cmath>
#include <cassert>
#include "vtkHyperOctreeClipCutPointsGrabber.h"
#include "vtkIncrementalPointLocator.h"

vtkStandardNewMacro(vtkHyperOctreeCutter);
vtkCxxSetObjectMacro(vtkHyperOctreeCutter,CutFunction,vtkImplicitFunction);

//----------------------------------------------------------------------------
// Construct with user-specified implicit function; value
// set to 0.0; and generate cut scalars turned off.
vtkHyperOctreeCutter::vtkHyperOctreeCutter(vtkImplicitFunction *cf)
{
  this->ContourValues = vtkContourValues::New();
  this->SortBy = VTK_SORT_BY_VALUE;

  this->CutFunction = cf;
  this->Locator = NULL;
  this->GenerateCutScalars = 0;

  this->SetNumberOfOutputPorts(1);

  // by default process active point scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);

  this->Input=0;
  this->Output=0;

  this->NewVerts=0;
  this->NewLines=0;
  this->NewPolys=0;

  this->InCD=0;
  this->OutCD=0;
  this->OutPD=0;
  this->Triangulator=0;
  this->Sibling=0;

  this->Tetra=0;
  this->Polygon=0;
  this->TetScalars=0;
  this->CellScalars=0;
  this->Pts=0;

  this->AllLess=0;
  this->AllGreater=0;
  this->Grabber=0;
}

//----------------------------------------------------------------------------
vtkHyperOctreeCutter::~vtkHyperOctreeCutter()
{
  this->ContourValues->Delete();
  if ( this->Locator )
  {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
  }
  this->SetCutFunction(NULL);
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If Cut function is modified,
// then this object is modified as well.
vtkMTimeType vtkHyperOctreeCutter::GetMTime()
{
  vtkMTimeType mTime=this->Superclass::GetMTime();
  vtkMTimeType contourValuesMTime=this->ContourValues->GetMTime();
  vtkMTimeType time;

  mTime = ( contourValuesMTime > mTime ? contourValuesMTime : mTime );

  if ( this->CutFunction != NULL )
  {
    time = this->CutFunction->GetMTime();
    mTime = ( time > mTime ? time : mTime );
  }
  if ( this->Locator != NULL )
  {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
  }

  return mTime;
}

//----------------------------------------------------------------------------
//
// Cut through data generating surface.
//
int vtkHyperOctreeCutter::RequestData(vtkInformation *vtkNotUsed(request),
                                      vtkInformationVector **inputVector,
                                      vtkInformationVector *outputVector)
{
  if ( !this->CutFunction)
  {
    vtkErrorMacro(<<"No cut function specified.");
    return 0;
  }

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  this->Input = vtkHyperOctree::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  this->Output=vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));


  vtkIdType numPts=this->Input->GetMaxNumberOfPoints(0);
  vtkIdType numCells = this->Input->GetNumberOfLeaves();

  vtkPoints *newPoints = vtkPoints::New();
  newPoints->Allocate(numPts,numPts/2);

  // allocate the output and associated helper classes
  vtkIdType estimatedSize = numCells;
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024)
  {
    estimatedSize = 1024;
  }

  this->NewVerts=vtkCellArray::New();
  this->NewVerts->Allocate(estimatedSize,estimatedSize/2);
  this->NewLines = vtkCellArray::New();
  this->NewLines->Allocate(estimatedSize,estimatedSize/2);
  this->NewPolys = vtkCellArray::New();
  this->NewPolys->Allocate(estimatedSize,estimatedSize/2);

  // locator used to merge potentially duplicate points
  if ( this->Locator == NULL )
  {
    this->CreateDefaultLocator();
  }

  this->Locator->InitPointInsertion (newPoints, this->Input->GetBounds());

  this->InCD=this->Input->GetPointData();
  this->OutCD=this->Output->GetCellData();
  this->OutCD->CopyAllocate(this->InCD,estimatedSize,estimatedSize/2);

  this->OutPD=this->Output->GetPointData();
  if ( !this->GenerateCutScalars &&
       !this->GetInputArrayToProcess(0,inputVector))
  {
    this->OutPD->CopyScalarsOff();
  }
  else
  {
    this->OutPD->CopyScalarsOn();
  }

  vtkHyperOctreeCursor *cursor=this->Input->NewCellCursor();
  this->Sibling=cursor->Clone();

  cursor->ToRoot();

  double bounds[6];
  this->Input->GetBounds(bounds);

  switch(this->Input->GetDimension())
  {
    case 3:
      this->Tetra=vtkTetra::New();
      this->TetScalars=vtkDoubleArray::New();
      this->TetScalars->SetNumberOfComponents(1);
      this->TetScalars->SetNumberOfTuples(4);
      this->Grabber=vtkHyperOctreeClipCutPointsGrabber::New();
      this->Grabber->SetDimension(3);
      this->Triangulator=this->Grabber->GetTriangulator();
      break;
    case 2:
      this->Grabber=vtkHyperOctreeClipCutPointsGrabber::New();
      this->Grabber->SetDimension(2);
      this->Polygon=this->Grabber->GetPolygon();
      break;
    default:
      // do nothing
      break;
  }
  this->CellScalars=vtkDoubleArray::New();
  this->Pts=vtkPoints::New();

  this->TotalCounter=0;
  this->TemplateCounter=0;

  int j=0;
  while(j<65536)
  {
    this->CellTypeCounter[j]=0; // up-to-65536 points per octant
    ++j;
  }

  int numContours=this->ContourValues->GetNumberOfContours();

  if ( this->SortBy == VTK_SORT_BY_CELL )
  {
    this->Iter=0;
    while(this->Iter < numContours)
    {
      this->CutNode(cursor,0,bounds);
      ++this->Iter;
    }
  }
  else
  { // VTK_SORT_BY_VALUE
    if(numContours>0)
    {
      this->AllLess=new int[numContours];
      this->AllGreater=new int[numContours];
      this->CutNode(cursor,0,bounds);
      delete[] this->AllLess;
      this->AllLess=0;
      delete[] this->AllGreater;
      this->AllGreater=0;
    }
  }

//  cout<<"ClipHyperOctree: "<<this->TemplateCounter<<" templates over "<<this->TotalCounter<<" octants, ratio="<<(this->TemplateCounter/static_cast<double>(this->TotalCounter))<<endl;

  j=0;
  while(j<65536)
  {
    if(this->CellTypeCounter[j]>0)
    {
//      cout<<this->CellTypeCounter[j]<<" with "<<j+1<<"points"<<endl;
    }
    ++j;
  }

  switch(this->Input->GetDimension())
  {
    case 3:
      this->Tetra->UnRegister(this);
      this->Tetra=0;
      this->TetScalars->UnRegister(this);
      this->TetScalars=0;
      this->Grabber->UnRegister(this);
      this->Grabber=0;
      this->Triangulator=0;
      break;
    case 2:
      this->Grabber->UnRegister(this);
      this->Grabber=0;
      this->Polygon=0;
      break;
    default:
      break;
  }

  this->CellScalars->UnRegister(this);
  this->CellScalars=0;
  this->Pts->UnRegister(this);
  this->Pts=0;

  cursor->UnRegister(this);
  this->Sibling->UnRegister(this);
  this->Sibling=0;

  this->OutPD=0;
  this->Input=0;
  this->InCD=0;
  this->Output->SetPoints(newPoints);
  newPoints->Delete();

  if (this->NewVerts->GetNumberOfCells()>0)
  {
    this->Output->SetVerts(this->NewVerts);
  }
  this->NewVerts->Delete();
  this->NewVerts=0;

  if (this->NewLines->GetNumberOfCells()>0)
  {
    this->Output->SetLines(this->NewLines);
  }
  this->NewLines->Delete();
  this->NewLines=0;

  if (this->NewPolys->GetNumberOfCells()>0)
  {
    this->Output->SetPolys(this->NewPolys);
  }
  this->NewPolys->Delete();
  this->NewPolys=0;

  this->OutCD=0;

  this->Locator->Initialize();//release any extra memory
  this->Output->Squeeze();
  this->Output=0;

  assert("post: input_is_null" && this->Input==0);
  assert("post: output_is_null" && this->Output==0);
  assert("post: incd_is_null" && this->InCD==0);
  assert("post: outpd_is_null" && this->OutPD==0);
  assert("post: outcd_is_null" && this->OutCD==0);

  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperOctreeCutter::CutNode(vtkHyperOctreeCursor *cursor,
                                   int level,
                                   double bounds[6])
{
  assert("pre: cursor_exists" && cursor!=0);
  assert("pre: positive_level" && level>=0);

  if(cursor->CurrentIsLeaf())
  {
    if(cursor->CurrentIsRoot() || (this->Input->GetDimension()==1))
    {
      // no parent=>no sibling=>no sibling which are not leaves=>easy
      // just create a voxel/pixel/line and cut it.

      vtkCell *cell=0;
      vtkIdType cellId=cursor->GetLeafId(); // only one cell.

      vtkDoubleArray *cellScalars;
      cellScalars=vtkDoubleArray::New();// scalar at each corner point.
      cellScalars->Allocate(VTK_CELL_SIZE);

      vtkIdType numPts=0;
      double pt[3];
      vtkVoxel *v;
      vtkPixel *p;
      vtkLine *l;
      switch(this->Input->GetDimension())
      {
        case 3:
          v=vtkVoxel::New();
          cell=v;
          numPts=8;
          pt[0]=bounds[0];
          pt[1]=bounds[2];
          pt[2]=bounds[4];
          v->GetPoints()->SetPoint(0,pt);
          pt[0]=bounds[1];
          v->GetPoints()->SetPoint(1,pt);
          pt[0]=bounds[0];
          pt[1]=bounds[3];
          v->GetPoints()->SetPoint(2,pt);
          pt[0]=bounds[1];
          v->GetPoints()->SetPoint(3,pt);
          pt[0]=bounds[0];
          pt[1]=bounds[2];
          pt[2]=bounds[5];
          v->GetPoints()->SetPoint(4,pt);
          pt[0]=bounds[1];
          v->GetPoints()->SetPoint(5,pt);
          pt[0]=bounds[0];
          pt[1]=bounds[3];
          v->GetPoints()->SetPoint(6,pt);
          pt[0]=bounds[1];
          v->GetPoints()->SetPoint(7,pt);
          break;
        case 2:
          p=vtkPixel::New();
          cell=p;
          numPts=4;
          pt[0]=bounds[0];
          pt[1]=bounds[2];
          pt[2]=bounds[4];
          p->GetPoints()->SetPoint(0,pt);
          pt[0]=bounds[1];
          p->GetPoints()->SetPoint(1,pt);
          pt[0]=bounds[0];
          pt[1]=bounds[3];
          p->GetPoints()->SetPoint(2,pt);
          pt[0]=bounds[1];
          p->GetPoints()->SetPoint(3,pt);
          break;
        case 1:
          l=vtkLine::New();
          cell=l;
          numPts=2;
          pt[0]=bounds[0];
          pt[1]=bounds[2];
          pt[2]=bounds[4];
          l->GetPoints()->SetPoint(0,pt);
          pt[0]=bounds[1];
          l->GetPoints()->SetPoint(1,pt);
          break;
        default:
          assert("check: impossible" && 0);
          break;
      }

      vtkDataArray *cutScalars=0;

      vtkPointData *inPD=this->Input->GetPointData();

      if(this->CutFunction!=0)
      {
        vtkDoubleArray *tmpScalars = vtkDoubleArray::New();
        tmpScalars->SetNumberOfTuples(numPts);
        tmpScalars->SetName("CutDataSetScalars");
        inPD = vtkPointData::New();
        inPD->ShallowCopy(this->Input->GetPointData());//copies original
        if(this->GenerateCutScalars)
        {
          inPD->SetScalars(tmpScalars);
        }
        for ( int i=0; i < numPts; i++ )
        {
          double s = this->CutFunction->FunctionValue(cell->GetPoints()->GetPoint(i));
          tmpScalars->SetTuple1(i,s);
        }
        cutScalars = tmpScalars;
      }

#if 0 // just to not break compilation
      outPD->InterpolateAllocate(inPD,estimatedSize,estimatedSize/2);
#endif

      for ( int i=0; i < numPts; i++ )
      {
        double s = cutScalars->GetComponent(i, 0);
        cellScalars->InsertTuple(i, &s);
      }

      if ( this->CutFunction )
      {
        cutScalars->UnRegister(this);
        inPD->UnRegister(this);
      }

      // perform cut
      if ( this->SortBy == VTK_SORT_BY_CELL )
      {
        double value = this->ContourValues->GetValue(this->Iter);

        cell->Contour(value, cellScalars, this->Locator,
                      this->NewVerts, this->NewLines, this->NewPolys,
                      inPD, this->OutPD,
                      static_cast<vtkCellData*>(this->InCD), cellId,
                      static_cast<vtkCellData*>(this->OutCD));
      }
      else
      {
        // VTK_SORT_BY_VALUE
        int numContours=this->ContourValues->GetNumberOfContours();
        int iter=0;
        while(iter<numContours)
        {
          double value = this->ContourValues->GetValue(iter);
          cell->Contour(value, cellScalars, this->Locator,
                        this->NewVerts, this->NewLines, this->NewPolys,
                        inPD, this->OutPD,
                        static_cast<vtkCellData*>(this->InCD), cellId,
                        static_cast<vtkCellData*>(this->OutCD));
          ++iter;
        }
      }

      cellScalars->UnRegister(this);
      cell->UnRegister(this);

    }
    else
    {
      // some parent=>have sibling=>some sibling may have children
      // => those children may create points on some face of cursor
      // => difficult case
      //
      // Even worst, if the siblings don't have children, the
      // sibling of the parent may have children that create points
      // on some face.
      //
      // Even if there is no children, the neighbor cell tessellation
      // has to be compatible with the current cell tessellation.
      // In any case, we need the ordered triangulator.


      // Add the points of the current leaf
      // use the bounds

      vtkIdType ptId;
      int i;
      int j;
      int k;

      int pi;
      int pj;
      int pk;

      int x;
      int y;
      int z;

      // resolution in point along each axis.
      int resolution=(1<<(this->Input->GetNumberOfLevels()-1))+1;

      int deltaLevel=this->Input->GetNumberOfLevels()-1-level;
      assert("check: positive_deltaLevel" && deltaLevel>=0);

      double ratio=1.0/(resolution-1);

      double pt[3];
      double pcoords[3];

      int allLess=1; // bool
      int allGreater=1; // bool
      double s;

      int numContours=0; // initialized to removed warnings

      if ( this->SortBy == VTK_SORT_BY_VALUE )
      {
        numContours=this->ContourValues->GetNumberOfContours();
        int iter=0;
        while(iter<numContours)
        {
          this->AllLess[iter]=1;
          this->AllGreater[iter]=1;
          ++iter;
        }
      }
      // index of the node
      if(this->Input->GetDimension()==3)
      {
        vtkIdType nbpts=this->Input->GetMaxNumberOfPointsOnBoundary(level);
        double pbounds[6]={0,1,0,1,0,1};

        this->Triangulator->InitTriangulation(pbounds,nbpts);

        this->Triangulator->PreSortedOff();
        this->Grabber->InitPointInsertion();

        i=cursor->GetIndex(0);
        j=cursor->GetIndex(1);
        k=cursor->GetIndex(2);
        z=0;
        pk=k;
        while(z<2)
        {
          pj=j;
          y=0;
          while(y<2)
          {
            pi=i;
            x=0;
            while(x<2)
            {
              pt[0]=bounds[x];
              pt[1]=bounds[2+y];
              pt[2]=bounds[4+z];

              assert("check: in_bounds" && pt[0]>=this->Input->GetBounds()[0] && pt[0]<=this->Input->GetBounds()[1] && pt[1]>=this->Input->GetBounds()[2] && pt[1]<=this->Input->GetBounds()[3] && pt[2]>=this->Input->GetBounds()[4] && pt[2]<=this->Input->GetBounds()[5]);
              // Get some parametric coords in [0,1]
              // [0,1] covers the whole dataset axis.
              pcoords[0]=(pi<<deltaLevel)*ratio;
              pcoords[1]=(pj<<deltaLevel)*ratio;
              pcoords[2]=(pk<<deltaLevel)*ratio;

              ptId=((pk<<deltaLevel)*resolution+(pj<<deltaLevel))*resolution
                +(pi<<deltaLevel);
              this->Triangulator->InsertPoint(ptId,pt,pcoords,0);


              // Test if the point is out or in the clipped part.
              // We have to put this code in the insertion loop of the
              // point because there is no method in vtkOrderedTriangulator
              // to access to inserted points.

              s=this->CutFunction->FunctionValue(pt);

              if ( this->SortBy == VTK_SORT_BY_CELL )
              {
                double value = this->ContourValues->GetValue(this->Iter);
                if(s>value)
                {
                  allLess=0;
                }
                else
                {
                  if(s<value)
                  {
                    allGreater=0;
                  }
                }
              }
              else
              {
                // VTK_SORT_BY_VALUE
                int iter=0;
                while(iter<numContours)
                {
                  double value = this->ContourValues->GetValue(iter);
                  if(s>value)
                  {
                    this->AllLess[iter]=0;
                  }
                  else
                  {
                    if(s<value)
                    {
                      this->AllGreater[iter]=0;
                    }
                  }
                  ++iter;
                }
              }

              ++x;
              ++pi;
            }
            ++y;
            ++pj;
          }
          ++z;
          ++pk;
        }
      }
      else
      {
        // this->Input->GetDimension()==2
        pt[2]=this->Input->GetOrigin()[2];
        y=0;
        while(y<2)
        {
          x=0;
          while(x<2)
          {
            pt[0]=bounds[x];
            pt[1]=bounds[2+y];

            // Test if the point is out or in the clipped part.
            // We have to put this code in the insertion loop of the
            // point because there is no method in vtkOrderedTriangulator
            // to access to inserted points.

            s=this->CutFunction->FunctionValue(pt);

            if ( this->SortBy == VTK_SORT_BY_CELL )
            {
              double value = this->ContourValues->GetValue(this->Iter);
              if(s>value)
              {
                allLess=0;
              }
              else
              {
                if(s<value)
                {
                  allGreater=0;
                }
              }
            }
            else
            {
              // VTK_SORT_BY_VALUE
              int iter=0;
              while(iter<numContours)
              {
                double value = this->ContourValues->GetValue(iter);
                if(s>value)
                {
                  this->AllLess[iter]=0;
                }
                else
                {
                  if(s<value)
                  {
                    this->AllGreater[iter]=0;
                  }
                }
                ++iter;
              }
            }
            ++x;
          }
          ++y;
        }
      }


      // see if we got a chance to either
      // 1. remove the leaf (!this->GenerateClippedOutput && allOut), no need
      // for triangulation, nor clipping, just skip the leaf.
      // 2. triangulate and passing the result without clipping each
      //    sub-tetra (allIn==1). Can work also if this->GenerateClippedOutput
      // is true. For one output, the sub-tetra will be passed (allIn), for the
      // other there will be nothing to pass or the clip. Or, if allIn is false
      // but allOut is true, there is nothing to do with the first output,
      // and passing everything to the second output.

      if ( this->SortBy == VTK_SORT_BY_CELL )
      {
        if(allLess || allGreater)
        {
//        cout<<"this child does not intersect the cut function"<<endl;
          return; // we've just save a lot of useless computation
        }
      }
      else
      {
        // VTK_SORT_BY_VALUE
        int skip=1;
        int iter=0;
        while(skip && iter<numContours)
        {
          skip=skip && (this->AllLess[iter] || this->AllGreater[iter]);
          ++iter;
        }
        if(skip)
        {
          return;
        }
      }

      int lastLevelLeaf=level>=(this->Input->GetNumberOfLevels()-1);

      if(this->Input->GetDimension()==3)
      {
        if(!lastLevelLeaf)
        {
          // Ok, now ask my parent if I have sibling with children on my
          // faces and even worst, if my parent has sibling with children
          // that have children on my face, or if the parent of my parent
          // has sibling with children that have children, that have children
          // on my face, until I reach the root...

          // list the 3 faces of the parent, the current node is laying on.
          int faces[3];

          int child=cursor->GetChildIndex();

          faces[0]=(child&1)==1; // false: -x, true: +x
          faces[1]=(child&2)==2; // false: -y, true: +y
          faces[2]=(child&4)==4; // false: -z, true: +z

          // sibling on faces that are not on a parent face
          int siblings[3];

          int inc=1;
          i=0;
          while(i<3)
          {
            if(faces[i])
            {
              siblings[i]=child-inc;
            }
            else
            {
              siblings[i]=child+inc;
            }
            ++i;
            inc<<=1;
          }

          this->Sibling->ToSameNode(cursor);
          this->Sibling->ToParent();
          // ask the 3 sibling, one on each face of the current node
          i=0;
          int faceOffset=0;
          while(i<3)
          {
            this->Sibling->ToChild(siblings[i]);
            assert("check: we are not visiting ourselves" && this->Sibling->GetChildIndex()!=child);
            if(!this->Sibling->CurrentIsLeaf())
            {
              assert("check: if the sibling is not a leaf we cannot be at the last level" && level<(this->Input->GetNumberOfLevels()-1));

              // get the points of this sibling on some given face
              int siblingFace=faceOffset;
              if(faces[i])
              {
                ++siblingFace;
              }
              this->Input->GetPointsOnFace(this->Sibling,siblingFace,level,
                this->Grabber);
            }
            this->Sibling->ToParent();
            ++i;
            faceOffset+=2;
          }

          // Get points on faces shared with the parent node.
          this->Input->GetPointsOnParentFaces(faces,level,cursor,
                                              this->Grabber);

          // Get the points from the edge-only neighbors.
          int childIndices[3];
          childIndices[2]=(child&4)>>2;
          childIndices[1]=(child&2)>>1;
          childIndices[0]=child&1;

          assert("check valid_range_c2" && childIndices[2]>=0 &&
                 childIndices[2]<=1);
          assert("check valid_range_c1" && childIndices[1]>=0 &&
                 childIndices[1]<=1);
          assert("check valid_range_c0" && childIndices[0]>=0 &&
                 childIndices[0]<=1);

          // First the edges aligned on X axis
          int axis=0;
          int a=2;
          int b=1;

          this->Sibling->ToSameNode(cursor);
          this->Sibling->ToParent();

          while(axis<3)
          {
            k=0;
            while(k<2)
            {
              j=0;
              while(j<2)
              {
                if(k!=childIndices[a]&&j!=childIndices[b])
                {
                  this->Sibling->ToChild((k<<a)+(j<<b)+
                                         (childIndices[axis]<<axis));
                  if(!this->Sibling->CurrentIsLeaf())
                  {
                    this->Input->GetPointsOnEdge(this->Sibling,level,axis,!k,
                                                 !j,this->Grabber);
                  }
                  this->Sibling->ToParent();
                }
                else
                {
                  this->Input->GetPointsOnParentEdge(cursor,level,axis,k,j,
                                                     this->Grabber);
                }
                ++j;
              }
              ++k;
            }
            ++axis;
            ++a;
            if(a>2)
            {
              a-=3;
            }
            ++b;
            if(b>2)
            {
              b-=3;
            }
          }
        } // if not leaf at last level
      }
      else
      {
        // this->Input->GetDimension()==2
        // counter- clockwise direction matters here.

        int edges[2];
        int child=cursor->GetChildIndex();

        this->Polygon->GetPointIds()->SetNumberOfIds(0);
        this->Polygon->GetPoints()->SetNumberOfPoints(0);

        if(!lastLevelLeaf)
        {
          this->Sibling->ToSameNode(cursor);
          this->Sibling->ToParent();

          // list the 2 edges of the parent, the current node is laying on.
          edges[0]=(child&1)==1; // false: -x, true: +x
          edges[1]=(child&2)==2; // false: -y, true: +y
        }
        else
        {
          edges[0]=0;
          edges[1]=0;
        }

        // Insert vertex (xmin,ymin)
        pt[0]=bounds[0];
        pt[1]=bounds[2];
        this->Polygon->GetPointIds()->InsertNextId(
          this->Polygon->GetPointIds()->GetNumberOfIds());
        this->Polygon->GetPoints()->InsertNextPoint(pt);

        if(!lastLevelLeaf)
        {
          // Process edge (-y)
          if(edges[1])
          {
            // sibling
            this->Sibling->ToChild(child-2);
            if(!this->Sibling->CurrentIsLeaf())
            {
              this->Input->GetPointsOnEdge2D(this->Sibling,3,level,
                                             this->Grabber); // 3==+y
            }
            this->Sibling->ToParent();
          }
          else
          {
            // parent
            this->Input->GetPointsOnParentEdge2D(cursor,2,level,
                                                 this->Grabber); // 2==-y
          }
        }

        // Insert vertex (xmax,ymin)
        pt[0]=bounds[1];
        this->Polygon->GetPointIds()->InsertNextId(
          this->Polygon->GetPointIds()->GetNumberOfIds());
        this->Polygon->GetPoints()->InsertNextPoint(pt);

        if(!lastLevelLeaf)
        {
          // Process edge (+x)
          if(edges[0])
          {
            // parent
            this->Input->GetPointsOnParentEdge2D(cursor,1,level,
                                                 this->Grabber); // 1==+x
          }
          else
          {
            // sibling
            this->Sibling->ToChild(child+1);
            if(!this->Sibling->CurrentIsLeaf())
            {
              this->Input->GetPointsOnEdge2D(this->Sibling,0,level,
                                             this->Grabber); //0==-x
            }
            this->Sibling->ToParent();
          }
        }

        // Insert vertex (xmax,ymax)
        pt[1]=bounds[3];
        this->Polygon->GetPointIds()->InsertNextId(
          this->Polygon->GetPointIds()->GetNumberOfIds());
        this->Polygon->GetPoints()->InsertNextPoint(pt);

        if(!lastLevelLeaf)
        {
          // Process edge (+y)
          if(edges[1])
          {
            // parent
            this->Input->GetPointsOnParentEdge2D(cursor,3,level,
                                                 this->Grabber); // 3==+y
          }
          else
          {
            // sibling
            this->Sibling->ToChild(child+2);
            if(!this->Sibling->CurrentIsLeaf())
            {
              this->Input->GetPointsOnEdge2D(this->Sibling,2,level,
                                             this->Grabber); //2==-y
            }
            this->Sibling->ToParent();
          }
        }


        // Insert vertex (xmin,ymax)
        pt[0]=bounds[0];
        this->Polygon->GetPointIds()->InsertNextId(
          this->Polygon->GetPointIds()->GetNumberOfIds());
        this->Polygon->GetPoints()->InsertNextPoint(pt);

        if(!lastLevelLeaf)
        {
          // Process edge (-x)
          if(edges[0])
          {
            // sibling
            this->Sibling->ToChild(child-1);
            if(!this->Sibling->CurrentIsLeaf())
            {
              this->Input->GetPointsOnEdge2D(this->Sibling,1,level,
                                             this->Grabber); // 1==+x
            }
            this->Sibling->ToParent();
          }
          else
          {
            // parent
            this->Input->GetPointsOnParentEdge2D(cursor,0,level,
                                                 this->Grabber); // 0==-x
          }
        }
      }

      // Here, we have to cut the sub-tetras or polygon.
      // We have to evaluate the cutfunction on each inserted point
      // BEFORE calling Triangulate().


      if(this->Input->GetDimension()==3)
      {
        int c=this->Triangulator->GetNumberOfPoints();
        double *globalPt;

        this->CellScalars->SetNumberOfComponents(1);
        this->CellScalars->SetNumberOfTuples(c);

        i=0;
        while(i<c)
        {
          globalPt=this->Triangulator->GetPointLocation(i);
          s=this->CutFunction->FunctionValue(globalPt);
          this->CellScalars->InsertValue(i,s);
          ++i;
        }

        if(c==8)
        {
          // only the vertices of a voxel: fast path.
          this->Triangulator->UseTemplatesOn();
          this->Triangulator->TemplateTriangulate(VTK_VOXEL,8,12);
          ++this->TotalCounter;
          ++this->TemplateCounter;
        }
        else
        {
          // slow path
          this->Triangulator->UseTemplatesOff();
          this->Triangulator->Triangulate();
          ++this->TotalCounter;
          if(this->Triangulator->GetNumberOfPoints()<=65536)
          {
            this->CellTypeCounter[this->Triangulator->GetNumberOfPoints()-1]++;
          }
        }

        // perform cut

        vtkIdType cellId=cursor->GetLeafId();

        vtkPointData *inPD=this->Input->GetPointData(); // void

        if ( this->SortBy == VTK_SORT_BY_CELL )
        {
          double value = this->ContourValues->GetValue(this->Iter);
          this->Triangulator->InitTetraTraversal();
          int done=this->Triangulator->GetNextTetra(0,this->Tetra,
                                                    this->CellScalars,
                                                    this->TetScalars)==0;
          while(!done)
          {
            this->Tetra->Contour(value, this->TetScalars, this->Locator,
                                 this->NewVerts,this->NewLines,this->NewPolys,
                                 inPD,static_cast<vtkPointData *>(this->OutPD),
                                 static_cast<vtkCellData *>(this->InCD),cellId,
                                 static_cast<vtkCellData *>(this->OutCD));
            done=this->Triangulator->GetNextTetra(0,this->Tetra,
                                                  this->CellScalars,
                                                  this->TetScalars)==0;
          } //while
        }
        else
        {
          // VTK_SORT_BY_VALUE
          this->Triangulator->InitTetraTraversal();
          int done=this->Triangulator->GetNextTetra(0,this->Tetra,
                                                    this->CellScalars,
                                                    this->TetScalars)==0;
          while(!done)
          {
            int iter=0;
            while(iter<numContours)
            {
              double value = this->ContourValues->GetValue(iter);
              this->Tetra->Contour(value, this->TetScalars, this->Locator,
                                   this->NewVerts,this->NewLines,
                                   this->NewPolys,inPD,
                                   static_cast<vtkPointData *>(this->OutPD),
                                   static_cast<vtkCellData *>(this->InCD),
                                   cellId,
                                   static_cast<vtkCellData *>(this->OutCD));
              ++iter;
            }
            done=this->Triangulator->GetNextTetra(0,this->Tetra,
                                                  this->CellScalars,
                                                  this->TetScalars)==0;
          } //while
        }
      }
      else
      {
        // this->Input->GetDimension()==2
        int c=this->Polygon->GetPoints()->GetNumberOfPoints();
        double *globalPt;

        this->CellScalars->SetNumberOfComponents(1);
        this->CellScalars->SetNumberOfTuples(c);

        i=0;
        while(i<c)
        {
          globalPt=this->Polygon->GetPoints()->GetPoint(i);
          s=this->CutFunction->FunctionValue(globalPt);
          this->CellScalars->InsertValue(i,s);
          ++i;
        }

        // perform cut

        vtkIdType cellId=cursor->GetLeafId();

        vtkPointData *inPD=this->Input->GetPointData(); // void

        if(this->SortBy == VTK_SORT_BY_CELL)
        {
          double value = this->ContourValues->GetValue(this->Iter);
          this->Polygon->Contour(value, this->CellScalars, this->Locator,
                                 this->NewVerts,this->NewLines,this->NewPolys,
                                 inPD,static_cast<vtkPointData *>(this->OutPD),
                                 static_cast<vtkCellData *>(this->InCD),cellId,
                                 static_cast<vtkCellData *>(this->OutCD));
        }
        else
        {
          // VTK_SORT_BY_VALUE
          int iter=0;
          while(iter<numContours)
          {
            double value = this->ContourValues->GetValue(iter);
            this->Polygon->Contour(value, this->CellScalars, this->Locator,
                                   this->NewVerts,this->NewLines,
                                   this->NewPolys,inPD,
                                   static_cast<vtkPointData *>(this->OutPD),
                                   static_cast<vtkCellData *>(this->InCD),
                                   cellId,
                                   static_cast<vtkCellData *>(this->OutCD));

            ++iter;
          }
        }
      }
    }
  }
  else
  {
    // not a leaf
    // try to reject the node (and so reject all its sub-hierarchy)
    // to speed-up the process
    int cutChildren=1;

    // if all the corner points are outside, we are good for
    // rejection.
    vtkIdType numPts=0;

    double pt[3];
    switch(this->Input->GetDimension())
    {
      case 3:
        numPts=8;
        this->Pts->SetNumberOfPoints(numPts);
        pt[0]=bounds[0];
        pt[1]=bounds[2];
        pt[2]=bounds[4];
        this->Pts->SetPoint(0,pt);
        pt[0]=bounds[1];
        this->Pts->SetPoint(1,pt);
        pt[0]=bounds[0];
        pt[1]=bounds[3];
        this->Pts->SetPoint(2,pt);
        pt[0]=bounds[1];
        this->Pts->SetPoint(3,pt);
        pt[0]=bounds[0];
        pt[1]=bounds[2];
        pt[2]=bounds[5];
        this->Pts->SetPoint(4,pt);
        pt[0]=bounds[1];
        this->Pts->SetPoint(5,pt);
        pt[0]=bounds[0];
        pt[1]=bounds[3];
        this->Pts->SetPoint(6,pt);
        pt[0]=bounds[1];
        this->Pts->SetPoint(7,pt);
        break;
      case 2:
        numPts=4;
        this->Pts->SetNumberOfPoints(numPts);
        pt[0]=bounds[0];
        pt[1]=bounds[2];
        pt[2]=bounds[4];
        this->Pts->SetPoint(0,pt);
        pt[0]=bounds[1];
        this->Pts->SetPoint(1,pt);
        pt[0]=bounds[0];
        pt[1]=bounds[3];
        this->Pts->SetPoint(2,pt);
        pt[0]=bounds[1];
        this->Pts->SetPoint(3,pt);
        break;
      case 1:
        numPts=2;
        this->Pts->SetNumberOfPoints(numPts);
        pt[0]=bounds[0];
        pt[1]=bounds[2];
        pt[2]=bounds[4];
        this->Pts->SetPoint(0,pt);
        pt[0]=bounds[1];
        this->Pts->SetPoint(1,pt);
        break;
      default:
        assert("check: impossible" && 0);
        break;
    }


    int allLess=1; // bool
    int allGreater=1; // bool
    int numContours=0; // initialized to removed warnings

    if(this->SortBy == VTK_SORT_BY_VALUE)
    {
      numContours=this->ContourValues->GetNumberOfContours();
      int iter=0;
      while(iter<numContours)
      {
        this->AllLess[iter]=1;
        this->AllGreater[iter]=1;
        ++iter;
      }
    }

    cutChildren=0;
    int i=0;
    while(!cutChildren && i<numPts)
    {
      double s = this->CutFunction->FunctionValue(this->Pts->GetPoint(i));

      if(this->SortBy == VTK_SORT_BY_CELL)
      {
        double value = this->ContourValues->GetValue(this->Iter);
        if(s>value)
        {
          allLess=0;
        }
        else
        {
          if(s<value)
          {
            allGreater=0;
          }
        }
      }
      else
      {
        // VTK_SORT_BY_VALUE
        int iter=0;
        while(iter<numContours)
        {
          double value = this->ContourValues->GetValue(iter);
          if(s>value)
          {
            this->AllLess[iter]=0;
          }
          else
          {
            if(s<value)
            {
              this->AllGreater[iter]=0;
            }
          }
          ++iter;
        }
      }
      if(this->SortBy == VTK_SORT_BY_CELL)
      {
        cutChildren=!allLess && !allGreater;
      }
      else
      {
        int iter=0;
        cutChildren=0;
        while(iter<numContours)
        {
          cutChildren=cutChildren||(!this->AllLess[iter] &&
                                    !this->AllGreater[iter]);
          ++iter;
        }
      }
      ++i;
    }

    if(cutChildren)
    {
      double newBounds[6];

      double midx=(bounds[0]+bounds[1])*0.5;
      double midy=(bounds[2]+bounds[3])*0.5;
      double midz=(bounds[4]+bounds[5])*0.5;

      int kmax=2; // initialized to avoid compiler warnings
      int jmax=2;
      switch(this->Input->GetDimension())
      {
        case 3:
          kmax=2;
          jmax=2;
          break;
        case 2:
          kmax=1;
          jmax=2;
          break;
        case 1:
          kmax=1;
          jmax=1;
          break;
        default:
          assert("check: impossible case" && 0);
          break;
      }

      int k=0;
      while(k<kmax)
      {
        if(k==0)
        {
          newBounds[4]=bounds[4];
          newBounds[5]=midz;
        }
        else
        {
          newBounds[4]=midz;
          newBounds[5]=bounds[5];
        }
        int j=0;
        while(j<jmax)
        {
          if(j==0)
          {
            newBounds[2]=bounds[2];
            newBounds[3]=midy;
          }
          else
          {
            newBounds[2]=midy;
            newBounds[3]=bounds[3];
          }
          i=0;
          while(i<2)
          {
            int child=(((k<<1)+j)<<1)+i;
            cursor->ToChild(child);
            if(i==0)
            {
              newBounds[0]=bounds[0];
              newBounds[1]=midx;
            }
            else
            {
              newBounds[0]=midx;
              newBounds[1]=bounds[1];
            }
            this->CutNode(cursor,level+1,newBounds);
            cursor->ToParent();
            ++i;
          }
          ++j;
        }
        ++k;
      }
    }
  }
}

//----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By default,
// an instance of vtkMergePoints is used.
void vtkHyperOctreeCutter::SetLocator(vtkIncrementalPointLocator *locator)
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
void vtkHyperOctreeCutter::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
  {
    this->Locator = vtkMergePoints::New();
    this->Locator->Register(this);
    this->Locator->Delete();
  }
}

//----------------------------------------------------------------------------
int vtkHyperOctreeCutter::RequestUpdateExtent(
  vtkInformation *,
  vtkInformationVector **inputVector,
  vtkInformationVector *)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperOctreeCutter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperOctree");
  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperOctreeCutter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->CutFunction )
  {
    os << indent << "Cut Function: " << this->CutFunction << "\n";
  }
  else
  {
    os << indent << "Cut Function: (none)\n";
  }

  os << indent << "Sort By: " << this->GetSortByAsString() << "\n";

  if ( this->Locator )
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }

  this->ContourValues->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Generate Cut Scalars: "
     << (this->GenerateCutScalars ? "On\n" : "Off\n");
}
