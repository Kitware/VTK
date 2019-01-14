/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestLine.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <cmath>

#include "vtkMinimalStandardRandomSequence.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkSmartPointer.h"

namespace
{
  static const double EPSILON = 1.e-6;


  static const int VTK_NO_INTERSECTION=0;
  static const int VTK_YES_INTERSECTION=2;
  static const int VTK_ON_LINE=3;

void GenerateIntersectingLineSegments(vtkMinimalStandardRandomSequence* seq,
                                      double* a1,
                                      double* a2,
                                      double* b1,
                                      double* b2,
                                      double& u,
                                      double& v)
{
  // Generate two line segments ((a1,a2) and (b1,b2)) that intersect, and set
  // u and v as the parametric points of intersection on the two respective
  // lines. First, an intersection is generated. Two lines are then generated
  // that cross through this intersection.

  double intersection[3];
  for (unsigned i=0;i<3;i++)
  {
    intersection[i] = seq->GetValue();
    seq->Next();
    a1[i] = seq->GetValue();
    seq->Next();
    b1[i] = seq->GetValue();
    seq->Next();
  }

  intersection[0] = 1.; intersection[1] = intersection[2] = 0.;
  a1[0] = a1[1] = a1[2] = 0.;
  b1[0] = b1[1] = 1.; b1[2] = 0.;

  double t1 = seq->GetValue();
  seq->Next();
  double t2 = seq->GetValue();
  seq->Next();

  double lenA = 0.;
  double lenB = 0.;
  double lenA1toIntersection = 0.;
  double lenB1toIntersection = 0.;

  for (unsigned i=0;i<3;i++)
  {
    a2[i] = a1[i] + (intersection[i]-a1[i])*(1.+t1);
    b2[i] = b1[i] + (intersection[i]-b1[i])*(1.+t2);
    lenA += (a2[i]-a1[i])*(a2[i]-a1[i]);
    lenB += (b2[i]-b1[i])*(b2[i]-b1[i]);
    lenA1toIntersection += (a1[i]-intersection[i])*(a1[i]-intersection[i]);
    lenB1toIntersection += (b1[i]-intersection[i])*(b1[i]-intersection[i]);
  }

  lenA = sqrt(lenA);
  lenB = sqrt(lenB);
  lenA1toIntersection = sqrt(lenA1toIntersection);
  lenB1toIntersection = sqrt(lenB1toIntersection);

  u = lenA1toIntersection/lenA;
  v = lenB1toIntersection/lenB;
}

void RandomSphere(vtkMinimalStandardRandomSequence* seq,
                  const double radius,
                  const double* offset,
                  double* value)
{
  // Generate a point within a sphere.

  double theta = 2.*vtkMath::Pi()*seq->GetValue();
  seq->Next();
  double phi = vtkMath::Pi()*seq->GetValue();
  seq->Next();
  value[0] = radius*cos(theta)*sin(phi) + offset[0];
  value[1] = radius*sin(theta)*sin(phi) + offset[1];
  value[2] = radius*cos(phi) + offset[2];
}

void GenerateNonintersectingLineSegments(vtkMinimalStandardRandomSequence* seq,
                                         double* a1,
                                         double* a2,
                                         double* b1,
                                         double* b2)
{
  // Generate two line segments ((a1,a2) and (b1,b2)) that do not intersect.
  // The endpoints of each line segment are generated from two non-overlapping
  // spheres, and the two spheres for each line segment are physically displaced
  // as well.

  static const double center[4][3] = {{0.,0.,0.},
                                      {1.,0.,0.},
                                      {0.,1.,0.},
                                      {1.,1.,0.}};

  static const double radius = 0.5 - 1.e-6;

  RandomSphere(seq,radius,center[0],a1);
  RandomSphere(seq,radius,center[1],a2);
  RandomSphere(seq,radius,center[2],b1);
  RandomSphere(seq,radius,center[3],b2);
}

void GenerateColinearLineSegments(vtkMinimalStandardRandomSequence* seq,
                                  double* a1,
                                  double* a2,
                                  double* b1,
                                  double* b2)
{
  // Generate two line segments ((a1,a2) and (b1,b2)) that are colinear.

  for (unsigned i=0;i<3;i++)
  {
    a1[i] = seq->GetValue();
    seq->Next();
    a2[i] = seq->GetValue();
    seq->Next();
    double tmp = seq->GetValue();
    seq->Next();
    b1[i] = a1[i] + tmp;
    b2[i] = a2[i] + tmp;
  }
}

void GenerateLinesAtKnownDistance(vtkMinimalStandardRandomSequence* seq,
                                  double& lineDist,
                                  double* a1,
                                  double* a2,
                                  double* b1,
                                  double* b2,
                                  double* a12,
                                  double* b12,
                                  double& u,
                                  double& v)
{
  // Generate two lines ((a1,a2) and (b1,b2)) set a known distance (lineDist)
  // apart. the parameter and value of the closest points for lines a and b are
  // a12, u and b12, v, respectively.

  double v1[3],v2[3],v3[3];
  for (unsigned i=0;i<3;i++)
  {
    v1[i] = seq->GetValue();
    seq->Next();
    v2[i] = seq->GetValue();
    seq->Next();
  }
  vtkMath::Cross(v1,v2,v3);
  vtkMath::Normalize(v1);
  vtkMath::Normalize(v2);
  vtkMath::Normalize(v3);

  double a1_to_a12 = .1 + seq->GetValue();
  seq->Next();
  double a12_to_a2 = .1 + seq->GetValue();
  seq->Next();
  double b1_to_b12 = .1 + seq->GetValue();
  seq->Next();
  double b12_to_b2 = .1 + seq->GetValue();
  seq->Next();

  lineDist = seq->GetValue();
  seq->Next();

  for (unsigned i=0;i<3;i++)
  {
    a12[i] = seq->GetValue();
    seq->Next();
    b12[i] = a12[i] + lineDist*v3[i];
    a1[i] = a12[i] - a1_to_a12*v1[i];
    a2[i] = a12[i] + a12_to_a2*v1[i];
    b1[i] = b12[i] - b1_to_b12*v2[i];
    b2[i] = b12[i] + b12_to_b2*v2[i];
  }
  u = a1_to_a12/(a1_to_a12 + a12_to_a2);
  v = b1_to_b12/(b1_to_b12 + b12_to_b2);
}

void GenerateLineAtKnownDistance(vtkMinimalStandardRandomSequence* seq,
                                  double* a1,
                                  double* a2,
                                  double* p,
                                  double& dist)
{
  // Generate a line (a1,a2) set a known distance (dist) from a generated point
  // p.

  double v1[3],v2[3],v3[3];
  for (unsigned i=0;i<3;i++)
  {
    v1[i] = seq->GetValue();
    seq->Next();
    v2[i] = seq->GetValue();
    seq->Next();
  }
  vtkMath::Cross(v1,v2,v3);
  vtkMath::Normalize(v1);
  vtkMath::Normalize(v2);
  vtkMath::Normalize(v3);

  double a1_to_a12 = .1 + seq->GetValue();
  seq->Next();
  double a12_to_a2 = .1 + seq->GetValue();
  seq->Next();

  dist = seq->GetValue();
  seq->Next();

  double nearest[3];
  for (unsigned i=0;i<3;i++)
  {
    nearest[i] = seq->GetValue();
    seq->Next();
    p[i] = nearest[i] + dist*v3[i];
    a1[i] = nearest[i] - a1_to_a12*v1[i];
    a2[i] = nearest[i] + a12_to_a2*v1[i];
  }
}

double PointToLineSegment(double* p1,double* p2,double* p,double* pn,double& u)
{
  // Helper function that computes the distance from a point to a line segment.
  // It is not used to actually test the vtkLine function for the same purpose,
  // but rather to determine correct values for these test functions.

  u = 0.;
  double dist2 = 0.;
  for (unsigned i=0;i<3;i++)
  {
    u += (p[i]-p1[i])*(p2[i]-p1[i]);
    dist2 += (p2[i]-p1[i])*(p2[i]-p1[i]);
  }
  u/=dist2;

  if (u<=0.)
  {
    for (unsigned i=0;i<3;i++)
    {
      u = 0.;
      pn[i] = p1[i];
    }
  }
  else if (u>=1.)
  {
    u = 1.;
    for (unsigned i=0;i<3;i++)
    {
      pn[i] = p2[i];
    }
  }
  else
  {
    for (unsigned i=0;i<3;i++)
    {
      pn[i] = p1[i] + u*(p2[i]-p1[i]);
    }
  }

  double dist = 0.;
  for (unsigned i=0;i<3;i++)
  {
    dist += (p[i]-pn[i])*(p[i]-pn[i]);
  }
  return std::sqrt(dist);
}

void GenerateLineSegmentsAtKnownDistance(vtkMinimalStandardRandomSequence* seq,
                                         double& lineDist,
                                         double* a1,
                                         double* a2,
                                         double* b1,
                                         double* b2,
                                         double* a12,
                                         double* b12,
                                         double& u,
                                         double& v)
{
  // Generate two line segments ((a1,a2) and (b1,b2)) set a known distance
  // (lineDist) apart. the parameter and value of the closest points for lines
  // a and b are a12, u and b12, v, respectively.

  GenerateLinesAtKnownDistance(seq,lineDist,a1,a2,b1,b2,a12,b12,u,v);

  double modify = seq->GetValue();
  seq->Next();

  if (modify < 0.25)
  {
    double t = seq->GetValue();
    seq->Next();

    for (unsigned i=0;i<3;i++)
    {
      a12[i] = a2[i] = a1[i] + (a12[i]-a1[i])*t;
    }

    u = 1.;
    lineDist = PointToLineSegment(b1,b2,a2,b12,v);
  }
  else if (modify < 0.5)
  {
    double t = seq->GetValue();
    seq->Next();

    for (unsigned i=0;i<3;i++)
    {
      b12[i] = b2[i] = b1[i] + (b12[i]-b1[i])*t;
    }

    lineDist = PointToLineSegment(a1,a2,b2,a12,u);
    v = 1.;
  }
}

int TestLineIntersection_PositiveResult(vtkMinimalStandardRandomSequence* seq,
                                        unsigned nTests)
{
  double a1[3],a2[3],b1[3],b2[3],u,v;

  for (unsigned i=0;i<nTests;i++)
  {
    GenerateIntersectingLineSegments(seq,a1,a2,b1,b2,u,v);

    double uv[2];
    int returnValue = vtkLine::Intersection3D(a1,a2,b1,b2,uv[0],uv[1]);

    if (returnValue != VTK_YES_INTERSECTION)
    {
      return EXIT_FAILURE;
    }

    if (std::fabs(u-uv[0])>EPSILON && std::fabs(v-uv[1])>EPSILON)
    {
      return EXIT_FAILURE;
    }
  }

    return EXIT_SUCCESS;
}

int TestLineIntersection_NegativeResult(vtkMinimalStandardRandomSequence* seq,
                                        unsigned nTests)
{
  double a1[3],a2[3],b1[3],b2[3],u,v;

  for (unsigned i=0;i<nTests;i++)
  {
    GenerateNonintersectingLineSegments(seq,a1,a2,b1,b2);

    int returnValue = vtkLine::Intersection3D(a1,a2,b1,b2,u,v);

    if (returnValue != VTK_NO_INTERSECTION)
    {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

int TestLineIntersection_ColinearResult(vtkMinimalStandardRandomSequence* seq,
                                        unsigned nTests)
{
  double a1[3],a2[3],b1[3],b2[3],u,v;

  for (unsigned i=0;i<nTests;i++)
  {
    GenerateColinearLineSegments(seq,a1,a2,b1,b2);

    int returnValue = vtkLine::Intersection3D(a1,a2,b1,b2,u,v);

    if (returnValue != VTK_ON_LINE)
    {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

int TestLineIntersection(vtkMinimalStandardRandomSequence* seq,
                                        unsigned nTests)
{
  if (TestLineIntersection_PositiveResult(seq,nTests) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }
  if (TestLineIntersection_NegativeResult(seq,nTests) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }
  if (TestLineIntersection_ColinearResult(seq,nTests) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int TestDistanceBetweenLines(vtkMinimalStandardRandomSequence* seq,
                             unsigned nTests)
{
  double a1[3],a2[3],b1[3],b2[3],a12[3],b12[3],u,v,dist;

  for (unsigned i=0;i<nTests;i++)
  {
    GenerateLinesAtKnownDistance(seq,dist,a1,a2,b1,b2,a12,b12,u,v);

    double p1[3],p2[3],t1,t2;
    double d = vtkLine::DistanceBetweenLines(a1,a2,b1,b2,p1,p2,t1,t2);

    if (std::fabs(dist*dist-d) > EPSILON)
    {
      return EXIT_FAILURE;
    }

    for (unsigned j=0;j<3;j++)
    {
      if (std::fabs(a12[j]-p1[j]) > EPSILON||std::fabs(b12[j]-p2[j]) > EPSILON)
      {
        return EXIT_FAILURE;
      }
    }

    if (std::fabs(u-t1) > EPSILON || std::fabs(v-t2) > EPSILON)
    {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

int TestDistanceBetweenLineSegments(vtkMinimalStandardRandomSequence* seq,
                                    unsigned nTests)
{
  double a1[3],a2[3],b1[3],b2[3],a12[3],b12[3],u,v,dist;

  for (unsigned i=0;i<nTests;i++)
  {
    GenerateLineSegmentsAtKnownDistance(seq,dist,a1,a2,b1,b2,a12,b12,u,v);

    double p1[3],p2[3],t1,t2;
    double d = vtkLine::DistanceBetweenLineSegments(a1,a2,b1,b2,p1,p2,t1,t2);

    if (std::fabs(dist*dist-d) > EPSILON)
    {
      return EXIT_FAILURE;
    }

    for (unsigned j=0;j<3;j++)
    {
      if (std::fabs(a12[j]-p1[j]) > EPSILON ||
          std::fabs(b12[j]-p2[j]) > EPSILON)
      {
        return EXIT_FAILURE;
      }
    }

    if (std::fabs(u-t1) > EPSILON || std::fabs(v-t2) > EPSILON)
    {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

int TestDistanceToLine(vtkMinimalStandardRandomSequence* seq,
                       unsigned nTests)
{
  const double epsilon = 256*std::numeric_limits<double>::epsilon();

  double a1[3],a2[3],p[3],dist;

  for (unsigned i=0;i<nTests;i++)
  {
    GenerateLineAtKnownDistance(seq,a1,a2,p,dist);

    double d = vtkLine::DistanceToLine(p,a1,a2);

    if (std::fabs(dist*dist-d) > epsilon)
    {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
}

int UnitTestLine(int,char *[])
{
  vtkMinimalStandardRandomSequence* sequence =
    vtkMinimalStandardRandomSequence::New();

  sequence->SetSeed(1);

  const unsigned nTest = 1.e4;

  std::cout<<"Testing vtkLine::vtkLine::Intersection3D"<<std::endl;
  if (TestLineIntersection(sequence,nTest) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }
  std::cout<<"Testing vtkLine::DistanceBetweenLines"<<std::endl;
  if (TestDistanceBetweenLines(sequence,nTest) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }
  std::cout<<"Testing vtkLine::DistanceBetweenLineSegments"<<std::endl;
  if (TestDistanceBetweenLineSegments(sequence,nTest) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }
  std::cout<<"Testing vtkLine::DistanceToLine"<<std::endl;
  if (TestDistanceToLine(sequence,nTest) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }

  sequence->Delete();
  return EXIT_SUCCESS;
}
