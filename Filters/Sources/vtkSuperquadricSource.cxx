/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSuperquadricSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/* vtkSuperquadric originally written by Michael Halle,
   Brigham and Women's Hospital, July 1998.

   Based on "Rigid physically based superquadrics", A. H. Barr,
   in "Graphics Gems III", David Kirk, ed., Academic Press, 1992.
*/
#include "vtkSuperquadricSource.h"

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

#include <math.h>

vtkStandardNewMacro(vtkSuperquadricSource);

static void evalSuperquadric(double u, double v,
                             double du, double dv,
                             double e, double n,
                             double dims[3],
                             double alpha,
                             double xyz[3],
                             double nrm[3]);

// Description:
vtkSuperquadricSource::vtkSuperquadricSource(int res)
{
  res = res < 4 ? 4 : res;

  this->AxisOfSymmetry = 1; // y-axis symmetry
  this->Toroidal = 0;
  this->Thickness = 0.3333;
  this->PhiRoundness = 0.0;
  this->SetPhiRoundness(1.0);
  this->ThetaRoundness = 0.0;
  this->SetThetaRoundness(1.0);
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
  this->Scale[0] = this->Scale[1] = this->Scale[2] = 1.0;
  this->Size = .5;
  this->ThetaResolution = 0;
  this->SetThetaResolution(res);
  this->PhiResolution = 0;
  this->SetPhiResolution(res);
  this->OutputPointsPrecision = SINGLE_PRECISION;

  this->SetNumberOfInputPorts(0);
}

void vtkSuperquadricSource::SetPhiResolution(int i)
{
  if(i < 4)
    {
    i = 4;
    }
  i = (i+3)/4*4;  // make it divisible by 4
  if(i > VTK_MAX_SUPERQUADRIC_RESOLUTION)
    {
    i =  VTK_MAX_SUPERQUADRIC_RESOLUTION;
    }

  if (this->PhiResolution != i)
    {
    this->PhiResolution = i;
    this->Modified ();
    }
}

void vtkSuperquadricSource::SetThetaResolution(int i)
{
  if(i < 8)
    {
    i = 8;
    }
  i = (i+7)/8*8; // make it divisible by 8
  if(i > VTK_MAX_SUPERQUADRIC_RESOLUTION)
    {
    i =  VTK_MAX_SUPERQUADRIC_RESOLUTION;
    }

  if (this->ThetaResolution != i)
    {
    this->ThetaResolution = i;
    this->Modified ();
    }
}

void vtkSuperquadricSource::SetThetaRoundness(double e)
{
  if(e < VTK_MIN_SUPERQUADRIC_ROUNDNESS)
    {
    e = VTK_MIN_SUPERQUADRIC_ROUNDNESS;
    }

  if (this->ThetaRoundness != e)
    {
    this->ThetaRoundness = e;
    this->Modified();
    }
}

void vtkSuperquadricSource::SetPhiRoundness(double e)
{
  if(e < VTK_MIN_SUPERQUADRIC_ROUNDNESS)
    {
    e = VTK_MIN_SUPERQUADRIC_ROUNDNESS;
    }

  if (this->PhiRoundness != e)
    {
    this->PhiRoundness = e;
    this->Modified();
    }
}

static const double SQ_SMALL_OFFSET = 0.01;

int vtkSuperquadricSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int i, j;
  vtkIdType numPts;
  vtkPoints *newPoints;
  vtkFloatArray *newNormals;
  vtkFloatArray *newTCoords;
  vtkCellArray *newPolys;
  vtkIdType *ptidx;
  double pt[3], nv[3], dims[3];
  double len;
  double alpha;
  double deltaPhi, deltaTheta, phi, theta;
  double phiLim[2], thetaLim[2];
  double deltaPhiTex, deltaThetaTex;
  int base, pbase;
  vtkIdType numStrips;
  int ptsPerStrip;
  int phiSubsegs, thetaSubsegs, phiSegs, thetaSegs;
  int iq, jq, rowOffset;
  double thetaOffset, phiOffset;
  double texCoord[2];
  double tmp;


  dims[0] = this->Scale[0] * this->Size;
  dims[1] = this->Scale[1] * this->Size;
  dims[2] = this->Scale[2] * this->Size;

  if(this->Toroidal)
    {
    phiLim[0] = -vtkMath::Pi();
    phiLim[1] =  vtkMath::Pi();

    thetaLim[0] = -vtkMath::Pi();
    thetaLim[1] =  vtkMath::Pi();

    alpha = (1.0 / this->Thickness);
    dims[0] /= (alpha + 1.0);
    dims[1] /= (alpha + 1.0);
    dims[2] /= (alpha + 1.0);
    }
  else
    {
    //Ellipsoidal
    phiLim[0] = -vtkMath::Pi() / 2.0;
    phiLim[1] =  vtkMath::Pi() / 2.0;

    thetaLim[0] = -vtkMath::Pi();
    thetaLim[1] =  vtkMath::Pi();

    alpha = 0.0;
    }

  deltaPhi = (phiLim[1] - phiLim[0]) / this->PhiResolution;
  deltaPhiTex = 1.0 / this->PhiResolution;
  deltaTheta = (thetaLim[1] - thetaLim[0]) / this->ThetaResolution;
  deltaThetaTex = 1.0 / this->ThetaResolution;

  phiSegs = 4;
  thetaSegs = 8;

  phiSubsegs = this->PhiResolution / phiSegs;
  thetaSubsegs = this->ThetaResolution / thetaSegs;

  numPts = (this->PhiResolution + phiSegs)*(this->ThetaResolution + thetaSegs);
  // creating triangles
  numStrips = this->PhiResolution * thetaSegs;
  ptsPerStrip = thetaSubsegs*2 + 2;

  //
  // Set things up; allocate memory
  //
  newPoints = vtkPoints::New();

  // Set the desired precision for the points in the output.
  if(this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
    {
    newPoints->SetDataType(VTK_DOUBLE);
    }
  else
    {
    newPoints->SetDataType(VTK_FLOAT);
    }

  newPoints->Allocate(numPts);
  newNormals = vtkFloatArray::New();
  newNormals->SetNumberOfComponents(3);
  newNormals->Allocate(3*numPts);
  newNormals->SetName("Normals");
  newTCoords = vtkFloatArray::New();
  newTCoords->SetNumberOfComponents(2);
  newTCoords->Allocate(2*numPts);
  newTCoords->SetName("TextureCoords");

  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(numStrips,ptsPerStrip));

  // generate!
  for(iq = 0; iq < phiSegs; iq++)
    {
    for(i = 0; i <= phiSubsegs; i++)
      {
      phi = phiLim[0] + deltaPhi*(i + iq*phiSubsegs);
      texCoord[1] = deltaPhiTex*(i + iq*phiSubsegs);

      // SQ_SMALL_OFFSET makes sure that the normal vector isn't
      // evaluated exactly on a crease;  if that were to happen,
      // large shading errors can occur.
      if(i == 0)
        {
        phiOffset =  SQ_SMALL_OFFSET*deltaPhi;
        }
      else if (i == phiSubsegs)
        {
        phiOffset = -SQ_SMALL_OFFSET*deltaPhi;
        }
      else
        {
        phiOffset =  0.0;
        }

      for(jq = 0; jq < thetaSegs; jq++)
        {
        for(j = 0; j <= thetaSubsegs; j++)
          {
          theta = thetaLim[0] + deltaTheta*(j + jq*thetaSubsegs);
          texCoord[0] = deltaThetaTex*(j + jq*thetaSubsegs);

          if(j == 0)
            {
            thetaOffset =  SQ_SMALL_OFFSET*deltaTheta;
            }
          else if (j == thetaSubsegs)
            {
            thetaOffset = -SQ_SMALL_OFFSET*deltaTheta;
            }
          else
            {
            thetaOffset =  0.0;
            }

          // This gives a superquadric with axis of symmetry: z
          evalSuperquadric(theta, phi,
                           thetaOffset, phiOffset,
                           this->ThetaRoundness, this->PhiRoundness,
                           dims, alpha, pt, nv);
          switch (this->AxisOfSymmetry)
            {
            case 0:
              // x-axis
              tmp   = pt[0];
              pt[0] = pt[2];
              pt[2] = tmp;
              pt[1] = -pt[1];

              tmp   = nv[0];
              nv[0] = nv[2];
              nv[2] = tmp;
              nv[1] = -nv[1];
              break;
            case 1:
              // y-axis
              tmp   = pt[1];
              pt[1] = pt[2];
              pt[2] = tmp;
              pt[0] = -pt[0];

              tmp   = nv[1];
              nv[1] = nv[2];
              nv[2] = tmp;
              nv[0] = -nv[0];
              break;
            case 2:
            default:
              // Default case is managed above
              break;
          }

          if((len = vtkMath::Norm(nv)) == 0.0)
            {
            len = 1.0;
            }
          nv[0] /= len; nv[1] /= len; nv[2] /= len;

          if(!this->Toroidal &&
             ((iq == 0 && i == 0) || (iq == (phiSegs-1) && i == phiSubsegs)))
            {

            // we're at a pole:
            // make sure the pole is at the same location for all evals
            // (the superquadric evaluation is numerically unstable
            // at the poles)
            switch (this->AxisOfSymmetry)
              {
              case 0:
                // x-axis
                pt[1] = pt[2] = 0.0;
                break;

              case 1:
                // y-axis
                pt[0] = pt[2] = 0.0;
                break;

              case 2:
              default:
                // z-axis
                pt[0] = pt[1] = 0.0;
                break;
              }
            }

          pt[0] += this->Center[0];
          pt[1] += this->Center[1];
          pt[2] += this->Center[2];

          newPoints->InsertNextPoint(pt);
          newNormals->InsertNextTuple(nv);
          newTCoords->InsertNextTuple(texCoord);
          }
        }
      }
    }

  // mesh!
  // build triangle strips for efficiency....
  ptidx = new vtkIdType[ptsPerStrip];

  rowOffset = this->ThetaResolution+thetaSegs;

  for(iq = 0; iq < phiSegs; iq++)
    {
    for(i = 0; i < phiSubsegs; i++)
      {
      pbase = rowOffset*(i +iq*(phiSubsegs+1));
      for(jq = 0; jq < thetaSegs; jq++)
        {
        base = pbase + jq*(thetaSubsegs+1);
        for(j = 0; j <= thetaSubsegs; j++)
          {
          ptidx[2*j] = base + rowOffset + j;
          ptidx[2*j+1] = base + j;
          }
        newPolys->InsertNextCell(ptsPerStrip, ptidx);
        }
      }
    }
  delete[] ptidx;

  output->SetPoints(newPoints);
  newPoints->Delete();

  output->GetPointData()->SetNormals(newNormals);
  newNormals->Delete();

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();

  output->SetStrips(newPolys);
  newPolys->Delete();

  return 1;
}

void vtkSuperquadricSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Toroidal: " << (this->Toroidal ? "On\n" : "Off\n");
  os << indent << "Axis Of Symmetry: " << this->AxisOfSymmetry << "\n";
  os << indent << "Size: " << this->Size << "\n";
  os << indent << "Thickness: " << this->Thickness << "\n";
  os << indent << "Theta Resolution: " << this->ThetaResolution << "\n";
  os << indent << "Theta Roundness: " << this->ThetaRoundness << "\n";
  os << indent << "Phi Resolution: " << this->PhiResolution << "\n";
  os << indent << "Phi Roundness: " << this->PhiRoundness << "\n";
  os << indent << "Center: (" << this->Center[0] << ", "
     << this->Center[1] << ", " << this->Center[2] << ")\n";
  os << indent << "Scale: (" << this->Scale[0] << ", "
     << this->Scale[1] << ", " << this->Scale[2] << ")\n";
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision
     << "\n";
}

static double cf(double w, double m, double a = 0)
{
  double c;
  double sgn;

  if (w == vtkMath::Pi() || w == -vtkMath::Pi())
    {
    c = -1.0;
    }
  else
    {
    c = cos(w);
    }
  sgn = c < 0.0 ? -1.0 : 1.0;
  return a + sgn*pow(sgn*c, m);
}

static double sf(double w, double m)
{
  double s;
  double sgn;

  if (w == vtkMath::Pi() || w == -vtkMath::Pi())
    {
    s = 0.0;
    }
  else
    {
    s = sin(w);
    }
  sgn = s < 0.0 ? -1.0 : 1.0;
  return sgn*pow(sgn*s, m);
}

static void evalSuperquadric(double theta, double phi,  // parametric coords
                             double dtheta, double dphi, // offsets for normals
                             double rtheta, double rphi,  // roundness params
                             double dims[3],     // x, y, z dimensions
                             double alpha,       // hole size
                             double xyz[3],      // output coords
                             double nrm[3])      // output normals
{
  // axis of symmetry: z

  double cf1, cf2;

  cf1 = cf(phi, rphi, alpha);
  xyz[0] = -dims[0] * cf1 * sf(theta, rtheta);
  xyz[1] =  dims[1] * cf1 * cf(theta, rtheta);
  xyz[2] =  dims[2]       * sf(phi, rphi);

  cf2 = cf(phi+dphi, 2.0-rphi);
  nrm[0] = -1.0/dims[0] * cf2 * sf(theta+dtheta, 2.0-rtheta);
  nrm[1] =  1.0/dims[1] * cf2 * cf(theta+dtheta, 2.0-rtheta);
  nrm[2] =  1.0/dims[2]       * sf(phi+dphi, 2.0-rphi);
}

