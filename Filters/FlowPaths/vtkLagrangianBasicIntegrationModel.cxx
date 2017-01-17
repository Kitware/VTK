/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangianBasicIntegrationModel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLagrangianBasicIntegrationModel.h"

#include "vtkCellData.h"
#include "vtkCellLocator.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkGenericCell.h"
#include "vtkIntArray.h"
#include "vtkLagrangianParticle.h"
#include "vtkLagrangianParticleTracker.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkQuad.h"
#include "vtkSetGet.h"
#include "vtkSmartPointer.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVector.h"

#include <cassert>
#include <set>
#include <sstream>
#include <vector>

//----------------------------------------------------------------------------
// Internal Classes
//----------------------------------------------------------------------------

//created by Shaun David Ramsey and Kristin Potter copyright (c) 2003
//email ramsey()cs.utah.edu with any quesitons
/*=========================================================================
  This copyright notice is available at:
http://www.opensource.org/licenses/mit-license.php

Copyright (c) 2003 Shaun David Ramsey, Kristin Potter, Charles Hansen

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sel copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
=========================================================================*/
/**
 * @class   vtkLagrangianBilinearQuadIntersection
 * @brief   Class to perform non planar quad intersection
 *
 * Class for non planar intersection
 * This class is based on
 * http://shaunramsey.com/research/bp/
 * which does not work in the general case
 * hence the ugly transformation patch.
*/

class vtkLagrangianBilinearQuadIntersection
{
public:

  vtkLagrangianBilinearQuadIntersection(const vtkVector3d& pt00,
    const vtkVector3d& Pt01, const vtkVector3d& Pt10, const vtkVector3d& Pt11);
  ~vtkLagrangianBilinearQuadIntersection();

  /**
   * Compute cartesian coordinates of point in the quad
   * using parameteric coordinates
   */
  vtkVector3d ComputeCartesianCoordinates(double u, double v);

  /**
   * Compute the intersection between a ray r->d and the quad
   */
  bool RayIntersection(const vtkVector3d& r, const vtkVector3d& d,
                       vtkVector3d& uv);

  /**
   * find roots of ax^2+bx+c=0  in the interval min,max.
   * place the roots in u[2] and return how many roots found
   */
  static int QuadraticRoot(double a, double b, double c,
    double min, double max, double *u);

  /**
   * Compute interesection factor
   */
  static double ComputeIntersectionFactor(const vtkVector3d& dir, const vtkVector3d& orig,
    const vtkVector3d& srfpos);

  /**
   * Compute best denominator
   */
  static double GetBestDenominator(double v, double m1, double m2, double J1, double J2,
    double K1, double K2, double R1, double R2);

private:

  vtkVector3d* Point00;
  vtkVector3d* Point01;
  vtkVector3d* Point10;
  vtkVector3d* Point11;
  int AxesSwapping;
};

#define RAY_EPSILON 1e-12 // some small epsilon for flt pt
#define USER_SURFACE_TYPE 100 // Minimal value for user defined surface type

//----------------------------------------------------------------------------
vtkLagrangianBilinearQuadIntersection::vtkLagrangianBilinearQuadIntersection(
  const vtkVector3d& pt00, const vtkVector3d& pt01,
  const vtkVector3d& pt10, const vtkVector3d& pt11)
{
  this->Point00 = new vtkVector3d(pt00.GetData());
  this->Point01 = new vtkVector3d(pt01.GetData());
  this->Point10 = new vtkVector3d(pt10.GetData());
  this->Point11 = new vtkVector3d(pt11.GetData());
  this->AxesSwapping = 0;
}

//----------------------------------------------------------------------------
vtkLagrangianBilinearQuadIntersection::~vtkLagrangianBilinearQuadIntersection()
{
  delete this->Point00;
  delete this->Point01;
  delete this->Point10;
  delete this->Point11;
}

//----------------------------------------------------------------------------
vtkVector3d vtkLagrangianBilinearQuadIntersection::ComputeCartesianCoordinates(
  double u, double v)
{
  vtkVector3d respt;
  respt.SetX(((1.0 - u) * (1.0 - v) * this->Point00->GetX() +
    (1.0 - u) * v * this->Point01->GetX() +
    u * (1.0 - v) * this->Point10->GetX() +
    u * v * this->Point11->GetX()));
  respt.SetY(((1.0 - u) * (1.0 - v) * this->Point00->GetY() +
    (1.0 - u) * v * this->Point01->GetY() +
    u * (1.0 - v) * this->Point10->GetY() +
    u * v * this->Point11->GetY()));
  respt.SetZ(((1.0 - u) * (1.0 - v) * this->Point00->GetZ() +
    (1.0 - u) * v * this->Point01->GetZ() +
    u * (1.0 - v) * this->Point10->GetZ() +
    u * v * this->Point11->GetZ()));

  int nbOfSwap = this->AxesSwapping;
  while (nbOfSwap != 0)
  {
    double tmp = respt.GetZ();
    respt.SetZ(respt.GetY());
    respt.SetY(respt.GetX());
    respt.SetX(tmp);
    nbOfSwap--;
  }
  return respt;
}

//----------------------------------------------------------------------------
double vtkLagrangianBilinearQuadIntersection::GetBestDenominator(double v,
  double M1, double M2, double J1, double J2,
  double K1, double K2, double R1, double R2)
{
  double denom = (v * (M1 - M2) + J1 - J2);
  double d2 = (v * M1 + J1);
  if (fabs(denom) > fabs(d2)) // which denominator is bigger
  {
    return (v * (K2 - K1) + R2 - R1) / denom;
  }
  return -(v * K1 + R1) / d2;
}

//----------------------------------------------------------------------------
double vtkLagrangianBilinearQuadIntersection::ComputeIntersectionFactor(
  const vtkVector3d& dir, const vtkVector3d& orig, const vtkVector3d& srfpos)
{
  // if x is bigger than y and z
  if (fabs(dir.GetX()) >= fabs(dir.GetY()) && fabs(dir.GetX()) >= fabs(dir.GetZ()))
  {
    return (srfpos.GetX() - orig.GetX()) / dir.GetX();
  }
  // if y is bigger than x and z
  else if (fabs(dir.GetY()) >= fabs(dir.GetZ())) // && fabs(dir.GetY()) >= fabs(dir.GetX()))
  {
    return (srfpos.GetY() - orig.GetY()) / dir.GetY();
  }
  // otherwise x isn't bigger than both and y isn't bigger than both
  else //if(fabs(dir.GetZ()) >= fabs(dir.GetX()) && fabs(dir.GetZ()) >= fabs(dir.GetY()))
  {
    return (srfpos.GetZ() - orig.GetZ()) / dir.GetZ();
  }
}

//----------------------------------------------------------------------------
bool vtkLagrangianBilinearQuadIntersection::RayIntersection(const vtkVector3d& r,
  const vtkVector3d& q, vtkVector3d &uv)
{
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Equation of the ray intersection:
  // P(u, v) = (1-u)(1-v)this->Point00->+ (1-u)vthis->Point01->+
  //   u(1-v)this->Point10->+ uvthis->Point11
  // Equation of the ray:
  // R(t) = r + tq
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  vtkVector3d pos1, pos2; //vtkVector3d pos = ro + t * rd;
  int num_sol; // number of solutions to the quadratic
  double vsol[2]; // the two roots from quadraticroot
  double t2, u; // the t values of the two roots

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Variables for substitition
  // a = this->Point11->- this->Point10->- this->Point01->+ this->Point00
  // b = this->Point10->- this->Point00
  // c = this->Point01->- this->Point00
  // d = this->Point00-> (d is shown below in the #ifdef ray area)
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Retrieve the xyz of the q part of ray
  double qx = q.GetX();
  double qy = q.GetY();
  double qz = q.GetZ();

  double rx = r.GetX();
  double ry = r.GetY();
  double rz = r.GetZ();

  this->AxesSwapping = 0;
  while (((qx == qz) && (qz == 0)) || ((qy == qz) && (qz == 0)))
  {
    this->AxesSwapping++;
    double tmp;

    tmp = qx;
    qx = qy;
    qy = qz;
    qz = tmp;

    tmp = rx;
    rx = ry;
    ry = rz;
    rz = tmp;

    tmp = this->Point00->GetX();
    this->Point00->SetX(this->Point00->GetY());
    this->Point00->SetY(this->Point00->GetZ());
    this->Point00->SetZ(tmp);

    tmp = this->Point01->GetX();
    this->Point01->SetX(this->Point01->GetY());
    this->Point01->SetY(this->Point01->GetZ());
    this->Point01->SetZ(tmp);

    tmp = this->Point10->GetX();
    this->Point10->SetX(this->Point10->GetY());
    this->Point10->SetY(this->Point10->GetZ());
    this->Point10->SetZ(tmp);

    tmp = this->Point11->GetX();
    this->Point11->SetX(this->Point11->GetY());
    this->Point11->SetY(this->Point11->GetZ());
    this->Point11->SetZ(tmp);
  }

  // Find a w.r.t. x, y, z
  double ax = this->Point11->GetX() - this->Point10->GetX() - this->Point01->GetX() +
    this->Point00->GetX();
  double ay = this->Point11->GetY() - this->Point10->GetY() - this->Point01->GetY() +
    this->Point00->GetY();
  double az = this->Point11->GetZ() - this->Point10->GetZ() - this->Point01->GetZ() +
    this->Point00->GetZ();

  // Find b w.r.t. x, y, z
  double bx = this->Point10->GetX() - this->Point00->GetX();
  double by = this->Point10->GetY() - this->Point00->GetY();
  double bz = this->Point10->GetZ() - this->Point00->GetZ();

  // Find c w.r.t. x, y, z
  double cx = this->Point01->GetX() - this->Point00->GetX();
  double cy = this->Point01->GetY() - this->Point00->GetY();
  double cz = this->Point01->GetZ() - this->Point00->GetZ();

  // Find d w.r.t. x, y, z - subtracting r just after
  double dx = this->Point00->GetX() - rx;
  double dy = this->Point00->GetY() - ry;
  double dz = this->Point00->GetZ() - rz;

  // Find A1 and A2
  double A1 = ax * qz - az * qx;
  double A2 = ay * qz - az * qy;

  // Find B1 and B2
  double B1 = bx * qz - bz * qx;
  double B2 = by * qz - bz * qy;

  // Find C1 and C2
  double C1 = cx * qz - cz * qx;
  double C2 = cy * qz - cz * qy;

  // Find D1 and D2
  double D1 = dx * qz - dz * qx;
  double D2 = dy * qz - dz * qy;

  double A = A2 * C1 - A1 * C2;
  double B = A2 * D1 - A1 * D2 + B2 * C1 - B1 * C2;
  double C = B2 * D1 - B1 * D2;

  uv.SetX(-2); uv.SetY(-2); uv.SetZ(-2);
  num_sol = vtkLagrangianBilinearQuadIntersection::QuadraticRoot(A, B, C,
    -RAY_EPSILON, 1 + RAY_EPSILON, vsol);
  switch (num_sol)
  {
    case 0:
      return false; // no solutions found
    case 1:
      uv.SetY(vsol[0]);
      uv.SetX(vtkLagrangianBilinearQuadIntersection::GetBestDenominator(uv.GetY(),
        A2, A1, B2, B1, C2, C1, D2, D1));
      pos1 = this->ComputeCartesianCoordinates(uv.GetX(), uv.GetY());
      uv.SetZ(vtkLagrangianBilinearQuadIntersection::ComputeIntersectionFactor(q, r, pos1));

      if (uv.GetX() < 1 + RAY_EPSILON && uv.GetX() >
        -RAY_EPSILON && uv.GetZ() > 0)//vars okay?
      {
        return true;
      }
      else
      {
        return false; // no other soln - so ret false
      }
    case 2: // two solutions found
      uv.SetY(vsol[0]);
      uv.SetX(vtkLagrangianBilinearQuadIntersection::GetBestDenominator(uv.GetY(),
        A2, A1, B2, B1, C2, C1, D2, D1));
      pos1 = this->ComputeCartesianCoordinates(uv.GetX(), uv.GetY());
      uv.SetZ(vtkLagrangianBilinearQuadIntersection::ComputeIntersectionFactor(q, r, pos1));

      if (uv.GetX() < 1 + RAY_EPSILON && uv.GetX() > -RAY_EPSILON && uv.GetZ() > 0)
      {
        u = vtkLagrangianBilinearQuadIntersection::GetBestDenominator(vsol[1],
          A2, A1, B2, B1, C2, C1, D2, D1);
        if (u < 1 + RAY_EPSILON && u > RAY_EPSILON)
        {
          pos2 = this->ComputeCartesianCoordinates(u, vsol[1]);
          t2 = vtkLagrangianBilinearQuadIntersection::ComputeIntersectionFactor(q, r, pos2);
          if (t2 < 0 || uv.GetZ() < t2) // t2 is bad or t1 is better
          {
            return true;
          }
          // other wise both t2 > 0 and t2 < t1
          uv.SetY(vsol[1]);
          uv.SetX(u);
          uv.SetZ(t2);
          return true;
        }
        return true; // u2 is bad but u1 vars are still okay
      }
      else // doesn't fit in the root - try other one
      {
        uv.SetY(vsol[1]);
        uv.SetX(vtkLagrangianBilinearQuadIntersection::GetBestDenominator(vsol[1],
          A2, A1, B2, B1, C2, C1, D2, D1));
        pos1 = this->ComputeCartesianCoordinates(uv.GetX(), uv.GetY());
        uv.SetZ(vtkLagrangianBilinearQuadIntersection::ComputeIntersectionFactor(q, r, pos1));
        if (uv.GetX() < 1 + RAY_EPSILON && uv.GetX() > -RAY_EPSILON && uv.GetZ() > 0)
        {
          return true;
        }
        else
        {
          return false;
        }
      }
    default:
      return false;
  }
}

//----------------------------------------------------------------------------
int vtkLagrangianBilinearQuadIntersection::QuadraticRoot(double a, double b, double c,
  double min, double max, double* u)
{
  if (a == 0.0) // then its close to 0
  {
    if (b != 0.0) // not close to 0
    {
      u[0] = - c / b;
      if (u[0] > min && u[0] < max) //its in the interval
      {
        return 1; //1 soln found
      }
      else //its not in the interval
      {
        return 0;
      }
    }
    else
    {
      return 0;
    }
  }
  double d = b * b - 4 * a * c; //discriminant
  if (d <= 0.0) // single or no root
  {
    if (d == 0.0) // close to 0
    {
      u[0] = -b / a;
      if (u[0] > min && u[0] < max) // its in the interval
      {
        return 1;
      }
      else //its not in the interval
      {
        return 0;
      }
    }
    else // no root d must be below 0
    {
      return 0;
    }
  }
#ifdef WIN32
  double q = -0.5 * (b + _copysign(sqrt(d), b));
#else
  double q = -0.5 * (b + copysign(sqrt(d), b));
#endif
  u[0] = c / q;
  u[1] = q / a;

  if ((u[0] > min && u[0] < max)
      && (u[1] > min && u[1] < max))
  {
    return 2;
  }
  else if (u[0] > min && u[0] < max) //then one wasn't in interval
  {
    return 1;
  }
  else if (u[1] > min && u[1] < max)
  { // make it easier, make u[0] be the valid one always
    double dummy;
    dummy = u[0];
    u[0] = u[1];
    u[1] = dummy; // just in case somebody wants to check it
    return 1;
  }
  return 0;
}
//----------------------------------------------------------------------------
// End Of Internal Classes
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
typedef std::vector<vtkSmartPointer<vtkAbstractCellLocator> > LocatorsTypeBase;
class vtkLocatorsType : public LocatorsTypeBase
{
};

typedef std::vector<vtkDataSet*> DataSetsTypeBase;
class vtkDataSetsType : public DataSetsTypeBase
{
};

typedef std::pair<unsigned int, vtkDataSet*> SurfaceItem;
typedef std::vector<SurfaceItem> SurfaceTypeBase;
class vtkSurfaceType : public SurfaceTypeBase
{
};

typedef std::pair<unsigned int, double> PassThroughItem;
typedef std::set<PassThroughItem> PassThroughSetType;

//----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkLagrangianBasicIntegrationModel, Locator, vtkAbstractCellLocator);

//----------------------------------------------------------------------------
vtkLagrangianBasicIntegrationModel::vtkLagrangianBasicIntegrationModel():
  Locator(NULL),
  LastLocator(NULL),
  LastDataSet(NULL),
  WeightsSize(0),
  CurrentParticle(NULL),
  TmpParticle(NULL),
  TmpArray(NULL),
  Tolerance(1.0e-8),
  NonPlanarQuadSupport(false),
  UseInitialIntegrationTime(false),
  Tracker(NULL)
{
  SurfaceArrayDescription surfaceTypeDescription;
  surfaceTypeDescription.nComp = 1;
  surfaceTypeDescription.type = VTK_INT;
  surfaceTypeDescription.enumValues.push_back(std::make_pair(SURFACE_TYPE_MODEL, "ModelDefined"));
  surfaceTypeDescription.enumValues.push_back(std::make_pair(SURFACE_TYPE_TERM, "Terminate"));
  surfaceTypeDescription.enumValues.push_back(std::make_pair(SURFACE_TYPE_BOUNCE, "Bounce"));
  surfaceTypeDescription.enumValues.push_back(std::make_pair(SURFACE_TYPE_BREAK, "BreakUp"));
  surfaceTypeDescription.enumValues.push_back(std::make_pair(SURFACE_TYPE_PASS, "PassThrough"));
  this->SurfaceArrayDescriptions["SurfaceType"] = surfaceTypeDescription;

  this->SeedArrayNames->InsertNextValue("ParticleInitialVelocity");
  this->SeedArrayComps->InsertNextValue(3);
  this->SeedArrayTypes->InsertNextValue(VTK_DOUBLE);
  this->SeedArrayNames->InsertNextValue("ParticleInitialIntegrationTime");
  this->SeedArrayComps->InsertNextValue(1);
  this->SeedArrayTypes->InsertNextValue(VTK_DOUBLE);

  this->Locators = new vtkLocatorsType;
  this->DataSets = new vtkDataSetsType;
  this->Cell = vtkGenericCell::New();
  this->LastWeights = new double[this->WeightsSize];
  this->Surfaces = new vtkSurfaceType;
  this->SurfaceLocators = new vtkLocatorsType;

  // Using a vtkCellLocator by default
  vtkAbstractCellLocator* locator = vtkCellLocator::New();
  this->SetLocator(locator);
  locator->Delete();
}

//----------------------------------------------------------------------------
vtkLagrangianBasicIntegrationModel::~vtkLagrangianBasicIntegrationModel()
{
  this->ClearDataSets(true);
  this->Cell->Delete();
  delete[] this->LastWeights;
  this->SetLocator(NULL);
  this->LastLocator = NULL;
  delete this->Locators;
  delete this->DataSets;
  delete this->Surfaces;
  delete this->SurfaceLocators;

  if (this->TmpParticle != NULL)
  {
    delete this->TmpParticle;
  }

  if (this->TmpArray != NULL)
  {
    this->TmpArray->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkLagrangianBasicIntegrationModel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Locator)
  {
    os << indent << "Locator: " << endl;
    this->Locator->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Locator: " << this->Locator << endl;
  }
  if (this->CurrentParticle)
  {
    os << indent << "CurrentParticle: " << endl;
    this->CurrentParticle->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "CurrentParticle: " << this->CurrentParticle << endl;
  }
  os << indent << "Tolerance: " << this->Tolerance << endl;
}

//----------------------------------------------------------------------------
void vtkLagrangianBasicIntegrationModel::SetTracker(
  vtkLagrangianParticleTracker* tracker)
{
  this->Tracker = tracker;
}

//----------------------------------------------------------------------------
void vtkLagrangianBasicIntegrationModel::AddDataSet(vtkDataSet * dataset,
  bool surface, unsigned int surfaceFlatIndex)
{
  // Sanity check
  if (!dataset)
  {
    vtkErrorMacro(<< "Dataset NULL!");
    return;
  }

  if (!this->Locator)
  {
    vtkErrorMacro(<< "Locator NULL");
    return;
  }

  // insert the dataset into DataSet vector
  if (surface)
  {
    dataset->Register(this);
    this->Surfaces->push_back(std::make_pair(surfaceFlatIndex, dataset));
  }
  else
  {
    this->DataSets->push_back(dataset);
  }

  // insert a locator into Locators vector, non-null only for vtkPointSet
  vtkSmartPointer<vtkAbstractCellLocator> locator = 0;
  if (dataset->IsA("vtkPointSet"))
  {
    if (surface)
    {
      locator.TakeReference(vtkCellLocator::New());
    }
    else
    {
      locator.TakeReference(this->Locator->NewInstance());
    }
    locator->SetDataSet(dataset);
    locator->CacheCellBoundsOn();
    locator->AutomaticOn();
    locator->BuildLocator();
  }

  // Add locator
  if (surface)
  {
    this->SurfaceLocators->push_back(locator);
  }
  else
  {
    this->Locators->push_back(locator);

    // Resize LastWeights if necessary
    int size = dataset->GetMaxCellSize();
    if (size > this->WeightsSize)
    {
      this->WeightsSize = size;
      delete[] this->LastWeights;
      this->LastWeights = new double[size];
    }
  }
}

//----------------------------------------------------------------------------
void vtkLagrangianBasicIntegrationModel::ClearDataSets(bool surface)
{
  if (surface)
  {
    for (size_t iDs = 0; iDs < this->Surfaces->size(); iDs++)
    {
      (*this->Surfaces)[iDs].second->UnRegister(this);
    }
    this->Surfaces->clear();
    this->SurfaceLocators->clear();
  }
  else
  {
    this->DataSets->clear();
    this->Locators->clear();
    this->LastDataSet = NULL;
    this->LastLocator = NULL;

    this->WeightsSize = 0;
    delete[] this->LastWeights;
    this->LastWeights = NULL;
  }
}

//----------------------------------------------------------------------------
int vtkLagrangianBasicIntegrationModel::FunctionValues(double* x, double* f)
{
  // Sanity check
  if (this->DataSets->empty())
  {
    vtkErrorMacro(<< "Please add a dataset to the integration model before integrating.");
    return 0;
  }
  vtkAbstractCellLocator* loc;
  vtkDataSet* ds;
  vtkIdType cellId;
  if (this->FindInLocators(x, ds, cellId, loc, this->LastWeights))
  {
    // Evalute integration model velocity field with the found cell
    if (this->FunctionValues(ds, cellId, this->LastWeights, x, f) != 0)
    {
      this->LastDataSet = ds;
      this->LastLocator = loc;
      if (this->CurrentParticle)
      {
        // Found a cell, keep an hand to it in the particle
        this->CurrentParticle->SetLastCell(ds, cellId);
      }
      return 1;
    }
  }

  // Can't evaluate
  return 0;
}

//---------------------------------------------------------------------------
vtkLagrangianParticle* vtkLagrangianBasicIntegrationModel::ComputeSurfaceInteraction(
  vtkLagrangianParticle* particle, std::queue<vtkLagrangianParticle*>& particles,
  unsigned int& surfaceFlatIndex, PassThroughParticlesType& passThroughParticles)
{
  vtkDataSet* surface = NULL;
  double interFactor = 1.0;
  vtkIdType cellId = -1;
  int surfaceType = -1;
  PassThroughSetType passThroughInterSet;
  bool perforation;
  do
  {
    passThroughInterSet.clear();
    perforation = false;
    for (size_t iDs = 0; iDs < this->Surfaces->size(); iDs++)
    {
      vtkAbstractCellLocator* loc = (*this->SurfaceLocators)[iDs].GetPointer();
      vtkDataSet* tmpSurface = (*this->Surfaces)[iDs].second;
      vtkNew<vtkIdList> cellList;
      loc->FindCellsAlongLine(particle->GetPosition(), particle->GetNextPosition(),
        this->Tolerance, cellList.Get());
      for (vtkIdType i = 0; i < cellList->GetNumberOfIds(); i++)
      {
        double tmpFactor;
        double tmpPoint[3];
        vtkIdType tmpCellId = cellList->GetId(i);
        vtkCell* cell = tmpSurface->GetCell(tmpCellId);
        if (this->IntersectWithLine(cell, particle->GetPosition(),
          particle->GetNextPosition(), this->Tolerance,
          tmpFactor, tmpPoint) == 0)
        {
          // FindCellAlongsLines sometimes get false positives
          continue;
        }
        if (tmpFactor < interFactor)
        {
          // Recover surface type for this cell
          int nComponent;
          double* surfaceTypePtr;

          // "SurfaceType" is at index 2
          int surfaceIndex = 2;

          vtkIdType surfaceTupleId = tmpCellId;

          // When using field data surface type, tuple index is 0
          int ret = this->GetFlowOrSurfaceDataFieldAssociation(surfaceIndex);
          if (ret == -1)
          {
            vtkErrorMacro(<< "Surface Type is not correctly set in surface dataset");
            return NULL;
          }
          if (ret == vtkDataObject::FIELD_ASSOCIATION_NONE)
          {
            surfaceTupleId = 0;
          }
          if (!this->GetFlowOrSurfaceData(surfaceIndex, tmpSurface, surfaceTupleId, NULL,
            surfaceTypePtr, nComponent) || nComponent != 1)
          {
            vtkErrorMacro(<< "Surface Type is not set in surface dataset or"
              " have incorrect number of components, cannot use surface interaction");
            return NULL;
          }
          int tmpSurfaceType = static_cast<int>(*surfaceTypePtr);
          if (tmpSurfaceType == vtkLagrangianBasicIntegrationModel::SURFACE_TYPE_PASS)
          {
            // Pass Through Surface, store for later
            passThroughInterSet.insert(std::make_pair((*this->Surfaces)[iDs].first, tmpFactor));
          }
          else
          {
            if (tmpSurface == particle->GetLastSurfaceDataSet() &&
              tmpCellId == particle->GetLastSurfaceCellId())
            {
              perforation = this->CheckSurfacePerforation(particle, tmpSurface, tmpCellId);
              if (perforation)
              {
                break;
              }
              continue;
            }

            // Interacting surface
            interFactor = tmpFactor;
            surface = tmpSurface;
            surfaceFlatIndex = (*this->Surfaces)[iDs].first;
            surfaceType = tmpSurfaceType;
            cellId = tmpCellId;
          }
        }
      }
    }
  } while (perforation);

  PassThroughSetType::iterator it;
  for (it = passThroughInterSet.begin(); it != passThroughInterSet.end(); ++it)
  {
    PassThroughItem item = *it;
    // As one cas see in the test above, if a pass through surface intersects at the exact
    // same location than the point computed using the intersection factor,
    // we do not store the intersection.
    // pass through are considered non prioritary, and do not intersects
    // when at the exact the same place as the main intersection
    if (item.second < interFactor)
    {
      vtkLagrangianParticle* clone = particle->CloneParticle();
      clone->SetInteraction(vtkLagrangianParticle::SURFACE_INTERACTION_PASS);
      this->InterpolateNextParticleVariables(clone, item.second);
      passThroughParticles.push(std::make_pair(item.first, clone));
    }
  }

  // Store surface cache (even NULL one)
  particle->SetLastSurfaceCell(surface, cellId);

  bool recordInteraction = false;
  vtkLagrangianParticle* interactionParticle = NULL;
  if (cellId != -1)
  {
    // There is an actual interaction
    // Position next point onto surface
    this->InterpolateNextParticleVariables(particle, interFactor, true);
    interactionParticle = particle->CloneParticle();
    switch (surfaceType)
    {
      case vtkLagrangianBasicIntegrationModel::SURFACE_TYPE_TERM:
        recordInteraction = this->TerminateParticle(particle);
        break;
      case vtkLagrangianBasicIntegrationModel::SURFACE_TYPE_BOUNCE:
        recordInteraction = this->BounceParticle(particle, surface, cellId);
        break;
      case vtkLagrangianBasicIntegrationModel::SURFACE_TYPE_BREAK:
        recordInteraction = this->BreakParticle(particle, surface, cellId, particles);
        break;
      case vtkLagrangianBasicIntegrationModel::SURFACE_TYPE_PASS:
        vtkErrorMacro(<< "Something went wrong with pass-through surface, "
          "next results will be invalid.");
        return NULL;
      default:
        if (surfaceType != SURFACE_TYPE_MODEL && surfaceType < USER_SURFACE_TYPE)
        {
          vtkWarningMacro("Please do not use user defined surface type under " << USER_SURFACE_TYPE
            << " as they may be used in the future by the Lagrangian Particle Tracker");
        }
        recordInteraction = this->InteractWithSurface(surfaceType, particle,
          surface, cellId, particles);
        break;
    }
  }
  if (!recordInteraction)
  {
    delete interactionParticle;
    interactionParticle = NULL;
  }
  return interactionParticle;
}

//----------------------------------------------------------------------------
bool vtkLagrangianBasicIntegrationModel::TerminateParticle(vtkLagrangianParticle* particle)
{
  particle->SetTermination(vtkLagrangianParticle::PARTICLE_TERMINATION_SURF_TERMINATED);
  particle->SetInteraction(vtkLagrangianParticle::SURFACE_INTERACTION_TERMINATED);
  return true;
}

//----------------------------------------------------------------------------
bool vtkLagrangianBasicIntegrationModel::BounceParticle(vtkLagrangianParticle* particle,
  vtkDataSet* surface, vtkIdType cellId)
{
  particle->SetInteraction(vtkLagrangianParticle::SURFACE_INTERACTION_BOUNCE);

  // Recover surface normal
  // Surface should have been computed already
  assert(surface->GetCellData()->GetNormals() != NULL);
  double normal[3];
  surface->GetCellData()->GetNormals()->GetTuple(cellId, normal);

  // Change velocity for bouncing and set interaction point
  double* nextVel = particle->GetNextVelocity();
  double dot = vtkMath::Dot(normal, nextVel);
  for (int i = 0; i < 3; i++)
  {
    nextVel[i] = nextVel[i] - 2 * dot * normal[i];
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkLagrangianBasicIntegrationModel::BreakParticle(vtkLagrangianParticle* particle,
  vtkDataSet* surface, vtkIdType cellId, std::queue<vtkLagrangianParticle*>& particles)
{
  // Terminate particle
  particle->SetTermination(vtkLagrangianParticle::PARTICLE_TERMINATION_SURF_BREAK);
  particle->SetInteraction(vtkLagrangianParticle::SURFACE_INTERACTION_BREAK);

  // Recover surface normal
  // Surface should have been computed already
  assert(surface->GetCellData()->GetNormals() != NULL);
  double normal[3];
  surface->GetCellData()->GetNormals()->GetTuple(cellId, normal);

  // Create new particles
  vtkLagrangianParticle* particle1 =
    particle->NewParticle(this->Tracker->GetNewParticleId());
  vtkLagrangianParticle* particle2 =
    particle->NewParticle(this->Tracker->GetNewParticleId());

  // Compute bounce for each new particle
  double* nextVel = particle->GetNextVelocity();
  double* part1Vel = particle1->GetVelocity();
  double* part2Vel = particle2->GetVelocity();
  double dot = vtkMath::Dot(normal, nextVel);
  double cross [3];
  vtkMath::Cross(normal, nextVel, cross);
  double bounceNorm = vtkMath::Norm(nextVel);

  for (int i = 0; i < 3; i++)
  {
    part1Vel[i] = nextVel[i] - 2 * dot * normal[i] + cross[i];
    part2Vel[i] = nextVel[i] - 2 * dot * normal[i] - cross[i];
  }
  double part1Norm = vtkMath::Norm(part1Vel);
  double part2Norm = vtkMath::Norm(part2Vel);
  for (int i = 0; i < 3; i++)
  {
    part1Vel[i] = part1Vel[i] / part1Norm * bounceNorm;
    part2Vel[i] = part2Vel[i] / part2Norm * bounceNorm;
  }

  // push new particle in queue
  particles.push(particle1);
  particles.push(particle2);
  return true;
}

//----------------------------------------------------------------------------
bool vtkLagrangianBasicIntegrationModel::InteractWithSurface(
  int vtkNotUsed(surfaceType), vtkLagrangianParticle* particle,
  vtkDataSet* vtkNotUsed(surface), vtkIdType vtkNotUsed(cellId),
  std::queue<vtkLagrangianParticle*>& vtkNotUsed(particles))
{
  return this->TerminateParticle(particle);
}

//----------------------------------------------------------------------------
bool vtkLagrangianBasicIntegrationModel::IntersectWithLine(vtkCell* cell,
  double p1[3], double p2[3], double tol, double& t, double x[3])
{
  // Non planar quad support
  if (this->NonPlanarQuadSupport)
  {
    vtkQuad* quad = vtkQuad::SafeDownCast(cell);
    if (quad != NULL)
    {
      if (p1[0] == p2[0] && p1[1] == p2[1] && p1[2] == p2[2])
      {
        // the 2 points are the same, no intersection
        return false;
      }

      vtkPoints* points = quad->GetPoints();

      // create 4 points
      double p[3];
      points->GetPoint(0, p);
      vtkVector3d P00(p[0], p[1], p[2]);
      points->GetPoint(3, p);
      vtkVector3d P01(p[0], p[1], p[2]);
      points->GetPoint(1, p);
      vtkVector3d P10(p[0], p[1], p[2]);
      points->GetPoint(2, p);
      vtkVector3d P11(p[0], p[1], p[2]);

      // Create the bilinear intersection helper class
      vtkLagrangianBilinearQuadIntersection bp(P00, P01, P10, P11);

      // Create the ray
      vtkVector3d r(p1[0], p1[1], p1[2]); //origin of the ray
      vtkVector3d q(p2[0] -p1[0], p2[1] -p1[1], p2[2] -p1[2]); // a ray direction
      // the original t before q is normalised
      double tOrig = q.Norm();
      q.Normalize();

      vtkVector3d uv; // variables returned
      if (bp.RayIntersection(r, q, uv)) // run intersection test
      {
        // we have an intersection
        t = uv.GetZ() / tOrig;
        if (t >= 0.0 && t <= 1.0)
        {
          // Recover intersection between p1 and p2
          vtkVector3d intersec = bp.ComputeCartesianCoordinates(uv.GetX(), uv.GetY());
          x[0] = intersec.GetX();
          x[1] = intersec.GetY();
          x[2] = intersec.GetZ();
          return true;
        }
        else
        {
          // intersection outside of p1p2
          return false;
        }
      }
      else
      {
        // no intersection
        return false;
      }
    }
  }

  // Standard cell intersection
  double pcoords[3];
  int subId;
  int ret = cell->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId);
  if( ret != 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

//----------------------------------------------------------------------------
void vtkLagrangianBasicIntegrationModel::InterpolateNextParticleVariables(
  vtkLagrangianParticle* particle, double interpolationFactor, bool forceInside)
{
  if (forceInside)
  {
    // Reducing interpolationFactor to ensure we stay in domain
    double magnitude = particle->GetPositionVectorMagnitude();
    interpolationFactor *=
      (magnitude - this->Tolerance / interpolationFactor) / magnitude;
  }

  double * current = particle->GetEquationVariables();
  double * next = particle->GetNextEquationVariables();
  for (int i = 0; i < particle->GetNumberOfVariables(); i++)
  {
    next[i] = current[i] + (next[i] - current[i]) * interpolationFactor;
  }
  double& stepTime = particle->GetStepTimeRef();
  stepTime *= interpolationFactor;
}

//----------------------------------------------------------------------------
bool vtkLagrangianBasicIntegrationModel::CheckSurfacePerforation(
  vtkLagrangianParticle* particle, vtkDataSet* surface, vtkIdType cellId)
{
  // Recover surface normal
  // Surface should have been computed already
  assert(surface->GetCellData()->GetNormals() != NULL);
  double normal[3];
  surface->GetCellData()->GetNormals()->GetTuple(cellId, normal);

  // Recover particle vector
  double prevToCurr[3];
  double currToNext[3];
  for (int i = 0; i < 3; i++)
  {
    prevToCurr[i] = particle->GetPosition()[i] - particle->GetPrevPosition()[i];
    currToNext[i] = particle->GetNextPosition()[i] - particle->GetPosition()[i];
  }

  // Check directions
  double dot = vtkMath::Dot(normal, currToNext);
  double prevDot = vtkMath::Dot(normal, prevToCurr);
  double* nextVel = particle->GetNextVelocity();
  double velDot = vtkMath::Dot(normal, nextVel);
  if (dot == 0 || prevDot == 0 || prevDot * dot > 0)
  {
    // vector does not project on the same directions, perforation !
    for (int i = 0; i < 3; i++)
    {
      // Simple perforation management via symmetry
      currToNext[i] = currToNext[i] - 2 * dot * normal[i];
      particle->GetNextPosition()[i] = particle->GetPosition()[i] + currToNext[i];
      nextVel[i] = nextVel[i] - 2 * velDot * normal[i];
    }
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkLagrangianBasicIntegrationModel::SetInputArrayToProcess(int idx,
  int port, int connection, int fieldAssociation, const char *name)
{
  // Store the array metadata
  vtkLagrangianBasicIntegrationModel::ArrayVal vals;
  vals.val[0] = port;
  vals.val[1] = connection;
  vals.val[2] = fieldAssociation;
  ArrayMapVal array = ArrayMapVal(vals, name);
  this->InputArrays[idx] = array;
  this->Modified();
}

//----------------------------------------------------------------------------
bool vtkLagrangianBasicIntegrationModel::FindInLocators(double* x)
{
  vtkIdType cellId;
  vtkDataSet* dataset;
  return this->FindInLocators(x, dataset, cellId);
}

//----------------------------------------------------------------------------
bool vtkLagrangianBasicIntegrationModel::FindInLocators(double* x,
  vtkDataSet*& dataset, vtkIdType& cellId)
{
  vtkAbstractCellLocator* loc;
  double* weights = new double[this->WeightsSize];
  bool ret = this->FindInLocators(x, dataset, cellId, loc, weights);
  delete[] weights;
  return ret;
}

//----------------------------------------------------------------------------
bool vtkLagrangianBasicIntegrationModel::FindInLocators(double* x,
  vtkDataSet*& dataset, vtkIdType& cellId, vtkAbstractCellLocator*& loc,
  double*& weights)
{
  // Sanity check
  if (this->DataSets->empty())
  {
    return false;
  }

  vtkNew<vtkGenericCell> cell;

  // We have a cache
  if (this->LastDataSet != NULL)
  {
    cellId = this->FindInLocator(this->LastDataSet, this->LastLocator, x,
      cell.Get(), weights);
    if (cellId != -1)
    {
      dataset = this->LastDataSet;
      loc = this->LastLocator;
      return true;
    }
  }

  // No cache or Cache miss, try other datasets
  for (size_t iDs = 0; iDs < this->DataSets->size(); iDs++)
  {
    loc = (*this->Locators)[iDs].GetPointer();
    dataset = (*this->DataSets)[iDs];
    if (dataset != this->LastDataSet)
    {
      cellId = this->FindInLocator(dataset, loc, x, cell.Get(), weights);
      if (cellId != -1)
      {
        return true;
      }
    }
  }
  return false;
}

//----------------------------------------------------------------------------
vtkIdType vtkLagrangianBasicIntegrationModel::FindInLocator(vtkDataSet* ds,
  vtkAbstractCellLocator* loc, double * x, vtkGenericCell* cell, double* weights)
{
  double pcoords[3];
  vtkIdType cellId;
  if (loc)
  {
    // Use locator to find the cell containing x
    cellId = loc->FindCell(x, this->Tolerance, cell, pcoords, weights);
  }
  else
  {
    // No locator, ds is vtkImageData or vtkRectilinearGrid,
    // which does not require any cellToUse when calling FindCell.
    int subId;
    cellId = ds->FindCell(x, NULL, 0, this->Tolerance, subId, pcoords, weights);
  }

  // Ignore Ghost cells
  if (cellId != -1 && ds->GetCellGhostArray() &&
    ds->GetCellGhostArray()->GetValue(cellId) & vtkDataSetAttributes::DUPLICATECELL)
  {
    return -1;
  }
  return cellId;
}

//----------------------------------------------------------------------------
vtkAbstractArray* vtkLagrangianBasicIntegrationModel::GetSeedArray(int idx,
  vtkLagrangianParticle* particle)
{
  return this->GetSeedArray(idx, particle->GetSeedData());
}

//----------------------------------------------------------------------------
vtkAbstractArray* vtkLagrangianBasicIntegrationModel::GetSeedArray(int idx,
  vtkPointData* pointData)
{
  // Check the provided index
  if (this->InputArrays.count(idx) == 0)
  {
    vtkErrorMacro(<< "No arrays at index:" << idx);
    return NULL;
  }

  ArrayMapVal arrayIndexes = this->InputArrays[idx];

  // Check port, should be 1 for Source
  if (arrayIndexes.first.val[0] != 1)
  {
    vtkErrorMacro(<< "This input array at idx " << idx << " named " <<
      arrayIndexes.second << " is not a particle data array");
    return NULL;
  }

  // Check connection, should be 0, no multiple connection
  if (arrayIndexes.first.val[1] != 0)
  {
    vtkErrorMacro(<< "This filter does not support multiple connections by port");
    return NULL;
  }

  // Check field association
  switch (arrayIndexes.first.val[2])
  {
    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
    {
      // Recover array
      vtkAbstractArray* array = pointData->GetAbstractArray(
        arrayIndexes.second.c_str());
      if (!array)
      {
        vtkErrorMacro(<< "This input array at idx " << idx << " named " <<
          arrayIndexes.second << " cannot be found, please check arrays.");
      }
      return array;
    }
    default:
      vtkErrorMacro(<< "Only FIELD_ASSOCIATION_POINTS are supported in particle data input");
      return NULL;
  }
  return NULL; // never reached
}

//----------------------------------------------------------------------------
bool vtkLagrangianBasicIntegrationModel::GetFlowOrSurfaceData(int idx, vtkDataSet* dataSet,
  vtkIdType tupleId, double* weights, double*& data, int& nComponents)
{
  // Check index
  if (this->InputArrays.count(idx) == 0)
  {
    vtkErrorMacro(<< "No arrays at index:" << idx);
    return false;
  }

  ArrayMapVal arrayIndexes = this->InputArrays[idx];

  // Check port, should be 0 for Input or 2 for Surface
  if (arrayIndexes.first.val[0] != 0 && arrayIndexes.first.val[0] != 2)
  {
    vtkErrorMacro(<< "This input array at idx " << idx << " named " <<
      arrayIndexes.second << " is not a flow or surface data array");
    return false;
  }

  // Check connection, should be 0, no multiple connection supported
  if (arrayIndexes.first.val[1] != 0)
  {
    vtkErrorMacro(<< "This filter does not support multiple connections by port");
    return false;
  }

  // Check dataset is present
  if (dataSet == NULL)
  {
    vtkErrorMacro(<< "Please provide a dataSet when calling this method "
      "for input arrays coming from the flow or surface");
    return false;
  }

  // Check fieldAssociation
  switch (arrayIndexes.first.val[2])
  {
    // Point need interpolation
    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
    {
      if (!weights)
      {
        vtkErrorMacro(<< "This input array at idx " << idx << " named " <<
          arrayIndexes.second << " is a PointData, yet no weights have been provided");
        return false;
      }
      vtkDataArray* array =
        dataSet->GetPointData()->GetArray(arrayIndexes.second.c_str());
      if (!array)
      {
        vtkErrorMacro(<< "This input array at idx " << idx << " named " <<
          arrayIndexes.second << " cannot be found, please check arrays.");
        return false;
      }
      if (tupleId >= dataSet->GetNumberOfCells())
      {
        vtkErrorMacro(<< "This input array at idx " << idx << " named " <<
          arrayIndexes.second << " does not contain cellId :" << tupleId
          << " . Please check arrays.");
        return false;
      }
      // Setup the tmpArray and Interpolate
      nComponents = array->GetNumberOfComponents();
      if (this->TmpArray != NULL)
      {
        this->TmpArray->Delete();
      }
      this->TmpArray = array->NewInstance();
      this->TmpArray->SetNumberOfComponents(nComponents);
      this->TmpArray->SetNumberOfTuples(1);
      this->TmpArray->InterpolateTuple(
        0, dataSet->GetCell(tupleId)->GetPointIds(), array, weights);

      // Recover data
      data = this->TmpArray->GetTuple(0);
      return true;
    }
    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
    {
      if (tupleId >= dataSet->GetNumberOfCells())
      {
        vtkErrorMacro(<< "This input array at idx " << idx << " named " <<
          arrayIndexes.second << " does not contain cellId :" << tupleId
          << " . Please check arrays.");
        return false;
      }
      vtkDataArray* array = dataSet->GetCellData()->GetArray(arrayIndexes.second.c_str());
      if (!array)
      {
        vtkErrorMacro(<< "This input array at idx " << idx << " named " <<
          arrayIndexes.second << " cannot be found, please check arrays.");
        return false;
      }
      nComponents = array->GetNumberOfComponents();
      data = array->GetTuple(tupleId);
      return true;
    }
    case vtkDataObject::FIELD_ASSOCIATION_NONE:
    {
      vtkDataArray* array = dataSet->GetFieldData()->GetArray(arrayIndexes.second.c_str());
      if (!array || tupleId >= array->GetNumberOfTuples())
      {
        vtkErrorMacro(<< "This input array at idx " << idx << " named " <<
          arrayIndexes.second << " cannot be found in FieldData or does not contain"
          "tuple index: " << tupleId << " , please check arrays.");
        return false;
      }
      nComponents = array->GetNumberOfComponents();
      data = array->GetTuple(tupleId);
      return true;
    }
    default:
      vtkErrorMacro(<< "Only FIELD_ASSOCIATION_POINTS and FIELD_ASSOCIATION_CELLS "
        << "are supported in this method");
  }
  return false;
}

//----------------------------------------------------------------------------
int vtkLagrangianBasicIntegrationModel::GetFlowOrSurfaceDataFieldAssociation(int idx)
{
  // Check index
  if (this->InputArrays.count(idx) == 0)
  {
    vtkErrorMacro(<< "No arrays at index:" << idx);
    return -1;
  }

  ArrayMapVal arrayIndexes = this->InputArrays[idx];

  // Check port, should be 0 for Input
  if (arrayIndexes.first.val[0] != 0 && arrayIndexes.first.val[0] != 2)
  {
    vtkErrorMacro(<< "This input array at idx " << idx << " named " <<
      arrayIndexes.second << " is not a flow or surface data array");
    return -1;
  }

  // Check connection, should be 0, no multiple connection supported
  if (arrayIndexes.first.val[1] != 0)
  {
    vtkErrorMacro(<< "This filter does not support multiple connections by port");
    return -1;
  }

  return arrayIndexes.first.val[2];
}

//---------------------------------------------------------------------------
vtkStringArray* vtkLagrangianBasicIntegrationModel::GetSeedArrayNames()
{
  return this->SeedArrayNames.Get();
}

//---------------------------------------------------------------------------
vtkIntArray* vtkLagrangianBasicIntegrationModel::GetSeedArrayComps()
{
  return this->SeedArrayComps.Get();
}

//---------------------------------------------------------------------------
vtkIntArray* vtkLagrangianBasicIntegrationModel::GetSeedArrayTypes()
{
  return this->SeedArrayTypes.Get();
}

//---------------------------------------------------------------------------
vtkStringArray* vtkLagrangianBasicIntegrationModel::GetSurfaceArrayNames()
{
  this->SurfaceArrayNames->SetNumberOfValues(0);
  std::map<std::string, SurfaceArrayDescription>::const_iterator it;
  for (it = this->SurfaceArrayDescriptions.begin(); it !=
    this->SurfaceArrayDescriptions.end(); ++it)
  {
    this->SurfaceArrayNames->InsertNextValue(it->first.c_str());
  }
  return this->SurfaceArrayNames.Get();
}

//---------------------------------------------------------------------------
vtkIntArray* vtkLagrangianBasicIntegrationModel::GetSurfaceArrayComps()
{
  this->SurfaceArrayComps->SetNumberOfValues(0);
  std::map<std::string, SurfaceArrayDescription>::const_iterator it;
  for (it = this->SurfaceArrayDescriptions.begin(); it !=
    this->SurfaceArrayDescriptions.end(); ++it)
  {
    this->SurfaceArrayComps->InsertNextValue(it->second.nComp);
  }
  return this->SurfaceArrayComps.Get();
}

vtkStringArray* vtkLagrangianBasicIntegrationModel::GetSurfaceArrayEnumValues()
{
  this->SurfaceArrayEnumValues->SetNumberOfValues(0);
  std::map<std::string, SurfaceArrayDescription>::const_iterator it;
  for (it = this->SurfaceArrayDescriptions.begin(); it !=
    this->SurfaceArrayDescriptions.end(); ++it)
  {
    this->SurfaceArrayEnumValues->InsertVariantValue(
      this->SurfaceArrayEnumValues->GetNumberOfValues(), it->second.enumValues.size());
    for (size_t i = 0; i < it->second.enumValues.size(); i++)
    {
      this->SurfaceArrayEnumValues->InsertVariantValue(
        this->SurfaceArrayEnumValues->GetNumberOfValues(), it->second.enumValues[i].first);
      this->SurfaceArrayEnumValues->InsertNextValue(it->second.enumValues[i].second.c_str());
    }
  }
  return this->SurfaceArrayEnumValues.Get();
}

vtkDoubleArray* vtkLagrangianBasicIntegrationModel::GetSurfaceArrayDefaultValues()
{
  this->SurfaceArrayDefaultValues->SetNumberOfValues(0);
  std::map<std::string, SurfaceArrayDescription>::const_iterator it;
  for (it = this->SurfaceArrayDescriptions.begin(); it !=
    this->SurfaceArrayDescriptions.end(); ++it)
  {
    double* defaultValues = new double [it->second.nComp];
    for (size_t iDs = 0; iDs < this->Surfaces->size(); iDs++)
    {
      this->ComputeSurfaceDefaultValues(it->first.c_str(), (*this->Surfaces)[iDs].second,
        it->second.nComp, defaultValues);
      this->SurfaceArrayDefaultValues->InsertNextTuple(defaultValues);
    }
    delete[] defaultValues;
  }
  return this->SurfaceArrayDefaultValues.Get();
}

//---------------------------------------------------------------------------
vtkIntArray* vtkLagrangianBasicIntegrationModel::GetSurfaceArrayTypes()
{
  this->SurfaceArrayTypes->SetNumberOfValues(0);
  std::map<std::string, SurfaceArrayDescription>::const_iterator it;
  for (it = this->SurfaceArrayDescriptions.begin(); it !=
    this->SurfaceArrayDescriptions.end(); ++it)
  {
    this->SurfaceArrayTypes->InsertNextValue(it->second.type);
  }
  return this->SurfaceArrayTypes.Get();
}

//---------------------------------------------------------------------------
bool vtkLagrangianBasicIntegrationModel::ManualIntegration(
  double* vtkNotUsed(xcur), double* vtkNotUsed(xnext),
  double vtkNotUsed(t), double& vtkNotUsed(delT), double& vtkNotUsed(delTActual),
  double vtkNotUsed(minStep), double vtkNotUsed(maxStep),
  double vtkNotUsed(maxError), double& vtkNotUsed(error),
  int& vtkNotUsed(integrationResult))
{
  return false;
}

//---------------------------------------------------------------------------
void vtkLagrangianBasicIntegrationModel::ComputeSurfaceDefaultValues(
  const char* arrayName, vtkDataSet* vtkNotUsed(dataset),
  int nComponents, double* defaultValues)
{
  double defVal = (strcmp(arrayName, "SurfaceType") == 0) ? static_cast<double>(SURFACE_TYPE_TERM) : 0.0;
  std::fill(defaultValues, defaultValues + nComponents, defVal);
}
