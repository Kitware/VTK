/*=========================================================================

  Program:   Visualization Toolkit
  Module:  vtkDijkstraGraphGeodesicPath.cxx
  Language:  C++
  Date:    $Date$
  Version:   $Revision$
  
  Made by Rasmus Paulsen
  email:  rrp(at)imm.dtu.dk
  web:    www.imm.dtu.dk/~rrp/VTK

  This class is not mature enough to enter the official VTK release.
=========================================================================*/
#include "vtkDijkstraGraphGeodesicPath.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkExecutive.h"
#include "vtkMath.h"
#include "vtkIdList.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkCellArray.h"

vtkCxxRevisionMacro(vtkDijkstraGraphGeodesicPath, "1.1");
vtkStandardNewMacro(vtkDijkstraGraphGeodesicPath);

//----------------------------------------------------------------------------
vtkDijkstraGraphGeodesicPath::vtkDijkstraGraphGeodesicPath()
{
  this->IdList = vtkIdList::New();
  this->d    = vtkFloatArray::New();
  this->pre  = vtkIntArray::New();
  this->f    = vtkIntArray::New();
  this->s    = vtkIntArray::New();
  this->H    = vtkIntArray::New();
  this->p    = vtkIntArray::New();
  this->Hsize  = 0;
  this->StartVertex = 0;
  this->EndVertex   = 0;  
  this->StopWhenEndReached = 0;
  this->UseScalarWeights = 0;
  this->Adj = NULL;
  this->n = 0;
  this->AdjacencyGraphSize = 0;
}

//----------------------------------------------------------------------------
vtkDijkstraGraphGeodesicPath::~vtkDijkstraGraphGeodesicPath()
{
  if (this->IdList)
    this->IdList->Delete();
  if (this->d)
    this->d->Delete();
  if (this->pre)
    this->pre->Delete();
  if (this->f)
    this->f->Delete();
  if (this->s)
    this->s->Delete();
  if (this->H)
    this->H->Delete();
  if (this->p)
    this->p->Delete();
  
  DeleteAdjacency();
}

//----------------------------------------------------------------------------
int vtkDijkstraGraphGeodesicPath::RequestData(
  vtkInformation *           vtkNotUsed( request ),
  vtkInformationVector **    inputVector,
  vtkInformationVector *     outputVector) 
{
  vtkInformation * inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo =   outputVector->GetInformationObject(0);

  vtkPolyData *input = vtkPolyData::SafeDownCast(  
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input)
    {
    return 1;
    }

  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input)
    {
    return 1;
    }
  
  this->Initialize();
  this->ShortestPath(this->StartVertex, this->EndVertex);
  this->TraceShortestPath(input, output, this->StartVertex, this->EndVertex);
  
}

//----------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::Initialize()
{
  vtkPolyData *input = vtkPolyData::SafeDownCast(
      this->GetExecutive()->GetInputData(0, 0));

  this->BuildAdjacency( input );
  
  this->IdList->Reset();
  
  this->n = input->GetNumberOfPoints();
  
  this->d->SetNumberOfComponents(1);
  this->d->SetNumberOfTuples(this->n);
  this->pre->SetNumberOfComponents(1);
  this->pre->SetNumberOfTuples(this->n);
  this->f->SetNumberOfComponents(1);
  this->f->SetNumberOfTuples(this->n);
  this->s->SetNumberOfComponents(1);
  this->s->SetNumberOfTuples(this->n);
  this->p->SetNumberOfComponents(1);
  this->p->SetNumberOfTuples(this->n);
  
  // The heap has elements from 1 to n
  this->H->SetNumberOfComponents(1);
  this->H->SetNumberOfTuples(this->n+1);
  
  this->Hsize = 0;
}

//----------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::DeleteAdjacency()
{
  const int npoints = this->AdjacencyGraphSize;
  
  if (this->Adj)
    {
    for (int i = 0; i < npoints; i++)
      {
      this->Adj[i]->Delete();
      }
    delete [] this->Adj;
    }
  this->Adj = NULL;
}

//----------------------------------------------------------------------------
// The edge cost function should be implemented as a callback function to
// allow more advanced weighting
double vtkDijkstraGraphGeodesicPath::EdgeCost(
     vtkPolyData *pd, vtkIdType u, vtkIdType v)
{
  double p1[3];
  pd->GetPoint(u,p1);
  double p2[3];
  pd->GetPoint(v,p2);
  
  double w = sqrt(vtkMath::Distance2BetweenPoints(p1, p2));
  
  if (this->UseScalarWeights)
    {
    // Note this edge cost is not symmetric!
    vtkFloatArray *scalars = (vtkFloatArray*)pd->GetPointData()->GetScalars();
    //    float s1 = scalars->GetValue(u);
    float s2 = scalars->GetValue(v);
    
    if (s2)
      {
      w  /= (s2*s2);
      }
    }
  return w;
}


//----------------------------------------------------------------------------
// This is probably a horribly inefficient way to do it.
void vtkDijkstraGraphGeodesicPath::BuildAdjacency(vtkPolyData *pd)
{
  int i;
  
  int npoints = pd->GetNumberOfPoints();
  int ncells = pd->GetNumberOfCells();
  
  this->DeleteAdjacency();
  
  this->Adj = new vtkIdList*[npoints];

  // Remember size, so it can be deleted again
  this->AdjacencyGraphSize = npoints;

  for (i = 0; i < npoints; i++)
    {
    this->Adj[i] = vtkIdList::New();
    }
  
  for (i = 0; i < ncells; i++)
    {
    // Possible types
    //    VTK_VERTEX, VTK_POLY_VERTEX, VTK_LINE, 
    //    VTK_POLY_LINE,VTK_TRIANGLE, VTK_QUAD, 
    //    VTK_POLYGON, or VTK_TRIANGLE_STRIP.

    vtkIdType ctype = pd->GetCellType(i);
    
    // Until now only handle polys and triangles
    // TODO: All types
    if (ctype == VTK_POLYGON || ctype == VTK_TRIANGLE || ctype == VTK_LINE)
      {
      vtkIdType *pts;
      vtkIdType npts;
      pd->GetCellPoints (i, npts, pts);
      
      vtkIdType u = pts[0];
      vtkIdType v = pts[npts-1];
      
      Adj[u]->InsertUniqueId(v);
      Adj[v]->InsertUniqueId(u);
      for (int j = 0; j < npts-1; j++)
        {
        vtkIdType u = pts[j];
        vtkIdType v = pts[j+1];
        Adj[u]->InsertUniqueId(v);
        Adj[v]->InsertUniqueId(u);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::TraceShortestPath(
               vtkPolyData *inPd, vtkPolyData *outPd,
               vtkIdType startv, vtkIdType endv)
{
  vtkPoints   *points = vtkPoints::New();
  vtkCellArray *lines = vtkCellArray::New();
  
  // n is far to many. Adjusted later
  lines->InsertNextCell(this->n);
  
  // trace backward
  int npoints = 0;
  int v = endv;
  double pt[3];
  vtkIdType id;
  while (v != startv)
    {
    IdList->InsertNextId(v);
    
    inPd->GetPoint(v,pt);
    id = points->InsertNextPoint(pt);
    lines->InsertCellPoint(id);
    npoints++;
    
    v = this->pre->GetValue(v);
    }

  this->IdList->InsertNextId(v);
  
  inPd->GetPoint(v,pt);
  id = points->InsertNextPoint(pt);
  lines->InsertCellPoint(id);
  npoints++;
        
  lines->UpdateCellCount(npoints);
  outPd->SetPoints(points);
  points->Delete();
  outPd->SetLines(lines);
  lines->Delete();
}


//----------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::InitSingleSource(int startv)
{
  for (int v = 0; v < this->n; v++)
    {
    // d will be updated with first visit of vertex
    this->d->SetValue(v, -1);
    this->pre->SetValue(v, -1);
    this->s->SetValue(v, 0);
    this->f->SetValue(v, 0);
    }
  
  this->d->SetValue(startv, 0);
}


//----------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::Relax(int u, int v, double w)
{
  if (this->d->GetValue(v) > this->d->GetValue(u) + w)
    {
    this->d->SetValue(v, this->d->GetValue(u) + w);
    this->pre->SetValue(v, u);
    
    this->HeapDecreaseKey(v);
    }
}

//----------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::ShortestPath(int startv, int endv)
{
  vtkPolyData *input = vtkPolyData::SafeDownCast(
      this->GetExecutive()->GetInputData(0, 0));
  
  int i, u, v;
  
  this->InitSingleSource(startv);
  
  this->HeapInsert(startv);
  this->f->SetValue(startv, 1);
  
  int stop = 0;
  while ((u = this->HeapExtractMin()) >= 0 && !stop)
    {
    // u is now in s since the shortest path to u is determined
    this->s->SetValue(u, 1);
    // remove u from the front set
    this->f->SetValue(u, 0);
    
    if (u == endv && this->StopWhenEndReached)
      stop = 1;
    
    // Update all vertices v adjacent to u
    for (i = 0; i < this->Adj[u]->GetNumberOfIds(); i++)
      {
      v = this->Adj[u]->GetId(i);
      
      // s is the set of vertices with determined shortest path...do not use them again
      if (!this->s->GetValue(v))
        {
        // Only relax edges where the end is not in s and edge is in the front set
        double w = this->EdgeCost(input, u, v);
        
        if (this->f->GetValue(v))
        {
          Relax(u, v, w);
        }
        // add edge v to front set
        else
        {
          this->f->SetValue(v, 1);
          this->d->SetValue(v, this->d->GetValue(u) + w);
          
          // Set Predecessor of v to be u
          this->pre->SetValue(v, u);
          
          this->HeapInsert(v);
          }
        }
      }
    }
}


//----------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::Heapify(int i)
{
  // left node
  int l = i * 2;
  
  // right node
  int r = i * 2 + 1;
  
  int smallest = -1;
  
  // The value of element v is d(v)
  // the heap stores the vertex numbers
  if (   l <= this->Hsize 
      && (this->d->GetValue(this->H->GetValue(l)) < 
             this->d->GetValue(this->H->GetValue(i))))
    {
    smallest = l;
    }
  else
    {
    smallest = i;
    }
  
  if ( r <= this->Hsize && 
      (this->d->GetValue(this->H->GetValue(r)) < 
      this->d->GetValue(this->H->GetValue(smallest))))
    {
    smallest = r;
    }
  
  if (smallest != i)
    {
    int t = this->H->GetValue(i);
    
    this->H->SetValue(i, this->H->GetValue(smallest));
    
    // where is H(i)
    this->p->SetValue(this->H->GetValue(i), i);
    
    // H and p is kinda inverse
    this->H->SetValue(smallest, t);
    this->p->SetValue(t, smallest);
    
    this->Heapify(smallest);
    }
}

//----------------------------------------------------------------------------
// Insert vertex v. Weight is given in d(v)
// H has indices 1..n
void vtkDijkstraGraphGeodesicPath::HeapInsert(int v)
{
  if (this->Hsize >= this->H->GetNumberOfTuples()-1)
    return;
  
  this->Hsize++;
  int i = this->Hsize;
  
  while (i > 1 && 
         (this->d->GetValue(this->H->GetValue(i/2)) 
                   > this->d->GetValue(v)))
    {
    this->H->SetValue(i, this->H->GetValue(i/2));
    this->p->SetValue(this->H->GetValue(i), i);
    i /= 2;
    }
  // H and p is kinda inverse
  this->H->SetValue(i, v);
  this->p->SetValue(v, i);
}

//----------------------------------------------------------------------------
int vtkDijkstraGraphGeodesicPath::HeapExtractMin()
{
  if (this->Hsize == 0)
    return -1;
  
  int minv = this->H->GetValue(1);
  this->p->SetValue(minv, -1);
  
  this->H->SetValue(1, this->H->GetValue(this->Hsize));
  this->p->SetValue(this->H->GetValue(1), 1);
  
  this->Hsize--;
  this->Heapify(1);
  
  return minv;
}

//----------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::HeapDecreaseKey(int v)
{
  // where in H is vertex v
  int i = this->p->GetValue(v);
  if (i < 1 || i > this->Hsize)
    return;
  
  while (i > 1 && 
      this->d->GetValue(this->H->GetValue(i/2)) > this->d->GetValue(v))
    {
    this->H->SetValue(i, this->H->GetValue(i/2));
    this->p->SetValue(this->H->GetValue(i), i);
    i /= 2;
    }
  
  // H and p is kinda inverse
  this->H->SetValue(i, v);
  this->p->SetValue(v, i);
}

//----------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  // Add all members later
}

