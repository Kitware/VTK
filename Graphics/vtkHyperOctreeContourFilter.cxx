/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctreeContourFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperOctreeContourFilter.h"

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
#include <math.h>
#include <assert.h>
#include <set>
#include "vtkBitArray.h"
#include "vtkIncrementalPointLocator.h"

#include "vtkHyperOctreePointsGrabber.h"


//----------------------------------------------------------------------------
void vtkHyperOctreeContourFilter::PrintSelf(ostream& os, vtkIndent indent)
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

class vtkHyperOctreeContourPointsGrabber; 

class vtkHyperOctreeContourPointsGrabber : public vtkHyperOctreePointsGrabber
{
public:
  static vtkHyperOctreeContourPointsGrabber *New();
  
  vtkTypeMacro(vtkHyperOctreeContourPointsGrabber,vtkHyperOctreePointsGrabber);
  
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set the dimension of the hyperoctree.
  // \pre valid_dim: (dim==2 || dim==3)
  // \post is_set: GetDimension()==dim
  virtual void SetDimension(int dim);
  
  // Description:
  // Initialize the points insertion scheme.
  // Actually, it is just a trick to initialize the IdSet from the filter.
  // The IdSet class cannot be shared with the filter because it is a Pimpl.
  // It is used by clip,cut and contour filters to build the points
  // that lie on an hyperoctant.
  // \pre only_in_3d: GetDimension()==3
  virtual void InitPointInsertion();
  
  // Description:
  // Insert a point, assuming the point is unique and does not require a
  // locator. Tt does not mean it does not use a locator. It just mean that
  // some implementation may skip the use of a locator.
  virtual void InsertPoint(vtkIdType ptId,
                           double pt[3],
                           double pcoords[3],
                           int ijk[3]);
  
  // Description:
  // Insert a point using a locator.
  virtual void InsertPointWithMerge(vtkIdType ptId,
                                    double pt[3],
                                    double pcoords[3],
                                    int ijk[3]);
  
  // Description:
  // Insert a point in the quadtree case.
  virtual void InsertPoint2D(double pt[3],
                             int ijk[3]);
  
  // Description:
  // Return the ordered triangulator.
  vtkOrderedTriangulator *GetTriangulator();
  
  // Description:
  // Return the polygon.
  vtkPolygon *GetPolygon();
  
  // Description:
  // Init the locator.
  void InitLocator(vtkPoints *pts,
                   double bounds[6]);
  
  // Description:
  // Return the id in the locator after a call to InsertPoint*().
  vtkIdType GetLastPtId();
  
  void SetFilter(vtkHyperOctreeContourFilter *filter);
  
protected:
  // Constructor with default bounds (0,1, 0,1, 0,1).
  vtkHyperOctreeContourPointsGrabber();
  ~vtkHyperOctreeContourPointsGrabber();
  
  vtkHyperOctreeContourFilter *Filter;
  vtkOrderedTriangulator *Triangulator;
  vtkPolygon *Polygon;
  vtkMergePoints *Locator;
  vtkIdType LastPtId;
  vtkHyperOctreeIdSet *IdSet;
private:
  vtkHyperOctreeContourPointsGrabber(const vtkHyperOctreeContourPointsGrabber&);  // Not implemented.
  void operator=(const vtkHyperOctreeContourPointsGrabber&);    // Not implemented.
};
  
vtkStandardNewMacro(vtkHyperOctreeContourFilter);

//----------------------------------------------------------------------------
// Construct with user-specified implicit function; value
// set to 0.0; and generate cut scalars turned off.
vtkHyperOctreeContourFilter::vtkHyperOctreeContourFilter()
{
  this->ContourValues = vtkContourValues::New();
  
  this->Locator = NULL;
  
  this->SetNumberOfOutputPorts(1);

  // by default process active cell scalars
  // This is points because octree returns dual grid.
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
  
  this->Tetra=0;
  this->TetScalars=0;
  this->PointScalars=0;
  this->CellScalars=0;
  
  this->Cursor=0;
  this->NeighborCursor=0;
  this->Sibling=0;
  
  this->InScalars=0;
  this->Grabber=0;
  this->Polygon=0;
  this->SortBy=VTK_SORT_BY_VALUE;
  
  this->Line=0;
}

//----------------------------------------------------------------------------
vtkHyperOctreeContourFilter::~vtkHyperOctreeContourFilter()
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
unsigned long vtkHyperOctreeContourFilter::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long contourValuesMTime=this->ContourValues->GetMTime();
  unsigned long time;
  
  mTime = ( contourValuesMTime > mTime ? contourValuesMTime : mTime );
  
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
int vtkHyperOctreeContourFilter::RequestData(
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
  if(this->Locator == NULL)
    {
    this->CreateDefaultLocator();
    }
  
  this->Locator->InitPointInsertion (newPoints, this->Input->GetBounds());
 
  this->InCD=static_cast<vtkCellData*>(this->Input->GetLeafData());
  // Scalars are added to this, so we need to make a copy.
  this->InPD = vtkPointData::New();
  // Since the dataset API returns the dual, cell and point data are switched.
  this->InPD->ShallowCopy(this->Input->GetCellData());
  
  this->OutCD=this->Output->GetCellData();
  this->OutCD->CopyAllocate(this->InCD,estimatedSize,estimatedSize/2);
    
  this->OutPD=this->Output->GetPointData();
  this->OutPD->CopyAllocate(this->Input->GetPointData(),estimatedSize,estimatedSize/2);
  this->OutPD->CopyScalarsOn();
  
  static double bounds[6]={0,1,0,1,0,1};
  
  vtkPoints *originalPoints=0;
  
  switch(this->Input->GetDimension())
    {
    case 3:
      this->Tetra=vtkTetra::New();
      this->TetScalars=vtkDoubleArray::New();
      this->TetScalars->SetNumberOfComponents(1);
      this->TetScalars->SetNumberOfTuples(4);
      this->Grabber=vtkHyperOctreeContourPointsGrabber::New();
      this->Grabber->SetFilter(this);
      this->Grabber->SetDimension(3);
      originalPoints=vtkPoints::New();
      originalPoints->Allocate(numPts,numPts/2);
      this->Grabber->InitLocator(originalPoints,bounds);
      this->Triangulator=this->Grabber->GetTriangulator();
      break;
    case 2:
      this->Grabber=vtkHyperOctreeContourPointsGrabber::New();
      this->Grabber->SetFilter(this);
      this->Grabber->SetDimension(2);
      originalPoints=vtkPoints::New();
      originalPoints->Allocate(numPts,numPts/2);
      
      this->Grabber->InitLocator(originalPoints,this->Input->GetBounds());
      this->Polygon=this->Grabber->GetPolygon();
      break;
    case 1:
      this->Line=vtkLine::New();
      break;
    default:
      assert("check: impossible case" && 0); // do nothing
      break;
    }
  this->PointScalars=vtkDoubleArray::New();
  this->PointScalars->SetName(this->InScalars->GetName());
  this->PointScalars->Allocate(estimatedSize);
  
  this->TotalCounter=0;
  this->TemplateCounter=0;
  
  int j=0;
  while(j<65536)
    {
    this->CellTypeCounter[j]=0; // up-to-65536 points per octant
    ++j;
    }
  
  this->Cursor=this->Input->NewCellCursor();
  this->NeighborCursor=this->Input->NewCellCursor();
  this->Sibling=this->Input->NewCellCursor();
  
  this->PointScalars->SetNumberOfComponents(1);
  this->PointScalars->Allocate(estimatedSize);
  this->Cursor->ToRoot();
  
  
  this->CellScalars=vtkDoubleArray::New();
  
  // let's go
  if(this->Input->GetDimension()==1)
    {
    // got the first leaf on the left side.
    this->Cursor->ToRoot();
    while(!this->Cursor->CurrentIsLeaf())
      {
      this->Cursor->ToChild(0);
      }
    this->LeftValue=this->InScalars->GetTuple1(this->Cursor->GetLeafId());
    this->LeftCoord=this->Input->GetOrigin()[0];
    this->Cursor->ToRoot();
    this->ContourNode1D();
    }
  else
    {
    this->ContourNode();
    }
  
  this->CellScalars->UnRegister(this);
  this->CellScalars=0;
  
  this->PointScalars->UnRegister(this);
  this->PointScalars=0;
  
  this->InCD=0;
  
  this->Cursor->UnRegister(this);
  this->Cursor=0;
  this->NeighborCursor->UnRegister(this);
  this->NeighborCursor=0;
  this->Sibling->UnRegister(this);
  this->Sibling=0;
 
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
      originalPoints->UnRegister(this);
      break;
    case 2:
      this->Polygon=0;
      this->Grabber->UnRegister(this);
      this->Grabber=0;
      originalPoints->UnRegister(this);
      break;
    case 1:
      this->Line->UnRegister(this);
      this->Line=0;
      break;
    default:
      assert("check: impossible case" && 0);
      break;
    }
  
  this->OutPD=0;
  this->Input=0;
  
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
  this->InPD->Delete();
  this->InPD = 0;
  
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
void vtkHyperOctreeContourFilter::ContourNode1D()
{
  if(!this->Cursor->CurrentIsLeaf())
    {
    int child=0;
    while(child<2)
      {
      this->Cursor->ToChild(child);
      this->ContourNode1D();
      this->Cursor->ToParent();
      ++child;
      }
    }
  else
    {
    vtkIdType cellId=this->Cursor->GetLeafId();
    double cellValue=this->InScalars->GetTuple1(cellId);
    int level=this->Cursor->GetCurrentLevel();
    
    int target[3];
    target[1]=0;
    target[2]=0;
    target[0]=this->Cursor->GetIndex(0);
    double rightValue;
  
    if(target[0]>=(1<<(level-1)))
      {
      rightValue=cellValue;
      }
    else
      {
      this->NeighborCursor->MoveToNode(target,level);
      rightValue=(cellValue+this->InScalars->GetTuple1(this->NeighborCursor->GetLeafId()))*0.5;
      }
    
    // build a line and contour it.
    double pt[3];
    pt[1]=this->Input->GetOrigin()[1];
    pt[2]=this->Input->GetOrigin()[2];
    // build the point
    pt[0]=this->LeftCoord;
    this->Line->GetPoints()->SetPoint(0,pt);
    pt[0]+=this->Input->GetSize()[0]/(1<<level);
    this->LeftCoord=pt[0]; // for the next step
    this->Line->GetPoints()->SetPoint(1,pt);
    
    this->CellScalars->SetNumberOfComponents(1);
    this->CellScalars->SetNumberOfTuples(2);
    
    this->CellScalars->SetValue(0,this->LeftValue);
    this->CellScalars->SetValue(1,rightValue);
      
    // It was a bad idea to modify the input PointData.
    // We made a copy.
    //vtkPointData *inPD=this->Input->GetPointData(); // void
    this->InPD->SetScalars(this->PointScalars); // void
    
    if ( this->SortBy == VTK_SORT_BY_CELL )
      {
      double value = this->ContourValues->GetValue(this->Iter);
      this->Line->Contour(value, this->CellScalars, this->Locator, 
                          this->NewVerts,this->NewLines,this->NewPolys,
                          this->InPD,this->OutPD,this->InCD,cellId,
                          this->OutCD);
      }
    else
      {
      // VTK_SORT_BY_VALUE
      int iter=0;
      int numContours=this->ContourValues->GetNumberOfContours();
      while(iter<numContours)
        {
        double value = this->ContourValues->GetValue(iter);
        this->Line->Contour(value, this->CellScalars, this->Locator, 
                            this->NewVerts,this->NewLines,
                            this->NewPolys,this->InPD,this->OutPD,
                            this->InCD,cellId,this->OutCD);
          
          ++iter;
          }
        }
    
    
    // initialize the left value for the next leaf.
    this->LeftValue=rightValue;
    }
}

//----------------------------------------------------------------------------
// Description:
// Do the recursive contour of the node pointed by Cursor.
void vtkHyperOctreeContourFilter::ContourNode()
{
  if(!this->Cursor->CurrentIsLeaf())
    {
    int child=0;
    int c=this->Cursor->GetNumberOfChildren();
    while(child<c)
      {
      this->Cursor->ToChild(child);
      this->ContourNode();
      this->Cursor->ToParent();
      ++child;
      }
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
    int i;
    int j;
    int k;
    
    int x;
    int y;
    int z;
    
    // resolution in point along each axis.
    int resolution=(1<<(this->Input->GetNumberOfLevels()-1))+1;

    int level=this->Cursor->GetCurrentLevel();
    int deltaLevel=this->Input->GetNumberOfLevels()-1-level;
    assert("check: positive_deltaLevel" && deltaLevel>=0);
    
    double ratio=1.0/(resolution-1);
    
    double pt[3];
    double pcoords[3];
    int pijk[3];
    int qijk[3];
    
    int numContours=0; // initialized to removed warnings
    
    if ( this->SortBy == VTK_SORT_BY_VALUE )
      {
      numContours=this->ContourValues->GetNumberOfContours();
      }
    // index of the node
    if(this->Input->GetDimension()==3)
      {
      vtkIdType nbpts=this->Input->GetMaxNumberOfPointsOnBoundary(level);
      double pbounds[6]={0,1,0,1,0,1};
      double *size=this->Input->GetSize();
      double *origin=this->Input->GetOrigin();
      
      this->Triangulator->InitTriangulation(pbounds,nbpts);
      
      this->Triangulator->PreSortedOff();
      this->Grabber->InitPointInsertion();
      
      i=this->Cursor->GetIndex(0);
      j=this->Cursor->GetIndex(1);
      k=this->Cursor->GetIndex(2);
      z=0;
      pijk[2]=k;
      while(z<2)
        {
        pijk[1]=j;
        y=0;
        while(y<2)
          {
          pijk[0]=i;
          x=0;
          while(x<2)
            {
            // Get some parametric coords in [0,1]
            // [0,1] covers the whole dataset axis.
            int coord=0;
            while(coord<3)
              {
              qijk[coord]=pijk[coord]<<deltaLevel;
              pcoords[coord]=qijk[coord]*ratio;
              pt[coord]=pcoords[coord]*size[coord]+origin[coord];
              ++coord;
              }
            
            assert("check: in_bounds" && pt[0]>=this->Input->GetBounds()[0] && pt[0]<=this->Input->GetBounds()[1] && pt[1]>=this->Input->GetBounds()[2] && pt[1]<=this->Input->GetBounds()[3] && pt[2]>=this->Input->GetBounds()[4] && pt[2]<=this->Input->GetBounds()[5]);
            
            this->Grabber->InsertPoint(0,pt,pcoords,qijk);
            ++x;
            ++pijk[0];
            }
          ++y;
          ++pijk[1];
          }
        ++z;
        ++pijk[2];
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
        
        // list the 3 faces of the parent, the current node is lying on.
        int faces[3];
        
        int child=this->Cursor->GetChildIndex();
        
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
        
        this->Sibling->ToSameNode(this->Cursor);
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
        this->Input->GetPointsOnParentFaces(faces,level,this->Cursor,
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
        
        this->Sibling->ToSameNode(this->Cursor);
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
                this->Input->GetPointsOnParentEdge(this->Cursor,level,axis,k,j,
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
      int child=this->Cursor->GetChildIndex();
      double *size=this->Input->GetSize();
      double *origin=this->Input->GetOrigin();
      
      this->Polygon->GetPointIds()->SetNumberOfIds(0);
      this->Polygon->GetPoints()->SetNumberOfPoints(0);
      
      if(!lastLevelLeaf)
        {
        this->Sibling->ToSameNode(this->Cursor);
        this->Sibling->ToParent();
        
        // list the 2 edges of the parent, the current node is lying on.
        edges[0]=(child&1)==1; // false: -x, true: +x
        edges[1]=(child&2)==2; // false: -y, true: +y
        }
      else
        {
        edges[0]=0;
        edges[1]=0;
        }
      
      i=this->Cursor->GetIndex(0);
      j=this->Cursor->GetIndex(1);
    
      
      // Insert vertex (xmin,ymin)
      pijk[0]=i;
      pijk[1]=j;
      
      qijk[0]=pijk[0]<<deltaLevel;
      qijk[1]=pijk[1]<<deltaLevel;
      
      pcoords[0]=qijk[0]*ratio;
      pcoords[1]=qijk[1]*ratio;
      
      pt[0]=pcoords[0]*size[0]+origin[0];
      pt[1]=pcoords[1]*size[1]+origin[1];
      pt[2]=origin[2];
      
      this->Grabber->InsertPoint2D(pt,qijk);
      
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
          this->Input->GetPointsOnParentEdge2D(this->Cursor,2,level,
                                               this->Grabber); // 2==-y
          }
        }
      
      // Insert vertex (xmax,ymin)
      pijk[0]=i+1;
      qijk[0]=pijk[0]<<deltaLevel;
      pcoords[0]=qijk[0]*ratio;
      pt[0]=pcoords[0]*size[0]+origin[0];
      
      this->Grabber->InsertPoint2D(pt,qijk);
      
      if(!lastLevelLeaf)
        {
        // Process edge (+x)
        if(edges[0])
          {
          // parent
          this->Input->GetPointsOnParentEdge2D(this->Cursor,1,level,
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
      pijk[1]=j+1;
      qijk[1]=pijk[1]<<deltaLevel;
      pcoords[1]=qijk[1]*ratio;
      pt[1]=pcoords[1]*size[1]+origin[1];
      this->Grabber->InsertPoint2D(pt,qijk);
    
      if(!lastLevelLeaf)
        {
        // Process edge (+y)
        if(edges[1])
          {
          // parent
          this->Input->GetPointsOnParentEdge2D(this->Cursor,3,level,
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
      pijk[0]=i;
      qijk[0]=pijk[0]<<deltaLevel;
      pcoords[0]=qijk[0]*ratio;
      pt[0]=pcoords[0]*size[0]+origin[0];
      
      this->Grabber->InsertPoint2D(pt,qijk);
      
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
          this->Input->GetPointsOnParentEdge2D(this->Cursor,0,level,
                                               this->Grabber); // 0==-x
          }
        }
      }
    
    if(this->Input->GetDimension()==3)
      {     
      int c=this->Triangulator->GetNumberOfPoints();
      
      
      this->CellScalars->SetNumberOfComponents(1);
      this->CellScalars->SetNumberOfTuples(c);
      
      
      // very important: we have to build the scalars value on the cell
      // BEFORE a call to Triangulate().
      i=0;
      while(i<c)
        {
        vtkIdType ptId=this->Triangulator->GetPointId(i);
        this->CellScalars->InsertValue(i,this->PointScalars->GetValue(ptId));
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
      
      // perform contour
      
      vtkIdType cellId=this->Cursor->GetLeafId();
           
      // I made a copy of the input point data so it is ok modify.
      this->InPD->SetScalars(this->PointScalars); // otherwise, it crashes
       
      if ( this->SortBy == VTK_SORT_BY_CELL )
        {
        double value = this->ContourValues->GetValue(this->Iter);
        this->Triangulator->InitTetraTraversal();
        int numTetras=0; // debug
        int done=this->Triangulator->GetNextTetra(0,this->Tetra,
                                                  this->CellScalars,
                                                  this->TetScalars)==0;
        while(!done)
          {
          ++numTetras;
          this->Tetra->Contour(value, this->TetScalars, this->Locator, 
                               this->NewVerts,this->NewLines,this->NewPolys,
                               this->InPD,this->OutPD,this->InCD,cellId,
                               this->OutCD);
          done=this->Triangulator->GetNextTetra(0,this->Tetra,
                                                this->CellScalars,
                                                this->TetScalars)==0;
          } //while
        }
      else
        {
        // VTK_SORT_BY_VALUE
        this->Triangulator->InitTetraTraversal();
        int numTetras=0; // debug
        int done=this->Triangulator->GetNextTetra(0,this->Tetra,
                                                  this->CellScalars,
                                                  this->TetScalars)==0;
        while(!done)
          {
          ++numTetras;
          int iter=0;
          while(iter<numContours)
            {
            double value = this->ContourValues->GetValue(iter);
            this->Tetra->Contour(value, this->TetScalars, this->Locator, 
                                 this->NewVerts,this->NewLines,
                                 this->NewPolys,this->InPD,this->OutPD,
                                 this->InCD,cellId,this->OutCD);
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
     
      // perform contour
      
      vtkIdType cellId=this->Cursor->GetLeafId();
      
      int c=this->Polygon->GetPoints()->GetNumberOfPoints();
     
      this->CellScalars->SetNumberOfComponents(1);
      this->CellScalars->SetNumberOfTuples(c);
      
      i=0;
      while(i<c)
        {
        vtkIdType ptId=this->Polygon->GetPointId(i);
        this->CellScalars->SetValue(i,this->PointScalars->GetValue(ptId));
        ++i;
        }
      
      // I made a copy of the input point data, so it is OK to modify.
      this->InPD->SetScalars(this->PointScalars); // void
      
      if ( this->SortBy == VTK_SORT_BY_CELL )
        {
        double value = this->ContourValues->GetValue(this->Iter);
        this->Polygon->Contour(value, this->CellScalars, this->Locator, 
                               this->NewVerts,this->NewLines,this->NewPolys,
                               this->InPD,this->OutPD,this->InCD,cellId,
                               this->OutCD);
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
                                 this->NewPolys,this->InPD,this->OutPD,
                                 this->InCD,cellId,this->OutCD);
          
          ++iter;
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
// Description:
// (i,j,k) are point coordinates at last level.
// \pre all_set: this->Input->GetDimension()==2 implies ptIndices[2]==0
double vtkHyperOctreeContourFilter::ComputePointValue(int ptIndices[3])
{
  // this->Input->GetDimension()==2 implies ptIndices[2]==0
  // A implies B: !A||B
  assert("pre: all_set" &&
         (this->Input->GetDimension()!=2 || ptIndices[2]==0));
  
  int target[3];
  double result=0;
  double nb=0;
  
  int kmax;
  int jmax;
  int level=this->Input->GetNumberOfLevels();
  int maxCellIdx=1<<(level-1);
  
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
    default:
      kmax=1;
      jmax=1;
      break;
    }
  
  int k=0;
  while(k<kmax)
    {
    target[2]=ptIndices[2]-k;
    int j=0;
    while(j<jmax)
      {
      target[1]=ptIndices[1]-j;
      int i=0;
      while(i<2)
        {
        target[0]=ptIndices[0]-i;
        if(target[0]>=0 && target[0]<maxCellIdx && target[1]>=0 && target[1]<maxCellIdx &&target[2]>=0 && target[2]<maxCellIdx )
          {
          this->NeighborCursor->MoveToNode(target,level-1);
          result+=this->InScalars->GetTuple1(this->NeighborCursor->GetLeafId());
          ++nb;
          }
        ++i;
        }
      ++j;
      }
    ++k;
    }
  
  if(nb>1)
    {
    result/=nb;
    }
  return result;
}

//----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By default, 
// an instance of vtkMergePoints is used.
void vtkHyperOctreeContourFilter::SetLocator(vtkIncrementalPointLocator *locator)
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
void vtkHyperOctreeContourFilter::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
    {
    this->Locator = vtkMergePoints::New();
    this->Locator->Register(this);
    this->Locator->Delete();
    }
}

//----------------------------------------------------------------------------
int vtkHyperOctreeContourFilter::RequestUpdateExtent(
  vtkInformation *,
  vtkInformationVector **inputVector,
  vtkInformationVector *)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperOctreeContourFilter::FillInputPortInformation(int,
                                                          vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperOctree");
  return 1;
}


vtkStandardNewMacro(vtkHyperOctreeContourPointsGrabber);

//-----------------------------------------------------------------------------
// Description:
// Default constructor.
vtkHyperOctreeContourPointsGrabber::vtkHyperOctreeContourPointsGrabber()
{
  this->Triangulator=vtkOrderedTriangulator::New();
  this->Locator=vtkMergePoints::New();
  this->Polygon=0;
  this->Dimension=3;
  this->IdSet=new vtkHyperOctreeIdSet;
  this->Filter=0;
}

//-----------------------------------------------------------------------------
// Description:
// Destructor.
vtkHyperOctreeContourPointsGrabber::~vtkHyperOctreeContourPointsGrabber()
{
  if(this->Triangulator!=0)
    {
    this->Triangulator->UnRegister(this);
    delete this->IdSet;
    }
  if(this->Polygon!=0)
    {
    this->Polygon->UnRegister(this);
    }
  this->Locator->UnRegister(this);
}

//-----------------------------------------------------------------------------
// Description:
// Set the dimension of the hyperoctree.
// \pre valid_dim: (dim==2 || dim==3)
// \post is_set: GetDimension()==dim
void vtkHyperOctreeContourPointsGrabber::SetDimension(int dim)
{
  assert("pre: valid_dim" && (dim==2 || dim==3));
  if(dim!=this->Dimension)
    {
    if(dim==3)
      {
      this->Polygon->UnRegister(this);
      this->Polygon=0;
      this->Triangulator=vtkOrderedTriangulator::New();
      this->IdSet=new vtkHyperOctreeIdSet;
      }
    else
      {
       this->Triangulator->UnRegister(this);
       this->Triangulator=0;
       delete this->IdSet;
       this->Polygon=vtkPolygon::New();
      }
    this->Dimension=dim;
    }
  assert("post: is_set" && GetDimension()==dim);
}

//-----------------------------------------------------------------------------
// Description:
// Initialize the points insertion scheme.
// Actually, it is just a trick to initialize the IdSet from the filter.
// The IdSet class cannot be shared with the filter because it is a Pimpl.
// It is used by clip,cut and contour filters to build the points
// that lie on an hyperoctant.
// \pre only_in_3d: GetDimension()==3
void vtkHyperOctreeContourPointsGrabber::InitPointInsertion()
{
  assert("pre: only_in_3d" && this->GetDimension()==3);
  this->IdSet->Set.clear();
}

//-----------------------------------------------------------------------------
// Description:
// Insert a point, assuming the point is unique and does not require a
// locator. Tt does not mean it does not use a locator. It just mean that
// some implementation may skip the use of a locator.
void vtkHyperOctreeContourPointsGrabber::InsertPoint(vtkIdType vtkNotUsed(ptId),
                                                     double pt[3],
                                                     double pcoords[3],
                                                     int ijk[3])
{
  if(this->Locator->InsertUniquePoint(pcoords,this->LastPtId))
    {
    double value=this->Filter->ComputePointValue(ijk);
    this->Filter->PointScalars->InsertValue(this->LastPtId,value);
    }
  this->Triangulator->InsertPoint(this->LastPtId,pt,pcoords,0);
}


//-----------------------------------------------------------------------------
// Description:
// Insert a point using a locator.
void vtkHyperOctreeContourPointsGrabber::InsertPointWithMerge(
  vtkIdType vtkNotUsed(ptId),
  double pt[3],
  double pcoords[3],
  int ijk[3])
{
  if(this->Locator->InsertUniquePoint(pcoords,this->LastPtId))
    {
    double value=this->Filter->ComputePointValue(ijk);
    this->Filter->PointScalars->InsertValue(this->LastPtId,value);
    }
  if(this->IdSet->Set.find(this->LastPtId)==this->IdSet->Set.end()) // not find
    {
    this->IdSet->Set.insert(this->LastPtId);
    this->Triangulator->InsertPoint(this->LastPtId,pt,pcoords,0);
    }
}

//-----------------------------------------------------------------------------
// Description:
// Insert a point in the quadtree case.
void vtkHyperOctreeContourPointsGrabber::InsertPoint2D(double pt[3],
                                                       int ijk[3])
{
  if(this->Locator->InsertUniquePoint(pt,this->LastPtId))
    {
    ijk[2]=0;
    double value=this->Filter->ComputePointValue(ijk);
    this->Filter->PointScalars->InsertValue(this->LastPtId,value);
    }
    
  this->Polygon->GetPointIds()->InsertNextId(this->LastPtId);
  this->Polygon->GetPoints()->InsertNextPoint(pt);
}

//-----------------------------------------------------------------------------
// Description:
// Return the ordered triangulator.
vtkOrderedTriangulator *vtkHyperOctreeContourPointsGrabber::GetTriangulator()
{
  return this->Triangulator;
}

//-----------------------------------------------------------------------------
// Description:
// Return the polygon.
vtkPolygon *vtkHyperOctreeContourPointsGrabber::GetPolygon()
{
  return this->Polygon;
}

//-----------------------------------------------------------------------------
void vtkHyperOctreeContourPointsGrabber::PrintSelf(ostream& os,
                                                   vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
// Description:
// Return the id in the locator after a call to InsertPoint*().
vtkIdType vtkHyperOctreeContourPointsGrabber::GetLastPtId()
{
  return this->LastPtId;
}
//-----------------------------------------------------------------------------
// Description:
// Init the bounds of the locator.
void vtkHyperOctreeContourPointsGrabber::InitLocator(vtkPoints *pts,
                                                     double bounds[6])
{
  this->Locator->InitPointInsertion(pts,bounds);
}
//-----------------------------------------------------------------------------
void vtkHyperOctreeContourPointsGrabber::SetFilter(
  vtkHyperOctreeContourFilter *filter)
{
  this->Filter=filter;
}
