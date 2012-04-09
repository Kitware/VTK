/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTessellatedBoxSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTessellatedBoxSource.h"
#include "vtkObjectFactory.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkCellArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <assert.h>

vtkStandardNewMacro(vtkTessellatedBoxSource);

// ----------------------------------------------------------------------------
vtkTessellatedBoxSource::vtkTessellatedBoxSource()
{
  this->Bounds[0]=-0.5;
  this->Bounds[1]=0.5;
  this->Bounds[2]=-0.5;
  this->Bounds[3]=0.5;
  this->Bounds[4]=-0.5;
  this->Bounds[5]=0.5;

  this->Level=0;
  this->DuplicateSharedPoints=0;
  this->Quads=0;

  this->SetNumberOfInputPorts(0); // this is a source.
}

// ----------------------------------------------------------------------------
vtkTessellatedBoxSource::~vtkTessellatedBoxSource()
{
}

//----------------------------------------------------------------------------
//
int vtkTessellatedBoxSource::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX(),
               this->Bounds,6);
  return 1;
}

// Duplicate point method.
// Each face of the box is defined by the 3 points: an origin, a point along
// a first axis, a point along a second axis. The axes are oriented
// counterclockwise.
// Point id use voxel numbering, not hexahedron numbering.
// voxelPointId=boundingBoxQuads[faceId][facePointId]
// facePointId=0 : origin
// facePointId=1 : point along the first axis
// facePointId=2 : point along the second axis
static int boundingBoxQuads[6][3]={{0,4,2}, // -x face
                                   {5,1,7}, // +x face
                                   {0,1,4}, // -y face
                                   {6,7,2}, // +y face
                                   {1,0,3}, // -z face
                                   {4,5,6}};// +z face

// Minimal number of points method.
// Each  edge of the box is defined by two vertices in increasing id order.
// vertexId=edges[edge][lowestId=0, highestId=1]
static int edges[12][2]={{0,1}, // 0
                         {0,2}, // 1
                         {0,4}, // 2
                         {1,3}, // 3
                         {1,5}, // 4
                         {2,3}, // 5
                         {2,6}, // 6
                         {3,7}, // 7
                         {4,5}, // 8
                         {4,6}, // 9
                         {5,7}, // 10
                         {6,7}}; // 11

// Minimal number of points method.
// signed (edge id+1)=faces[face][edge]
// +1 because we cannot encode -0....
static int faces[6][4]={{3,10,-7,-2}, // 0: -x face
                        {-5,4,8,-11}, // 1: +x face
                        {1,5,-9,-3}, // 2: -y face
                        {12,-8,-6,7}, // 3: +y face
                        {-1,2,6,-4}, // 4: -z face
                        {9,11,-12,-10}}; // 5: +z face

// ----------------------------------------------------------------------------
// Description:
// Called by the superclass. Actual creation of the points and cells
// happens here.
int vtkTessellatedBoxSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo=outputVector->GetInformationObject(0);

  // get the output
  vtkPolyData *output =
    vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // get the bounds
  double bounds[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX(),bounds);

  vtkPoints *points=output->GetPoints();
  if(points==0)
    {
    points = vtkPoints::New();
    output->SetPoints(points);
    points->Delete();
    }

  // Always create a new vtkCellArray, otherwise it uses the this->Dummy of
  // vtkPolyData...
  vtkCellArray *polys = vtkCellArray::New();
  output->SetPolys(polys);
  polys->Delete();

  if(this->DuplicateSharedPoints)
    {
    this->DuplicateSharedPointsMethod(bounds,points,polys);
    }
  else
    {
    this->MinimalPointsMethod(bounds,points,polys);
    }
  return 1;
}

void vtkTessellatedBoxSource::DuplicateSharedPointsMethod(
  double *bounds,
  vtkPoints *points,
  vtkCellArray *polys)
{
  int numberOfPoints=(this->Level+2)*(this->Level+2)*6;
  int changed=points->GetNumberOfPoints()!=numberOfPoints;

  if(changed)
    {
    // Topology changed.
    points->SetNumberOfPoints(numberOfPoints);
    polys->Initialize();
    }

  // Iterate over the 6 faces.
  double facePoints[3][3]; // 3 points of 3 coordinates

  int face=0;
  vtkIdType firstPointId=0;
  while(face<6)
    {
    int i=0;
    while(i<3)
      {
      int pointId=boundingBoxQuads[face][i];
      facePoints[i][0]=bounds[pointId&1];
      facePoints[i][1]=bounds[2+((pointId>>1)&1)];
      facePoints[i][2]=bounds[4+((pointId>>2)&1)];
      ++i;
      }
    this->BuildFace(points,polys,firstPointId,facePoints,changed);
    firstPointId+=(this->Level+2)*(this->Level+2);
    ++face;
    }
}

// ----------------------------------------------------------------------------
void vtkTessellatedBoxSource::MinimalPointsMethod(double *bounds,
                                                    vtkPoints *points,
                                                    vtkCellArray *polys)
{
  int numberOfInternalPointsPerEdge=this->Level;
  int numberOfInternalPointsPerFace=this->Level*this->Level;
  int numberOfPoints=8+12*numberOfInternalPointsPerEdge
    +6*numberOfInternalPointsPerFace;

  int changed=points->GetNumberOfPoints()!=numberOfPoints;
  if(changed)
    {
    // Topology changed.
    points->SetNumberOfPoints(numberOfPoints);
    polys->Initialize();
    }

  // Compute point coordinates.

  // First the 8 vertices
  // Then the 12 * (level) internal edge points (static list)
  // Then the 6 * (level*level) internal face points (static list)

  // The 8 vertices, voxel numbering: id=4*k+2*j+i
  int i=0;
  double p[3];
  while(i<8)
    {
    p[0]=bounds[i&1];
    p[1]=bounds[2+((i>>1)&1)];
    p[2]=bounds[4+((i>>2)&1)];
    points->SetPoint(i,p);
    ++i;
    }

  // The 12*(level) internal edge points
  // edges are describe by there lowest vertex Id and highest vertex Id.
  // Numbering start from edges with vertex 0 for lowest Id, in increasing
  // order.
  int currentPointId=i;
  double edgeDirection[3];
  if(this->Level>0)
    {
    int e=0;
    while(e<12)
      {
      assert("check: valid_currentPointId" &&
             (currentPointId==(8+e*this->Level)));
      double firstPoint[3];
      double lastPoint[3];

      points->GetPoint(edges[e][0],firstPoint);
      points->GetPoint(edges[e][1],lastPoint);
      i=0;
      while(i<3)
        {
        edgeDirection[i]=(lastPoint[i]-firstPoint[i])/(this->Level+1);
        p[i]=firstPoint[i];
        ++i;
        }

      int j=1;
      while(j<=this->Level)
        {
        i=0;
        while(i<3)
          {
          p[i]+=edgeDirection[i];
          ++i;
          }
        points->SetPoint(currentPointId,p);
        ++currentPointId;
        ++j;
        }
      ++e;
      }

    assert("check: valid_currentPointId" &&
           (currentPointId==(8+12*this->Level)));

    int f=0;
    while(f<6)
      {
      assert("check: valid_currentPointId" &&
             (currentPointId==(8+12*this->Level+f*this->Level*this->Level)));
      double faceDirections[2][3];
      int facePointId[3];
      e=faces[f][0];
      if(e<0)
        {
        e=-e;
        facePointId[0]=edges[e-1][1];
        facePointId[1]=edges[e-1][0];
        }
      else
        {
        assert("check: not_null_edge" && e>0);
        facePointId[0]=edges[e-1][0];
        facePointId[1]=edges[e-1][1];
        }
      e=faces[f][3];
      if(e<0)
        {
        e=-e;
        facePointId[2]=edges[e-1][1];
        }
      else
        {
        facePointId[2]=edges[e-1][0];
        }

      double facePoints[3][3];

      i=0;
      while(i<3)
        {
        int pointId=facePointId[i];
        facePoints[i][0]=bounds[pointId&1];
        facePoints[i][1]=bounds[2+((pointId>>1)&1)];
        facePoints[i][2]=bounds[4+((pointId>>2)&1)];
        ++i;
        }

      int j=0;
      while(j<2)
        {
        i=0;
        while(i<3)
          {
          faceDirections[j][i]=
            (facePoints[j+1][i]-facePoints[0][i])/(this->Level+1);
          ++i;
          }
        ++j;
        }

      // internal face points
      j=1;
      while(j<(this->Level+1))
        {
        i=1;
        while(i<(this->Level+1))
          {
          int comp=0;
          while(comp<3)
            {
            p[comp]=facePoints[0][comp]+i*faceDirections[0][comp]+
              j*faceDirections[1][comp];
            ++comp;
            }
          points->SetPoint(currentPointId,p);
          ++currentPointId;
          ++i;
          }
        ++j;
        }

      ++f;
      }
    }
  assert("check: valid_currentPointId" &&
         (currentPointId==(8+12*this->Level+6*this->Level*this->Level)));

  // Faces
  vtkIdType poly[4]; // fit a triangle or a quad depending on the Quads flag.
  int f=0;
  while(f<6)
    {
    // Create the cells. Two triangle per subquad.
    int j=0;
    while(j<(this->Level+1))
      {
      i=0;
      while(i<(this->Level+1))
        {
        if(this->Quads)
          {
          // (i,j)
          poly[0]=this->LocalFacePointCoordinatesToPointId(f,i,j);
          // (i+1,j)
          poly[1]=this->LocalFacePointCoordinatesToPointId(f,i+1,j);
          // (i+1,j+1)
          poly[2]=this->LocalFacePointCoordinatesToPointId(f,i+1,j+1);
           // (i,j+1)
          poly[3]=this->LocalFacePointCoordinatesToPointId(f,i,j+1);
          polys->InsertNextCell(4,poly);
          }
        else
          {
          // First triangle
          // (i,j)
          poly[0]=this->LocalFacePointCoordinatesToPointId(f,i,j);
          // (i+1,j)
          poly[1]=this->LocalFacePointCoordinatesToPointId(f,i+1,j);
          // (i+1,j+1)
          poly[2]=this->LocalFacePointCoordinatesToPointId(f,i+1,j+1);
          polys->InsertNextCell(3,poly);

          // Second triangle
          // poly[0]=(i,j), same than for the first triangle
          // (i+1,j+1)
          poly[1]=poly[2];
          // (i,j+1)
          poly[2]=this->LocalFacePointCoordinatesToPointId(f,i,j+1);
          polys->InsertNextCell(3,poly);
          }
        ++i;
        }
      ++j;
      }
    ++f;
    }
}

// ----------------------------------------------------------------------------
// Description:
// Compute the pointId of point (i,j) of face f.
// Used by MinimalPointsMethod().
// \pre valid_face: f>=0 && f<6
// \pre valid_i: i>=0 && i<=(this->Level+1)
// \pre valid_j: j>=0 && j<=(this->Level+1)
vtkIdType vtkTessellatedBoxSource::LocalFacePointCoordinatesToPointId(int f,
                                                                        int i,
                                                                        int j)
{
  assert("pre: valid_face" && f>=0 && f<6);
  assert("pre: valid_i" && i>=0 && i<=(this->Level+1));
  assert("pre: valid_j" && j>=0 && j<=(this->Level+1));

  vtkIdType result;

  int e;

  // vertex point?
  if(i==0 && j==0)
    {
    e=faces[f][0];
    if(e<0)
      {
      e=-e;
      result=edges[e-1][1];
      }
    else
      {
      result=edges[e-1][0];
      }
    }
  else
    {
    if(i==this->Level+1 && j==0)
      {
      e=faces[f][0];
      if(e<0)
        {
        e=-e;
        result=edges[e-1][0];
        }
      else
        {
        result=edges[e-1][1];
        }
      }
    else
      {
      if(i==this->Level+1 && j==this->Level+1)
        {
        e=faces[f][1];
        if(e<0)
          {
          e=-e;
          result=edges[e-1][0];
          }
        else
          {
          result=edges[e-1][1];
          }
        }
      else
        {
        if(i==0 && j==this->Level+1)
          {
          e=faces[f][3];
          if(e<0)
            {
            e=-e;
            result=edges[e-1][1];
            }
          else
            {
            result=edges[e-1][0];
            }
          }
        else
          {
          // internal edge point?
          if(i==0)
            {
            e=faces[f][3];
            if(e<0)
              {
              e=-e;
              result=8+(e-1)*this->Level+j-1;
              }
            else
              {
              result=8+(e-1)*this->Level+this->Level-j;
              }
            }
          else
            {
            if(i==this->Level+1)
              {
              e=faces[f][1];
              if(e<0)
                {
                e=-e;
                result=8+(e-1)*this->Level+this->Level-j;
                }
              else
                {
                result=8+(e-1)*this->Level+j-1;
                }
              }
            else
              {
              if(j==0)
                {
                e=faces[f][0];
                if(e<0)
                  {
                  e=-e;
                  result=8+(e-1)*this->Level+this->Level-i;
                  }
                else
                  {
                  result=8+(e-1)*this->Level+i-1;
                  }
                }
              else
                {
                if(j==this->Level+1)
                  {
                  e=faces[f][2];
                  if(e<0)
                    {
                    e=-e;
                    result=8+(e-1)*this->Level+i-1;
                    }
                  else
                    {
                    result=8+(e-1)*this->Level+this->Level-i;
                    }
                  }
                else
                  {
                  // internal face point.
                  result=(8+(12+f*this->Level)*this->Level)+
                    (j-1)*this->Level+(i-1);
                  }
                }
              }
            }
          }
        }
      }
    }
  assert("post: valid_result" && result>=0 &&
         result<(8+12*this->Level+6*this->Level*this->Level));
  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Build one of the face of the box with some level of tessellation.
// facePoints[0] is the lower-left point
// facePoints[1] is the point along the first axis
// facePoints[2] is the point along the second axis
// \pre positive_id: firstPointId>=0
// \pre points_exists: points!=0
// \pre polys_exists: polys!=0
void vtkTessellatedBoxSource::BuildFace(vtkPoints *points,
                                          vtkCellArray *polys,
                                          vtkIdType firstPointId,
                                          double facePoints[3][3],
                                          int changed)
{
  assert("pre: positive_id" && firstPointId>=0);
  assert("pre: points_exists" && points!=0);
  assert("pre: polys_exists" && polys!=0);

  double direction[2][3];

  int j=0;
  while(j<2)
    {
    int i=0;
    while(i<3)
      {
      direction[j][i]=(facePoints[j+1][i]-facePoints[0][i])/(this->Level+1);
      ++i;
      }
    ++j;
    }

  // Create the point positions.

  double p[3];

  j=0;
  while(j<=(this->Level+1))
    {
    int i=0;
    while(i<=(this->Level+1))
      {
      int comp=0;
      while(comp<3)
        {
        p[comp]=facePoints[0][comp]+i*direction[0][comp]+
          j*direction[1][comp];
        ++comp;
        }
      points->SetPoint(firstPointId+ j*(this->Level+2) + i,p);

      ++i;
      }
    ++j;
    }

  if(changed)
    {
    vtkIdType poly[4]; // fit a triangle or a quad depending on the Quads flag.

    // Create the cells. Two triangle per subquad.
    j=0;
    while(j<(this->Level+1))
      {
      int i=0;
      while(i<(this->Level+1))
        {
        if(this->Quads)
          {
          // (i,j)
          poly[0]=firstPointId+j*(this->Level+2)+i;
          // (i+1,j)
          poly[1]=firstPointId+j*(this->Level+2)+i+1;
          // (i+1,j+1)
          poly[2]=firstPointId+(j+1)*(this->Level+2)+i+1;
          // (i,j+1)
          poly[3]=firstPointId+(j+1)*(this->Level+2)+i;
          polys->InsertNextCell(4,poly);
          }
        else
          {
          // First triangle
          // (i,j)
          poly[0]=firstPointId+j*(this->Level+2)+i;
          // (i+1,j)
          poly[1]=firstPointId+j*(this->Level+2)+i+1;
          // (i+1,j+1)
          poly[2]=firstPointId+(j+1)*(this->Level+2)+i+1;
          polys->InsertNextCell(3,poly);

          // Second triangle
          // (i,j)
          poly[0]=firstPointId+j*(this->Level+2)+i;
          // (i+1,j+1)
          poly[1]=firstPointId+(j+1)*(this->Level+2)+i+1;
          // (i,j+1)
          poly[2]=firstPointId+(j+1)*(this->Level+2)+i;
          polys->InsertNextCell(3,poly);
          }
        ++i;
        }
      ++j;
      }
    }
}

// ----------------------------------------------------------------------------
void vtkTessellatedBoxSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Bounds: (" << this->Bounds[0];
  int i=1;
  while(i<6)
    {
    os << ", " << this->Bounds[i];
    ++i;
    }
  os << ")" << endl;
  os << indent << "Level: " << this->Level << endl;

  os << indent << "DuplicateSharedPoints: ";
  if(this->DuplicateSharedPoints)
    {
    os << "true";
    }
  else
    {
    os << "false";
    }

  os << indent << "Quads: ";
  if(this->Quads)
    {
    os << "true";
    }
  else
    {
    os << "false";
    }
  os << endl;
}
