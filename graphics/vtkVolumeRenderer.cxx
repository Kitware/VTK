/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRenderer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include <stdlib.h>
#include <string.h>

#include <math.h>
#include "vtkVolumeRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkVoxel.h"

// Description:
// Create an instance of a VolumeRenderer with its step size to one.
vtkVolumeRenderer::vtkVolumeRenderer()
{
  this->StepSize = 1.0;
  this->Image = NULL;
}

// Description:
// Main routine to do the volume rendering.
void vtkVolumeRenderer::Render(vtkRenderer *ren)
{
  int *tempip;
  float *tempfp;
  int pos[2];
  int size[2];
  vtkVolume *aVolume;
  float p1World[4], p2World[4];
  int steps;
  float *rays;
  int x,y,i;
  unsigned char *originalImage;
  unsigned char resultColor[4];
  static float  Vecs[6][3];
  float xrat,yrat;
  int yoffset;

  // Send a render to the volumes
  for (this->Volumes.InitTraversal(), i = 0; 
       (aVolume = this->Volumes.GetNextItem()); i++)
    {
    aVolume->Render();
    }

  // get some neccessary info
  tempip = ren->GetRenderWindow()->GetSize();
  tempfp = ren->GetViewport();
  pos[0] = (int)(tempfp[0]*tempip[0]);
  pos[1] = (int)(tempfp[1]*tempip[1]);
  size[0] = (int)((tempfp[2] - tempfp[0])*tempip[0]);
  size[1] = (int)((tempfp[3] - tempfp[1])*tempip[1]);

  // get the current image
  originalImage = 
    ren->GetRenderWindow()->GetPixelData(pos[0], pos[1],
					 pos[0] + size[0]-1, 
					 pos[1] + size[1]-1,0);

  // calculate camera,ren,volume vector values
  this->ComputeRayValues(ren,Vecs,size,&steps);

  // allocate the memory for image and rays
  if (this->Image) delete [] this->Image;
  this->Image = new unsigned char [size[0]*size[1]*3];
  rays = new float [this->Volumes.GetNumberOfItems()*4*steps];

  for (x = 0; x < size[0]; x++)
    {
    xrat = (float)x/(size[0] - 1.0);
    for (y = 0; y < size[1]; y++)
      {
      yrat = (float)y/(size[1] - 1.0);
      // get wc points
      for (i = 0; i < 3; i++)
	{
	p1World[i] = Vecs[0][i] + Vecs[1][i]*xrat + Vecs[2][i]*yrat;
	}
      p1World[3] = 1.0;
      for (i = 0; i < 3; i++)
	{
	p2World[i] = Vecs[3][i] + Vecs[4][i]*xrat + Vecs[5][i]*yrat;
	}
      p2World[3] = 1.0;

      // loop through actors 
      for (this->Volumes.InitTraversal(), i = 0; 
	   (aVolume = this->Volumes.GetNextItem()); i++)
	{
	// if it's invisible, we can skip the rest 
	if (aVolume->GetVisibility() == 1.0)
	  {
          aVolume->GetLookupTable()->SetTableRange(aVolume->GetScalarRange());
          this->TraceOneRay(p1World,p2World, aVolume, 
			    steps, rays + i*steps*4);
	  }
	}
      // composite the rays and write the result
      yoffset = y*size[0];
      this->Composite(rays,steps,i,resultColor);
      this->Image[(yoffset+x)*3] = 
	(unsigned char)(resultColor[0]*resultColor[3]/255.0 + 
	originalImage[(yoffset+x)*3]*(255 - resultColor[3])/255.0);
      this->Image[(yoffset+x)*3 + 1] = (unsigned char)
	(resultColor[1]*resultColor[3]/255.0 + 
	originalImage[(yoffset+x)*3 + 1]*(255 - resultColor[3])/255.0);
      this->Image[(yoffset+x)*3 + 2] = (unsigned char)
	(resultColor[2]*resultColor[3]/255.0 + 
	originalImage[(yoffset+x)*3 + 2]*(255 - resultColor[3])/255.0);
      }
    }

  // write out the resulting image
  vtkDebugMacro(<< "Copying Result " << size[0] << "," << size[1] << "\n");

  ren->GetRenderWindow()->SetPixelData(pos[0], pos[1],
				       pos[0] + size[0]-1, pos[1] + size[1]-1,
				       this->Image,0);
}

// Description:
// Calculates six vectors from the camera, renderer, and volume information.
// These six vectors can be combined to determine the start and end
// world coordinate points for the rays to be cast.
void vtkVolumeRenderer::ComputeRayValues(vtkRenderer *ren, float Vecs[6][3],
				         int *size,int *steps)
{
  int i;
  vtkVolume *aVolume;
  vtkCamera *cam;
  float minz = 1.0e30;
  float maxz = 0;
  float *position;
  float cameraFP[4];
  float *VPN;
  float *bounds;
  float xmax,xmin,ymax,ymin,zmax,zmin;
  float frontZ, backZ;
  float *tempfp;

  // get camera values
  cam = ren->GetActiveCamera();
  position = cam->GetPosition();
  cam->ComputeViewPlaneNormal();
  VPN = cam->GetViewPlaneNormal();

  // loop through actors to calc the front and back clipping planes
  for (this->Volumes.InitTraversal(), i = 0; 
       (aVolume = this->Volumes.GetNextItem()); i++)
    {
    // if it's invisible, we can skip the rest 
    if (aVolume->GetVisibility() == 1.0)
      {
      bounds = aVolume->GetBounds();
      if ((bounds[0] - position[0])*VPN[0] < (bounds[1] - position[0])*VPN[0])
	{
	xmax = -1.0*(bounds[0] - position[0])*VPN[0];
	xmin = -1.0*(bounds[1] - position[0])*VPN[0];
	}
      else
	{
	xmin = -1.0*(bounds[0] - position[0])*VPN[0];
	xmax = -1.0*(bounds[1] - position[0])*VPN[0];
	}
      if ((bounds[2] - position[1])*VPN[1] < (bounds[3] - position[1])*VPN[1])
	{
	ymax = -1.0*(bounds[2] - position[1])*VPN[1];
	ymin = -1.0*(bounds[3] - position[1])*VPN[1];
	}
      else
	{
	ymin = -1.0*(bounds[2] - position[1])*VPN[1];
	ymax = -1.0*(bounds[3] - position[1])*VPN[1];
	}
      if ((bounds[4] - position[2])*VPN[2] < (bounds[5] - position[2])*VPN[2])
	{
	zmax = -1.0*(bounds[4] - position[2])*VPN[2];
	zmin = -1.0*(bounds[5] - position[2])*VPN[2];
	}
      else
	{
	zmin = -1.0*(bounds[4] - position[2])*VPN[2];
	zmax = -1.0*(bounds[5] - position[2])*VPN[2];
	}
      if ((xmax + ymax + zmax) > maxz)
	{
	maxz = xmax + ymax + zmax;
	}
      if ((xmin + ymin + zmin) < minz)
	{
	minz = xmin + ymin + zmin;
	}
      }
    }

  if (minz < 0) 
    {
    minz = 0;
    }
  // back off a little and expand some
  minz = 0.95*minz;
  maxz = 1.05*maxz;

  // also take into account the camera clipping planes
  tempfp = cam->GetClippingRange();
  if (tempfp[0] > minz)
    {
    minz = tempfp[0];
    }
  if (tempfp[1] < maxz)
    {
    maxz = tempfp[1];
    }

  // calc the maximum number of steps a ray will take
  *steps = (int) ((double) (maxz - minz)/
     (cos(3.1415926*cam->GetViewAngle()/180.0)*this->StepSize));

  // calc the z val for front clipping plane
  for (i = 0; i < 3; i++)
    {
    cameraFP[i] = position[i] - minz*VPN[i];
    }
  cameraFP[3] = 1.0;
  ren->SetWorldPoint(cameraFP);
  ren->WorldToDisplay();
  frontZ = (ren->GetDisplayPoint())[2];

  for (i = 0; i < 3; i++)
    {
    cameraFP[i] = position[i] - maxz*VPN[i];
    }
  cameraFP[3] = 1.0;
  ren->SetWorldPoint(cameraFP);
  ren->WorldToDisplay();
  backZ = (ren->GetDisplayPoint())[2];

  // Calc the beginning and end WC points
  ren->SetDisplayPoint(0,0,frontZ);
  ren->DisplayToWorld();
  tempfp = ren->GetWorldPoint();
  for (i=0; i < 3; i++) 
    {
    Vecs[0][i] = (tempfp[i] / tempfp[3]);
    }
  ren->SetDisplayPoint(size[0] - 1,0,frontZ);
  ren->DisplayToWorld();
  tempfp = ren->GetWorldPoint();
  for (i=0; i < 3; i++) 
    {
    Vecs[1][i] = tempfp[i] / tempfp[3] - Vecs[0][i];
    }
  ren->SetDisplayPoint(0,size[1] - 1,frontZ);
  ren->DisplayToWorld();
  tempfp = ren->GetWorldPoint();
  for (i=0; i < 3; i++) 
    {
    Vecs[2][i] = tempfp[i] / tempfp[3] - Vecs[0][i];;
    }

  ren->SetDisplayPoint(0,0,backZ);
  ren->DisplayToWorld();
  tempfp = ren->GetWorldPoint();
  for (i=0; i < 3; i++) 
    {
    Vecs[3][i] = (tempfp[i] / tempfp[3]);
    }
  ren->SetDisplayPoint(size[0] - 1,0,backZ);
  ren->DisplayToWorld();
  tempfp = ren->GetWorldPoint();
  for (i=0; i < 3; i++) 
    {
    Vecs[4][i] = tempfp[i] / tempfp[3] - Vecs[3][i];
    }
  ren->SetDisplayPoint(0,size[1] - 1,backZ);
  ren->DisplayToWorld();
  tempfp = ren->GetWorldPoint();
  for (i=0; i < 3; i++) 
    {
    Vecs[5][i] = tempfp[i] / tempfp[3] - Vecs[3][i];
    }
}

// Description:
// Composite the traced rays into a resulting pixel color.
void vtkVolumeRenderer::Composite(float *rays,int steps, int numRays,
				 unsigned char *resultColor)
{
  int i,j;
  float alpha;
  float *color;
  float alpha2;
  float fresCol[4];
  
  fresCol[0] = 0;
  fresCol[1] = 0;
  fresCol[2] = 0;
  alpha = 0;

  for (i = 0; ((i < steps)&&(alpha < 0.98)); i++)
    {
    for (j = 0; j < numRays; j++)
      {
      if (rays[(j*steps+i)*4+3] > 0)
	{
	color = rays + (j*steps+i)*4;
	alpha2 = color[3];
	fresCol[0] = fresCol[0] + color[0]*(1-alpha)*alpha2;
	fresCol[1] = fresCol[1] + color[1]*(1-alpha)*alpha2;
	fresCol[2] = fresCol[2] + color[2]*(1-alpha)*alpha2;
	alpha = alpha + alpha2*(1 - alpha);
	}
      }
    }
  resultColor[0] = (unsigned char) ((float)fresCol[0]*255.0);
  resultColor[1] = (unsigned char) ((float)fresCol[1]*255.0);
  resultColor[2] = (unsigned char) ((float)fresCol[2]*255.0);
  resultColor[3] = (unsigned char) ((float)alpha*255.0);
}

// Description:
// Traces one ray through one volume.
void vtkVolumeRenderer::TraceOneRay(float p1World[4],float p2World[4], 
				   vtkVolume *vol, 
				   int steps, float *resultRay)
{
  int i,j;
  float p1Mapper[4], p2Mapper[4];
  float ray[3];
  vtkStructuredPoints *strPts;
  float *bounds;
  float hitPosition[3];
  float t,t2;
  float p1Coords[3], p2Coords[3];
  float origin[3], Spacing[3];
  float pcoords[3];
  float sf[8];
  vtkScalars *scalars;
  int dimensions[3];
  int index[3];
  float currentAlpha = 0;
  float mag;
  float calcSteps;
  unsigned char temp_col[4];
  static vtkIdList ptIds(8); ptIds.SetNumberOfIds(8);
  static vtkFloatScalars voxelValues(8);
  int kOffset, ptId, newVoxel, idx;
  float value;

  // clear the memory for the ray
  for (i = 0; i < steps*4; i++)
    {
    resultRay[i] = 0;
    }

  //  Transform ray (defined from position of p1 to p2) into coordinates 
  //  of mapper (not transformed to actors coordinates!  Reduces overall 
  //  computation!!!). Get the actors composite matrix, invert it, and
  //  use the inverted matrix to transform the ray points into mapper
  //  coordinates. 
  this->Transform.SetMatrix(vol->GetMatrix());
  this->Transform.Push();
  this->Transform.Inverse();
  
  this->Transform.SetPoint(p1World);
  this->Transform.GetPoint(p1Mapper);
  
  this->Transform.SetPoint(p2World);
  this->Transform.GetPoint(p2Mapper);

  for (i=0; i<3; i++) 
    {
    p1Mapper[i] /= p1Mapper[3];
    p2Mapper[i] /= p2Mapper[3];
    ray[i] = p2Mapper[i] - p1Mapper[i];
    }
  
  this->Transform.Pop();

  //  Have the ray endpoints in mapper space, now need to compare this
  //  with the mapper bounds to see whether intersection is possible.
  if ((strPts = vol->GetInput()) != NULL)
    {
    //  Get the bounding box of the modeller.
    bounds = strPts->GetBounds();
    if (vtkCell::HitBBox(bounds, (float *)p1Mapper, ray, hitPosition, t) )
      {
      // find the exit point of the ray
      for (i=0; i<3; i++) 
	{
	ray[i] = p1Mapper[i] - p2Mapper[i];
	}
      vtkCell::HitBBox(bounds, (float *)p2Mapper, ray, hitPosition, t2);
      t2 = 1.0 - t2;

      // calc wc length of ray
      mag = 0;
      for (i = 0; i < 3; i++)
	{
	ray[i] = (p2World[i] - p1World[i]);
	mag += ray[i]*ray[i];
	}
      mag = sqrt(mag);
      calcSteps = mag/this->StepSize;

      // convert the ends into local coordinates
      strPts->GetOrigin(origin);
      strPts->GetSpacing(Spacing);
      
      for (i = 0; i < 3; i++)
	{
	p1Coords[i] = (p1Mapper[i] - origin[i])/Spacing[i];
	p2Coords[i] = (p2Mapper[i] - origin[i])/Spacing[i];
	ray[i] = (p2Coords[i] - p1Coords[i])/calcSteps;
	}
      
      // get the scalar data
      if ((scalars = vol->GetInput()->GetPointData()->GetScalars()) == NULL)
	{
	vtkErrorMacro(<< "No scalar data for Volume\n");
	return;
	}
      vol->GetInput()->GetDimensions(dimensions);
      kOffset = dimensions[0] * dimensions[1];

      // move t to the nearest exact point
      j = (int)((t*calcSteps) + 1);
      t = j/calcSteps;

      for (i = 0; i < 3; i++)
	{
	hitPosition[i] = p1Coords[i] + (ray[i]*j);
        index[i] = (int)hitPosition[i];
	}
      
      for ( newVoxel=1; (t < t2) && (currentAlpha < (254/255.0));  )
	{

	for (i = 0; i < 3; i++) pcoords[i] = (hitPosition[i] - index[i]);
	vtkVoxel::InterpolationFunctions(pcoords,sf);

        if ( newVoxel )
          {
          ptId = index[0] + index[1]*dimensions[0] + index[2]*kOffset;
          ptIds.SetId(0,ptId);
          ptIds.SetId(1,ptId+1);
          ptIds.SetId(2,ptId+dimensions[0]);
          ptIds.SetId(3,ptId+1+dimensions[0]);
          ptIds.SetId(4,ptId+kOffset);
          ptIds.SetId(5,ptId+1+kOffset);
          ptIds.SetId(6,ptId+dimensions[0]+kOffset);
          ptIds.SetId(7,ptId+1+dimensions[0]+kOffset);

          scalars->GetScalars(ptIds,voxelValues);
          }

        for (value=0.0, i=0; i < 8; i++)
          value += voxelValues.GetScalar(i) * sf[i];

	// map through the lookup table
        memcpy(temp_col,vol->GetLookupTable()->MapValue(value),4);
	resultRay[j*4] = temp_col[0]/ 255.0;
	resultRay[j*4+1] = temp_col[1]/ 255.0;
	resultRay[j*4+2] = temp_col[2]/ 255.0;
	mag = temp_col[3]*this->StepSize;
	if (mag > 255) mag = 255;
	resultRay[j*4+3] = mag/255.0;
	currentAlpha = currentAlpha + (1 - currentAlpha)*resultRay[j*4+3];

	for (newVoxel=0, i=0; i < 3; i++)
	  {
	  hitPosition[i] += ray[i];
          if ( (idx = (int)hitPosition[i]) != index[i] )
            {
            index[i] = idx;
            newVoxel = 1;
            }
	  }

	t += (1.0/calcSteps);
	j++;

	}
      }
    }
}



// Description:
// Add a volume to the list of volumes.
void vtkVolumeRenderer::AddVolume(vtkVolume *actor)
{
  this->Volumes.AddItem(actor);
}

// Description:
// Remove a volume from the list of volumes.
void vtkVolumeRenderer::RemoveVolume(vtkVolume *actor)
{
  this->Volumes.RemoveItem(actor);
}


void vtkVolumeRenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os,indent);

  os << indent << "Volumes:\n";
  this->Volumes.PrintSelf(os,indent.GetNextIndent());
}

