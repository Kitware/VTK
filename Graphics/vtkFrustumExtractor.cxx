/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFrustumExtractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFrustumExtractor.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkIdList.h"
#include "vtkImplicitFunction.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPlane.h"
#include "vtkPlanes.h"
#include "vtkSignedCharArray.h"
#include "vtkCellArray.h"
#include "vtkTimerLog.h"
#include "vtkPoints.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkHexahedron.h"
#include "vtkLine.h"

vtkCxxRevisionMacro(vtkFrustumExtractor, "1.6");
vtkStandardNewMacro(vtkFrustumExtractor);
vtkCxxSetObjectMacro(vtkFrustumExtractor,Frustum,vtkPlanes);

//set to 4 to ignore the near and far planes which are almost always passed
#define MAXPLANE 6

//----------------------------------------------------------------------------
vtkFrustumExtractor::vtkFrustumExtractor(vtkPlanes *f)
{
  this->PassThrough = 0;
  this->ExactTest = 1;

  this->NumRejects = 0;
  this->NumIsects = 0;
  this->NumAccepts = 0;

  this->ClipPoints = vtkPoints::New();
  this->ClipPoints->SetNumberOfPoints(8);
  double verts[24] =
    {
    0.0,0.0,0.0,
    0.0,0.0,0.0,
    0.0,0.0,0.0,
    0.0,0.0,0.0,
    0.0,0.0,0.0,
    0.0,0.0,0.0,
    0.0,0.0,0.0,
    0.0,0.0,0.0
    };
  this->Frustum = f;
  if (this->Frustum)
    {
    this->Frustum->Register(this);
    }
  else
    {
    this->Frustum = vtkPlanes::New();
    this->CreateFrustum(verts);
    }
}

//----------------------------------------------------------------------------
vtkFrustumExtractor::~vtkFrustumExtractor()
{
  this->Frustum->Delete();
  this->ClipPoints->Delete();
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If implicit function is modified,
// then this object is modified as well.
unsigned long vtkFrustumExtractor::GetMTime()
{
  unsigned long mTime=this->MTime.GetMTime();
  unsigned long impFuncMTime;

  if ( this->Frustum != NULL )
    {
    impFuncMTime = this->Frustum->GetMTime();
    mTime = ( impFuncMTime > mTime ? impFuncMTime : mTime );
    }

  return mTime;
}

//--------------------------------------------------------------------------
void vtkFrustumExtractor::CreateFrustum(double verts[24])
{
  //for debugging
  for (int i = 0; i < 8; i++)
    {    
    this->ClipPoints->SetPoint(i, &verts[i*3]);
    }

  vtkPoints *points = vtkPoints::New();
  points->SetNumberOfPoints(6);

  vtkDoubleArray *norms = vtkDoubleArray::New();
  norms->SetNumberOfComponents(3);
  norms->SetNumberOfTuples(6);

  //left
  this->ComputePlane(0, &verts[0*3], &verts[2*3], &verts[3*3], points, norms);
  //right
  this->ComputePlane(1, &verts[7*3], &verts[6*3], &verts[4*3], points, norms);
  //bottom
  this->ComputePlane(2, &verts[5*3], &verts[4*3], &verts[0*3], points, norms);
  //top
  this->ComputePlane(3, &verts[2*3], &verts[6*3], &verts[7*3], points, norms);
  //near
  this->ComputePlane(4, &verts[6*3], &verts[2*3], &verts[0*3], points, norms);
  //far
  this->ComputePlane(5, &verts[1*3], &verts[3*3], &verts[7*3], points, norms);

  this->Frustum->SetPoints(points);
  this->Frustum->SetNormals(norms);
  points->Delete();
  norms->Delete();
}

//--------------------------------------------------------------------------
void vtkFrustumExtractor::ComputePlane(int idx, double v0[3], double v1[3], double v2[3], vtkPoints *points, vtkDoubleArray *norms)
{
  points->SetPoint(idx, v0[0], v0[1], v0[2]);

  double e0[3];
  e0[0] = v1[0]-v0[0];
  e0[1] = v1[1]-v0[1];
  e0[2] = v1[2]-v0[2];

  double e1[3];
  e1[0] = v2[0]-v0[0];
  e1[1] = v2[1]-v0[1];
  e1[2] = v2[2]-v0[2];

  double n[3];
  vtkMath::Cross(e0,e1,n);
  vtkMath::Normalize(n);

  norms->SetTuple(idx, n);
}

//----------------------------------------------------------------------------
//needed because parent class sets output type to input type
//and we sometimes want to change it to make an UnstructuredGrid regardless of 
//input type
int vtkFrustumExtractor::RequestDataObject(
  vtkInformation*, 
  vtkInformationVector** inputVector , 
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  if (input)
    {
    // for each output
    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
      {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataSet *output = vtkDataSet::SafeDownCast(
        info->Get(vtkDataObject::DATA_OBJECT()));
    
      if (!output || 
          (this->PassThrough && !output->IsA(input->GetClassName())) ||
          (!this->PassThrough && !output->IsA("vtkUnstructuredGrid"))
        ) 
        {
        if (this->PassThrough)
          { 
          vtkDataSet* newOutput;
          newOutput = input->NewInstance();
          newOutput->SetPipelineInformation(info);
          newOutput->Delete();
          this->GetOutputPortInformation(0)->Set(
            vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
          }
        else
          {
          vtkUnstructuredGrid* newOutput = vtkUnstructuredGrid::New();
          newOutput->SetPipelineInformation(info);
          newOutput->Delete();
          this->GetOutputPortInformation(0)->Set(
            vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
          }
        }
      }
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkFrustumExtractor::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  //cerr << "Extracting geometry" << endl;

  if ( !this->Frustum )
    {
    vtkErrorMacro(<<"No frustum specified");
    return 1;
    }

  if ( this->Frustum->GetNumberOfPlanes() != 6 )
    {
    vtkErrorMacro(<<"Frustum must have six planes.");
    return 0;
    }

  // get the input and ouptut
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  double bounds[6];
  vtkIdType i; 
  double x[3];
  input->GetBounds(bounds);
  if (!this->OverallBoundsTest(bounds) )
    {
    return 1;
    }

  vtkIdType ptId, numPts, newPointId;
  vtkIdType numCells, cellId, newCellId, numCellPts;
  vtkIdType *pointMap;
  vtkCell *cell;
  vtkIdList *cellPts;
  vtkIdList *newCellPts;
  int isect;

  vtkSignedCharArray *pointInArray = vtkSignedCharArray::New();
  vtkSignedCharArray *cellInArray = vtkSignedCharArray::New();
  vtkPoints *newPts = vtkPoints::New();

  /*
  int NUMCELLS = 0;
  int NUMPTS = 0;
  cerr << "{" << endl;
  vtkTimerLog *timer = vtkTimerLog::New();
  timer->StartTimer();
  */

  vtkDataSet *outputDS = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkUnstructuredGrid *outputUG = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPointData *pd = input->GetPointData();
  vtkCellData *cd = input->GetCellData();
  vtkPointData *outputPD = outputDS->GetPointData();
  vtkCellData *outputCD = outputDS->GetCellData();
  
  numPts = input->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();
  pointMap = new vtkIdType[numPts]; // maps old point ids into new
  newCellPts = vtkIdList::New();
  newCellPts->Allocate(VTK_CELL_SIZE);

  if (this->PassThrough)
    {
    //the output is a copy of the input, with two new arrays defined
    outputDS->ShallowCopy(input);

    pointInArray->SetNumberOfComponents(1);
    pointInArray->SetNumberOfTuples(numPts);
    for (i=0; i < numPts; i++)
      {
      pointInArray->SetValue(i, -1);
      }
    pointInArray->SetName("vtkInsidedness");
    outputPD->AddArray(pointInArray);
    outputPD->SetScalars(pointInArray);

    cellInArray->SetNumberOfComponents(1);
    cellInArray->SetNumberOfTuples(numCells);
    for (i=0; i < numCells; i++)
      {
      cellInArray->SetValue(i, -1);
      }
    cellInArray->SetName("vtkInsidedness");
    outputCD->AddArray(cellInArray);
    outputCD->SetScalars(cellInArray);
    }
  else 
    {
    //the output is a new unstructured grid
/*
    //for debugging, shows rough outline of the selection frustum
    //only valid if CreateFrustum was called
    outputUG->Allocate(numCells/4); //allocate storage for geometry/topology
    vtkLine *linesRays = vtkLine::New();
    linesRays->GetPointIds()->SetId(0, 0);
    linesRays->GetPointIds()->SetId(1, 1);
    outputUG->InsertNextCell(linesRays->GetCellType(), linesRays->GetPointIds());
    linesRays->GetPointIds()->SetId(0, 2);
    linesRays->GetPointIds()->SetId(1, 3);
    outputUG->InsertNextCell(linesRays->GetCellType(), linesRays->GetPointIds());
    linesRays->GetPointIds()->SetId(0, 4);
    linesRays->GetPointIds()->SetId(1, 5);
    outputUG->InsertNextCell(linesRays->GetCellType(), linesRays->GetPointIds());
    linesRays->GetPointIds()->SetId(0, 6);
    linesRays->GetPointIds()->SetId(1, 7);
    outputUG->InsertNextCell(linesRays->GetCellType(), linesRays->GetPointIds());
    outputUG->SetPoints(this->ClipPoints);
    linesRays->Delete();
    return 1;
*/

    outputUG->Allocate(numCells/4); //allocate storage for geometry/topology
    newPts->Allocate(numPts/4,numPts);
    outputPD->CopyAllocate(pd);
    outputCD->CopyAllocate(cd);
    }

  if (this->ExactTest)
    {
    //cell based isect test, a cell is inside if any part of it is inside the
    //frustum, a point is inside if it belongs to an inside cell, or is not 
    //in any cell but is inside the frustum

    //initialize all points to say not looked at
    for (ptId=0; ptId < numPts; ptId++)
      {
      pointMap[ptId] = -1;
      }

    /*
    timer->StopTimer();
    cerr << "  PTINIT " << timer->GetElapsedTime() << endl;
    timer->StartTimer();
    */
    // Loop over all cells to see whether they are inside.
    for (cellId=0; cellId < numCells; cellId++)
      {
      input->GetCellBounds(cellId, bounds);

      cell = input->GetCell(cellId);
      cellPts = cell->GetPointIds();
      numCellPts = cell->GetNumberOfPoints();    
      newCellPts->Reset();
      
      isect = this->ABoxFrustumIsect(bounds, cell);
      if (isect == 1)
        {
        /*
        NUMCELLS++;
        */

        //intersects, put all of the points inside
        for (i=0; i < numCellPts; i++)
          {
          ptId = cellPts->GetId(i);
          newPointId = pointMap[ptId];
          if (newPointId < 0)
            {             
            /*
            NUMPTS++;
            */

            input->GetPoint(ptId, x);      
            if (this->PassThrough)
              {
              pointInArray->SetValue(ptId, 1);
              newPointId = ptId;
              }
            else
              {
              newPointId = newPts->InsertNextPoint(x);
              outputPD->CopyData(pd,ptId,newPointId);
              }
            pointMap[ptId] = newPointId;
            }
          newCellPts->InsertId(i,newPointId);
          }

        if (this->PassThrough)
          {
          cellInArray->SetValue(cellId, 1);
          }
        else
          {
          newCellId = outputUG->InsertNextCell(cell->GetCellType(),newCellPts);
          outputCD->CopyData(cd,cellId,newCellId);
          }
        }

      if (isect == -1) //complete reject, remember these points are outside
        {
        for (i=0; i < numCellPts; i++)
          {
          ptId = cellPts->GetId(i);
          pointMap[ptId] = -2;
          }
        }
      }//for all cells

    /*
    timer->StopTimer();
    cerr << "  CLIN " << timer->GetElapsedTime() << endl;
    timer->StartTimer();    
    cerr << "  AFTERCELL NUMPTS = " << NUMPTS << endl;
    */

    //there could be some points that are not used by any cell
    for (ptId = 0; ptId < numPts; ptId++)
      {
      if (pointMap[ptId] == -1) //point wasn't attached to a cell
        {
        input->GetPoint(ptId,x);      
        if (this->Frustum->EvaluateFunction(x) < 0.0)
          {
          /*
          NUMPTS++;
          */
          if (this->PassThrough)
            {
            pointInArray->SetValue(ptId, 1);
            }
          else
            {
            newPointId = newPts->InsertNextPoint(x);
            outputPD->CopyData(pd,ptId,newPointId);
            }
          }
        }
      }
    }
  else
    {
    //point based isect test, a cell is inside if all of its points are inside

    //run through points and decide which ones are inside
    for (ptId = 0; ptId < numPts; ptId++)
      {
      input->GetPoint(ptId,x);      
      pointMap[ptId] = -1;
      if (this->Frustum->EvaluateFunction(x) < 0.0)
        {
        /*
        NUMPTS++;
        */
        if (this->PassThrough)
          {
          newPointId = ptId;
          pointInArray->SetValue(ptId,1);
          }
        else
          {
          newPointId = newPts->InsertNextPoint(x);
          outputPD->CopyData(pd,ptId,newPointId);
          }
        pointMap[ptId] = newPointId;
        }
      }

    /*
    timer->StopTimer();
    cerr << "  PTIN " << timer->GetElapsedTime() << endl;
    timer->StartTimer();
    */

    //run through cells and accept only those with all points inside
    for (cellId = 0;  cellId < numCells; cellId++)
      {
      cell = input->GetCell(cellId);
      cellPts = cell->GetPointIds();
      numCellPts = cell->GetNumberOfPoints();      
      newCellPts->Reset();

      isect = 1;
      for (i=0; i < numCellPts; i++)
        {
        ptId = cellPts->GetId(i);
        newPointId = pointMap[ptId];
        if ( newPointId < 0 )
          {
          isect = 0;
          /*
          this->NumRejects++;
          */
          break; //this cell won't be inserted
          }
        newCellPts->InsertId(i,newPointId);
        }
      if (isect)
        {
        /*
        NUMCELLS++;
        this->NumAccepts++;
        */
        if (this->PassThrough)
          {
          cellInArray->SetValue(cellId,1);
          }
        else
          {
          newCellId = outputUG->InsertNextCell(cell->GetCellType(),newCellPts);
          outputCD->CopyData(cd,cellId,newCellId);
          }
        }
      }
    } 

  /*
  cerr << "  REJECTS " << this->NumRejects << " ";
  this->NumRejects = 0;
  cerr << "  ACCEPTS " << this->NumAccepts << " ";
  this->NumAccepts = 0;
  cerr << "  ISECTS " << this->NumIsects << endl;
  this->NumIsects = 0;
  cerr << "  @END NUMPTS = " << NUMPTS << endl;
  cerr << "  @END NUMCELLS = " << NUMCELLS << endl;
  timer->StopTimer();
  cerr << "  " << timer->GetElapsedTime() << endl;
  timer->Delete();
  cerr << "}" << endl;
  */

  // Update ourselves and release memory
  delete [] pointMap;
  newCellPts->Delete();
  pointInArray->Delete();
  cellInArray->Delete();
  if (!this->PassThrough)
    {
    outputUG->SetPoints(newPts);
    }
  newPts->Delete();  
  outputDS->Squeeze();


  return 1;
}

//--------------------------------------------------------------------------
int vtkFrustumExtractor::OverallBoundsTest(double *bounds) 
{
  vtkIdType i; 
  double x[3];

  //find the near and far vertices to each plane for quick in/out tests
  for (i = 0; i < MAXPLANE; i++)
    {
    this->Frustum->GetNormals()->GetTuple(i, x);
    int xside = (x[0] > 0) ? 1:0;
    int yside = (x[1] > 0) ? 1:0;
    int zside = (x[2] > 0) ? 1:0;
    this->np_vertids[i][0] = (1-xside)*4+(1-yside)*2+(1-zside);
    this->np_vertids[i][1] = xside*4+yside*2+zside;
    }

  vtkHexahedron *vox = vtkHexahedron::New();
  vox->GetPointIds()->SetId(0, 0); 
  vox->GetPointIds()->SetId(1, 1); 
  vox->GetPointIds()->SetId(2, 3); 
  vox->GetPointIds()->SetId(3, 2); 
  vox->GetPointIds()->SetId(4, 4); 
  vox->GetPointIds()->SetId(5, 5); 
  vox->GetPointIds()->SetId(6, 7); 
  vox->GetPointIds()->SetId(7, 6); 

  vtkPoints *p = vox->GetPoints();
  p->SetPoint(0, bounds[0], bounds[2], bounds[4]);
  p->SetPoint(1, bounds[0], bounds[2], bounds[5]);
  p->SetPoint(2, bounds[0], bounds[3], bounds[4]);
  p->SetPoint(3, bounds[0], bounds[3], bounds[5]);
  p->SetPoint(4, bounds[1], bounds[2], bounds[4]);
  p->SetPoint(5, bounds[1], bounds[2], bounds[5]);
  p->SetPoint(6, bounds[1], bounds[3], bounds[4]);
  p->SetPoint(7, bounds[1], bounds[3], bounds[5]);

  int rc;
  rc = this->ABoxFrustumIsect(bounds, vox);
  vox->Delete();
  return (rc > 0);
}

//--------------------------------------------------------------------------
//Intersect the cell (with its associated bounds) with the clipping frustum.
//Return 1 if partially inside, 0 or -1 if not inside.
//Also return a distance to the near plane.
int vtkFrustumExtractor::ABoxFrustumIsect(double *bounds, vtkCell *cell)
{
  if (bounds[0] > bounds[1] ||
      bounds[2] > bounds[3] ||
      bounds[4] > bounds[5]) 
    {
    return this->IsectDegenerateCell(cell);
    }
    
  //convert bounds to 8 vertices
  double verts[8][3];
  verts[0][0] = bounds[0];
  verts[0][1] = bounds[2];
  verts[0][2] = bounds[4];
  verts[1][0] = bounds[0];
  verts[1][1] = bounds[2];
  verts[1][2] = bounds[5];
  verts[2][0] = bounds[0];
  verts[2][1] = bounds[3];
  verts[2][2] = bounds[4];
  verts[3][0] = bounds[0];
  verts[3][1] = bounds[3];
  verts[3][2] = bounds[5];
  verts[4][0] = bounds[1];
  verts[4][1] = bounds[2];
  verts[4][2] = bounds[4];
  verts[5][0] = bounds[1];
  verts[5][1] = bounds[2];
  verts[5][2] = bounds[5];
  verts[6][0] = bounds[1];
  verts[6][1] = bounds[3];
  verts[6][2] = bounds[4];
  verts[7][0] = bounds[1];
  verts[7][1] = bounds[3];
  verts[7][2] = bounds[5];

  int pid;
  vtkPlane *plane;
  int intersect = 0;
  double dist;
  int nvid;
  int pvid;

  //reject if any plane rejects the entire bbox
  for (pid = 0; pid < MAXPLANE; pid++)
    {
    plane = this->Frustum->GetPlane(pid);
    nvid = this->np_vertids[pid][0];
    dist = plane->EvaluateFunction(verts[nvid]);
    if (dist > 0.0)     
      {
      /*
      this->NumRejects++;
      */
      return -1;
      }
    pvid = this->np_vertids[pid][1];
    dist = plane->EvaluateFunction(verts[pvid]);
    if (dist > 0.0)
      {
      intersect = 1;
      }
    }

  //accept if entire bbox is inside all planes
  if (!intersect) 
    {
    /*
    this->NumAccepts++;
    */
    return 1;
    }

  //otherwise we have to do clipping tests to decide if actually insects
  /*
  this->NumIsects++;
  */
  vtkCell *face;
  vtkCell *edge;
  vtkPoints *pts;
  double *vertbuffer;
  int maxedges = 24;
  //be ready to resize if we hit a polygon with many vertices
  vertbuffer = new double[3*maxedges*3];
  double *vlist = &vertbuffer[0*maxedges*3];
  double *wvlist = &vertbuffer[1*maxedges*3];
  double *ovlist = &vertbuffer[2*maxedges*3];

  int nfaces = cell->GetNumberOfFaces();
  if (nfaces < 1) 
    {
    //some 2D cells have no faces, only edges 
    int nedges = cell->GetNumberOfEdges();
    if (nedges < 1)
      {
      delete vertbuffer;
      return this->IsectDegenerateCell(cell);
      }
    if (nedges+4 > maxedges)
      {
      delete vertbuffer;
      maxedges=(nedges+4)*2;
      vertbuffer = new double[3*maxedges*3];
      vlist = &vertbuffer[0*maxedges*3];
      wvlist = &vertbuffer[1*maxedges*3];
      ovlist = &vertbuffer[2*maxedges*3];
      }

    edge = cell->GetEdge(0);
    pts = edge->GetPoints();
    pts->GetPoint(0, &vlist[0*3]);
    pts->GetPoint(1, &vlist[1*3]);

    for (int e = 1; e < nedges-1; e++)
      {
      edge = cell->GetEdge(e);
      pts = edge->GetPoints();
      pts->GetPoint(1, &vlist[(e+1)*3]); //get second point of the edge
      }      
    if (this->FrustumClipPolygon(nedges, vlist, wvlist, ovlist))
      {
      delete vertbuffer;
      return 1;
      }
    }
  else
    {
    
    //go around edges of each face and clip to planes
    //if nothing remains at the end, then we do not intersect and have to reject
    for (int f = 0; f < nfaces; f++)
      {
      face = cell->GetFace(f);    
      int nedges = face->GetNumberOfEdges();
      if (nedges < 1) 
        {
        if (this->IsectDegenerateCell(face))
          {
          delete vertbuffer;
          return 1;
          }
        continue;
        }
      if (nedges+4 > maxedges)
        {
        delete vertbuffer;
        maxedges=(nedges+4)*2;
        vertbuffer = new double[3*maxedges*3];
        vlist = &vertbuffer[0*maxedges*3];
        wvlist = &vertbuffer[1*maxedges*3];
        ovlist = &vertbuffer[2*maxedges*3];
        }
      edge = face->GetEdge(0);
      pts = edge->GetPoints();
      pts->GetPoint(0, &vlist[0*3]);
      pts->GetPoint(1, &vlist[1*3]); 
      for (int e = 1; e < nedges-1; e++)
        {
        edge = face->GetEdge(e);
        pts = edge->GetPoints();
        pts->GetPoint(1, &vlist[(e+1)*3]); //get second point of the edge
        }
      
      if (this->FrustumClipPolygon(nedges, vlist, wvlist, ovlist))
        {
        delete vertbuffer;
        return 1;
        }
      }
    }

  delete vertbuffer;
  return 0;
}

//--------------------------------------------------------------------------
//handle degenerate cells by testing each point, if any in, then in
int vtkFrustumExtractor::IsectDegenerateCell(vtkCell *cell)
{
  int npts = cell->GetNumberOfPoints();
  vtkPoints *pts = cell->GetPoints();
  double x[3];
  for (int i = 0; i < npts; i++)
    {
    pts->GetPoint(i, x);
    if (this->Frustum->EvaluateFunction(x) < 0.0)
      {
      return 1;
      }      
    }
  return 0;
}

//--------------------------------------------------------------------------
//clips the polygon against the frustum
//if there is no intersection, returns 0
//if there is an intersection, returns 1
// update ovlist to contain the resulting clipped vertices
int vtkFrustumExtractor::FrustumClipPolygon(int nverts, double *ivlist, double *wvlist, double *ovlist)
{
  int nwverts = nverts;
  memcpy((void*)wvlist, (void*)ivlist, nverts*sizeof(double)*3);

  int noverts = 0;
  int pid;
  for (pid = 0; pid < MAXPLANE; pid++)
    {
    noverts = 0;
    this->PlaneClipPolygon(nwverts, wvlist, pid, noverts, ovlist);    
    if (noverts == 0)
      {
      return 0;
      }
    memcpy((void*)wvlist, (void*)ovlist, noverts*sizeof(double)*3);
    nwverts = noverts;
    }

  return 1;
}

//--------------------------------------------------------------------------
//clips a polygon against the numbered plane, resulting vertices are stored
//in ovlist, noverts
void vtkFrustumExtractor::PlaneClipPolygon(int nverts, double *ivlist, int pid, int &noverts, double *ovlist)
{
  int vid;  
  //run around the polygon and clip to this edge
  for (vid = 0; vid < nverts-1; vid++)
    {
    this->PlaneClipEdge(&ivlist[vid*3], &ivlist[(vid+1)*3], pid, noverts, ovlist);
    }
  this->PlaneClipEdge(&ivlist[(nverts-1)*3], &ivlist[0*3], pid, noverts, ovlist);
}

//--------------------------------------------------------------------------
//clips a line segment against the numbered plane.
//intersection point and the second vertex are added to overts if on or inside
void vtkFrustumExtractor::PlaneClipEdge(double *V0, double *V1, int pid, int &noverts, double *overts)
{
  double t = 0.0;
  double ISECT[3];
  int rc = vtkPlane::IntersectWithLine(
    V0, V1, 
    this->Frustum->GetNormals()->GetTuple(pid), 
    this->Frustum->GetPoints()->GetPoint(pid), 
    t, ISECT);

  if (rc)
    {
    overts[noverts*3+0] = ISECT[0];
    overts[noverts*3+1] = ISECT[1];
    overts[noverts*3+2] = ISECT[2];
    noverts++;
    }
  
  vtkPlane *plane = this->Frustum->GetPlane(pid);

  if (plane->EvaluateFunction(V1) < 0.0)
    {
    overts[noverts*3+0] = V1[0];
    overts[noverts*3+1] = V1[1];
    overts[noverts*3+2] = V1[2];
    noverts++;
    }

  return;
}

//----------------------------------------------------------------------------
void vtkFrustumExtractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Frustum: " 
     << (void *)this->Frustum << "\n";

  os << indent << "ClipPoints: " << this->ClipPoints << "\n";

  os << indent << "PassThrough: " 
     << (this->PassThrough ? "On\n" : "Off\n");

  os << indent << "ExactTest: "
     << this->ExactTest << "\n";
}
