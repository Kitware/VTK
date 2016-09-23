/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClipVolume.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkClipVolume.h"

#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkGarbageCollector.h"
#include "vtkImageData.h"
#include "vtkImplicitFunction.h"
#include "vtkMergePoints.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkExecutive.h"
#include "vtkObjectFactory.h"
#include "vtkOrderedTriangulator.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVoxel.h"
#include "vtkGenericCell.h"
#include "vtkTetra.h"
#include "vtkCellArray.h"
#include "vtkIdTypeArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkIncrementalPointLocator.h"

vtkStandardNewMacro(vtkClipVolume);
vtkCxxSetObjectMacro(vtkClipVolume,ClipFunction,vtkImplicitFunction);

// Construct with user-specified implicit function; InsideOut turned off; value
// set to 0.0; and generate clip scalars turned off. The merge tolerance is set
// to 0.01.
vtkClipVolume::vtkClipVolume(vtkImplicitFunction *cf)
{
  this->ClipFunction = cf;
  this->InsideOut = 0;
  this->Locator = NULL;
  this->Value = 0.0;
  this->GenerateClipScalars = 0;
  this->Mixed3DCellGeneration = 1;

  this->GenerateClippedOutput = 0;
  this->MergeTolerance = 0.01;

  this->Triangulator = vtkOrderedTriangulator::New();
  this->Triangulator->PreSortedOn();

  // optional clipped output
  this->SetNumberOfOutputPorts(2);
  vtkUnstructuredGrid *output2 = vtkUnstructuredGrid::New();
  this->GetExecutive()->SetOutputData(1, output2);
  output2->Delete();

  // by default process active point scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);
}

vtkClipVolume::~vtkClipVolume()
{
  if ( this->Locator )
  {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
  }

  this->Triangulator->Delete();
  this->SetClipFunction(NULL);
}

vtkUnstructuredGrid *vtkClipVolume::GetClippedOutput()
{
  return vtkUnstructuredGrid::SafeDownCast(
    this->GetExecutive()->GetOutputData(1));
}

// Overload standard modified time function. If Clip functions is modified,
// then this object is modified as well.
vtkMTimeType vtkClipVolume::GetMTime()
{
  vtkMTimeType mTime, time;

  mTime=this->Superclass::GetMTime();

  if ( this->Locator != NULL )
  {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
  }

  if ( this->ClipFunction != NULL )
  {
    time = this->ClipFunction->GetMTime();
    mTime = ( time > mTime ? time : mTime );
  }

  return mTime;
}

//
// Clip through volume generating tetrahedra
//
int vtkClipVolume::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkImageData *input = vtkImageData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkUnstructuredGrid *clippedOutput = this->GetClippedOutput();
  vtkCellArray *outputConn;
  vtkIdTypeArray *outputLoc;
  vtkUnsignedCharArray *outputTypes;
  vtkIdType cellId, newCellId, i;
  int j, k, flip;
  vtkPoints *cellPts;
  vtkDataArray *clipScalars;
  vtkFloatArray *cellScalars;
  vtkPoints *newPoints;
  vtkIdList *cellIds;
  double value, s, x[3], origin[3], spacing[3];
  vtkIdType estimatedSize, numCells=input->GetNumberOfCells();
  vtkIdType numPts=input->GetNumberOfPoints();
  vtkPointData *inPD=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *inCD=input->GetCellData(), *outCD=output->GetCellData();
  vtkCellData *clippedCD=clippedOutput->GetCellData();
  vtkCellData *outputCD;
  int dims[3], dimension, numICells, numJCells, numKCells, sliceSize;
  int extOffset;
  int above, below;
  vtkIdList *tetraIds;
  vtkPoints *tetraPts;
  int ii, jj, id, ntetra;
  vtkIdType pts[4], npts, *dpts, *outputCount;

  vtkDebugMacro(<< "Clipping volume");

  // Initialize self; create output objects
  //
  input->GetDimensions(dims);
  input->GetOrigin(origin);
  input->GetSpacing(spacing);

  extOffset =
    input->GetExtent()[0] + input->GetExtent()[2] + input->GetExtent()[4];

  for (dimension=3, i=0; i<3; i++)
  {
    if ( dims[i] <= 1 )
    {
      dimension--;
    }
  }
  if ( dimension < 3 )
  {
    vtkErrorMacro("This filter only clips 3D volume data");
    return 1;
  }

  if ( !this->ClipFunction && this->GenerateClipScalars )
  {
    vtkErrorMacro(<<"Cannot generate clip scalars without clip function");
    return 1;
  }

  // Create objects to hold output of clip operation
  //
  estimatedSize = numCells;
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024)
  {
    estimatedSize = 1024;
  }

  newPoints = vtkPoints::New();
  newPoints->Allocate(estimatedSize/2,estimatedSize/2);
  this->NumberOfCells = 0;
  this->Connectivity = vtkCellArray::New();
  this->Connectivity->Allocate(estimatedSize*2); //allocate storage for cells
  this->Locations = vtkIdTypeArray::New();
  this->Locations->Allocate(estimatedSize);
  this->Types = vtkUnsignedCharArray::New();
  this->Types->Allocate(estimatedSize);

  // locator used to merge potentially duplicate points
  if ( this->Locator == NULL )
  {
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion (newPoints, input->GetBounds());

  // Determine whether we're clipping with input scalars or a clip function
  // and do necessary setup.
  if ( this->ClipFunction )
  {
    vtkFloatArray *tmpScalars = vtkFloatArray::New();
    tmpScalars->Allocate(numPts);
    inPD = vtkPointData::New();
    inPD->ShallowCopy(input->GetPointData());
    if ( this->GenerateClipScalars )
    {
      inPD->SetScalars(tmpScalars);
    }
    for ( i=0; i < numPts; i++ )
    {
      s = this->ClipFunction->FunctionValue(input->GetPoint(i));
      tmpScalars->InsertTuple(i,&s);
    }
    clipScalars = tmpScalars;
  }
  else //using input scalars
  {
    clipScalars = this->GetInputArrayToProcess(0,inputVector);
    if ( !clipScalars )
    {
      vtkErrorMacro(<<"Cannot clip without clip function or input scalars");
      return 1;
    }
  }

  if ( !this->GenerateClipScalars && !input->GetPointData()->GetScalars())
  {
    outPD->CopyScalarsOff();
  }
  else
  {
    outPD->CopyScalarsOn();
  }
  outPD->InterpolateAllocate(inPD,estimatedSize,estimatedSize/2);
  outCD->CopyAllocate(inCD,estimatedSize,estimatedSize/2);
  clippedCD->CopyAllocate(inCD,estimatedSize,estimatedSize/2);

  // If generating second output, setup clipped output
  if ( this->GenerateClippedOutput )
  {
    this->NumberOfClippedCells = 0;
    this->ClippedConnectivity = vtkCellArray::New();
    this->ClippedConnectivity->Allocate(estimatedSize); //storage for cells
    this->ClippedLocations = vtkIdTypeArray::New();
    this->ClippedLocations->Allocate(estimatedSize);
    this->ClippedTypes = vtkUnsignedCharArray::New();
    this->ClippedTypes->Allocate(estimatedSize);
  }

  // perform clipping on voxels - compute approriate numbers
  value = this->Value;
  numICells = dims[0] - 1;
  numJCells = dims[1] - 1;
  numKCells = dims[2] - 1;
  sliceSize = numICells * numJCells;

  tetraIds = vtkIdList::New(); tetraIds->Allocate(20);
  cellScalars = vtkFloatArray::New(); cellScalars->Allocate(8);
  tetraPts = vtkPoints::New(); tetraPts->Allocate(20);
  vtkGenericCell *cell=vtkGenericCell::New();
  vtkTetra *clipTetra = vtkTetra::New();

  // Interior voxels (i.e., inside the clip region) are tetrahedralized using
  // 5 tetrahedra. This requires swapping the face diagonals on alternating
  // voxels to insure compatibility. Loop over i-j-k directions so that we
  // can control the direction of face diagonals on voxels (i.e., the flip
  // variable). The flip variable also controls the generation of tetrahedra
  // in boundary voxels in ClipTets() and the ordered Delaunay triangulation
  // used in ClipVoxel().
  int abort=0;
  for ( k=0; k < numKCells && !abort; k++)
  {
    // Check for progress and abort on every z-slice
    this->UpdateProgress(static_cast<double>(k) / numKCells);
    abort = this->GetAbortExecute();
    for ( j=0; j < numJCells; j++)
    {
      for ( i=0; i < numICells; i++ )
      {
        flip = (extOffset+i+j+k) & 0x1;
        cellId = i + j*numICells + k*sliceSize;

        input->GetCell(cellId,cell);
        if ( cell->GetCellType() == VTK_EMPTY_CELL )
        {
          continue;
        }
        cellPts = cell->GetPoints();
        cellIds = cell->GetPointIds();

        // gather scalar values for the cell and keep
        for ( above=below=0, ii=0; ii < 8; ii++ )
        {
          s = clipScalars->GetComponent(cellIds->GetId(ii),0);
          cellScalars->SetComponent(ii, 0, s);
          if ( s >= value )
          {
            above = 1;
          }
          else
          {
            below = 1;
          }
        }

        // take into account inside/out flag
        if ( this->InsideOut )
        {
          above = !above;
          below = !below;
        }

        // See whether voxel is fully inside or outside and triangulate
        // according to the flup variable.
        if ( (above && !below) ||
             (this->GenerateClippedOutput && (below && !above)) )
        {
          cell->Triangulate(flip, tetraIds, tetraPts);
          ntetra = tetraPts->GetNumberOfPoints() / 4;

          if (above && !below)
          {
            outputConn = this->Connectivity;
            outputLoc = this->Locations;
            outputTypes = this->Types;
            outputCount = &this->NumberOfCells;
            outputCD = outCD;
          }
          else
          {
            outputConn = this->ClippedConnectivity;
            outputLoc = this->ClippedLocations;
            outputTypes = this->ClippedTypes;
            outputCount = &this->NumberOfClippedCells;
            outputCD = clippedCD;
          }

          for (ii=0; ii<ntetra; ii++)
          {
            id = ii*4;
            for (jj=0; jj<4; jj++)
            {
              tetraPts->GetPoint(id+jj, x);
              if ( this->Locator->InsertUniquePoint(x, pts[jj]) )
              {
                outPD->CopyData(inPD,tetraIds->GetId(id+jj),pts[jj]);
              }
            }
            newCellId = outputConn->InsertNextCell(4,pts);
            (*outputCount)++;
            outputLoc->InsertNextValue(outputConn->GetTraversalLocation());
            outputConn->GetNextCell(npts,dpts); //updates traversal location
            outputTypes->InsertNextValue( VTK_TETRA );
            outputCD->CopyData(inCD,cellId,newCellId);
          }//for each tetra produced by triangulation
        }

        else if (above == below ) // clipped voxel, have to triangulate
        {
          if ( this->Mixed3DCellGeneration ) //use vtkTetra clipping templates
          {
            cell->Triangulate(flip, tetraIds, tetraPts);
            this->ClipTets(value, clipTetra, clipScalars, cellScalars,
                           tetraIds, tetraPts, inPD, outPD,
                           inCD, cellId, outCD, clippedCD, this->InsideOut);
          }
          else //use vtkOrderedTriangulator to produce tetrahedra
          {
            this->ClipVoxel(value, cellScalars, flip, origin, spacing,
                            cellIds, cellPts, inPD, outPD,
                            inCD, cellId, outCD, clippedCD);
          }
        } // using ordered triangulator
      }// for i
    }// for j
  }// for k

  // Create the output
  output->SetPoints(newPoints);
  output->SetCells(this->Types,this->Locations,this->Connectivity);
  this->Types->Delete();
  this->Locations->Delete();
  this->Connectivity->Delete();
  output->Squeeze();
  vtkDebugMacro(<<"Created: "
                << newPoints->GetNumberOfPoints() << " points, "
                << output->GetNumberOfCells() << " tetra" );

  if ( this->GenerateClippedOutput )
  {
    clippedOutput->SetPoints(newPoints);
    clippedOutput->SetCells(this->ClippedTypes,this->ClippedLocations,
                            this->ClippedConnectivity);
    this->ClippedTypes->Delete();
    this->ClippedLocations->Delete();
    this->ClippedConnectivity->Delete();
    clippedOutput->GetPointData()->PassData(outPD);
    clippedOutput->Squeeze();
    vtkDebugMacro(<<"Created (clipped output): "
                  << clippedOutput->GetNumberOfCells() << " tetra");
  }

  // Update ourselves.  Because we don't know upfront how many cells
  // we've created, take care to reclaim memory.
  //
  if ( this->ClipFunction )
  {
    clipScalars->Delete();
    inPD->Delete();
  }

  // Clean up
  newPoints->Delete();
  cell->Delete();
  tetraIds->Delete();
  tetraPts->Delete();
  cellScalars->Delete();
  clipTetra->Delete();

  this->Locator->Initialize();//release any extra memory

  return 1;
}


// Method to triangulate and clip voxel using vtkTetra::Clip() method.
// This produces a mixed mesh of tetrahedra and wedges but it is faster
// than using the ordered triangulator. It works by using the usual
// alternating five tetrahedra template per voxel, and then using the
// vtkTetra::Clip() method to produce the output.
//
void vtkClipVolume::ClipTets(double value, vtkTetra *clipTetra,
                             vtkDataArray *clipScalars,
                             vtkDataArray *cellScalars, vtkIdList *tetraIds,
                             vtkPoints *tetraPts, vtkPointData *inPD,
                             vtkPointData *outPD, vtkCellData *inCD,
                             vtkIdType cellId, vtkCellData *outCD,
                             vtkCellData *clippedCD, int insideOut)
{
  // Tessellate this cell as if it were inside
  vtkIdType ntetra = tetraPts->GetNumberOfPoints() / 4;
  int i, id, j, k, numNew;
  vtkIdType npts=0, *pts;

  // Clip each tetrahedron
  for (i=0; i<ntetra; i++)
  {
    id = i*4;
    for (j=0; j<4; j++)
    {
      clipTetra->PointIds->SetId(j,tetraIds->GetId(id+j));
      clipTetra->Points->SetPoint(j,tetraPts->GetPoint(id+j));
      cellScalars->
        SetComponent(j,0,clipScalars->GetComponent(tetraIds->GetId(id+j),0));
    }
    clipTetra->Clip(value, cellScalars, this->Locator, this->Connectivity,
                    inPD, outPD, inCD, cellId, outCD, insideOut);
    numNew = this->Connectivity->GetNumberOfCells() - this->NumberOfCells;
    this->NumberOfCells = this->Connectivity->GetNumberOfCells();
    for (k=0; k<numNew; k++)
    {
      this->Locations->
        InsertNextValue(this->Connectivity->GetTraversalLocation());
      this->Connectivity->GetNextCell(npts,pts);
      this->Types->InsertNextValue((npts==4?VTK_TETRA:VTK_WEDGE));
    }

    if ( this->GenerateClippedOutput )
    {
      clipTetra->Clip(value, cellScalars, this->Locator,
                      this->ClippedConnectivity, inPD, outPD,
                      inCD, cellId, clippedCD, !insideOut);
      numNew = this->ClippedConnectivity->GetNumberOfCells() -
        this->NumberOfClippedCells;
      this->NumberOfClippedCells =
        this->ClippedConnectivity->GetNumberOfCells();
      for (k=0; k<numNew; k++)
      {
        this->ClippedLocations->InsertNextValue(
          this->ClippedConnectivity->GetTraversalLocation() );
        this->ClippedConnectivity->GetNextCell(npts,pts);
        this->ClippedTypes->InsertNextValue((npts==4?VTK_TETRA:VTK_WEDGE));
      }
    }
  }
}

// Method to triangulate and clip voxel using ordered Delaunay
// triangulation to produce tetrahedra. Voxel is initially triangulated
// using 8 voxel corner points inserted in order (to control direction
// of face diagonals). Then edge intersection points are injected into the
// triangulation. The ordering controls the orientation of any face
// diagonals.
void vtkClipVolume::ClipVoxel(double value, vtkDataArray *cellScalars,
                              int flip, double vtkNotUsed(origin)[3],
                              double spacing[3], vtkIdList *cellIds,
                              vtkPoints *cellPts, vtkPointData *inPD,
                              vtkPointData *outPD, vtkCellData *inCD,
                              vtkIdType cellId, vtkCellData *outCD,
                              vtkCellData *clippedCD)
{
  double x[3], s1, s2, t, voxelOrigin[3];
  double bounds[6], p1[3], p2[3];
  int i, k, edgeNum, numPts, numNew;
  vtkIdType id, ptId, npts, *pts;
  static int edges[12][2] = { {0,1}, {2,3}, {4,5}, {6,7},
                              {0,2}, {1,3}, {4,6}, {5,7},
                              {0,4}, {1,5}, {2,6}, {3,7}};
  static int order[2][8] = { {0,3,5,6,1,2,4,7},
                             {1,2,4,7,0,3,5,6}};//injection order based on flip

  // compute bounds for voxel and initialize
  cellPts->GetPoint(0,voxelOrigin);
  for (i=0; i<3; i++)
  {
    bounds[2*i] = voxelOrigin[i];
    bounds[2*i+1] = voxelOrigin[i] + spacing[i];
  }

  // Initialize Delaunay insertion process with voxel triangulation.
  // No more than 20 points (8 corners + 12 edges) may be inserted.
  this->Triangulator->InitTriangulation(bounds,20);

  // Inject ordered voxel corner points into triangulation. Recall
  // that the PreSortedOn() flag was set in the triangulator.
  int type;
  vtkIdType internalId[8]; //used to merge points if nearby edge intersection
  for (numPts=0; numPts<8; numPts++)
  {
    ptId = order[flip][numPts];

    // Currently all points are injected because of the possibility
    // of intersection point merging.
    s1 = cellScalars->GetComponent(ptId,0);
    if ( (s1 >= value && !this->InsideOut) || (s1 < value && this->InsideOut) )
    {
      type = 0; //inside
    }
    else
    {
      //type 1 is "outside"; type 4 is don't insert
      type = (this->GenerateClippedOutput ? 1 : 4);
    }

    cellPts->GetPoint(ptId, x);
    if ( this->Locator->InsertUniquePoint(x, id) )
    {
      outPD->CopyData(inPD, cellIds->GetId(ptId), id);
    }
    internalId[ptId] = this->Triangulator->InsertPoint(id, x, x, type);
  }//for eight voxel corner points

  // For each edge intersection point, insert into triangulation. Edge
  // intersections come from clipping value. Have to be careful of
  // intersections near exisiting points (causes bad Delaunay behavior).
  for (edgeNum=0; edgeNum < 12; edgeNum++)
  {
    s1 = cellScalars->GetComponent(edges[edgeNum][0],0);
    s2 = cellScalars->GetComponent(edges[edgeNum][1],0);

    if ( (s1 < value && s2 >= value) || (s1 >= value && s2 < value) )
    {

      t = (value - s1) / (s2 - s1);

      // Check to see whether near the intersection is near a voxel corner.
      // If so,have to merge requiring a change of type to type=boundary.
      if ( t < this->MergeTolerance )
      {
        this->Triangulator->UpdatePointType(internalId[edges[edgeNum][0]], 2);
        continue;
      }
      else if (t > (1.0 - this->MergeTolerance) )
      {
        this->Triangulator->UpdatePointType(internalId[edges[edgeNum][1]], 2);
        continue;
      }

      // generate edge intersection point
      cellPts->GetPoint(edges[edgeNum][0],p1);
      cellPts->GetPoint(edges[edgeNum][1],p2);
      for (i=0; i<3; i++)
      {
        x[i] = p1[i] + t * (p2[i] - p1[i]);
      }

      // Incorporate point into output and interpolate edge data as necessary
      if ( this->Locator->InsertUniquePoint(x, ptId) )
      {
        outPD->InterpolateEdge(inPD, ptId, cellIds->GetId(edges[edgeNum][0]),
                               cellIds->GetId(edges[edgeNum][1]), t);
      }

      //Insert into Delaunay triangulation
      this->Triangulator->InsertPoint(ptId,x,x,2);

    }//if edge intersects value
  }//for all edges

  // triangulate the points
  this->Triangulator->Triangulate();

  // Add the triangulation to the mesh
  vtkIdType newCellId;
  this->Triangulator->AddTetras(0,this->Connectivity);
  numNew = this->Connectivity->GetNumberOfCells() - this->NumberOfCells;
  this->NumberOfCells = this->Connectivity->GetNumberOfCells();
  for (k=0; k<numNew; k++)
  {
    newCellId = this->Locations->
      InsertNextValue(this->Connectivity->GetTraversalLocation());
    this->Connectivity->GetNextCell(npts,pts); //updates traversal location
    this->Types->InsertNextValue(VTK_TETRA);
    outCD->CopyData(inCD,cellId,newCellId);
  }

  if ( this->GenerateClippedOutput )
  {
    this->Triangulator->AddTetras(1,this->ClippedConnectivity);
    numNew = this->ClippedConnectivity->GetNumberOfCells() -
      this->NumberOfClippedCells;
    this->NumberOfClippedCells = this->ClippedConnectivity->GetNumberOfCells();
    for (k=0; k<numNew; k++)
    {
      newCellId = this->ClippedLocations->
        InsertNextValue(this->ClippedConnectivity->GetTraversalLocation());
      this->ClippedConnectivity->GetNextCell(npts,pts);
      this->ClippedTypes->InsertNextValue(VTK_TETRA);
      clippedCD->CopyData(inCD,cellId,newCellId);
    }
  }
}


// Specify a spatial locator for merging points. By default,
// an instance of vtkMergePoints is used.
void vtkClipVolume::SetLocator(vtkIncrementalPointLocator *locator)
{
  if ( this->Locator == locator )
  {
    return;
  }
  if ( this->Locator )
  {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
  }

  if (locator)
  {
    locator->Register(this);
  }

  this->Locator = locator;
  this->Modified();
}

void vtkClipVolume::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
  {
    this->Locator = vtkMergePoints::New();
  }
}

int vtkClipVolume::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

void vtkClipVolume::PrintSelf(ostream& os, vtkIndent indent)
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
  os << indent << "Merge Tolerance: " << this->MergeTolerance << "\n";
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

  os << indent << "Mixed 3D Cell Type: "
     << (this->Mixed3DCellGeneration ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
void vtkClipVolume::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  // These filters share our input and are therefore involved in a
  // reference loop.
  vtkGarbageCollectorReport(collector, this->ClipFunction, "ClipFunction");
}
