/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSuperquadricSource.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Mike Halle, Brigham and Women's Hospital


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
/* vtkSuperquadric originally written by Michael Halle, 
   Brigham and Women's Hospital, July 1998.

   Based on "Rigid physically based superquadrics", A. H. Barr,
   in "Graphics Gems III", David Kirk, ed., Academic Press, 1992.
*/

#include <math.h>
#include "vtkSuperquadricSource.h"
#include "vtkPoints.h"
#include "vtkNormals.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkSuperquadricSource* vtkSuperquadricSource::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSuperquadricSource");
  if(ret)
    {
    return (vtkSuperquadricSource*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkSuperquadricSource;
}




static void evalSuperquadric(float u, float v, 
			     float du, float dv,
			     float n, float e, 
			     float dims[3],
			     float alpha,  
			     float xyz[3], 
			     float nrm[3]);
  
// Description:
vtkSuperquadricSource::vtkSuperquadricSource(int res)
{
  res = res < 4 ? 4 : res;

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

void vtkSuperquadricSource::SetThetaRoundness(float e) 
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

void vtkSuperquadricSource::SetPhiRoundness(float e) 
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

static const float SQ_SMALL_OFFSET = 0.01;

void vtkSuperquadricSource::Execute()
{
  int i, j;
  int numPts;
  vtkPoints *newPoints; 
  vtkNormals *newNormals;
  vtkTCoords *newTCoords;
  vtkCellArray *newPolys;
  int *ptidx;
  float pt[3], nv[3], dims[3];
  float len;
  float alpha;
  float deltaPhi, deltaTheta, phi, theta;
  float phiLim[2], thetaLim[2];
  float deltaPhiTex, deltaThetaTex;
  int base, pbase;
  int numStrips, ptsPerStrip;
  int phiSubsegs, thetaSubsegs, phiSegs, thetaSegs;
  int iq, jq, rowOffset;
  float thetaOffset, phiOffset;
  float texCoord[2];

  vtkPolyData *output = this->GetOutput();

  dims[0] = this->Scale[0] * this->Size;
  dims[1] = this->Scale[1] * this->Size;
  dims[2] = this->Scale[2] * this->Size;

  if(this->Toroidal) {
    phiLim[0] = -vtkMath::Pi();
    phiLim[1] =  vtkMath::Pi();

    thetaLim[0] = -vtkMath::Pi();
    thetaLim[1] =  vtkMath::Pi();

    alpha = (1.0 / this->Thickness);
    dims[0] /= (alpha + 1.0);
    dims[1] /= (alpha + 1.0);
    dims[2] /= (alpha + 1.0);
    
  }
  else { 
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
  newPoints->Allocate(numPts);
  newNormals = vtkNormals::New();
  newNormals->Allocate(numPts);
  newNormals->GetData()->SetName("Normals");
  newTCoords = vtkTCoords::New();
  newTCoords->Allocate(numPts);
  newTCoords->GetData()->SetName("TextureCoords");

  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(numStrips,ptsPerStrip));

  // generate!
  for(iq = 0; iq < phiSegs; iq++) {
    for(i = 0; i <= phiSubsegs; i++) {
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
      
      for(jq = 0; jq < thetaSegs; jq++) {
	for(j = 0; j <= thetaSubsegs; j++) {
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

	  evalSuperquadric(theta, phi, 
			   thetaOffset, phiOffset, 
			   this->PhiRoundness, this->ThetaRoundness,
			   dims, alpha, pt, nv);
	  
	  if((len = vtkMath::Norm(nv)) == 0.0)
	    {
	    len = 1.0;
	    }
	  nv[0] /= len; nv[1] /= len; nv[2] /= len;

	  if(!this->Toroidal && 
	     ((iq == 0 && i == 0) || (iq == (phiSegs-1) && i == phiSubsegs))) {

	    // we're at a pole:
	    // make sure the pole is at the same location for all evals
	    // (the superquadric evaluation is numerically unstable
	    // at the poles)

	    pt[0] = pt[2] = 0.0;
	  }
	  
	  pt[0] += this->Center[0];
	  pt[1] += this->Center[1];
	  pt[2] += this->Center[2];
	  
	  newPoints->InsertNextPoint(pt);
	  newNormals->InsertNextNormal(nv);
	  newTCoords->InsertNextTCoord(texCoord);
	}
      }
    }
  }

  // mesh!
  // build triangle strips for efficiency....
  ptidx = new int[ptsPerStrip];
  
  rowOffset = this->ThetaResolution+thetaSegs;
  
  for(iq = 0; iq < phiSegs; iq++) {
    for(i = 0; i < phiSubsegs; i++) {
      pbase = rowOffset*(i +iq*(phiSubsegs+1));
      for(jq = 0; jq < thetaSegs; jq++) {
	base = pbase + jq*(thetaSubsegs+1);
	for(j = 0; j <= thetaSubsegs; j++) {
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
}

void vtkSuperquadricSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataSource::PrintSelf(os,indent);

  os << indent << "Toroidal: " << (this->Toroidal ? "On\n" : "Off\n");
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
}

static float cf(float w, float m, float a) 
{
  float c;
  float sgn;

  c = cos(w);
  sgn = c < 0.0 ? -1.0 : 1.0;
  return a + sgn*pow(sgn*c, m);
}

static float sf(float w, float m) 
{
  float s;
  float sgn;

  s = sin(w);
  sgn = s < 0.0 ? -1.0 : 1.0;
  return sgn*pow(sgn*s, m);
}

static void evalSuperquadric(float u, float v,  // parametric coords
			     float du, float dv, // offsets for normals
			     float n, float e,  // roundness params
			     float dims[3],     // x, y, z dimensions
			     float alpha,       // hole size
			     float xyz[3],      // output coords
			     float nrm[3])      // output normals
  
{
  float cf1, cf2;

  cf1 = cf(v, n, alpha);
  xyz[0] = dims[0] * cf1 * sf(u, e);
  xyz[1] = dims[1]       * sf(v, n);
  xyz[2] = dims[2] * cf1 * cf(u, e, 0.0);

  cf2 = cf(v+dv, 2.0-n, 0.0);
  nrm[0] = 1.0/dims[0] * cf2 * sf(u+du, 2.0-e);
  nrm[1] = 1.0/dims[1]       * sf(v+dv, 2.0-n);
  nrm[2] = 1.0/dims[2] * cf2 * cf(u+du, 2.0-e, 0.0);
}

