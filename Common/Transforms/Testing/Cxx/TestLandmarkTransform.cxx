/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLandmarkTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLandmarkTransform.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkPoints.h"
#include "vtkTransform.h"
#include "vtkNew.h"

#include <iostream>
#include <sstream>

static int TestSpecificLandmarkTransform(
  int mode, int dimensionality, int npoints, double sigma,
  double scale1, double scale2, double scale3)
{
  int rval = 0; // will be set to 1 if failure
  std::ostringstream errstream;
  npoints = (npoints > 20 ? 20 : npoints);

  const double lcoords[20][3] = {
    { -0.8316301300814422, -0.06992580859519772, -1.6034524068257419 },
    { -2.151893827785692, 0.38244721645095636, -0.9275967632551845 },
    { 0.8147291118075928, -0.7016483698682392, 0.15003863332602096 },
    { 0.918239421266975, 0.5515514723709805, -1.0230600499321258 },
    { -0.4977939747967184, 1.5000786176083494, 0.892455159403953 },
    { 2.137759080794324, -0.7876029858279091, 0.23676951564894347 },
    { 0.07659657475437548, 0.37528421293358666, 1.061745743663681 },
    { -0.7908820649026604, 1.4270955106455065, 2.2665387247459576 },
    { -0.5663930529602919, 1.9402635876094498, 1.1531767242062774 },
    { 0.22529528853908187, -1.5938090446587108, -0.7004997748768814 },
    { 0.6165064084492409, -0.2761336076050157, -0.7930056820043028 },
    { -1.6122391974605947, -1.4200010952872733, 1.0567292903013055 },
    { 0.17993263043615856, -0.9038514957133562, -2.1611068227229695 },
    { -1.4186794357559613, 0.85026116269838, -1.7600646313947719 },
    { 0.9690209792801024, 0.7018737798529897, 0.3923799957082836 },
    { -0.6586203767750309, -2.1468680342265904, 0.762954972139701 },
    { 1.2872860659137344, 0.8557080868402649, 0.3905931440107816 },
    { -0.18996464681200217, 0.8315184491297033, -1.0227889589485941 },
    { 1.0636210067525393, -0.24736478911115908, -0.7581101375259237 },
    { -0.09448165336394657, -1.1381967760924927, -0.7171168342666931 },
  };

  const double lnoise[20][3] = {
    { 1.5137019295427425, 0.6858246680960894, 0.07331883771349512 },
    { -0.34081703057234036, 0.47987804772801446, 0.982197518178181 },
    { -0.1106079068591361, 1.0523148414328571, 0.17910578196163454 },
    { 0.05724784633562011, -0.08459760939107724, -0.7665637643588622 },
    { -0.4333381262791796, 0.018412807528038692, 0.6889623031683394 },
    { -1.1692454358073843, -0.6875830563599973, 0.9077463799204326 },
    { -1.9329042505569662, 1.0529789607437061, -0.29738186972471486 },
    { -0.12079407626315326, 0.9261998453458427, 1.0938543547601083 },
    { -0.6384715430732077, -0.2606527602354865, 1.417882645305744 },
    { -0.10127708027623447, -0.7470111486643078, 0.726100633329295 },
    { 0.36659507636859245, 1.4194144006017144, 0.41878644928947467 },
    { 1.0325034539790547, -0.2291631905797599, -1.3490582933020208 },
    { -0.7186165872334461, 0.4613954758072554, -1.1318559861004829 },
    { 2.455035378196603, -0.01476716688473253, -0.0890030227805104 },
    { 1.6498918075463915, 2.7557006973876508, -0.6466098561563114 },
    { 1.16764314555201, -1.5226214641344893, 0.13000979083980121 },
    { -0.9640219699623079, 1.3071375444488553, 0.5668689159057715 },
    { 0.40366181757487013, 2.308315254377135, 0.8202651493656881 },
    { -1.0267515231555335, -0.2853656137629097, -1.1599391275129292 },
    { -0.09199656043877075, 0.35274602605225164, 2.5626579880899327 },
  };

  // optionally reduce the dimensionality of the points,
  // to allow for more comprehensive testing
  vtkNew<vtkTransform> squash;
  squash->PostMultiply();
  // make the squash plane oblique
  squash->RotateWXYZ(50, 0.1, 0.3, -0.2);
  if (dimensionality == 0)
  {
    squash->Scale(0.0, 0.0, 0.0);
  }
  else if (dimensionality == 1)
  {
    squash->Scale(1.0, 0.0, 0.0);
  }
  else if (dimensionality == 2)
  {
    squash->Scale(1.0, 1.0, 0.0);
  }
  squash->RotateWXYZ(-50, 0.1, 0.3, -0.2);

  // generate the transform we want to recover
  vtkNew<vtkTransform> transform;
  transform->PostMultiply();
  transform->RotateWXYZ(-70, 0.2, -0.1, -0.8);
  transform->Scale(scale1, scale2, scale3);
  transform->RotateWXYZ(30, 1.0, -0.1, 0.5);
  transform->Translate(2.1, -6.5, -0.1);

  // create the two point sets
  vtkNew<vtkPoints> points1;
  vtkNew<vtkPoints> points2;
  double psigma = sigma/sqrt(3.0);
  for (int i = 0; i < npoints; i++)
  {
    double p[3] = { lcoords[i][0], lcoords[i][1], lcoords[i][2] };
    // optionally reduce the dimensionality
    squash->TransformPoint(p, p);
    points1->InsertNextPoint(p);
    // transform the point and add noise
    transform->TransformPoint(p, p);
    p[0] += psigma*lnoise[i][0];
    p[1] += psigma*lnoise[i][1];
    p[2] += psigma*lnoise[i][2];
    points2->InsertNextPoint(p);
  }

  // compute the landmark transform
  vtkNew<vtkLandmarkTransform> ltrans;
  ltrans->SetMode(mode);
  ltrans->SetSourceLandmarks(points1.Get());
  ltrans->SetTargetLandmarks(points2.Get());
  ltrans->Update();

  // check the determinant
  double det = ltrans->GetMatrix()->Determinant();
  if (det*det < 1e-12)
  {
    rval = 1;
    errstream << "Singular matrix, determinant = " << det << ". ";
  }
  else if (mode == VTK_LANDMARK_AFFINE)
  {
    if (det*scale1*scale2*scale3 < 0.0)
    {
      rval = 1;
      errstream << "Determinant has wrong sign: " << det << ". ";
    }
  }
  else if (mode == VTK_LANDMARK_SIMILARITY)
  {
    if (det < 0.0)
    {
      rval = 1;
      errstream << "Determinant has wrong sign: " << det << ". ";
    }
    else
    {
      double scale = scale1;
      if (dimensionality == 0 || npoints <= 1)
      {
        scale = 1.0;
      }
      for (int j = 0; j < 3; j++)
      {
        double v[3] = { 0.0, 0.0, 0.0 };
        v[j] = 1.0;
        ltrans->TransformVector(v, v);
        double s = vtkMath::Norm(v);
        if ((s - scale)*(s - scale) > 1.1*sigma)
        {
          rval = 1;
          errstream << "Scale should be " << scale << ": " << s << ". ";
          break;
        }
      }
    }
  }
  else if (mode == VTK_LANDMARK_RIGIDBODY)
  {
    if (det < 0.0)
    {
      rval = 1;
      errstream << "Determinant has wrong sign: " << det << ". ";
    }
    else if ((det - 1.0)*(det - 1.0) > 1e-12)
    {
      rval = 1;
      errstream << "Determinant should be 1.0: " << det << ". ";
    }
  }

  // apply the landmark transform and compare to original
  vtkNew<vtkPoints> points3;
  ltrans->TransformPoints(points1.Get(), points3.Get());

  double dsum = 0.0;
  double dmax = 0.0;
  for (int i = 0; i < npoints; i++)
  {
    double p2[3], p3[3];
    points2->GetPoint(i, p2);
    points3->GetPoint(i, p3);
    double d = vtkMath::Distance2BetweenPoints(p2, p3);
    dmax = (dmax > d ? dmax : d);
    dsum += d;
  }

  // we expect average error to be close to sigma
  double r = (npoints > 0 ? sqrt(dsum/npoints) : 0.0);
  if (r > 1.1*sigma)
  {
    rval = 1;
    errstream << "Average error is too high: "
              << "r = " << r << " vs. sigma " << sigma << ". ";
  }

  // we expect the max error to be around 2 sigma
  double e = sqrt(dmax);
  if (e > 2.5*sigma)
  {
    rval = 1;
    errstream << "Maximum error is too high: "
              << "e = " << e << " vs. sigma " << sigma << ". ";
  }

  // the transform should be inverse consistent, meaning that if we swap
  // the points we get the inverse matrix.
  vtkNew<vtkLandmarkTransform> ltrans2;
  ltrans2->SetMode(mode);
  ltrans2->SetSourceLandmarks(points2.Get());
  ltrans2->SetTargetLandmarks(points1.Get());
  ltrans2->Update();

  vtkNew<vtkMatrix4x4> testInverse;
  vtkMatrix4x4::Multiply4x4(ltrans->GetMatrix(), ltrans2->GetMatrix(),
                            testInverse.Get());
  double tol = 1e-6;
  double maxerr = 0.0;
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      double f = testInverse->GetElement(i, j);
      f = (i == j ? (1.0 - f) : f);
      f = fabs(f);
      maxerr = ((f > maxerr) ? f : maxerr);
    }
  }
  if (maxerr > tol)
  {
    rval = 1;
    errstream << "Backwards xform isn't inverse of forward xform: "
              << "error " << maxerr << " > " << tol << ". ";
  }

  if (rval != 0)
  {
    std::cerr << "Error for " << ltrans->GetModeAsString()
              << " with dimensionality=" << dimensionality
              << ", npoints=" << npoints
              << ", sigma=" << sigma
              << ", scale1=" << scale1
              << ", scale2=" << scale2
              << ", scale3=" << scale3
              << ": " << errstream.str() << std::endl;
  }

  return rval;
}

// The registration should be robust even if the points are poorly arranged.
// So we test with:
// 1) a full volumetric spread of points,
// 2) with a coplanar set of points,
// 3) with a colinear set of points,
// 4) and with a coincident set of points (all points at same position).
// Also, the registration should give sensible results even if there are
// only 1, 2, 3 or even no input points.

int TestLandmarkTransform(int,char *[])
{
  int rval = 0;

  struct Conditions
  {
    int mode;
    int dimensionality;
    int npoints;
    double sigma;
    double scale1;
    double scale2;
    double scale3;
  };

  // All sets of test conditions that are commented out are conditions
  // under which vtkLandmarkTransform currently fails.
  const Conditions benchmarks[] =
  {
    // rigid body with different dimensionalities
    { VTK_LANDMARK_RIGIDBODY, 0, 20, 1e-6, 1.0, 1.0, 1.0 },
    { VTK_LANDMARK_RIGIDBODY, 0, 20, 1e-1, 1.0, 1.0, 1.0 },
    { VTK_LANDMARK_RIGIDBODY, 1, 20, 1e-6, 1.0, 1.0, 1.0 },
    { VTK_LANDMARK_RIGIDBODY, 1, 20, 1e-1, 1.0, 1.0, 1.0 },
    { VTK_LANDMARK_RIGIDBODY, 2, 20, 1e-6, 1.0, 1.0, 1.0 },
    { VTK_LANDMARK_RIGIDBODY, 2, 20, 1e-1, 1.0, 1.0, 1.0 },
    { VTK_LANDMARK_RIGIDBODY, 3, 20, 1e-6, 1.0, 1.0, 1.0 },
    { VTK_LANDMARK_RIGIDBODY, 3, 20, 1e-1, 1.0, 1.0, 1.0 },
    // rigid body with different numbers of points
    { VTK_LANDMARK_RIGIDBODY, 3, 0, 1e-6, 1.0, 1.0, 1.0 },
    { VTK_LANDMARK_RIGIDBODY, 3, 0, 1e-1, 1.0, 1.0, 1.0 },
    { VTK_LANDMARK_RIGIDBODY, 3, 1, 1e-6, 1.0, 1.0, 1.0 },
    { VTK_LANDMARK_RIGIDBODY, 3, 1, 1e-1, 1.0, 1.0, 1.0 },
    { VTK_LANDMARK_RIGIDBODY, 3, 2, 1e-6, 1.0, 1.0, 1.0 },
    { VTK_LANDMARK_RIGIDBODY, 3, 2, 1e-1, 1.0, 1.0, 1.0 },
    { VTK_LANDMARK_RIGIDBODY, 3, 3, 1e-6, 1.0, 1.0, 1.0 },
    { VTK_LANDMARK_RIGIDBODY, 3, 3, 1e-1, 1.0, 1.0, 1.0 },
    // similarity with different dimensionalities
    { VTK_LANDMARK_SIMILARITY, 0, 20, 1e-6, 2.8, 2.8, 2.8 },
    { VTK_LANDMARK_SIMILARITY, 0, 20, 1e-1, 2.8, 2.8, 2.8 },
    { VTK_LANDMARK_SIMILARITY, 1, 20, 1e-6, 2.8, 2.8, 2.8 },
    { VTK_LANDMARK_SIMILARITY, 1, 20, 1e-1, 2.8, 2.8, 2.8 },
    { VTK_LANDMARK_SIMILARITY, 2, 20, 1e-6, 2.8, 2.8, 2.8 },
    { VTK_LANDMARK_SIMILARITY, 2, 20, 1e-1, 2.8, 2.8, 2.8 },
    { VTK_LANDMARK_SIMILARITY, 3, 20, 1e-6, 2.8, 2.8, 2.8 },
    { VTK_LANDMARK_SIMILARITY, 3, 20, 1e-1, 2.8, 2.8, 2.8 },
    // similarity with different numbers of points
    { VTK_LANDMARK_SIMILARITY, 3, 0, 1e-6, 2.8, 2.8, 2.8 },
    { VTK_LANDMARK_SIMILARITY, 3, 0, 1e-1, 2.8, 2.8, 2.8 },
    { VTK_LANDMARK_SIMILARITY, 3, 1, 1e-6, 2.8, 2.8, 2.8 },
    { VTK_LANDMARK_SIMILARITY, 3, 1, 1e-1, 2.8, 2.8, 2.8 },
    { VTK_LANDMARK_SIMILARITY, 3, 2, 1e-6, 2.8, 2.8, 2.8 },
    { VTK_LANDMARK_SIMILARITY, 3, 2, 1e-1, 2.8, 2.8, 2.8 },
    { VTK_LANDMARK_SIMILARITY, 3, 3, 1e-6, 2.8, 2.8, 2.8 },
    { VTK_LANDMARK_SIMILARITY, 3, 3, 1e-1, 2.8, 2.8, 2.8 },
    // affine with different dimensionalities
    //{ VTK_LANDMARK_AFFINE, 0, 20, 1e-6, 1.1, 4.2, 2.8 },
    //{ VTK_LANDMARK_AFFINE, 0, 20, 1e-1, 1.1, 4.2, 2.8 },
    //{ VTK_LANDMARK_AFFINE, 1, 20, 1e-6, 1.1, 4.2, 2.8 },
    //{ VTK_LANDMARK_AFFINE, 1, 20, 1e-1, 1.1, 4.2, 2.8 },
    //{ VTK_LANDMARK_AFFINE, 2, 20, 1e-6, 1.1, 4.2, 2.8 },
    //{ VTK_LANDMARK_AFFINE, 2, 20, 1e-1, 1.1, 4.2, 2.8 },
    { VTK_LANDMARK_AFFINE, 3, 20, 1e-6, 1.1, 4.2, 2.8 },
    //{ VTK_LANDMARK_AFFINE, 3, 20, 1e-1, 1.1, 4.2, 2.8 },
    // affine with different numbers of points
    //{ VTK_LANDMARK_AFFINE, 3, 0, 1e-6, 1.0, 1.0, 1.0 },
    //{ VTK_LANDMARK_AFFINE, 3, 0, 1e-1, 1.0, 1.0, 1.0 },
    //{ VTK_LANDMARK_AFFINE, 3, 1, 1e-6, 1.0, 1.0, 1.0 },
    //{ VTK_LANDMARK_AFFINE, 3, 1, 1e-1, 1.0, 1.0, 1.0 },
    //{ VTK_LANDMARK_AFFINE, 3, 2, 1e-6, 2.8, 2.8, 2.8 },
    //{ VTK_LANDMARK_AFFINE, 3, 2, 1e-1, 2.8, 2.8, 2.8 },
    //{ VTK_LANDMARK_AFFINE, 3, 3, 1e-6, 1.1, 4.2, 1.0 },
    //{ VTK_LANDMARK_AFFINE, 3, 3, 1e-1, 1.1, 4.2, 1.0 },
    { VTK_LANDMARK_AFFINE, 3, 4, 1e-6, 1.1, 4.2, 2.8 },
    { VTK_LANDMARK_AFFINE, 3, 4, 1e-1, 1.1, 4.2, 2.8 },
  };

  for (size_t i = 0; i < sizeof(benchmarks)/sizeof(Conditions); i++)
  {
    const Conditions *c = &benchmarks[i];
    rval |= TestSpecificLandmarkTransform(
      c->mode, c->dimensionality, c->npoints, c->sigma,
      c->scale1, c->scale2, c->scale3);
  }

  return rval;
}
