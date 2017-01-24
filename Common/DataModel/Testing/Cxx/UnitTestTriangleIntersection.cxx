/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestTriangleIntersection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>

#include "vtkMinimalStandardRandomSequence.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkSmartPointer.h"
#include "vtkTriangle.h"

#define VISUAL_DEBUG 0

#ifdef VISUAL_DEBUG
#include <vtkCellArray.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkActor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>

namespace
{
void DrawTriangles(double* p1, double* q1, double* r1,
                   double* p2, double* q2, double* r2)
{
  // Create a triangle
  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();
  vtkIdType pid[6];
  pid[0] = points->InsertNextPoint ( p1[0], p1[1], p1[2] );
  pid[1] = points->InsertNextPoint ( q1[0], q1[1], q1[2] );
  pid[2] = points->InsertNextPoint ( r1[0], r1[1], r1[2] );
  pid[3] = points->InsertNextPoint ( p2[0], p2[1], p2[2] );
  pid[4] = points->InsertNextPoint ( q2[0], q2[1], q2[2] );
  pid[5] = points->InsertNextPoint ( r2[0], r2[1], r2[2] );

  vtkSmartPointer<vtkCellArray> verts =
    vtkSmartPointer<vtkCellArray>::New();
  for (int i=0;i<6;i++)
  {
    verts->InsertNextCell ( 1, &pid[i] );
  }

  vtkSmartPointer<vtkTriangle> triangle1 =
    vtkSmartPointer<vtkTriangle>::New();
  triangle1->GetPointIds()->SetId ( 0, 0 );
  triangle1->GetPointIds()->SetId ( 1, 1 );
  triangle1->GetPointIds()->SetId ( 2, 2 );

  vtkSmartPointer<vtkTriangle> triangle2 =
    vtkSmartPointer<vtkTriangle>::New();
  triangle2->GetPointIds()->SetId ( 0, 3 );
  triangle2->GetPointIds()->SetId ( 1, 4 );
  triangle2->GetPointIds()->SetId ( 2, 5 );

  vtkSmartPointer<vtkCellArray> triangles =
    vtkSmartPointer<vtkCellArray>::New();
  triangles->InsertNextCell ( triangle1 );
  triangles->InsertNextCell ( triangle2 );

  vtkSmartPointer<vtkDoubleArray> pointIds =
    vtkSmartPointer<vtkDoubleArray>::New();
  pointIds->SetNumberOfTuples(6);
  pointIds->SetTuple1(0,0.);
  pointIds->SetTuple1(1,1.);
  pointIds->SetTuple1(2,2.);
  pointIds->SetTuple1(3,3.);
  pointIds->SetTuple1(4,4.);
  pointIds->SetTuple1(5,5.);

  // Create a polydata object
  vtkSmartPointer<vtkPolyData> trianglePolyData =
    vtkSmartPointer<vtkPolyData>::New();

  // Add the geometry and topology to the polydata
  trianglePolyData->SetPoints ( points );
  trianglePolyData->SetVerts ( verts );
  trianglePolyData->SetPolys ( triangles );
  trianglePolyData->GetPointData()->SetScalars( pointIds );

  // Create mapper and actor
  vtkSmartPointer<vtkPolyDataMapper> mapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputData(trianglePolyData);
  mapper->SetScalarRange(0.,5.);
  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetPointSize(5);

  // Create a renderer, render window, and an interactor
  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renderWindow =
    vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);
  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);

  // Add the actors to the scene
  renderer->AddActor(actor);
  renderer->SetBackground(.1, .2, .4); // Background color dark blue

  // Render and interact
  renderWindow->Render();
  renderWindowInteractor->Start();
  return;
}
}

#endif

namespace
{
static const double EPSILON = 1.e-6;

static const int VTK_NO_INTERSECTION=0;
static const int VTK_YES_INTERSECTION=1;

typedef vtkMinimalStandardRandomSequence vtkRandom;

std::string TriangleToString(double* t1,double* t2,double* t3)
{
  std::stringstream stream;
  stream << "("<<t1[0]<<","<<t1[1]<<","<<t1[2]<<") ("
         << t2[0]<<","<<t2[1]<<","<<t2[2]<<") ("
         << t3[0]<<","<<t3[1]<<","<<t3[2]<<")";
  return stream.str();
}

void ProjectPointOntoPlane(double* o, double* n, double* p)
{
  // Project point p onto a plane that crosses through point o and has normal n.

  double pMinuso[3];
  vtkMath::Subtract(p,o,pMinuso);

  double dot = vtkMath::Dot(pMinuso,n);

  for (int i=0;i<3;i++)
  {
    p[i] -= dot*n[i];
  }
}

void GeneratePointInPlane(vtkRandom* seq, double* o, double* n, double* p)
{
  // Generate a point p that lies in a plane that crosses through point o and
  // has normal n.

  for (int i=0;i<3;i++)
  {
    seq->Next();
    p[i] = -1. + 2.*seq->GetValue();
  }

  ProjectPointOntoPlane(o,n,p);
}

void ReflectPointThroughPlane(double* o, double* n, double* p)
{
  // Reflect point p through a plane that crosses through point o and has normal
  // n.

  double dot = 0.;
  for (int i=0;i<3;i++)
  {
    dot += (p[i] - o[i])*n[i];
  }

  for (int i=0;i<3;i++)
  {
    p[i] -= dot*n[i];
  }
}

void GeneratePointInHalfPlane(vtkRandom* seq, double* o, double* n, double* o2,
                              double* n2, double* p)
{
  // Generate a point p that lies in a plane that crosses through point o, has
  // normal n, and lies on the positive side of the halfspace defined by point
  // o1 and normal n2.

  GeneratePointInPlane( seq, o, n, p );

  double pMinuso2[3];
  vtkMath::Subtract( p, o2, pMinuso2 );

  if ( vtkMath::Dot( n2, pMinuso2 ) < 0. )
  {
    ReflectPointThroughPlane( o2, n2, p );
  }
}

void GenerateTriangleInPlane(vtkRandom* seq, double* o, double* n, double* t1,
                             double* t2, double* t3)
{
  do
  {
    GeneratePointInPlane( seq, o, n, t1 );
    GeneratePointInPlane( seq, o, n, t2 );
    GeneratePointInPlane( seq, o, n, t3 );
  }
  while ( vtkTriangle::TriangleArea( t1, t2, t3 ) < EPSILON );
}

void GenerateTriangleInHalfPlane(vtkRandom* seq, double* o, double* n,
                                 double* o2, double* n2, double* t1, double* t2,
                                 double* t3)
{
  do
  {
    GeneratePointInHalfPlane( seq, o, n, o2, n2, t1 );
    GeneratePointInHalfPlane( seq, o, n, o2, n2, t2 );
    GeneratePointInHalfPlane( seq, o, n, o2, n2, t3 );
  }
  while ( vtkTriangle::TriangleArea( t1, t2, t3 ) < EPSILON );
}

void RandomSphere(vtkRandom* seq, const double radius, const double* offset,
                  double* value)
{
  // Generate a point on a sphere.

  seq->Next();
  double theta = 2.*vtkMath::Pi()*seq->GetValue();
  seq->Next();
  double phi = vtkMath::Pi()*seq->GetValue();
  value[0] = radius*cos(theta)*sin(phi) + offset[0];
  value[1] = radius*sin(theta)*sin(phi) + offset[1];
  value[2] = radius*cos(phi) + offset[2];
}

int TestNegativeResult(vtkRandom* seq, unsigned nTests)
{
  double origin[3] = {0.,0.,0.};
  double n1[3], n2[3], o1[3], t1[3][3], t2[3][3];

  for (unsigned test=0;test<nTests;test++)
  {
    RandomSphere( seq, 1, origin, n1 );
    RandomSphere( seq, 1, origin, n2 );

    for (int i=0;i<3;i++)
    {
      seq->Next();
      o1[i] = seq->GetValue();
    }

    GenerateTriangleInPlane( seq, o1, n1, t1[0], t1[1], t1[2]);
    double dividingPlaneOrigin[3];
    for (int i=0;i<3;i++)
    {
      dividingPlaneOrigin[i] = t1[0][i] + EPSILON*n1[i];
    }
    GenerateTriangleInHalfPlane( seq, o1, n2, dividingPlaneOrigin, n1, t2[0], t2[1], t2[2]);

    int returnValue = vtkTriangle::TrianglesIntersect( t1[0], t1[1], t1[2],
                                                       t2[0], t2[1], t2[2] );

      if (returnValue != VTK_NO_INTERSECTION)
      {
        std::cout<<"Triangle "<<TriangleToString(t1[0],t1[1],t1[2])<<std::endl;
        std::cout<<" intersects "<<TriangleToString(t2[0],t2[1],t2[2])<<" and shouldn't."<<std::endl;
#ifdef VISUAL_DEBUG
        DrawTriangles(t1[0],t1[1],t1[2],t2[0],t2[1],t2[2]);
#endif
        return EXIT_FAILURE;
      }
    }

  return EXIT_SUCCESS;
}

int TestCoplanarNegativeResult(vtkRandom* seq, unsigned nTests)
{
  double origin[3] = {0.,0.,0.};
  double n1[3], n2[3], nn2[3], o1[3], o2[3], t1[3][3], t2[3][3];

  for (unsigned test=0;test<nTests;test++)
  {
    RandomSphere( seq, 1, origin, n1 );
    RandomSphere( seq, 1, origin, n2 );

    n1[0] = n1[1] = 0.;
    n1[2] = 1.;

    n2[0] = 1.;
    n2[1] = n2[2] = 0.;

    double dot = vtkMath::Dot(n1,n2);

    for (int i=0;i<3;i++)
    {
      n2[i] -= dot*n1[i];
      nn2[i] = -1.*n2[i];
      seq->Next();
      o1[i] = seq->GetValue();
    }

    o1[0] = o1[1] = o1[2] = 1.;

    GeneratePointInPlane( seq, o1, n1, o2 );

    o2[0] = o2[1] = 0.;
    o2[2] = 1.;

    double dividingPlaneOrigin[3];
    for (int i=0;i<3;i++)
    {
      dividingPlaneOrigin[i] = o2[i] + 10000.*EPSILON*nn2[i];
    }

    GenerateTriangleInHalfPlane( seq, o1, n1, o2, n2,  t1[0], t1[1], t1[2]);
    GenerateTriangleInHalfPlane( seq, o1, n1, dividingPlaneOrigin, nn2, t2[0], t2[1], t2[2]);

    int returnValue = vtkTriangle::TrianglesIntersect( t1[0], t1[1], t1[2],
                                                       t2[0], t2[1], t2[2] );

    if (returnValue != VTK_NO_INTERSECTION)
    {
      std::cout<<"Triangle "<<TriangleToString(t1[0],t1[1],t1[2])<<std::endl;
      std::cout<<" intersects "<<TriangleToString(t2[0],t2[1],t2[2])<<" and shouldn't."<<std::endl;
#ifdef VISUAL_DEBUG
      DrawTriangles(t1[0],t1[1],t1[2],t2[0],t2[1],t2[2]);
#endif
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

void ProjectAlongRay(vtkRandom* seq, double* x0, double* x1, double* p)
{
  // Assign point p a value along the ray originating at x0 and passing through
  // x1. The resulting line segment (x0, p) will cross through x1.

  double n[3];
  vtkMath::Subtract(x1,x0,n);
  vtkMath::Normalize(n);

  seq->Next();
  double len = seq->GetValue();
  for (int i=0;i<3;i++)
  {
    p[i] = x1[i] + len*n[i];
  }
}

void GenerateOverlappingSegments(vtkRandom* seq, double* p1, double* p2,
                                 double* x1, double* x2, double* y1, double* y2)
{
  // Given a line through (p1,p2), generate line segments (x1,x2) and (y1,y2)
  // that lie on the line and overlap.

  std::vector<double> random( 4, 0. );
  for (int i=0;i<4;i++)
  {
    seq->Next();
    random[i] = seq->GetValue();
  }
  std::sort( random.begin(), random.end() );

  seq->Next();
  double sequence = seq->GetValue();

  double par[4]; // parametric values for x1,x2,y1,y2

  if (sequence < .25)
  {
    par[0] = random[0];
    par[1] = random[2];
    par[2] = random[1];
    par[3] = random[3];
  }
  else if (sequence < .5)
  {
    par[0] = random[2];
    par[1] = random[0];
    par[2] = random[3];
    par[3] = random[1];
  }
  else if (sequence < .75)
  {
    par[0] = random[0];
    par[1] = random[3];
    par[2] = random[1];
    par[3] = random[2];
  }
  else
  {
    par[0] = random[1];
    par[1] = random[2];
    par[2] = random[0];
    par[3] = random[3];
  }

  for (int i=0;i<3;i++)
  {
    x1[i] = p1[i] + par[0]*p2[i];
    x2[i] = p1[i] + par[1]*p2[i];
    y1[i] = p1[i] + par[2]*p2[i];
    y2[i] = p1[i] + par[3]*p2[i];
  }
}

void RandomPoint(vtkRandom* seq, double* p)
{
  // Set p to be a random point in the box (-1,1) x (-1,1) x (-1,1).
  for (int i=0;i<3;i++)
  {
    seq->Next();
    p[i] = -1. + 2.*seq->GetValue();
  }
}

int TestPositiveResult(
  vtkRandom* seq, unsigned nTests)
{
  double p1[3], p2[3], l1[2][3], l2[2][3], t1[3][3], t2[3][3];

  for (unsigned test=0;test<nTests;test++)
  {
    RandomPoint( seq, p1 );
    RandomPoint( seq, p2 );
    RandomPoint( seq, t1[0] );
    RandomPoint( seq, t2[0] );

    GenerateOverlappingSegments( seq, p1, p2, l1[0], l1[1], l2[0], l2[1] );

    ProjectAlongRay( seq, t1[0], l1[0], t1[1]);
    ProjectAlongRay( seq, t1[0], l1[1], t1[2]);
    ProjectAlongRay( seq, t2[0], l2[0], t2[1]);
    ProjectAlongRay( seq, t2[0], l2[1], t2[2]);

    int returnValue = vtkTriangle::TrianglesIntersect( t1[0], t1[1], t1[2],
                                                       t2[0], t2[1], t2[2] );

    if (returnValue != VTK_YES_INTERSECTION)
    {
      std::cout<<"Triangle "<<TriangleToString(t1[0],t1[1],t1[2])<<std::endl;
      std::cout<<" does not intersect "<<TriangleToString(t2[0],t2[1],t2[2])
               <<" and should."<<std::endl;
#ifdef VISUAL_DEBUG
      DrawTriangles(t1[0],t1[1],t1[2],t2[0],t2[1],t2[2]);
#endif
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

int TestCoplanarPositiveResult(vtkRandom* seq, unsigned nTests)
{
  double p1[3], p2[3], l1[2][3], l2[2][3], t1[3][3], t2[3][3], orgn[3], n[3];
  double v1[3], v2[3];

  for (unsigned test=0;test<nTests;test++)
  {
    RandomPoint( seq, p1 );
    RandomPoint( seq, p2 );
    RandomPoint( seq, t1[0] );
    RandomPoint( seq, t2[0] );

    GenerateOverlappingSegments( seq, p1, p2, l1[0], l1[1], l2[0], l2[1] );

    ProjectAlongRay( seq, t1[0], l1[0], t1[1]);
    ProjectAlongRay( seq, t1[0], l1[1], t1[2]);
    ProjectAlongRay( seq, t2[0], l2[0], t2[1]);
    ProjectAlongRay( seq, t2[0], l2[1], t2[2]);

    RandomPoint( seq, orgn );

    if (vtkTriangle::TriangleArea( t1[0], t1[1], t1[2] ) <
        vtkTriangle::TriangleArea( t2[0], t2[1], t2[2] ))
    {
      for (unsigned i=0;i<3;i++)
      {
        v1[i] = t1[1][i] - t1[0][i];
        v2[i] = t1[2][i] - t1[0][i];
      }
      vtkMath::Cross( v1, v2, n );
    }
    else
    {
      for (unsigned i=0;i<3;i++)
      {
        v1[i] = t2[1][i] - t2[0][i];
        v2[i] = t2[2][i] - t2[0][i];
      }
      vtkMath::Cross( v1, v2, n );
    }
    vtkMath::Normalize(n);

    for (unsigned i=0;i<3;i++)
    {
      ProjectPointOntoPlane(orgn, n, t1[i]);
      ProjectPointOntoPlane(orgn, n, t2[i]);
    }

    int returnValue = vtkTriangle::TrianglesIntersect( t1[0], t1[1], t1[2],
                                                       t2[0], t2[1], t2[2] );

    if (returnValue != VTK_YES_INTERSECTION)
    {
      std::cout<<"Triangle "<<TriangleToString(t1[0],t1[1],t1[2])<<std::endl;
      std::cout<<" does not intersect "<<TriangleToString(t2[0],t2[1],t2[2])
               <<" and should."<<std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

int factorial(int n)
{
  return (n == 1 || n == 0) ? 1 : factorial(n - 1) * n;
}

int TestReciprocalResult(vtkRandom* seq, unsigned nTests)
{
  std::array<std::array<double,3>,6> p;

  nTests /= factorial(6);

  for (unsigned test=0;test<nTests;test++)
  {
    for (int i=0;i<6;i++)
    {
      RandomPoint( seq, &p[i][0] );
    }
    std::sort(std::begin(p), std::end(p));
    do
    {
      int returnValue1 =
        vtkTriangle::TrianglesIntersect( &p[0][0], &p[1][0], &p[2][0],
                                         &p[3][0], &p[4][0], &p[5][0] );
      int returnValue2 =
        vtkTriangle::TrianglesIntersect( &p[3][0], &p[4][0], &p[5][0],
                                         &p[0][0], &p[1][0], &p[2][0] );

      if (returnValue1 != returnValue2)
      {
        std::cout<<"Triangles "<<TriangleToString(&p[0][0], &p[1][0], &p[2][0])
                 <<" and "<<TriangleToString(&p[3][0], &p[4][0], &p[5][0])
                 <<" disagree about intersection."<<std::endl;
        std::cout<<"return values: "<<returnValue1<<" "<<returnValue2<<std::endl;
#ifdef VISUAL_DEBUG
        DrawTriangles(&p[0][0], &p[1][0], &p[2][0], &p[3][0], &p[4][0], &p[5][0]);
#endif
        return EXIT_FAILURE;
      }
    }
    while (std::next_permutation(std::begin(p), std::end(p)));
  }

  return EXIT_SUCCESS;
}

int TestTriangleIntersection(vtkRandom* seq, unsigned nTests)
{
  if (TestPositiveResult(seq,nTests) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }
  if (TestNegativeResult(seq,nTests) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }
  if (TestCoplanarPositiveResult(seq, nTests) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }
  if (TestCoplanarNegativeResult(seq, nTests) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }
  if (TestReciprocalResult(seq, nTests) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
}

int UnitTestTriangleIntersection(int,char *[])
{
  vtkRandom* sequence = vtkRandom::New();

  sequence->SetSeed(2);

  const unsigned nTest = 1.e5;

  std::cout<<"Testing vtkTriangle::TriangleIntersection"<<std::endl;
  if (TestTriangleIntersection(sequence,nTest) == EXIT_FAILURE)
    {
    return EXIT_FAILURE;
    }

  sequence->Delete();
  return EXIT_SUCCESS;
}
