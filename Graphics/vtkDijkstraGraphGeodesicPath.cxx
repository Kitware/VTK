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

#include "vtkCellArray.h"
#include "vtkDijkstraGraphInternals.h"
#include "vtkExecutive.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"


vtkCxxRevisionMacro(vtkDijkstraGraphGeodesicPath, "1.9");
vtkStandardNewMacro(vtkDijkstraGraphGeodesicPath);
vtkCxxSetObjectMacro(vtkDijkstraGraphGeodesicPath,RepelVertices,vtkPoints);

//----------------------------------------------------------------------------
vtkDijkstraGraphGeodesicPath::vtkDijkstraGraphGeodesicPath()
{
  this->IdList = vtkIdList::New();
  this->Internals = new vtkDijkstraGraphInternals;
  this->HeapSize = 0;
  this->StopWhenEndReached = 0;
  this->UseScalarWeights = 0;
  this->NumberOfVertices = 0;
  this->RepelPathFromVertices = 0;
  this->RepelVertices = NULL;
}

//----------------------------------------------------------------------------
vtkDijkstraGraphGeodesicPath::~vtkDijkstraGraphGeodesicPath()
{
  if (this->IdList)
    {
    this->IdList->Delete();
    }
  if (this->Internals)
    {
    delete this->Internals;
    }
  this->SetRepelVertices(NULL);
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
  if (!output)
    {
    return 0;
    }
  
  if ( this->AdjacencyBuildTime.GetMTime() < input->GetMTime() )
    {
    this->Initialize( input );
    }
  else
    {
    this->Reset();
    }

  if (this->NumberOfVertices == 0)
    {
    return 0;
    }
    
  this->ShortestPath( input, this->StartVertex, this->EndVertex );
  this->TraceShortestPath( input, output, this->StartVertex, this->EndVertex );
  return 1;
}

//----------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::Initialize( vtkDataSet *inData )
{
  if( this->NumberOfVertices != inData->GetNumberOfPoints() )
    {
    this->NumberOfVertices = inData->GetNumberOfPoints();

    this->Internals->CumulativeWeights.resize( this->NumberOfVertices );
    this->Internals->Predecessors.resize( this->NumberOfVertices );
    this->Internals->OpenVertices.resize( this->NumberOfVertices );
    this->Internals->ClosedVertices.resize( this->NumberOfVertices );
    this->Internals->HeapIndices.resize( this->NumberOfVertices );
    this->Internals->Adjacency.resize( this->NumberOfVertices );
    this->Internals->BlockedVertices.resize( this->NumberOfVertices );

    // The heap has elements from 1 to n
    this->Internals->Heap.resize( this->NumberOfVertices + 1 );
    }

  this->Reset();
  this->BuildAdjacency( inData );
}

//----------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::Reset()
{
  vtkstd::fill( this->Internals->CumulativeWeights.begin(), 
    this->Internals->CumulativeWeights.end(), -1.0 );
  vtkstd::fill( this->Internals->Predecessors.begin(), 
    this->Internals->Predecessors.end(), -1 );
  vtkstd::fill( this->Internals->OpenVertices.begin(), 
    this->Internals->OpenVertices.end(), false );
  vtkstd::fill( this->Internals->ClosedVertices.begin(), 
    this->Internals->ClosedVertices.end(), false );
  vtkstd::fill( this->Internals->BlockedVertices.begin(), 
    this->Internals->BlockedVertices.end(), false );

  this->IdList->Reset();
  this->HeapSize = 0;
}

//----------------------------------------------------------------------------
double vtkDijkstraGraphGeodesicPath::CalculateStaticEdgeCost(
     vtkDataSet *inData, vtkIdType u, vtkIdType v)
{
  double p1[3];
  inData->GetPoint(u,p1);
  double p2[3];
  inData->GetPoint(v,p2);
  
  double w = sqrt(vtkMath::Distance2BetweenPoints(p1, p2));
  
  if (this->UseScalarWeights)
    {
    // Note this edge cost is not symmetric!
    vtkFloatArray *scalars =
      static_cast<vtkFloatArray*>(inData->GetPointData()->GetScalars());
    //    float s1 = scalars->GetValue(u);
    double s2 = static_cast<double>(scalars->GetValue(v));
    
    double wt = s2*s2;
    if (wt != 0.0)
      {
      w  /= wt;
      }
    }
  return w;
}

//----------------------------------------------------------------------------
// This is probably a horribly inefficient way to do it.
void vtkDijkstraGraphGeodesicPath::BuildAdjacency(vtkDataSet *inData)
{
  vtkPolyData *pd = vtkPolyData::SafeDownCast( inData );
  int ncells = pd->GetNumberOfCells();

  vtkstd::pair<vtkstd::map<int,double>::iterator,bool> ret;

  for ( int i = 0; i < ncells; i++)
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
      pd->GetCellPoints(i, npts, pts);
      
      vtkIdType u = pts[0];
      vtkIdType v = pts[npts-1];
      
      double cost = this->CalculateStaticEdgeCost( inData, u, v );
      ret = this->Internals->Adjacency[u].insert( vtkstd::pair<int,double>( v, cost ) );
      if ( !ret.second )
        {
        this->Internals->Adjacency[u][v] = cost;
        }

      cost = this->CalculateStaticEdgeCost( inData, v, u );
      ret = this->Internals->Adjacency[v].insert( vtkstd::pair<int,double>( u, cost ) );
      if ( !ret.second )
        {
        this->Internals->Adjacency[v][u] = cost;
        }

      for (int j = 0; j < npts-1; j++)
        {
        u = pts[j];
        v = pts[j+1];
        cost = this->CalculateStaticEdgeCost( inData, u, v );
        ret = this->Internals->Adjacency[u].insert( vtkstd::pair<int,double>( v, cost ) );
        if ( !ret.second )
          {
          this->Internals->Adjacency[u][v] = cost;
          }

        cost = this->CalculateStaticEdgeCost( inData, v, u );
        ret = this->Internals->Adjacency[v].insert( vtkstd::pair<int,double>( u, cost ) );
        if ( !ret.second )
          {
          this->Internals->Adjacency[v][u] = cost;
          }
        }
      }
    }

  this->AdjacencyBuildTime.Modified();
}

//----------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::TraceShortestPath(
               vtkDataSet *inData, vtkPolyData *outPoly,
               vtkIdType startv, vtkIdType endv)
{
  vtkPoints   *points = vtkPoints::New();
  vtkCellArray *lines = vtkCellArray::New();
  
  // n is far to many. Adjusted later
  lines->InsertNextCell(this->NumberOfVertices);
  
  // trace backward
  int v = endv;
  double pt[3];
  vtkIdType id;
  while (v != startv)
    {
    IdList->InsertNextId(v);
    
    inData->GetPoint(v,pt);
    id = points->InsertNextPoint(pt);
    lines->InsertCellPoint(id);
    
    v = this->Internals->Predecessors[v];
    }

  this->IdList->InsertNextId(v);
  
  inData->GetPoint(v,pt);
  id = points->InsertNextPoint(pt);
  lines->InsertCellPoint(id);
        
  lines->UpdateCellCount( points->GetNumberOfPoints() );
  outPoly->SetPoints(points);
  points->Delete();
  outPoly->SetLines(lines);
  lines->Delete();
}

//----------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::Relax(int u, int v, double w)
{
  double du = this->Internals->CumulativeWeights[u] + w;
  if (this->Internals->CumulativeWeights[v] > du)
    {
    this->Internals->CumulativeWeights[v] = du;
    this->Internals->Predecessors[v] = u;
    
    this->HeapDecreaseKey(v);
    }
}

//----------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::ShortestPath( vtkDataSet *inData,
                                                int startv, int endv )
{
  int u, v;

  if( this->RepelPathFromVertices && this->RepelVertices )
    {
    // loop over the pts and if they are in the image
    // get the associated index for that point and mark it as blocked
    for( int i = 0; i < this->RepelVertices->GetNumberOfPoints(); ++i )
      {
        double* pt = this->RepelVertices->GetPoint( i );
        u = inData->FindPoint( pt );
        if ( u < 0 || u == startv || u == endv ) 
          {
          continue;
          }
        this->Internals->BlockedVertices[u] = true;
      }
    }

  this->Internals->CumulativeWeights[startv] = 0;
  
  this->HeapInsert(startv);
  this->Internals->OpenVertices[startv] = true;
  
  bool stop = false;
  while ((u = this->HeapExtractMin()) >= 0 && !stop)
    {
    // u is now in s since the shortest path to u is determined
    this->Internals->ClosedVertices[u] = true;
    // remove u from the front set
    this->Internals->OpenVertices[u] = false;
    
    if (u == endv && this->StopWhenEndReached)
      {
      stop = true;
      }
    
    vtkstd::map<int,double>::iterator it = this->Internals->Adjacency[u].begin();
     
    // Update all vertices v adjacent to u
    for ( ; it != this->Internals->Adjacency[u].end(); ++it )
      {
      v = (*it).first;
      
      // s is the set of vertices with determined shortest path...do not use them again
      if ( !this->Internals->ClosedVertices[v] )
        {
        // Only relax edges where the end is not in s and edge is in the front set
        double w; 
        if ( this->Internals->BlockedVertices[v] )
        {
          w = VTK_FLOAT_MAX;
        }
        else
        {
          w = (*it).second + this->CalculateDynamicEdgeCost( inData, u, v );
        }
        
        if ( this->Internals->OpenVertices[v] )
          {
          this->Relax(u, v, w);
          }
        // add edge v to front set
        else
          {
          this->Internals->OpenVertices[v] = true;
          this->Internals->CumulativeWeights[v] = this->Internals->CumulativeWeights[u] + w;

          // Set Predecessor of v to be u
          this->Internals->Predecessors[v] = u;

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
  unsigned int l = i * 2;
  
  // right node
  unsigned int r = i * 2 + 1;
  
  int smallest = -1;
  
  // The value of element v is CumulativeWeights(v)
  // the heap stores the vertex numbers
  if ( l <= this->HeapSize && 
      ( this->Internals->CumulativeWeights[ this->Internals->Heap[l] ] < 
        this->Internals->CumulativeWeights[ this->Internals->Heap[i] ] ) )
    {
    smallest = l;
    }
  else
    {
    smallest = i;
    }
  
  if ( r <= this->HeapSize && 
      ( this->Internals->CumulativeWeights[ this->Internals->Heap[ r ] ] < 
        this->Internals->CumulativeWeights[ this->Internals->Heap[ smallest ] ] ) )
    {
    smallest = r;
    }
  
  if ( smallest != i )
    {
    int t = this->Internals->Heap[i];
    
    this->Internals->Heap[ i ] = this->Internals->Heap[ smallest ];
    
    // where is Heap(i)
    this->Internals->HeapIndices[ this->Internals->Heap[i] ] = i;
    
    // Heap and HeapIndices are kinda inverses
    this->Internals->Heap[ smallest ] = t;
    this->Internals->HeapIndices[ t ] = smallest;
    
    this->Heapify( smallest );
    }
}

//----------------------------------------------------------------------------
// Insert vertex v. Weight is given in CumulativeWeights(v)
// Heap has indices 1..n
void vtkDijkstraGraphGeodesicPath::HeapInsert(int v)
{
  if ( this->HeapSize >= (this->Internals->Heap.size() - 1) )
    {
    return;
    }
  
  this->HeapSize++;
  int i = this->HeapSize;
  
  while ( i > 1 && 
         ( this->Internals->CumulativeWeights[ this->Internals->Heap[i/2] ] > this->Internals->CumulativeWeights[v] ) )
    {
    this->Internals->Heap[ i ] = this->Internals->Heap[i/2];
    this->Internals->HeapIndices[ this->Internals->Heap[i] ] = i;
    i /= 2;
    }
   // Heap and HeapIndices are kinda inverses
  this->Internals->Heap[ i ] = v;
  this->Internals->HeapIndices[ v ] = i;
}

//----------------------------------------------------------------------------
int vtkDijkstraGraphGeodesicPath::HeapExtractMin()
{
  if ( this->HeapSize == 0 )
    {
    return -1;
    }
  
  int minv = this->Internals->Heap[ 1 ];
  this->Internals->HeapIndices[ minv ] = -1;
  
  this->Internals->Heap[ 1 ] = this->Internals->Heap[ this->HeapSize ];
  this->Internals->HeapIndices[ this->Internals->Heap[1] ]= 1;
  
  this->HeapSize--;
  this->Heapify( 1 );
  
  return minv;
}

//----------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::HeapDecreaseKey(int v)
{
  // where in Heap is vertex v
  int i = this->Internals->HeapIndices[ v ];
  if ( i < 1 || i > static_cast<int>(this->HeapSize) )
    {
    return;
    }
  
  while ( i > 1 && 
          this->Internals->CumulativeWeights[ this->Internals->Heap[ i/2 ] ] > this->Internals->CumulativeWeights[ v ] )
    {
    this->Internals->Heap[ i ] = this->Internals->Heap[i/2];
    this->Internals->HeapIndices[ this->Internals->Heap[i] ] = i;
    i /= 2;
    }
  
  // Heap and HeapIndices are kinda inverses
  this->Internals->Heap[ i ] = v;
  this->Internals->HeapIndices[ v ] = i;
}

//----------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "StopWhenEndReached: ";
  if ( this->StopWhenEndReached )
    {
    os << "On\n";
    }
  else
    {
    os << "Off\n";
    }
  os << indent << "UseScalarWeights: ";
  if ( this->UseScalarWeights )
    {
    os << "On\n";
    }
  else
    {
    os << "Off\n";
    }
  os << indent << "RepelPathFromVertices: ";
  if ( this->RepelPathFromVertices )
    {
    os << "On\n";
    }
  else
    {
    os << "Off\n";
    }
  os << indent << "Number of vertices in input data: " << this->NumberOfVertices << endl;
}
