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

#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkOutputWindow.h"
#include "vtkPoints.h"
#include "vtkThinPlateSplineTransform.h"
#include "vtkTransform.h"

#include <iostream>
#include <sstream>

struct Conditions
{
  bool regularizeBulkTransform;
  int npoints;
  double noiseSigma;
  int sourceDimensionality;
  double sourceRotX;
  double sourceRotY;
  double sourceRotZ;
  double sourceScaleX;
  double sourceScaleY;
  double sourceScaleZ;
  double sourceTransX;
  double sourceTransY;
  double sourceTransZ;
  int targetDimensionality;
  double targetRotX;
  double targetRotY;
  double targetRotZ;
  double targetScaleX;
  double targetScaleY;
  double targetScaleZ;
  double targetTransX;
  double targetTransY;
  double targetTransZ;
  bool testForwardTransform;
  bool testInverseTransform;
};

static void SetTransform(vtkTransform* transform, int dimensionality, double rotX, double rotY,
  double rotZ, double scaleX, double scaleY, double scaleZ, double transX, double transY,
  double transZ)
{
  transform->Translate(transX, transY, transZ);
  transform->RotateX(rotX);
  transform->RotateY(rotY);
  transform->RotateZ(rotZ);
  transform->Scale(scaleX, scaleY, scaleZ);
  switch (dimensionality)
  {
    case 0:
      transform->Scale(0.0, 0.0, 0.0);
      break;
    case 1:
      transform->Scale(1.0, 0.0, 0.0);
      break;
    case 2:
      transform->Scale(1.0, 1.0, 0.0);
      break;
  }
}

static int TestTransform(Conditions c)
{
  int errorCode = 0; // will be set to 1 if failure
  std::ostringstream errstream;
  int npoints = (c.npoints > 20 ? 20 : c.npoints);

  const double landmarkPointCoords[20][3] = {
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

  const double landmarkPointNoise[20][3] = {
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

  // There can be some inaccuracies in forward computation when all points are coplanar.
  const double forwardErrorTolerance =
    (c.sourceDimensionality == 3 && c.targetDimensionality == 3 ? 0.0 : 0.001);
  // There can always be some inaccuracies in inverse computation.
  const double inverseErrorTolerance = 0.001;

  vtkNew<vtkTransform> sourceTransform;
  SetTransform(sourceTransform, c.sourceDimensionality, c.sourceRotX, c.sourceRotY, c.sourceRotZ,
    c.sourceScaleX, c.sourceScaleY, c.sourceScaleZ, c.sourceTransX, c.sourceTransY, c.sourceTransZ);

  // generate the transform we want to recover
  vtkNew<vtkTransform> targetTransform;
  SetTransform(targetTransform, c.targetDimensionality, c.targetRotX, c.targetRotY, c.targetRotZ,
    c.targetScaleX, c.targetScaleY, c.targetScaleZ, c.targetTransX, c.targetTransY, c.targetTransZ);

  // create the two point sets
  vtkNew<vtkPoints> sourcePoints;
  vtkNew<vtkPoints> targetPoints;
  double psigma = c.noiseSigma / sqrt(3.0);
  for (int i = 0; i < npoints; i++)
  {
    double sourcePoint[3] = { landmarkPointCoords[i][0], landmarkPointCoords[i][1],
      landmarkPointCoords[i][2] };
    sourceTransform->TransformPoint(sourcePoint, sourcePoint);
    sourcePoints->InsertNextPoint(sourcePoint);

    double targetPoint[3] = { landmarkPointCoords[i][0], landmarkPointCoords[i][1],
      landmarkPointCoords[i][2] };
    targetTransform->TransformPoint(targetPoint, targetPoint);
    targetPoint[0] += psigma * landmarkPointNoise[i][0];
    targetPoint[1] += psigma * landmarkPointNoise[i][1];
    targetPoint[2] += psigma * landmarkPointNoise[i][2];
    targetPoints->InsertNextPoint(targetPoint);
  }

  vtkNew<vtkThinPlateSplineTransform> ltrans;
  ltrans->SetBasisToR();
  ltrans->SetRegularizeBulkTransform(c.regularizeBulkTransform);
  ltrans->SetSourceLandmarks(sourcePoints);
  ltrans->SetTargetLandmarks(targetPoints);
  ltrans->Update();

  if (c.testForwardTransform)
  {
    vtkNew<vtkPoints> transformedSourcePoints;
    ltrans->TransformPoints(sourcePoints, transformedSourcePoints);

    double dsum = 0.0;
    double dmax = 0.0;
    for (int i = 0; i < npoints; i++)
    {
      double targetPoint[3] = { 0.0 };
      targetPoints->GetPoint(i, targetPoint);
      double transformedSourcePoint[3] = { 0.0 };
      transformedSourcePoints->GetPoint(i, transformedSourcePoint);
      double d = vtkMath::Distance2BetweenPoints(targetPoint, transformedSourcePoint);
      dmax = (dmax > d ? dmax : d);
      dsum += d;
    }

    // we expect average error to be close to noiseSigma
    double r = (npoints > 0 ? sqrt(dsum / npoints) : 0.0);
    if (r > 1.1 * c.noiseSigma + forwardErrorTolerance)
    {
      errorCode = 1;
      errstream << "Forward transform average error is too high: "
                << "r = " << r << " vs. noiseSigma " << c.noiseSigma << ". " << std::endl;
    }

    // we expect the max error to be around 2 noiseSigma
    double e = sqrt(dmax);
    if (e > 2.5 * c.noiseSigma + forwardErrorTolerance)
    {
      errorCode = 1;
      errstream << "Forward transform maximum error is too high: "
                << "e = " << e << " vs. noiseSigma " << c.noiseSigma << ". " << std::endl;
    }
  }

  // Test inverse transform
  if (c.testInverseTransform)
  {
    ltrans->Inverse();
    vtkNew<vtkPoints> transformedTargetPoints;
    ltrans->TransformPoints(targetPoints, transformedTargetPoints);

    double dsum = 0.0;
    double dmax = 0.0;
    for (int i = 0; i < npoints; i++)
    {
      double sourcePoint[3] = { 0.0 };
      sourcePoints->GetPoint(i, sourcePoint);
      double transformedTargetPoint[3] = { 0.0 };
      transformedTargetPoints->GetPoint(i, transformedTargetPoint);
      double d = vtkMath::Distance2BetweenPoints(sourcePoint, transformedTargetPoint);
      dmax = (dmax > d ? dmax : d);
      dsum += d;
    }

    // we expect average error to be close to noiseSigma
    double r = (npoints > 0 ? sqrt(dsum / npoints) : 0.0);
    if (r > 1.1 * c.noiseSigma + inverseErrorTolerance)
    {
      errorCode = 1;
      errstream << "Inverse transform average error is too high: "
                << "r = " << r << " vs. noiseSigma " << c.noiseSigma << ". " << std::endl;
    }

    // we expect the max error to be around 2 noiseSigma
    double e = sqrt(dmax);
    if (e > 2.5 * c.noiseSigma + inverseErrorTolerance)
    {
      errorCode = 1;
      errstream << "Inverse transform maximum error is too high: "
                << "e = " << e << " vs. noiseSigma " << c.noiseSigma << ". " << std::endl;
    }
  }

  if (errorCode != 0)
  {
    std::cerr << "Error for test case with " << npoints
              << " points, regularizeBulkTransform = " << c.regularizeBulkTransform << std::endl
              << "  Source: dimensionality = " << c.sourceDimensionality << std::endl
              << "         rotation = " << c.sourceRotX << " " << c.sourceRotY << " "
              << c.sourceRotZ << std::endl
              << "            scale = " << c.sourceScaleX << " " << c.sourceScaleY << " "
              << c.sourceScaleZ << std::endl
              << "      translation = " << c.sourceTransX << " " << c.sourceTransY << " "
              << c.sourceTransZ << std::endl
              << "  Target: dimensionality = " << c.targetDimensionality << std::endl
              << "         rotation = " << c.targetRotX << " " << c.targetRotY << " "
              << c.targetRotZ << std::endl
              << "            scale = " << c.targetScaleX << " " << c.targetScaleY << " "
              << c.targetScaleZ << std::endl
              << "      translation = " << c.targetTransX << " " << c.targetTransY << " "
              << c.targetTransZ << std::endl
              << "       noiseSigma = " << c.noiseSigma << std::endl
              << "Details:" << std::endl
              << errstream.str() << std::endl;
  }

  return errorCode;
}

// The registration should be robust even if the points are poorly arranged.
// So we test with:
// 1) a full volumetric spread of points,
// 2) with a coplanar set of points,
// 3) with a colinear set of points,
// 4) and with a coincident set of points (all points at same position).
// Also, the registration should give sensible results even if there are
// only 1, 2, 3 or even no input points.

int TestThinPlateSplineTransform(int, char*[])
{
  Conditions condition;
  condition.regularizeBulkTransform = true;
  condition.npoints = 20;
  condition.noiseSigma = 0.0;
  condition.sourceDimensionality = 3;
  condition.sourceRotX = condition.sourceRotY = condition.sourceRotZ = 0.0;
  condition.sourceScaleX = 20.0;
  condition.sourceScaleY = 30.0;
  condition.sourceScaleZ = 40.0;
  condition.sourceTransX = condition.sourceTransY = condition.sourceTransZ = 0.0;
  condition.targetDimensionality = 3;
  condition.targetRotX = condition.targetRotY = condition.targetRotZ = 0.0;
  condition.targetScaleX = condition.sourceScaleX;
  condition.targetScaleY = condition.sourceScaleY;
  condition.targetScaleZ = condition.sourceScaleZ;
  condition.targetTransX = condition.targetTransY = condition.targetTransZ = 0.0;
  condition.testForwardTransform = true;
  condition.testInverseTransform = true;

  int numberOfErrors = 0;
  // Test with and without bulk transform regularization
  for (int regularizeBulkTransform = 0; regularizeBulkTransform < 2; regularizeBulkTransform++)
  {
    condition.regularizeBulkTransform = (regularizeBulkTransform != 0);
    // Test with target points distributed in a 3D cube, plane, and line
    for (int targetDimensionality = 3; targetDimensionality >= 1; targetDimensionality--)
    {
      condition.targetDimensionality = targetDimensionality;
      // Test with source points distributed in a 3D cube, plane, and line
      for (int sourceDimensionality = 3; sourceDimensionality >= 1; sourceDimensionality--)
      {
        condition.sourceDimensionality = sourceDimensionality;
        // Test with noise of 0.0, 5.0, and 10.0
        for (int noise = 0; noise < 3; noise++)
        {
          condition.noiseSigma = double(noise) * 5.0;
          // Test with and without source translation
          for (int sourceTranslated = 0; sourceTranslated < 2; sourceTranslated++)
          {
            if (sourceTranslated)
            {
              condition.sourceTransX = 21.5;
              condition.sourceTransY = -11.5;
              condition.sourceTransZ = 41.5;
            }
            else
            {
              condition.sourceTransX = 0.0;
              condition.sourceTransY = 0.0;
              condition.sourceTransZ = 0.0;
            }
            // Test with and without target translation
            for (int targetTranslated = 0; targetTranslated < 2; targetTranslated++)
            {
              if (targetTranslated)
              {
                condition.targetTransX = 42.1;
                condition.targetTransY = 25.3;
                condition.targetTransZ = 31.9;
              }
              else
              {
                condition.targetTransX = 0.0;
                condition.targetTransY = 0.0;
                condition.targetTransZ = 0.0;
              }
              // Test with various source rotations
              // Test when points are rotated exactly by 90deg.
              // These can cause singularities in the bulk transformation matrix therefore must be
              // tested carefully.
              for (int sourceRotated = 0; sourceRotated < 5; sourceRotated++)
              {
                if (sourceRotated == 0)
                {
                  condition.sourceRotX = 0.0;
                  condition.sourceRotY = 0.0;
                  condition.sourceRotZ = 0.0;
                }
                else if (sourceRotated == 1)
                {
                  condition.sourceRotX = 90.0;
                  condition.sourceRotY = 0.0;
                  condition.sourceRotZ = 0.0;
                }
                else if (sourceRotated == 2)
                {
                  condition.sourceRotX = 0.0;
                  condition.sourceRotY = 90.0;
                  condition.sourceRotZ = 0.0;
                }
                else if (sourceRotated == 3)
                {
                  condition.sourceRotX = 0.0;
                  condition.sourceRotY = 0.0;
                  condition.sourceRotZ = 90.0;
                }
                else if (sourceRotated == 4)
                {
                  condition.sourceRotX = 20.0;
                  condition.sourceRotY = -11.0;
                  condition.sourceRotZ = 132.0;
                }
                // Test with various target rotations
                for (int targetRotated = 0; targetRotated < 5; targetRotated++)
                {
                  if (targetRotated == 0)
                  {
                    condition.targetRotX = 0.0;
                    condition.targetRotY = 0.0;
                    condition.targetRotZ = 0.0;
                  }
                  else if (targetRotated == 1)
                  {
                    condition.targetRotX = 90.0;
                    condition.targetRotY = 0.0;
                    condition.targetRotZ = 0.0;
                  }
                  else if (targetRotated == 2)
                  {
                    condition.targetRotX = 0.0;
                    condition.targetRotY = 90.0;
                    condition.targetRotZ = 0.0;
                  }
                  else if (targetRotated == 3)
                  {
                    condition.targetRotX = 0.0;
                    condition.targetRotY = 0.0;
                    condition.targetRotZ = 90.0;
                  }
                  else if (targetRotated == 4)
                  {
                    condition.targetRotX = -18.0;
                    condition.targetRotY = 37.2;
                    condition.targetRotZ = 23.7;
                  }

                  if (condition.sourceDimensionality == 3 && condition.targetDimensionality == 3)
                  {
                    // Points are not coplanar, we test forward and inverse transforms
                    condition.testForwardTransform = true;
                    condition.testInverseTransform = true;
                  }
                  else
                  {
                    // Points are coplanar, there are some limitations of what transforms are tested
                    if (condition.regularizeBulkTransform)
                    {
                      // If regularization is enabled then both forward and inverse transforms are
                      // computed but only if all points are in XY plane.
                      bool allPointsInXYPlane = condition.sourceDimensionality == 2 &&
                        condition.targetDimensionality == 2 && sourceRotated == 0 &&
                        targetRotated == 0 && sourceTranslated == 0 && targetTranslated == 0 &&
                        noise == 0;
                      condition.testForwardTransform = allPointsInXYPlane;
                      condition.testInverseTransform = allPointsInXYPlane;
                    }
                    else
                    {
                      // If regularization is disabled then all coplanar configuration work but
                      // only for forward transform.
                      condition.testForwardTransform = true;
                      condition.testInverseTransform = false;
                    }
                  }

                  // Test this condition
                  if (TestTransform(condition))
                  {
                    numberOfErrors++;
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  if (numberOfErrors > 0)
  {
    std::cerr << std::endl << "Number of errors: " << numberOfErrors << std::endl;
    return 1;
  }

  return 0;
}
