/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClipHyperOctree.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkClipHyperOctree.h"

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
#include "vtkHyperOctreeClipCutPointsGrabber.h"
#include "vtkIncrementalPointLocator.h"

#include <cmath>
#include <cassert>

vtkStandardNewMacro(vtkClipHyperOctree);
vtkCxxSetObjectMacro(vtkClipHyperOctree,ClipFunction,vtkImplicitFunction);

//----------------------------------------------------------------------------
// Construct with user-specified implicit function; InsideOut turned off; value
// set to 0.0; and generate clip scalars turned off.
vtkClipHyperOctree::vtkClipHyperOctree(vtkImplicitFunction *cf)
{
  this->ClipFunction = cf;
  this->InsideOut = 0;
  this->Locator = NULL;
  this->Locator2=0;
  this->Value = 0.0;
  this->GenerateClipScalars = 0;

  this->GenerateClippedOutput = 0;

  this->SetNumberOfOutputPorts(2);
  vtkUnstructuredGrid *output2 = vtkUnstructuredGrid::New();
  this->GetExecutive()->SetOutputData(1, output2);
  output2->Delete();

  // by default process active point scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);

  this->Input=0;
  this->Output=0;
  this->ClippedOutput=0;
  this->Conn[0]=0;
  this->Conn[1]=0;
  this->Types[0]=0;
  this->Types[1]=0;
  this->Locs[0]=0;
  this->Locs[1]=0;
  this->InCD=0;
  this->OutCD[0]=0;
  this->OutCD[1]=0;
  this->OutPD[0]=0;
  this->OutPD[1]=0;
  this->Triangulator=0;
  this->Sibling=0;

  this->Tetra=0;
  this->Polygon=0;
  this->TetScalars=0;
  this->CellScalars=0;
  this->Pts=0;
  this->Grabber=0;
}

//----------------------------------------------------------------------------
vtkClipHyperOctree::~vtkClipHyperOctree()
{
  if ( this->Locator )
  {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
  }
  this->SetClipFunction(NULL);
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If Clip functions is modified,
// then this object is modified as well.
vtkMTimeType vtkClipHyperOctree::GetMTime()
{
  vtkMTimeType mTime=this->Superclass::GetMTime();
  vtkMTimeType time;

  if ( this->ClipFunction != NULL )
  {
    time = this->ClipFunction->GetMTime();
    mTime = ( time > mTime ? time : mTime );
  }
  if ( this->Locator != NULL )
  {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
  }

  return mTime;
}

vtkUnstructuredGrid *vtkClipHyperOctree::GetClippedOutput()
{
  if (!this->GenerateClippedOutput)
  {
    return NULL;
  }
  return vtkUnstructuredGrid::SafeDownCast(
    this->GetExecutive()->GetOutputData(1));
}

//----------------------------------------------------------------------------
//
// Clip through data generating surface.
//
int vtkClipHyperOctree::RequestData(vtkInformation *vtkNotUsed(request),
                                    vtkInformationVector **inputVector,
                                    vtkInformationVector *outputVector)
{
  if ( !this->ClipFunction)
  {
    vtkErrorMacro(<<"As HyperOctree does not support point data yet, a clip function has to be provided.");
    return 1;
  }

  if ( !this->ClipFunction && this->GenerateClipScalars )
  {
    vtkErrorMacro(<<"Cannot generate clip scalars if no clip function defined");
    return 1;
  }

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  this->Input = vtkHyperOctree::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  this->Output=vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  this->ClippedOutput = this->GetClippedOutput();

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
  this->Conn[0] = vtkCellArray::New();
  this->Conn[0]->Allocate(estimatedSize,estimatedSize/2);
  this->Conn[0]->InitTraversal();
  this->Types[0] = vtkUnsignedCharArray::New();
  this->Types[0]->Allocate(estimatedSize,estimatedSize/2);
  this->Locs[0] = vtkIdTypeArray::New();
  this->Locs[0]->Allocate(estimatedSize,estimatedSize/2);
  if ( this->GenerateClippedOutput )
  {
    // numOutputs = 2;
    this->Conn[1] = vtkCellArray::New();
    this->Conn[1]->Allocate(estimatedSize,estimatedSize/2);
    this->Conn[1]->InitTraversal();
    this->Types[1] = vtkUnsignedCharArray::New();
    this->Types[1]->Allocate(estimatedSize,estimatedSize/2);
    this->Locs[1] = vtkIdTypeArray::New();
    this->Locs[1]->Allocate(estimatedSize,estimatedSize/2);
  }

  vtkPoints *newPoints2=0;

  // locator used to merge potentially duplicate points
  if ( this->Locator == NULL )
  {
    this->CreateDefaultLocator();
  }

  if(this->GenerateClippedOutput)
  {
    this->Locator2=this->Locator->NewInstance();
    newPoints2 = vtkPoints::New();
    newPoints2->Allocate(numPts,numPts/2);
    this->Locator2->InitPointInsertion (newPoints2, this->Input->GetBounds());
  }

  this->Locator->InitPointInsertion (newPoints, this->Input->GetBounds());

  this->InCD=static_cast<vtkCellData *>(this->Input->GetLeafData());
  this->OutCD[0] = this->Output->GetCellData();
  this->OutCD[0]->CopyAllocate(this->InCD,estimatedSize,estimatedSize/2);
  if ( this->GenerateClippedOutput )
  {
    this->OutCD[1] = this->ClippedOutput->GetCellData();
    this->OutCD[1]->CopyAllocate(this->InCD,estimatedSize,estimatedSize/2);
  }

  this->OutPD[0]=this->Output->GetPointData();
  if ( !this->GenerateClipScalars &&
       !this->GetInputArrayToProcess(0,inputVector))
  {
    this->OutPD[0]->CopyScalarsOff();
  }
  else
  {
    this->OutPD[0]->CopyScalarsOn();
  }

  if(this->GenerateClippedOutput)
  {
    this->OutPD[1]=this->ClippedOutput->GetPointData();
    if ( !this->GenerateClipScalars &&
         !this->GetInputArrayToProcess(0,inputVector))
    {
      this->OutPD[1]->CopyScalarsOff();
    }
    else
    {
      this->OutPD[1]->CopyScalarsOn();
    }
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
  this->ClipNode(cursor,0,bounds);

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
      this->Triangulator=0;
      this->Grabber->UnRegister(this);
      this->Grabber=0;
      break;
    case 2:
      this->Polygon=0;
      this->Grabber->UnRegister(this);
      this->Grabber=0;
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

  this->OutPD[0]=0;
  this->Input=0;
  this->InCD=0;
  this->Output->SetPoints(newPoints);
  newPoints->Delete();
  this->Output->SetCells(this->Types[0], this->Locs[0], this->Conn[0]);
  this->Conn[0]->Delete();
  this->Conn[0]=0;
  this->Types[0]->Delete();
  this->Types[0]=0;
  this->Locs[0]->Delete();
  this->Locs[0]=0;
  this->OutCD[0]=0;

  if(this->GenerateClippedOutput)
  {
    this->ClippedOutput->SetPoints(newPoints2);
    this->ClippedOutput->SetCells(this->Types[1], this->Locs[1],this->Conn[1]);
    this->Conn[1]->Delete();
    this->Conn[1]=0;
    this->Types[1]->Delete();
    this->Types[1]=0;
    this->Locs[1]->Delete();
    this->Locs[1]=0;
    newPoints2->Delete();
    this->Locator2->Delete();
    this->Locator2=0;
    this->OutCD[1]=0;
    this->OutPD[1]=0;
  }

  this->Locator->Initialize();//release any extra memory
  this->Output->Squeeze();
  this->Output=0;
  if(this->GenerateClippedOutput)
  {
    this->ClippedOutput->Squeeze();
    this->ClippedOutput=0;
  }

  assert("post: input_is_null" && this->Input==0);
  assert("post: output_is_null" && this->Output==0);
  assert("post: clipped_output_is_null" && this->ClippedOutput==0);
  assert("post: locator2_is_null" && this->Locator2==0);
  assert("post: types_are_null" && this->Types[0]==0 && this->Types[1]==0);
  assert("post: conn_are_null" && this->Conn[0]==0 && this->Conn[1]==0);
  assert("post: locs_are_null" && this->Locs[0]==0 && this->Locs[1]==0);
  assert("post: incd_is_null" && this->InCD==0);
  assert("post: outpd_are_null" && this->OutPD[0]==0 && this->OutPD[1]==0);
  assert("post: outcd_are_null" && this->OutCD[0]==0 && this->OutCD[1]==0);

  return 1;
}

//----------------------------------------------------------------------------
void vtkClipHyperOctree::ClipNode(vtkHyperOctreeCursor *cursor,
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
      // just create a voxel/pixel/line and clip it.

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
      int coord;
      switch(this->Input->GetDimension())
      {
        case 3:
          v=vtkVoxel::New();
          cell=v;
          numPts=8;
          coord=0;
          while(coord<3)
          {
            pt[0]=bounds[coord*2];
            ++coord;
          }
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
          coord=0;
          while(coord<3)
          {
            pt[0]=bounds[coord*2];
            ++coord;
          }
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

      vtkDataArray *clipScalars=0;

      vtkPointData *inPD=this->Input->GetPointData();

      if(this->ClipFunction!=0)
      {
        vtkDoubleArray *tmpScalars = vtkDoubleArray::New();
        tmpScalars->SetNumberOfTuples(numPts);
        tmpScalars->SetName("ClipDataSetScalars");
        inPD = vtkPointData::New();
        inPD->ShallowCopy(this->Input->GetPointData());//copies original
        if(this->GenerateClipScalars)
        {
          inPD->SetScalars(tmpScalars);
        }
        for ( int i=0; i < numPts; i++ )
        {
          double s = this->ClipFunction->FunctionValue(cell->GetPoints()->GetPoint(i));
          tmpScalars->SetTuple1(i,s);
        }
        clipScalars = tmpScalars;
      }

#if 0 // just to not break compilation
      outPD->InterpolateAllocate(inPD,estimatedSize,estimatedSize/2);
#endif
      int i;
      for (i=0; i < numPts; i++ )
      {
        double s = clipScalars->GetComponent(i, 0);
        cellScalars->InsertTuple(i, &s);
      }

      if ( this->ClipFunction )
      {
        clipScalars->UnRegister(this);
        inPD->UnRegister(this);
      }

      // perform clipping
      int num[2]; num[0]=num[1]=0;
      int numNew[2]; numNew[0]=numNew[1]=0;

      cell->Clip(this->Value,cellScalars,this->Locator,this->Conn[0],inPD,
                 this->OutPD[0],this->InCD,cellId,this->OutCD[0],
                 this->InsideOut);
      numNew[0] = this->Conn[0]->GetNumberOfCells() - num[0];
      num[0] = this->Conn[0]->GetNumberOfCells();

      if(this->GenerateClippedOutput)
      {
        cell->Clip(this->Value, cellScalars, this->Locator2, this->Conn[1],
                   inPD, this->OutPD[1], this->InCD, cellId, this->OutCD[1],
                   !this->InsideOut);
        numNew[1] = this->Conn[1]->GetNumberOfCells() - num[1];
        num[1] = this->Conn[1]->GetNumberOfCells();
      }

      cellScalars->UnRegister(this);

      int numOutputs;
      if(this->GenerateClippedOutput)
      {
        numOutputs=2;
      }
      else
      {
        numOutputs=1;
      }

      vtkIdType npts=0;
      vtkIdType *pts;
      int cellType;

      for (i=0; i<numOutputs; i++) //for both outputs
      {
        for (int j=0; j < numNew[i]; j++)
        {
          this->Locs[i]->InsertNextValue(
            this->Conn[i]->GetTraversalLocation());
          this->Conn[i]->GetNextCell(npts,pts);

          //For each new cell added, got to set the type of the cell
          switch ( cell->GetCellDimension() )
          {
            case 1: //lines are generated---------------------------------
              cellType = (npts > 2 ? VTK_POLY_LINE : VTK_LINE);
              break;

            case 2: //polygons are generated------------------------------
              cellType = (npts == 3 ? VTK_TRIANGLE :
                          (npts == 4 ? VTK_QUAD : VTK_POLYGON));
              break;

            case 3: //tetrahedra or wedges are generated------------------
              cellType = (npts == 4 ? VTK_TETRA : VTK_WEDGE);
              break;
            default:
              assert("check: impossible case" && 0);
              cellType=0; // useless, only for removing warning about
              // unitialized function.
              break;
          } //switch

          this->Types[i]->InsertNextValue(cellType);
        } //for each new cell
      } //for both outputs
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

      int allOut=1; // bool
      int allIn=1; // bool
      int clipPoint; // bool
      double s;

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

              s=this->ClipFunction->FunctionValue(pt);
              if(this->InsideOut)
              {
                clipPoint=s<=this->Value; // keep point if true
              }
              else
              {
                clipPoint=s>=this->Value; // keep point if true
              }
              if(clipPoint)
              {
                allOut=0;
              }
              else
              {
                allIn=0;
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

            s=this->ClipFunction->FunctionValue(pt);
            if(this->InsideOut)
            {
              clipPoint=s<=this->Value; // keep point if true
            }
            else
            {
              clipPoint=s>=this->Value; // keep point if true
            }
            if(clipPoint)
            {
              allOut=0;
            }
            else
            {
              allIn=0;
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


      if(!this->GenerateClippedOutput && allOut)
      {
//        cout<<"this child is all out and we don't need to generate the other output"<<endl;
        return; // we've just save a lot of useless computation
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
              this->Input->GetPointsOnFace(this->Sibling,siblingFace,level,this->Grabber);
            }
            this->Sibling->ToParent();
            ++i;
            faceOffset+=2;
          }

          // Get points on faces shared with the parent node.
          this->Input->GetPointsOnParentFaces(faces,level,cursor,this->Grabber);

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
              this->Input->GetPointsOnEdge2D(this->Sibling,3,level,this->Grabber); // 3==+y
            }
            this->Sibling->ToParent();
          }
          else
          {
            // parent
            this->Input->GetPointsOnParentEdge2D(cursor,2,level,this->Grabber); // 2==-y
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
            this->Input->GetPointsOnParentEdge2D(cursor,1,level,this->Grabber); // 1==+x
          }
          else
          {
            // sibling
            this->Sibling->ToChild(child+1);
            if(!this->Sibling->CurrentIsLeaf())
            {
              this->Input->GetPointsOnEdge2D(this->Sibling,0,level,this->Grabber); //0==-x
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
            this->Input->GetPointsOnParentEdge2D(cursor,3,level,this->Grabber); // 3==+y
          }
          else
          {
            // sibling
            this->Sibling->ToChild(child+2);
            if(!this->Sibling->CurrentIsLeaf())
            {
              this->Input->GetPointsOnEdge2D(this->Sibling,2,level,this->Grabber); //2==-y
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
              this->Input->GetPointsOnEdge2D(this->Sibling,1,level,this->Grabber); // 1==+x
            }
            this->Sibling->ToParent();
          }
          else
          {
            // parent
            this->Input->GetPointsOnParentEdge2D(cursor,0,level,this->Grabber); // 0==-x
          }
        }
      }

      if(allIn || allOut)
      {
        vtkIdType cellId=cursor->GetLeafId();
        vtkPointData *inPD=this->Input->GetPointData(); // void

        // just pass the tetra or polygon to the output without clipping
        // TODO
        vtkIncrementalPointLocator *locator;
        if(allIn)
        {
          i=0;
          locator=this->Locator;
        }
        else // allOut, because here we know allIn || allOut is true.
        {
          i=1;
          locator=this->Locator2;
        }

        if(this->Input->GetDimension()==3)
        {
          if(this->Triangulator->GetNumberOfPoints()==8)
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

          vtkIdType numNew=this->Triangulator->AddTetras(0,locator,
                                                         this->Conn[i],inPD,
                                                         this->OutPD[i],
                                                         this->InCD,
                                                         cellId,
                                                         this->OutCD[i]);

          vtkIdType npts=0;
          vtkIdType *pts;
          int cellType;
          int numSimplices=0;
          for (j=0; j < numNew; j++)
          {
            ++numSimplices;
            this->Locs[i]->InsertNextValue(
              this->Conn[i]->GetTraversalLocation());
            this->Conn[i]->GetNextCell(npts,pts);

            //For each new cell added, got to set the type of the cell
            //tetrahedra or wedges are generated------------------
            cellType = (npts == 4 ? VTK_TETRA : VTK_WEDGE);

            this->Types[i]->InsertNextValue(cellType);
          } //for each new cell
          assert(numSimplices==numNew);
        }
        else
        {
          // this->Input->GetDimension()==2
          // Add the polygon

          // Insert the points
          vtkIdType c=this->Polygon->GetPoints()->GetNumberOfPoints();
          vtkIdType *pts=new vtkIdType[c];

          int p=0;
          while(p<c)
          {
            if(locator->InsertUniquePoint(
                 this->Polygon->GetPoints()->GetPoint(p),pts[p]))
            {
              this->OutPD[i]->CopyData(inPD,
                                       this->Polygon->GetPointIds()->GetId(p),
                                       pts[p]);
            }
            ++p;
          }

          // Insert the connectivity
          vtkIdType newCellId = this->Conn[i]->InsertNextCell(c,pts);
          this->OutCD[i]->CopyData(this->InCD,cellId,newCellId);
          delete[] pts;

          this->Locs[i]->InsertNextValue(
            this->Conn[i]->GetTraversalLocation());
          vtkIdType npts=0;
          this->Conn[i]->GetNextCell(npts,pts);
          int cellType = (npts == 3 ? VTK_TRIANGLE :
                          (npts == 4 ? VTK_QUAD : VTK_POLYGON));
          this->Types[i]->InsertNextValue(cellType);

        }
        return;
      }

      // Here, we have to clip the sub-tetras or polygon.
      // We have to evaluate the clipfunction on each inserted point
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
          s=this->ClipFunction->FunctionValue(globalPt);
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

        int done;

        int num[2];

        num[0]=this->Conn[0]->GetNumberOfCells();
        num[1]=0;
        if(this->GenerateClippedOutput)
        {
          num[1]=this->Conn[1]->GetNumberOfCells();
        }

        int numNew[2]; numNew[0]=numNew[1]=0;

        int numOutputs;
        if(this->GenerateClippedOutput)
        {
          numOutputs=2;
        }
        else
        {
          numOutputs=1;
        }

        vtkIdType npts=0;
        vtkIdType *pts;
        int cellType;

        vtkIdType cellId=cursor->GetLeafId();

        vtkPointData *inPD=this->Input->GetPointData(); // void

        this->Triangulator->InitTetraTraversal();
        int numTetras=0; // debug
        done=this->Triangulator->GetNextTetra(0,this->Tetra,this->CellScalars,
                                              this->TetScalars)==0;
        while(!done)
        {
          ++numTetras;
          this->Tetra->Clip(this->Value,this->TetScalars,this->Locator,
                            this->Conn[0],inPD,this->OutPD[0],this->InCD,
                            cellId,this->OutCD[0],this->InsideOut);

          numNew[0] = this->Conn[0]->GetNumberOfCells() - num[0];
          num[0] = this->Conn[0]->GetNumberOfCells();

          if(this->GenerateClippedOutput)
          {
            this->Tetra->Clip(this->Value, this->TetScalars, this->Locator2,
                              this->Conn[1],inPD, this->OutPD[1], this->InCD,
                              cellId, this->OutCD[1], !this->InsideOut);
            numNew[1] = this->Conn[1]->GetNumberOfCells() - num[1];
            num[1] = this->Conn[1]->GetNumberOfCells();
          }

          for (i=0; i<numOutputs; i++) //for both outputs
          {
            for (j=0; j < numNew[i]; j++)
            {
              this->Locs[i]->InsertNextValue(
                this->Conn[i]->GetTraversalLocation());
              this->Conn[i]->GetNextCell(npts,pts);

              //tetrahedra or wedges are generated------------------
              cellType = (npts == 4 ? VTK_TETRA : VTK_WEDGE);

              this->Types[i]->InsertNextValue(cellType);
            } //for each new cell
          } //for both outputs

          done=this->Triangulator->GetNextTetra(0,this->Tetra,
                                                this->CellScalars,
                                                this->TetScalars)==0;
        } //while
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
          s=this->ClipFunction->FunctionValue(globalPt);
          this->CellScalars->InsertValue(i,s);
          ++i;
        }

        int num[2];

        num[0]=this->Conn[0]->GetNumberOfCells();
        num[1]=0;
        if(this->GenerateClippedOutput)
        {
          num[1]=this->Conn[1]->GetNumberOfCells();
        }

        int numNew[2]; numNew[0]=numNew[1]=0;

        int numOutputs;
        if(this->GenerateClippedOutput)
        {
          numOutputs=2;
        }
        else
        {
          numOutputs=1;
        }

        vtkIdType npts=0;
        vtkIdType *pts;
        int cellType;

        vtkIdType cellId=cursor->GetLeafId();

        vtkPointData *inPD=this->Input->GetPointData(); // void

        this->Polygon->Clip(this->Value,this->CellScalars,this->Locator,
                            this->Conn[0],inPD,this->OutPD[0],this->InCD,
                            cellId,this->OutCD[0],this->InsideOut);

        numNew[0] = this->Conn[0]->GetNumberOfCells() - num[0];
        num[0] = this->Conn[0]->GetNumberOfCells();

        if(this->GenerateClippedOutput)
        {
          this->Polygon->Clip(this->Value, this->CellScalars, this->Locator2,
                              this->Conn[1],inPD, this->OutPD[1], this->InCD,
                              cellId, this->OutCD[1], !this->InsideOut);
          numNew[1] = this->Conn[1]->GetNumberOfCells() - num[1];
          num[1] = this->Conn[1]->GetNumberOfCells();
        }

        for (i=0; i<numOutputs; i++) //for both outputs
        {
          for (j=0; j < numNew[i]; j++)
          {
            this->Locs[i]->InsertNextValue(
              this->Conn[i]->GetTraversalLocation());
            this->Conn[i]->GetNextCell(npts,pts);

            //polygons are generated------------------------------
            cellType = (npts == 3 ? VTK_TRIANGLE :
                        (npts == 4 ? VTK_QUAD : VTK_POLYGON));
            this->Types[i]->InsertNextValue(cellType);
          } //for each new cell
        } //for both outputs
      }
    }
  }
  else
  {
    // not a leaf
    // try to reject the node (and so reject all its sub-hierarchy)
    // to speed-up the process
    int clipChildren=1;
    if(!this->GenerateClippedOutput)
    {
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

      clipChildren=0;
      int i=0;
      while(!clipChildren && i<numPts)
      {
        double s = this->ClipFunction->FunctionValue(this->Pts->GetPoint(i));
        if(this->InsideOut)
        {
          clipChildren=s<=this->Value; // keep point if true
        }
        else
        {
          clipChildren=s>=this->Value; // keep point if true
        }
        ++i;
      }
    }
    if(clipChildren)
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
          int i=0;
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
            this->ClipNode(cursor,level+1,newBounds);
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
void vtkClipHyperOctree::SetLocator(vtkIncrementalPointLocator *locator)
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
void vtkClipHyperOctree::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
  {
    this->Locator = vtkMergePoints::New();
    this->Locator->Register(this);
    this->Locator->Delete();
  }
}

//----------------------------------------------------------------------------
int vtkClipHyperOctree::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperOctree");
  return 1;
}

//----------------------------------------------------------------------------
void vtkClipHyperOctree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->ClipFunction )
  {
    os << indent << "Clip Function: " << this->ClipFunction << "\n";
  }
  else
  {
    os << indent << "Clip Function: (none)\n";
  }
  os << indent << "InsideOut: " << (this->InsideOut ? "On\n" : "Off\n");
  os << indent << "Value: " << this->Value << "\n";
  if ( this->Locator )
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }

  os << indent << "Generate Clip Scalars: "
     << (this->GenerateClipScalars ? "On\n" : "Off\n");

  os << indent << "Generate Clipped Output: "
     << (this->GenerateClippedOutput ? "On\n" : "Off\n");
}
