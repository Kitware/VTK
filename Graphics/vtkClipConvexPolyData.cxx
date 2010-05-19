/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClipConvexPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkClipConvexPolyData.h"

#include "vtkCellArray.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPlaneCollection.h"
#include "vtkPolyData.h"

#include <vtkstd/vector>
#include <vtkstd/set>
#include <vtkstd/algorithm>
#include <vtkstd/iterator>

vtkStandardNewMacro(vtkClipConvexPolyData);

vtkCxxSetObjectMacro(vtkClipConvexPolyData,Planes,vtkPlaneCollection);

// ----------------------------------------------------------------------------
class vtkCCPDVertex
{
public:
  double Point[3];
};

// ----------------------------------------------------------------------------
class vtkCCPDPolygon
{
public:
  vtkstd::vector<vtkCCPDVertex*> Vertices;
  vtkstd::vector<vtkCCPDVertex*> NewVertices;
};

// ----------------------------------------------------------------------------
class vtkClipConvexPolyDataInternals
{
public:
  vtkstd::vector<vtkCCPDPolygon*> Polygons;
};

// ----------------------------------------------------------------------------
// Construct an the hull object with no planes
vtkClipConvexPolyData::vtkClipConvexPolyData()
{
  this->Planes=NULL;
  this->Internal=new vtkClipConvexPolyDataInternals;
  
}

// ----------------------------------------------------------------------------
// Destructor for a hull object - remove the planes if necessary
vtkClipConvexPolyData::~vtkClipConvexPolyData()
{
  this->SetPlanes(NULL);
  this->ClearInternals();
  delete this->Internal;
}

// ----------------------------------------------------------------------------
// Description:
// Redefines this method, as this filter depends on time of its components
// (planes)
unsigned long int vtkClipConvexPolyData::GetMTime()
{
  unsigned long int result=Superclass::GetMTime();
  if(this->Planes!=0)
    {
    unsigned long int planesTime=this->Planes->GetMTime();
    if(planesTime>result)
      {
      result=planesTime;
      }
    }
  return result;
}

// ----------------------------------------------------------------------------
void vtkClipConvexPolyData::ClearInternals()
{
  unsigned int j;
  for(unsigned int i=0; i<this->Internal->Polygons.size(); i++)
    {
    for (j=0; j<this->Internal->Polygons[i]->Vertices.size(); j++)
      {
      delete this->Internal->Polygons[i]->Vertices[j];
      }
    this->Internal->Polygons[i]->Vertices.clear();

    for (j=0; j<this->Internal->Polygons[i]->NewVertices.size(); j++)
      {
      delete this->Internal->Polygons[i]->NewVertices[j];
      }
    this->Internal->Polygons[i]->NewVertices.clear();

    
    delete this->Internal->Polygons[i];
    }
  this->Internal->Polygons.clear(); 
}

// ----------------------------------------------------------------------------
void vtkClipConvexPolyData::ClearNewVertices()
{
  for(unsigned int i=0; i<this->Internal->Polygons.size(); i++)
    {
    for(unsigned int j=0; j<this->Internal->Polygons[i]->NewVertices.size();
        j++)
      {
      delete this->Internal->Polygons[i]->NewVertices[j];
      }
    this->Internal->Polygons[i]->NewVertices.clear();
    }
}

// ----------------------------------------------------------------------------
void vtkClipConvexPolyData::RemoveEmptyPolygons()
{
  bool done = false;
  
  while (!done )
    {
    done = true;
    for(unsigned int i=0; i<this->Internal->Polygons.size(); i++)
      {
      if ( this->Internal->Polygons[i]->Vertices.size() == 0 )
        {
        vtkstd::vector<vtkCCPDPolygon*>::iterator where =
          vtkstd::find(this->Internal->Polygons.begin(),
               this->Internal->Polygons.end(),
               this->Internal->Polygons[i]);
        if ( where != this->Internal->Polygons.end() )
          {
          delete this->Internal->Polygons[i];
          this->Internal->Polygons.erase(where);
          done = false;
          break;
          }
        }
      }
    }
}

// ----------------------------------------------------------------------------
// Create the n-sided convex hull from the input geometry according to the
// set of planes.
int vtkClipConvexPolyData::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // Pre-conditions
  if(this->Planes==0)
    {
    vtkErrorMacro("plane collection is null");
    return 0;
    }
  if(this->Planes->GetNumberOfItems()==0)
    {
    vtkErrorMacro("plane collection is empty");
    return 0;
    }
  
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  vtkCellArray *polys = input->GetPolys();
  vtkPoints *points = input->GetPoints();

  // Compute tolerance to be 0.00001 of diagonal
  double min[3] = {VTK_DOUBLE_MAX, VTK_DOUBLE_MAX, VTK_DOUBLE_MAX};
  double max[3] = {VTK_DOUBLE_MIN, VTK_DOUBLE_MIN, VTK_DOUBLE_MIN};
  
  size_t i, j;
  double tolerance;
  for ( i = 0; i < static_cast<size_t>(points->GetNumberOfPoints()); i++ )
    {
    double pt[3];
    points->GetPoint(static_cast<vtkIdType>(i), pt);
    for ( j = 0; j < 3; j++ )
      {
      min[j] = (pt[j] < min[j])?(pt[j]):(min[j]);
      max[j] = (pt[j] > max[j])?(pt[j]):(max[j]);
      }
    }
  tolerance = sqrt(vtkMath::Distance2BetweenPoints(min,max))*0.00001;

  vtkIdType npts;
  vtkIdType *pts;
  polys->InitTraversal();
  while ( polys->GetNextCell(npts, pts) )
    {
    vtkCCPDPolygon *polygon = new vtkCCPDPolygon;
    for ( i = 0; i < static_cast<size_t>(npts); i++ )
      {
      vtkCCPDVertex *v = new vtkCCPDVertex;
      points->GetPoint(pts[i], v->Point);
      polygon->Vertices.push_back(v);
      }
    this->Internal->Polygons.push_back(polygon);
    }

  this->Planes->InitTraversal();
  vtkPlane *plane;

  while ( (plane = this->Planes->GetNextItem()) )
    {
    if ( !this->HasDegeneracies( plane ) )
      {
      this->ClipWithPlane( plane, tolerance );
      }
    }

  // Create a new set of points and polygons into which the results will
  // be stored
  vtkPoints *outPoints = vtkPoints::New();
  vtkCellArray *outPolys  = vtkCellArray::New();

  for ( i = 0; i < this->Internal->Polygons.size(); i++ )
    {
    size_t numPoints = this->Internal->Polygons[i]->Vertices.size();
    vtkIdType *polyPts = new vtkIdType[numPoints];
    for ( j = 0; j < numPoints; j++ )
      {
      polyPts[j] = outPoints->InsertNextPoint(
        this->Internal->Polygons[i]->Vertices[j]->Point );
      }
    outPolys->InsertNextCell(static_cast<vtkIdType>(numPoints), polyPts );
    delete [] polyPts;
    }

  // Set the output vertices and polygons
  output->SetPoints(outPoints);
  output->SetPolys(outPolys);

  // Delete the temporary storage
  outPoints->Delete();
  outPolys->Delete();

  this->ClearInternals();
  
  return 1;
}

// ----------------------------------------------------------------------------
void vtkClipConvexPolyData::ClipWithPlane( vtkPlane *plane, double tolerance )
{
  double origin[3];
  double normal[4];
  
  plane->GetOrigin( origin );
  plane->GetNormal( normal );
  
  vtkMath::Normalize(normal);

  normal[3] = -(origin[0]*normal[0] +
                origin[1]*normal[1] +
                origin[2]*normal[2]);
  
  int numNewPoints = 0;
  unsigned int i;
  size_t j;
  // For each polygon
  for ( i = 0; i < this->Internal->Polygons.size(); i++ )
    {
    // the new polygons
    vtkCCPDPolygon *newPoly = new vtkCCPDPolygon;
    
    // For each vertex
    size_t numVertices = this->Internal->Polygons[i]->Vertices.size();
    for ( j = 0; j < numVertices; j++ )
      {
      double *p1=this->Internal->Polygons[i]->Vertices[j]->Point;
      double *p2=
        this->Internal->Polygons[i]->Vertices[(j+1)%numVertices]->Point;
      
      double p1D=p1[0]*normal[0]+p1[1]*normal[1]+p1[2]*normal[2] + normal[3];
      double p2D=p2[0]*normal[0]+p2[1]*normal[1]+p2[2]*normal[2] + normal[3];

      // Add p1 in if it is not clipped
      if ( p1D > 0 )
        {
        vtkCCPDVertex *v = new vtkCCPDVertex;
        v->Point[0] = p1[0];
        v->Point[1] = p1[1];
        v->Point[2] = p1[2];
        newPoly->Vertices.push_back(v);
        }
      
      // If the plane clips this edge - find the crossing point
      if ( p1D*p2D <= 0 )
        {
        double w = -p1D / (p2D - p1D);
        
        vtkCCPDVertex *v = new vtkCCPDVertex;
        v->Point[0] = p1[0] + w * (p2[0]-p1[0]);
        v->Point[1] = p1[1] + w * (p2[1]-p1[1]);
        v->Point[2] = p1[2] + w * (p2[2]-p1[2]);
        newPoly->Vertices.push_back(v);

        v = new vtkCCPDVertex;
        v->Point[0] = p1[0] + w * (p2[0]-p1[0]);
        v->Point[1] = p1[1] + w * (p2[1]-p1[1]);
        v->Point[2] = p1[2] + w * (p2[2]-p1[2]);
        this->Internal->Polygons[i]->NewVertices.push_back(v);
        numNewPoints++;
        }
      }
    
    // Remove the current polygon
    for ( j = 0; j < numVertices; j++ )
      {
      delete this->Internal->Polygons[i]->Vertices[j];
      }
    this->Internal->Polygons[i]->Vertices.clear();
    
    // copy in the new polygon
    numVertices = newPoly->Vertices.size();
    
    if ( numVertices > 0 )
      {
      for ( j = 0; j < numVertices; j++ )
        {
        this->Internal->Polygons[i]->Vertices.push_back(newPoly->Vertices[j]);
        }
      newPoly->Vertices.clear();
      }
    delete newPoly;
    }
  
  if ( numNewPoints )
    {
    if ( numNewPoints < 6 )
      {
      vtkErrorMacro(<< "Failure - not enough new points");
      return;
      }
  
    // Check that all new arrays contain exactly 0 or 2 points
    for ( i = 0; i < this->Internal->Polygons.size(); i++ )
      {
      if ( this->Internal->Polygons[i]->NewVertices.size() != 0 &&
           this->Internal->Polygons[i]->NewVertices.size() != 2 )
        {
        vtkErrorMacro( << "Horrible error - we have " <<
                       this->Internal->Polygons[i]->NewVertices.size()
                       << " crossing points");
        return;
        }
      }
    
    // Find the first polygon with a new point
    size_t idx = 0;
    bool idxFound=false;
    
    for ( i = 0; !idxFound && i < this->Internal->Polygons.size(); i++ )
      {
      idxFound=this->Internal->Polygons[i]->NewVertices.size() > 0;
      if(idxFound)
        {
        idx=i;
        }
      }
    
    if (!idxFound)
      {
      vtkErrorMacro( << "Couldn't find any new vertices!");
      return;
      }
    
    // the new polygon
    vtkCCPDPolygon *newPoly = new vtkCCPDPolygon;
    vtkCCPDVertex *v = new vtkCCPDVertex;
    v->Point[0] = this->Internal->Polygons[idx]->NewVertices[0]->Point[0];
    v->Point[1] = this->Internal->Polygons[idx]->NewVertices[0]->Point[1];
    v->Point[2] = this->Internal->Polygons[idx]->NewVertices[0]->Point[2];
    newPoly->Vertices.push_back(v);
    
    v = new vtkCCPDVertex;
    v->Point[0] = this->Internal->Polygons[idx]->NewVertices[1]->Point[0];
    v->Point[1] = this->Internal->Polygons[idx]->NewVertices[1]->Point[1];
    v->Point[2] = this->Internal->Polygons[idx]->NewVertices[1]->Point[2];
    newPoly->Vertices.push_back(v);
    
    double lastPoint[3];
    lastPoint[0] = this->Internal->Polygons[idx]->NewVertices[1]->Point[0];
    lastPoint[1] = this->Internal->Polygons[idx]->NewVertices[1]->Point[1];
    lastPoint[2] = this->Internal->Polygons[idx]->NewVertices[1]->Point[2];
    
    size_t lastPointIdx = idx;
    size_t subIdx;
    
    while ( static_cast<int>(newPoly->Vertices.size()) < numNewPoints / 2 )
      {
      // Find the index of a polygon match the lastPoint but
      // not the lastPointIdx
      subIdx = 0;
      bool foundSubIdx=false;
      for ( i = 0; i < this->Internal->Polygons.size(); i++ )
        {
          if ( i != lastPointIdx &&
               this->Internal->Polygons[i]->NewVertices.size() > 0 )
          {
          for ( j = 0; j < 2; j++ )
            {
            if ( vtkMath::Distance2BetweenPoints(lastPoint,
                   this->Internal->Polygons[i]->NewVertices[j]->Point)
                 < tolerance )
              {
              idx = i;
              subIdx =j;
              foundSubIdx=true;
              break;
              }
            }
          if ( foundSubIdx)
            {
            break;
            }
          }
        }
      
      v = new vtkCCPDVertex;
      v->Point[0] =
        this->Internal->Polygons[idx]->NewVertices[(subIdx+1)%2]->Point[0];
      v->Point[1] =
        this->Internal->Polygons[idx]->NewVertices[(subIdx+1)%2]->Point[1];
      v->Point[2] =
        this->Internal->Polygons[idx]->NewVertices[(subIdx+1)%2]->Point[2];
      newPoly->Vertices.push_back(v);

      lastPoint[0] =
        this->Internal->Polygons[idx]->NewVertices[(subIdx+1)%2]->Point[0];
      lastPoint[1] =
        this->Internal->Polygons[idx]->NewVertices[(subIdx+1)%2]->Point[1];
      lastPoint[2] =
        this->Internal->Polygons[idx]->NewVertices[(subIdx+1)%2]->Point[2];
      lastPointIdx = idx;
      }
    // check to see that the polygon vertices are in the right order.
    // cross product of p1p2 and p3p2 should point in the same direction
    // as the plane normal. Otherwise, reverse the order
    double *p1 = newPoly->Vertices[0]->Point;
    double *p2 = newPoly->Vertices[1]->Point;
    double *p3 = newPoly->Vertices[2]->Point;
    double v1[3];
    double v2[3];
    double cross[3];
    v1[0] = p1[0] - p2[0];
    v1[1] = p1[1] - p2[1];
    v1[2] = p1[2] - p2[2];
    v2[0] = p3[0] - p2[0];
    v2[1] = p3[1] - p2[1];
    v2[2] = p3[2] - p2[2];
    vtkMath::Cross(v1,v2,cross);
    vtkMath::Normalize(cross);
    if ( vtkMath::Dot(cross,normal) < 0 )
      {
      vtkstd::reverse(newPoly->Vertices.begin(), newPoly->Vertices.end());
      }
    this->Internal->Polygons.push_back(newPoly);
    } 
  this->RemoveEmptyPolygons();
  this->ClearNewVertices();
}

// ----------------------------------------------------------------------------
bool vtkClipConvexPolyData::HasDegeneracies(vtkPlane *plane)
{
  double origin[3];
  double normal[4];
  
  plane->GetOrigin( origin );
  plane->GetNormal( normal );
  
  normal[3] = -(origin[0]*normal[0] +
                origin[1]*normal[1] +
                origin[2]*normal[2]);
  
  unsigned int i;
  size_t j;
  // For each polygon
  int totalNumNewVertices = 0;
  for ( i = 0; i < this->Internal->Polygons.size(); i++ )
    {
    // For each vertex
    size_t numVertices = this->Internal->Polygons[i]->Vertices.size();
    int numNewVertices = 0;
    for ( j = 0; j < numVertices; j++ )
      {
      double *p1 = this->Internal->Polygons[i]->Vertices[j]->Point;
      double *p2 =
        this->Internal->Polygons[i]->Vertices[(j+1)%numVertices]->Point;
      
      double p1D=p1[0]*normal[0]+p1[1]*normal[1]+p1[2]*normal[2]+normal[3];
      double p2D=p2[0]*normal[0]+p2[1]*normal[1]+p2[2]*normal[2]+normal[3];

      // If the plane clips this edge - find the crossing point
      if ( p1D*p2D <= 0 )
        {
        numNewVertices++;
        }
      }
    if ( numNewVertices != 0 && numNewVertices != 2)
      {
      return true;
      }    
    totalNumNewVertices += numNewVertices;
    }  
  
  if ( totalNumNewVertices < 6 )
    {
    return true;
    }
  return false;
}

// ----------------------------------------------------------------------------
void vtkClipConvexPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Planes: " << this->Planes << endl;
}
