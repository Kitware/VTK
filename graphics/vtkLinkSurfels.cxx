/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinkSurfels.cxx
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
#include "vtkMath.h"
#include "vtkLinkSurfels.h"


//----------------------------------------------------------------------------
// Description:
// Construct instance of vtkImageLinkSurfels with GradientThreshold set to 
// 0.1, PhiThreshold set to 90 degrees and LinkThreshold set to 90 degrees.
vtkLinkSurfels::vtkLinkSurfels()
{
  this->GradientThreshold = 0.1;
  this->PhiThreshold = 90;
  this->LinkThreshold = 90;
}

void vtkLinkSurfels::Execute()
{
  vtkPointData *pd;
  vtkFloatPoints *newPts=0;
  vtkCellArray *newLines=0;
  vtkFloatScalars *inScalars;
  vtkFloatScalars *outScalars;
  vtkStructuredPoints *input = this->GetInput();
  int numPts;
  vtkFloatVectors *outVectors;
  vtkPolyData *output = this->GetOutput();
  int *dimensions;
  float *inDataPtr;
  vtkVectors *inVectors;
  int ptId;
  
  vtkDebugMacro(<< "Extracting structured points geometry");

  pd = input->GetPointData();
  dimensions = input->GetDimensions();
  inScalars = (vtkFloatScalars *)pd->GetScalars();
  inVectors = pd->GetVectors();
  if ((numPts=this->Input->GetNumberOfPoints()) < 2 || inScalars == NULL)
    {
    vtkErrorMacro(<<"No data to transform!");
    return;
    }

  // set up the input
  inDataPtr = inScalars->GetPtr(0);

  // Finally do edge following to extract the edge data from the Thin image
  newPts = new vtkFloatPoints;
  newLines = new vtkCellArray;
  outScalars = new vtkFloatScalars;
  outVectors = new vtkFloatVectors;

  vtkDebugMacro("doing surfel linking\n");

  this->LinkSurfels(dimensions[0], dimensions[1], dimensions[2],
		    inDataPtr, inVectors, 
		    newLines, newPts, outScalars, outVectors);
  
  output->SetPoints(newPts);
  output->SetLines(newLines);

  // Update ourselves
  outScalars->ComputeRange();
  output->GetPointData()->SetScalars(outScalars);
  output->GetPointData()->SetVectors(outVectors);
  
  newPts->Delete();
  newLines->Delete();
  outScalars->Delete();
  outVectors->Delete();
}

  

#define vtkLinkSurfelsTryPixel() \
/* make sure it passes the linkThresh test */\
if ((fabs(vtkMath::Dot(directions[i],vec1)) <= linkThresh)) \
    { \
  pos2 = x + xoffset[i] + ypos + yoffset[i]*xdim + zpos + zoffset[i]*xdim*ydim; \
  /* make sure we dont go off the edge and are >= GradientThresh */\
     /* and it hasn't already been set */\
  if ((x + xoffset[i] >= 0)&&(x + xoffset[i] < xdim)&& \
      (y + yoffset[i] >= 0)&&(y + yoffset[i] < ydim)&& \
      (z + zoffset[i] >= 0)&&(z + zoffset[i] < zdim)&& \
      ((!PrimaryB[z+zoffset[i]][y+yoffset[i]][x+xoffset[i]]) || \
       (!SecondaryB[z+zoffset[i]][y+yoffset[i]][x+xoffset[i]])) && \
      (image[pos2] >= this->GradientThreshold))  \
    { \
	/* satisfied the first test, now check second */\
	  inVectors->GetVector(pos2,vec2); \
	  vtkMath::Normalize(vec2); \
    if (vtkMath::Dot(vec1,vec2) >= phiThresh) \
      { \
	  /* pased phi - phi test does the forward neighbor */\
	     /* pass the link test */\
      if (fabs(vtkMath::Dot(directions[i],vec2)) <= linkThresh) \
	{ \
	    /* check against the current best solution */\
	error = 2.0 - fabs(vtkMath::Dot(directions[i],vec2)) -  \
	  fabs(vtkMath::Dot(directions[i],vec1)) + \
	  vtkMath::Dot(vec1,vec2); \
	if (error > bestError) \
	  { \
	  bestDirection = i; \
	  bestError = error; \
	  } \
	} \
      } \
    } \
}

#define vtkLinkSurfelsSecondaryTryPixel() \
/* make sure it is far enough from forward link */\
if ((fabs(vtkMath::Dot(directions[i],vec1)) <= linkThresh) && \
    (fabs(vtkMath::Dot(directions[i],directions[PrimaryF[z][y][x]-1])) \
     <=linkThresh)) \
{ \
  pos2 = x + xoffset[i] + ypos + yoffset[i]*xdim + zpos + zoffset[i]*xdim*ydim; \
  /* make sure we dont go off the edge and are >= GradientThresh */\
  /* and it hasn't already been set */\
  if ((x + xoffset[i] >= 0)&&(x + xoffset[i] < xdim)&& \
      (y + yoffset[i] >= 0)&&(y + yoffset[i] < ydim)&& \
      (z + zoffset[i] >= 0)&&(z + zoffset[i] < zdim)&& \
      ((!PrimaryB[z+zoffset[i]][y+yoffset[i]][x+xoffset[i]]) || \
       (!SecondaryB[z+zoffset[i]][y+yoffset[i]][x+xoffset[i]])) && \
      (image[pos2] >= this->GradientThreshold))  \
    { \
	/*satisfied the first test, now check second */\
	  inVectors->GetVector(pos2,vec2); \
	  vtkMath::Normalize(vec2); \
    if (vtkMath::Dot(vec1,vec2) >= phiThresh) \
					   { \
	  /* pased phi - phi test does the forward neighbor */\
	  /* pass the link test */\
      if (fabs(vtkMath::Dot(directions[i],vec2)) <= linkThresh) \
	{ \
	    /* check against the current best solution */\
	error = 3.0 - fabs(vtkMath::Dot(directions[i],vec2)) -  \
	  fabs(vtkMath::Dot(directions[i],directions[PrimaryF[z][y][x] -1])) \
	    - fabs(vtkMath::Dot(directions[i],vec1)) + \
	    vtkMath::Dot(vec1,vec2); \
	if (error > bestError) \
	  { \
	  bestSecondaryDirection = i; \
	  bestError = error; \
	  } \
	} \
      } \
    } \
}

// Description:
// This method links the edges for one image. 
void vtkLinkSurfels::LinkSurfels(int xdim, int ydim, int zdim,
				 float *image,
				 vtkVectors *inVectors,
				 vtkCellArray *newLines, 
				 vtkFloatPoints *newPts,
				 vtkFloatScalars *outScalars, 
				 vtkFloatVectors *outVectors)
{
  static float directions[26][3] = {
    // face neighbors
    { 0, 0, 1},
    { 0, 1, 0},
    { 1, 0, 0},
    {-1, 0, 0},
    { 0,-1, 0},
    { 0, 0,-1},
    
    // edge neighbors
    { 0,  0.707,  0.707}, 
    { 0.707,  0,  0.707},
    {-0.707,  0,  0.707},
    { 0, -0.707,  0.707},
    { 0.707,  0.707, 0}, 
    {-0.707,  0.707, 0},
    { 0.707, -0.707, 0},
    {-0.707, -0.707, 0},
    { 0,  0.707, -0.707},
    { 0.707, 0, -0.707},
    {-0.707, 0, -0.707},
    { 0, -0.707, -0.707},

    // vertex neighbors
    { 0.577, 0.577, 0.577},
    {-0.577, 0.577, 0.577},
    { 0.577,-0.577, 0.577},
    {-0.577,-0.577, 0.577},
    { 0.577, 0.577,-0.577},
    {-0.577, 0.577,-0.577},
    { 0.577,-0.577,-0.577},
    {-0.577,-0.577,-0.577}};

  static int xoffset[26] = {0,0,1,-1,0,0, 
			    0,1,-1,0,1,-1,1,-1,0,1,-1,0, 
			    1,-1,1,-1,1,-1,1,-1};
  static int yoffset[26] = {0,1,0,0,-1,0, 
			    1,0,0,-1,1,1,-1,-1,1,0,0,-1,
			    1,1,-1,-1,1,1,-1,-1};
  static int zoffset[26] = {1,0,0,0,0,-1, 
			    1,1,1,1,0,0,0,0,-1,-1,-1,-1,
			    1,1,1,1,-1,-1,-1,-1};
  static int opposite[26] = {5,4,3,2,1,0, 
			     17,16,15,14,13,12,11,10,9,8,7,6, 
			     25,24,23,22,21,20,19,18};
  
  char ***SecondaryF;
  char ***SecondaryB;
  char ***PrimaryF;
  char ***PrimaryB;
  unsigned int ***Id;
  int x,y,z;
  int ypos, zpos, pos2;
  int currX, currY, i;
  int newX, newY;
  int startX, startY;
  float vec[3], vec1[3], vec2[3];
  float linkThresh, phiThresh;
  int length, start;
  int bestDirection = 0;
  int bestSecondaryDirection = 0;
  float error, bestError;
  
  linkThresh = cos(this->LinkThreshold*3.1415926/180.0);
  phiThresh = cos(this->PhiThreshold*3.1415926/180.0);

  PrimaryF  = new char **[zdim];
  PrimaryB  = new char **[zdim];
  SecondaryF  = new char **[zdim];
  SecondaryB  = new char **[zdim];
  Id = new unsigned int **[zdim];
  for (z = 0; z < zdim; z++)
    {
    PrimaryF[z]  = new char *[ydim];
    PrimaryB[z]  = new char *[ydim];
    SecondaryF[z]  = new char *[ydim];
    SecondaryB[z]  = new char *[ydim];
    Id[z] = new unsigned int *[ydim];
    for (y = 0; y < ydim; y++)
      {
      PrimaryF[z][y]  = new char [xdim];
      PrimaryB[z][y]  = new char [xdim];
      SecondaryF[z][y]  = new char [xdim];
      SecondaryB[z][y]  = new char [xdim];
      Id[z][y] = new unsigned int [xdim];
      memset(PrimaryF[z][y],0,xdim);
      memset(PrimaryB[z][y],0,xdim);
      memset(SecondaryF[z][y],0,xdim);
      memset(SecondaryB[z][y],0,xdim);
      }
    }
  
  // first find all PrimaryF face links
  for (z = 0; z < zdim; z++)
    {
    zpos = z*ydim*xdim;
    for (y = 0; y < ydim; y++)
      {
      ypos = y*xdim;
      for (x = 0; x < xdim; x++)
	{
	// find PrimaryF and PrimaryB neighbor for this pixel
	// if its value is less than threshold then ignore it
	if (image[zpos + ypos + x] < this->GradientThreshold)
	  {
	  PrimaryF[z][y][x] = -1;
	  PrimaryB[z][y][x] = -1;
	  SecondaryF[z][y][x] = -1;
	  SecondaryB[z][y][x] = -1;
	  }
	else
	  {
	  // try all neighbors as PrimaryF, first try face neighbors
	  inVectors->GetVector(x+ypos+zpos,vec1);
	  vtkMath::Normalize(vec1);
	  bestError = 0;
	  for (i = 0; i < 3; i ++)
	    {
	    vtkLinkSurfelsTryPixel();
	    }
	  // record the match if any
	  if (bestError > 0)
	    {
	    PrimaryF[z][y][x] = bestDirection+1;
	    // was it primary or secondary for link
	    if (!PrimaryB[z + zoffset[bestDirection]][y+yoffset[bestDirection]]
		[x+xoffset[bestDirection]])
	      {
	      PrimaryB[z + zoffset[bestDirection]][y+yoffset[bestDirection]]
	        [x+xoffset[bestDirection]] = opposite[bestDirection]+1;
	      }
	    else
	      {
	      SecondaryB[z + zoffset[bestDirection]][y+yoffset[bestDirection]]
	        [x+xoffset[bestDirection]] = opposite[bestDirection]+1;
	      }
	    }
	  }
	}
      }
    }
    
  // now find all Secondary face links
  for (z = 0; z < zdim; z++)
    {
    zpos = z*ydim*xdim;
    for (y = 0; y < ydim; y++)
      {
      ypos = y*xdim;
      for (x = 0; x < xdim; x++)
	{
	// find PrimaryF and PrimaryB neighbor for this pixel
	// if its value is less than threshold then ignore it
	if ((image[zpos + ypos + x] >= this->GradientThreshold)&&
	    (PrimaryF[z][y][x] > 0))
	  {
	  // try all neighbors as PrimaryF, first try face neighbors
	  inVectors->GetVector(x+ypos+zpos,vec1);
	  vtkMath::Normalize(vec1);
	  bestError = 0;
	  for (i = 0; i < 3; i ++)
	    {
	    vtkLinkSurfelsSecondaryTryPixel();
	    }
	  // record the match if any
	  if (bestError > 0)
	    {
	    SecondaryF[z][y][x] = bestSecondaryDirection+1;
	    // was it primary or secondary for link
	    if (!PrimaryB[z +zoffset[bestSecondaryDirection]]
		[y+yoffset[bestSecondaryDirection]]
		[x+xoffset[bestSecondaryDirection]])
	      {
	      PrimaryB[z +zoffset[bestSecondaryDirection]]
		[y+yoffset[bestSecondaryDirection]]
		[x+xoffset[bestSecondaryDirection]] = 
		opposite[bestSecondaryDirection]+1;
	      }
	    else
	      {
	      SecondaryB[z +zoffset[bestSecondaryDirection]]
		[y+yoffset[bestSecondaryDirection]]
		[x+xoffset[bestSecondaryDirection]] = 
		opposite[bestSecondaryDirection]+1;
	      }
	    }
	  }
	}
      }
    }

  // now look for reverse Primary face links
  for (z = 0; z < zdim; z++)
    {
    zpos = z*ydim*xdim;
    for (y = 0; y < ydim; y++)
      {
      ypos = y*xdim;
      for (x = 0; x < xdim; x++)
	{
	// find PrimaryF and PrimaryB neighbor for this pixel
	// if its value is less than threshold then ignore it
	if ((image[zpos + ypos + x] >= this->GradientThreshold)&&
	    (PrimaryF[z][y][x] == 0))
	  {
	  // try all neighbors as PrimaryF, first try face neighbors
	  inVectors->GetVector(x+ypos+zpos,vec1);
	  vtkMath::Normalize(vec1);
	  bestError = 0;
	  for (i = 3; i < 6; i ++)
	    {
	    vtkLinkSurfelsTryPixel();
	    }
	  // record the match if any
	  if (bestError > 0)
	    {
	    PrimaryF[z][y][x] = bestDirection+1;
	    // was it primary or secondary for link
	    if (!PrimaryB[z + zoffset[bestDirection]][y+yoffset[bestDirection]]
		[x+xoffset[bestDirection]])
	      {
	      PrimaryB[z + zoffset[bestDirection]][y+yoffset[bestDirection]]
	        [x+xoffset[bestDirection]] = opposite[bestDirection]+1;
	      }
	    else
	      {
	      SecondaryB[z + zoffset[bestDirection]][y+yoffset[bestDirection]]
	        [x+xoffset[bestDirection]] = opposite[bestDirection]+1;
	      }
	    }
	  }
	}
      }
    }

  // now find all Secondary reverse face links
  for (z = 0; z < zdim; z++)
    {
    zpos = z*ydim*xdim;
    for (y = 0; y < ydim; y++)
      {
      ypos = y*xdim;
      for (x = 0; x < xdim; x++)
	{
	// find PrimaryF and PrimaryB neighbor for this pixel
	// if its value is less than threshold then ignore it
	if ((image[zpos + ypos + x] >= this->GradientThreshold)&&
	    (PrimaryF[z][y][x] > 0))
	  {
	  // try all neighbors as PrimaryF, first try face neighbors
	  inVectors->GetVector(x+ypos+zpos,vec1);
	  vtkMath::Normalize(vec1);
	  bestError = 0;
	  for (i = 3; i < 6; i ++)
	    {
	    vtkLinkSurfelsSecondaryTryPixel();
	    }
	  // record the match if any
	  if (bestError > 0)
	    {
	    SecondaryF[z][y][x] = bestSecondaryDirection+1;
	    // was it primary or secondary for link
	    if (!PrimaryB[z +zoffset[bestSecondaryDirection]]
		[y+yoffset[bestSecondaryDirection]]
		[x+xoffset[bestSecondaryDirection]])
	      {
	      PrimaryB[z +zoffset[bestSecondaryDirection]]
		[y+yoffset[bestSecondaryDirection]]
		[x+xoffset[bestSecondaryDirection]] = 
		opposite[bestSecondaryDirection]+1;
	      }
	    else
	      {
	      SecondaryB[z +zoffset[bestSecondaryDirection]]
		[y+yoffset[bestSecondaryDirection]]
		[x+xoffset[bestSecondaryDirection]] = 
		opposite[bestSecondaryDirection]+1;
	      }
	    }
	  }
	}
      }
    }

  /*
  // now find all Primary edge links
  for (z = 0; z < zdim; z++)
    {
    zpos = z*ydim*xdim;
    for (y = 0; y < ydim; y++)
      {
      ypos = y*xdim;
      for (x = 0; x < xdim; x++)
	{
	// find PrimaryF and PrimaryB neighbor for this pixel
	// if its value is less than threshold then ignore it
	if ((image[zpos + ypos + x] >= this->GradientThreshold)&&
	    (PrimaryF[z][y][x] == 0))
	  {
	  // try all neighbors as PrimaryF, first try face neighbors
	  inVectors->GetVector(x+ypos+zpos,vec1);
	  vtkMath::Normalize(vec1);
	  bestError = 0;
	  for (i = 6; i < 12; i++)
	    {
	    vtkLinkSurfelsTryPixel();
	    }
	  // record the match if any
	  if (bestError > 0)
	    {
	    PrimaryF[z][y][x] = bestDirection+1;
	    // was it primary or secondary for link
	    if (!PrimaryB[z + zoffset[bestDirection]][y+yoffset[bestDirection]]
		[x+xoffset[bestDirection]])
	      {
	      PrimaryB[z + zoffset[bestDirection]][y+yoffset[bestDirection]]
	        [x+xoffset[bestDirection]] = opposite[bestDirection]+1;
	      }
	    else
	      {
	      SecondaryB[z + zoffset[bestDirection]][y+yoffset[bestDirection]]
	        [x+xoffset[bestDirection]] = opposite[bestDirection]+1;
	      }
	    }
	  }
	}
      }
    }
  
  // now find all Secondary edge links
  for (z = 0; z < zdim; z++)
    {
    zpos = z*ydim*xdim;
    for (y = 0; y < ydim; y++)
      {
      ypos = y*xdim;
      for (x = 0; x < xdim; x++)
	{
	// find PrimaryF and PrimaryB neighbor for this pixel
	// if its value is less than threshold then ignore it
	if ((image[zpos + ypos + x] >= this->GradientThreshold)&&
	    (PrimaryF[z][y][x] > 0) && (SecondaryF[z][y][x] == 0))
	  {
	  // try all neighbors as PrimaryF, first try face neighbors
	  inVectors->GetVector(x+ypos+zpos,vec1);
	  vtkMath::Normalize(vec1);
	  bestError = 0;
	  for (i = 6; i < 12; i++)
	    {
	    vtkLinkSurfelsSecondaryTryPixel();
	    }
	  // record the match if any
	  if (bestError > 0)
	    {
	    SecondaryF[z][y][x] = bestSecondaryDirection+1;
	    // was it primary or secondary for link
	    if (!PrimaryB[z +zoffset[bestSecondaryDirection]]
		[y+yoffset[bestSecondaryDirection]]
		[x+xoffset[bestSecondaryDirection]])
	      {
	      PrimaryB[z +zoffset[bestSecondaryDirection]]
		[y+yoffset[bestSecondaryDirection]]
		[x+xoffset[bestSecondaryDirection]] = 
		opposite[bestSecondaryDirection]+1;
	      }
	    else
	      {
	      SecondaryB[z +zoffset[bestSecondaryDirection]]
		[y+yoffset[bestSecondaryDirection]]
		[x+xoffset[bestSecondaryDirection]] = 
		opposite[bestSecondaryDirection]+1;
	      }
	    }
	  }
	}
      }
    }

  // now find all Primary vertex links
  for (z = 0; z < zdim; z++)
    {
    zpos = z*ydim*xdim;
    for (y = 0; y < ydim; y++)
      {
      ypos = y*xdim;
      for (x = 0; x < xdim; x++)
	{
	// find PrimaryF and PrimaryB neighbor for this pixel
	// if its value is less than threshold then ignore it
	if ((image[zpos + ypos + x] >= this->GradientThreshold)&&
	    (PrimaryF[z][y][x] == 0))
	  {
	  // try all neighbors as PrimaryF, first try face neighbors
	  inVectors->GetVector(x+ypos+zpos,vec1);
	  vtkMath::Normalize(vec1);
	  bestError = 0;
	  for (i = 18; i < 22; i++)
	    {
	    vtkLinkSurfelsTryPixel();
	    }
	  // record the match if any
	  if (bestError > 0)
	    {
	    PrimaryF[z][y][x] = bestDirection+1;
	    // was it primary or secondary for link
	    if (!PrimaryB[z + zoffset[bestDirection]][y+yoffset[bestDirection]]
		[x+xoffset[bestDirection]])
	      {
	      PrimaryB[z + zoffset[bestDirection]][y+yoffset[bestDirection]]
	        [x+xoffset[bestDirection]] = opposite[bestDirection]+1;
	      }
	    else
	      {
	      SecondaryB[z + zoffset[bestDirection]][y+yoffset[bestDirection]]
	        [x+xoffset[bestDirection]] = opposite[bestDirection]+1;
	      }
	    }
	  }
	}
      }
    }
  
  // now find all Secondary vertex links
  for (z = 0; z < zdim; z++)
    {
    zpos = z*ydim*xdim;
    for (y = 0; y < ydim; y++)
      {
      ypos = y*xdim;
      for (x = 0; x < xdim; x++)
	{
	// find PrimaryF and PrimaryB neighbor for this pixel
	// if its value is less than threshold then ignore it
	if ((image[zpos + ypos + x] >= this->GradientThreshold)&&
	    (PrimaryF[z][y][x] > 0) && (SecondaryF[z][y][x] == 0))
	  {
	  // try all neighbors as PrimaryF, first try face neighbors
	  inVectors->GetVector(x+ypos+zpos,vec1);
	  vtkMath::Normalize(vec1);
	  bestError = 0;
	  for (i = 18; i < 22; i++)
	    {
	    vtkLinkSurfelsSecondaryTryPixel();
	    }
	  // record the match if any
	  if (bestError > 0)
	    {
	    SecondaryF[z][y][x] = bestSecondaryDirection+1;
	    // was it primary or secondary for link
	    if (!PrimaryB[z +zoffset[bestSecondaryDirection]]
		[y+yoffset[bestSecondaryDirection]]
		[x+xoffset[bestSecondaryDirection]])
	      {
	      PrimaryB[z +zoffset[bestSecondaryDirection]]
		[y+yoffset[bestSecondaryDirection]]
		[x+xoffset[bestSecondaryDirection]] = 
		opposite[bestSecondaryDirection]+1;
	      }
	    else
	      {
	      SecondaryB[z +zoffset[bestSecondaryDirection]]
		[y+yoffset[bestSecondaryDirection]]
		[x+xoffset[bestSecondaryDirection]] = 
		opposite[bestSecondaryDirection]+1;
	      }
	    }
	  }
	}
      }
    }
    */
  
  // now construct the chains
  length = 0;
  for (z = 0; z < zdim; z++)
    {
    zpos = z*ydim*xdim;
    for (y = 0; y < ydim; y++)
      {
      ypos = y*xdim;
      for (x = 0; x < xdim; x++)
	{
	// do we have part of a surfel chain ?
	// isolated surfels do not qualify
	if (PrimaryF[z][y][x] > 0)
	  {
	  outScalars->InsertNextScalar(image[x+ypos+zpos]);
	  inVectors->GetVector(x+ypos+zpos,vec1);
	  vtkMath::Normalize(vec1);
	  outVectors->InsertNextVector(vec1);
	  vec[0] = x;
	  vec[1] = y;
	  vec[2] = z;
	  newPts->InsertNextPoint(vec);
	  Id[z][y][x] = length;
	  length++;
	  }
	else
	  {
	  if (PrimaryB[z][y][x] > 0)
	    {
	    outScalars->InsertNextScalar(image[x+ypos+zpos]);
	    inVectors->GetVector(x+ypos+zpos,vec1);
	    vtkMath::Normalize(vec1);
	    outVectors->InsertNextVector(vec1);
	    vec[0] = x;
	    vec[1] = y;
	    vec[2] = z;
	    newPts->InsertNextPoint(vec);
	    Id[z][y][x] = length;
	    length++;
	    }
	  }
	}
      }
    }

  for (z = 0; z < zdim; z++)
    {
    for (y = 0; y < ydim; y++)
      {
      for (x = 0; x < xdim; x++)
	{
	// do we have part of a surfel chain ?
	// isolated surfels do not qualify
	if (PrimaryF[z][y][x] > 0)
	  {
	  // is it a patch or a line
	  if (SecondaryF[z][y][x] > 0)
	    {
	    // do the forward triangle
	    newLines->InsertNextCell(3);
	    newLines->InsertCellPoint(Id[z+zoffset[PrimaryF[z][y][x]-1]]
				      [y+yoffset[PrimaryF[z][y][x]-1]]
				      [x+xoffset[PrimaryF[z][y][x]-1]]);
	    newLines->InsertCellPoint(Id[z][y][x]);
	    newLines->InsertCellPoint(Id[z+zoffset[SecondaryF[z][y][x]-1]]
				      [y+yoffset[SecondaryF[z][y][x]-1]]
				      [x+xoffset[SecondaryF[z][y][x]-1]]);
	    // backward triangle
	    if ((PrimaryB[z][y][x] > 0)&&(SecondaryB[z][y][x] > 0))
	      {
	      newLines->InsertNextCell(3);
	      newLines->InsertCellPoint(Id[z+zoffset[PrimaryB[z][y][x]-1]]
					[y+yoffset[PrimaryB[z][y][x]-1]]
					[x+xoffset[PrimaryB[z][y][x]-1]]);
	      newLines->InsertCellPoint(Id[z][y][x]);
	      newLines->InsertCellPoint(Id[z+zoffset[SecondaryB[z][y][x]-1]]
					[y+yoffset[SecondaryB[z][y][x]-1]]
					[x+xoffset[SecondaryB[z][y][x]-1]]);
	      }
	    }
	  else
	    {
	    // do the forward line
	    newLines->InsertNextCell(2);
	    newLines->InsertCellPoint(Id[z+zoffset[PrimaryF[z][y][x]-1]]
				      [y+yoffset[PrimaryF[z][y][x]-1]]
				      [x+xoffset[PrimaryF[z][y][x]-1]]);
	    newLines->InsertCellPoint(Id[z][y][x]);
	    }
	  }
	}
      }
    }

  // free memory
  for (z = 0; z < zdim; z++)
    {
    for (y = 0; y < ydim; y++)
      {
      delete [] PrimaryF[z][y];
      delete [] PrimaryB[z][y];
      delete [] SecondaryF[z][y];
      delete [] SecondaryB[z][y];
      delete [] Id[z][y];
      }
    delete [] PrimaryF[z];
    delete [] PrimaryB[z];
    delete [] SecondaryF[z];
    delete [] SecondaryB[z];
    delete [] Id[z];
    }
  delete [] PrimaryF;
  delete [] PrimaryB;
  delete [] SecondaryF;
  delete [] SecondaryB;
  delete [] Id;
}

void vtkLinkSurfels::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "GradientThreshold:" << this->GradientThreshold << "\n";
  os << indent << "LinkThreshold:" << this->LinkThreshold << "\n";
  os << indent << "PhiThreshold:" << this->PhiThreshold << "\n";
}

