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

vtkCxxRevisionMacro(vtkDijkstraGraphGeodesicPath, "1.5");
vtkStandardNewMacro(vtkDijkstraGraphGeodesicPath);

//----------------------------------------------------------------------------
vtkDijkstraGraphGeodesicPath::vtkDijkstraGraphGeodesicPath()
{
  this->IdList = vtkIdList::New();
  this->d    = vtkFloatArray::New();
  this->pre  = vtkIntArray::New();
  this->f    = vtkIntArray::New();
  this->s    = vtkIntArray::New();
  this->Heap = vtkIntArray::New();
  this->p    = vtkIntArray::New();
  this->HeapSize  = 0;
  this->StartVertex = 0;
  this->EndVertex   = 0;  
  this->StopWhenEndReached = 0;
  this->UseScalarWeights = 0;
  this->Adjacency = NULL;
  this->NumberOfVertices = 0;
  this->AdjacencyGraphSize = 0;
}

//----------------------------------------------------------------------------
vtkDijkstraGraphGeodesicPath::~vtkDijkstraGraphGeodesicPath()
{
  if (this->IdList)
    {
    this->IdList->Delete();
    }
  if (this->d)
    {
    this->d->Delete();
    }
  if (this->pre)
    {
    this->pre->Delete();
    }
  if (this->f)
    {
    this->f->Delete();
    }
  if (this->s)
    {
    this->s->Delete();
    }
  if (this->Heap)
    {
    this->Heap->Delete();
    }
  if (this->p)
    {
    this->p->Delete();
    }

  this->DeleteAdjacency();
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
    return 0;
    }

  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input)
    {
    return 0;
    }
  
  if ( this->AdjacencyBuildTime.GetMTime() < input->GetMTime() )
    {
    this->Initialize();
    }
  else
    {
    this->Reset();
    }
    
  this->ShortestPath(this->StartVertex, this->EndVertex);
  this->TraceShortestPath(input, output, this->StartVertex, this->EndVertex);
  return 1;
}

//----------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::Initialize()
{
  vtkPolyData *input = vtkPolyData::SafeDownCast(
      this->GetExecutive()->GetInputData(0, 0));

  this->BuildAdjacency( input );

  this->NumberOfVertices = input->GetNumberOfPoints();

  this->d->SetNumberOfComponents(1);
  this->d->SetNumberOfTuples(this->NumberOfVertices);
  this->pre->SetNumberOfComponents(1);
  this->pre->SetNumberOfTuples(this->NumberOfVertices);
  this->f->SetNumberOfComponents(1);
  this->f->SetNumberOfTuples(this->NumberOfVertices);
  this->s->SetNumberOfComponents(1);
  this->s->SetNumberOfTuples(this->NumberOfVertices);
  this->p->SetNumberOfComponents(1);
  this->p->SetNumberOfTuples(this->NumberOfVertices);

  // The heap has elements from 1 to n
  this->Heap->SetNumberOfComponents(1);
  this->Heap->SetNumberOfTuples(this->NumberOfVertices+1);
}

//----------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::Reset()
{
  this->IdList->Reset();
  this->HeapSize = 0;
}

//----------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::DeleteAdjacency()
{
  const int npoints = this->AdjacencyGraphSize;
  
  if (this->Adjacency)
    {
    for (int i = 0; i < npoints; i++)
      {
      this->Adjacency[i]->Delete();
      }
    delete [] this->Adjacency;
    }
  this->Adjacency = NULL;
}

//----------------------------------------------------------------------------
// The edge cost function should be implemented as a callback function to
// allow more advanced weighting
double vtkDijkstraGraphGeodesicPath::CalculateEdgeCost(
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
    
    double wt = ((double)(s2))*((double)(s2));
    if (wt != 0.0)
      {
      w  /= wt;
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
  
  this->Adjacency = new vtkIdList*[npoints];

  // Remember size, so it can be deleted again
  this->AdjacencyGraphSize = npoints;

  for (i = 0; i < npoints; i++)
    {
    this->Adjacency[i] = vtkIdList::New();
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
      
      this->Adjacency[u]->InsertUniqueId(v);
      this->Adjacency[v]->InsertUniqueId(u);
      for (int j = 0; j < npts-1; j++)
        {
        vtkIdType u1 = pts[j];
        vtkIdType v1 = pts[j+1];
        this->Adjacency[u1]->InsertUniqueId(v1);
        this->Adjacency[v1]->InsertUniqueId(u1);
        }
      }
    }
  this->AdjacencyBuildTime.Modified();
}

//----------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::TraceShortestPath(
               vtkPolyData *inPd, vtkPolyData *outPd,
               vtkIdType startv, vtkIdType endv)
{
  vtkPoints   *points = vtkPoints::New();
  vtkCellArray *lines = vtkCellArray::New();
  
  // n is far to many. Adjusted later
  lines->InsertNextCell(this->NumberOfVertices);
  
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
  for (int v = 0; v < this->NumberOfVertices; v++)
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
      {
      stop = 1;
      }
    
    // Update all vertices v adjacent to u
    for (i = 0; i < this->Adjacency[u]->GetNumberOfIds(); i++)
      {
      v = this->Adjacency[u]->GetId(i);
      
      // s is the set of vertices with determined shortest path...do not use them again
      if (!this->s->GetValue(v))
        {
        // Only relax edges where the end is not in s and edge is in the front set
        double w = this->CalculateEdgeCost(input, u, v);
        
        if (this->f->GetValue(v))
          {
          this->Relax(u, v, w);
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
  if (   l <= this->HeapSize 
      && (this->d->GetValue(this->Heap->GetValue(l)) < 
             this->d->GetValue(this->Heap->GetValue(i))))
    {
    smallest = l;
    }
  else
    {
    smallest = i;
    }
  
  if ( r <= this->HeapSize && 
      (this->d->GetValue(this->Heap->GetValue(r)) < 
      this->d->GetValue(this->Heap->GetValue(smallest))))
    {
    smallest = r;
    }
  
  if (smallest != i)
    {
    int t = this->Heap->GetValue(i);
    
    this->Heap->SetValue(i, this->Heap->GetValue(smallest));
    
    // where is Heap(i)
    this->p->SetValue(this->Heap->GetValue(i), i);

    // Heap and p are kind of inverses
    this->Heap->SetValue(smallest, t);
    this->p->SetValue(t, smallest);

    this->Heapify(smallest);
    }
}

//----------------------------------------------------------------------------
// Insert vertex v. Weight is given in d(v)
// H has indices 1..n
void vtkDijkstraGraphGeodesicPath::HeapInsert(int v)
{
  if (this->HeapSize >= this->Heap->GetNumberOfTuples()-1)
    return;

  this->HeapSize++;
  int i = this->HeapSize;

  while (i > 1 &&
         (this->d->GetValue(this->Heap->GetValue(i/2))
                   > this->d->GetValue(v)))
    {
    this->Heap->SetValue(i, this->Heap->GetValue(i/2));
    this->p->SetValue(this->Heap->GetValue(i), i);
    i /= 2;
    }
  // Heap and p are kind of inverses
  this->Heap->SetValue(i, v);
  this->p->SetValue(v, i);
}

//----------------------------------------------------------------------------
int vtkDijkstraGraphGeodesicPath::HeapExtractMin()
{
  if (this->HeapSize == 0)
    return -1;
  
  int minv = this->Heap->GetValue(1);
  this->p->SetValue(minv, -1);
  
  this->Heap->SetValue(1, this->Heap->GetValue(this->HeapSize));
  this->p->SetValue(this->Heap->GetValue(1), 1);
  
  this->HeapSize--;
  this->Heapify(1);
  
  return minv;
}

//----------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::HeapDecreaseKey(int v)
{
  // where in Heap is vertex v
  int i = this->p->GetValue(v);
  if (i < 1 || i > this->HeapSize)
    return;

  while (i > 1 &&
      this->d->GetValue(this->Heap->GetValue(i/2)) > this->d->GetValue(v))
    {
    this->Heap->SetValue(i, this->Heap->GetValue(i/2));
    this->p->SetValue(this->Heap->GetValue(i), i);
    i /= 2;
    }

  // Heap and p are kind of inverses
  this->Heap->SetValue(i, v);
  this->p->SetValue(v, i);
}

//----------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "StopWhenEndReached: ";
  if (this->StopWhenEndReached)
    {
    os << "On\n";
    }
  else
    {
    os << "Off\n";
    }
  os << indent << "Verts in input mesh: " << this->NumberOfVertices << endl;

  // Add all members later
  // this->d
  // this->pre
  // this->Adjacency
  // this->IdList
  // this->p
  // this->UseScalarWeights
  // this->AdjacencyGraphSize
  // this->HeapSize
  // this->s
  // this->Heap
  // this->f
}

