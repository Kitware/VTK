/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinkEdgels.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1997 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkLinkEdgels.h"
#include "vtkMath.h"

// Description:
// Construct instance of vtkLinkEdgels with GradientThreshold set to 
// 0.1, PhiThreshold set to 90 degrees and LinkThreshold set to 90 degrees.
vtkLinkEdgels::vtkLinkEdgels()
{
  this->GradientThreshold = 0.1;
  this->PhiThreshold = 90;
  this->LinkThreshold = 90;
}

void vtkLinkEdgels::Execute()
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
  float *CurrMap, *inDataPtr;
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
  inDataPtr = inScalars->GetPointer(0);

  // Finally do edge following to extract the edge data from the Thin image
  newPts = vtkFloatPoints::New();
  newLines = vtkCellArray::New();
  outScalars = vtkFloatScalars::New();
  outVectors = vtkFloatVectors::New();

  vtkDebugMacro("doing edge linking\n");
  //
  // Traverse all points, for each point find Gradient in the Image map.
  //
  for (ptId=0; ptId < dimensions[2]; ptId++)
    {
    CurrMap = inDataPtr + dimensions[0]*dimensions[1]*ptId;
    
    this->LinkEdgels(dimensions[0],dimensions[1],CurrMap, inVectors,
		     newLines,newPts,outScalars,outVectors,ptId);
    }
  
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

// Description:
// This method links the edges for one image. 
void vtkLinkEdgels::LinkEdgels(int xdim, int ydim, float *image, 
				    vtkVectors *inVectors,
				    vtkCellArray *newLines, 
				    vtkFloatPoints *newPts,
				    vtkFloatScalars *outScalars, 
				    vtkFloatVectors *outVectors,
				    int z)
{
  int **forward;
  int **backward;
  int x,y,ypos,zpos;
  int currX, currY, i;
  int newX, newY;
  int startX, startY;
  float vec[3], vec1[3], vec2[3];
  float linkThresh, phiThresh;
  // these direction vectors are rotated 90 degrees
  // to convert gradient direction into edgel direction
  static float directions[8][2] = {
    {0,1},  {-0.707, 0.707},
    {-1,0}, {-0.707, -0.707},
    {0,-1}, {0.707, -0.707},
    {1,0},  {0.707, 0.707}}; 
  static int xoffset[8] = {1,1,0,-1,-1,-1,0,1};
  static int yoffset[8] = {0,1,1,1,0,-1,-1,-1};
  int length, start;
  int bestDirection;
  float error, bestError;

  forward  = new int *[ydim];
  backward = new int *[ydim];
  for (i = 0; i < ydim; i++)
    {
    forward[i]  = new int [xdim];
    backward[i] = new int [xdim];
    memset(forward[i],0,xdim*sizeof(int));
    memset(backward[i],0,xdim*sizeof(int));
    }

  zpos = z*xdim*ydim;
  linkThresh = cos(this->LinkThreshold*3.1415926/180.0);
  phiThresh = cos(this->PhiThreshold*3.1415926/180.0);

  // first find all forward & backwards links
  for (y = 0; y < ydim; y++)
    {
    ypos = y*xdim;
    for (x = 0; x < xdim; x++)
      {
      // find forward and backward neighbor for this pixel
      // if its value is less than threshold then ignore it
      if (image[x+ypos] < this->GradientThreshold)
	{
	forward[y][x] = -1;
	backward[y][x] = -1;
	}
      else
	{
	// try all neighbors as forward, first try four connected
	inVectors->GetVector(x+ypos+zpos,vec1); 
	vtkMath::Normalize(vec1); 
	// first eliminate based on phi1 - alpha
	bestError = 0;
	for (i = 0; i < 8; i += 2)
	  {
	  // make sure it passes the linkThresh test
	  if ((directions[i][0]*vec1[0]+directions[i][1]*vec1[1]) >= 
	      linkThresh)
	    {
	    // make sure we dont go off the edge and are >= GradientThresh
	    // and it hasn't already been set
	    if ((x + xoffset[i] >= 0)&&(x + xoffset[i] < xdim)&&
		(y + yoffset[i] >= 0)&&(y + yoffset[i] < ydim)&&
		(!backward[y+yoffset[i]][x+xoffset[i]])&&
		(image[x + xoffset[i] + (y+yoffset[i])*xdim] >=
		 this->GradientThreshold)) 
	      {
	      // satisfied the first test, now check second
	      inVectors->GetVector(x + xoffset[i] + 
				   (y + yoffset[i])*xdim + zpos,vec2); 
	      vtkMath::Normalize(vec2); 
	      if ((vec1[0]*vec2[0] + vec1[1]*vec2[1]) >= phiThresh)
		{
		// pased phi - phi test does the forward neighbor
		// pass the link test
		if ((directions[i][0]*vec2[0]+directions[i][1]*vec2[1]) >= 
		    linkThresh)
		  {
		  // check against the current best solution
		  error = (directions[i][0]*vec2[0]+directions[i][1]*vec2[1])
		    + (directions[i][0]*vec1[0]+directions[i][1]*vec1[1])
		    + (vec1[0]*vec2[0] + vec1[1]*vec2[1]);
		  if (error > bestError)
		    {
		    bestDirection = i;
		    bestError = error;
		    }
		  }
		}
	      }
	    }
	  }
	if (bestError > 0)
	  {
	  forward[y][x] = (bestDirection+1);
	  backward[y+yoffset[bestDirection]][x+xoffset[bestDirection]] 
	    = ((bestDirection+4)%8)+1;
	  }
	else
	  {
	  // check the eight connected neighbors now
	  for (i = 1; i < 8; i += 2)
	    {
	    // make sure it passes the linkThresh test
	    if ((directions[i][0]*vec1[0]+directions[i][1]*vec1[1]) >= 
		linkThresh)
	      {
	      // make sure we dont go off the edge and are >= GradientThresh
	      // and it hasn't already been set
	      if ((x + xoffset[i] >= 0)&&(x + xoffset[i] < xdim)&&
		  (y + yoffset[i] >= 0)&&(y + yoffset[i] < ydim)&&
		  (!backward[y+yoffset[i]][x+xoffset[i]])&&
		  (image[x + xoffset[i] + (y+yoffset[i])*xdim] >=
		   this->GradientThreshold)) 
		{
		// satisfied the first test, now check second
		inVectors->GetVector(x + xoffset[i] + 
				     (y + yoffset[i])*xdim + zpos,vec2); 
		vtkMath::Normalize(vec2); 
		if ((vec1[0]*vec2[0] + vec1[1]*vec2[1]) >= phiThresh)
		  {
		  // pased phi - phi test does the forward neighbor
		  // pass the link test
		  if ((directions[i][0]*vec2[0]+directions[i][1]*vec2[1]) >= 
		      linkThresh)
		    {
		    // check against the current best solution
		    error = (directions[i][0]*vec2[0]+directions[i][1]*vec2[1])
		      + (directions[i][0]*vec1[0]+directions[i][1]*vec1[1])
		      + (vec1[0]*vec2[0] + vec1[1]*vec2[1]);
		    if (error > bestError)
		      {
		      bestDirection = i;
		      bestError = error;
		      }
		    }
		  }
		}
	      }
	    }
	  if (bestError > 0)
	    {
	    forward[y][x] = (bestDirection+1);
	    backward[y+yoffset[bestDirection]][x+xoffset[bestDirection]] 
	      = ((bestDirection+4)%8)+1;
	    }
	  }
	}
      }
    }
  

  // now construct the chains
  vec[2] = z;
  for (y = 0; y < ydim; y++)
    {
    ypos = y*xdim;
    for (x = 0; x < xdim; x++)
      {
      // do we have part of an edgel chain ?
      // isolated edgels do not qualify
      if (backward[y][x] > 0)
	{
	// trace back to the beginning
	currX = x;
	currY = y;
	do
	  {
	  newX = currX + xoffset[backward[currY][currX] - 1];
	  currY += yoffset[backward[currY][currX] - 1];
	  currX = newX;
	  }
	while ((currX != x || currY != y) && backward[currY][currX]);

	// now trace to the end and build the digital curve
	length = 0;
	start = outScalars->GetNumberOfScalars();
	startX = currX;
	startY = currY;
	newX = currX;
	newY = currY;
	do
	  {
	  currX = newX;
	  currY = newY;
	  outScalars->InsertNextScalar(image[currX + currY*xdim]);
	  inVectors->GetVector(currX+currY*xdim+zpos,vec2); 
	  vtkMath::Normalize(vec2); 
          outVectors->InsertNextVector(vec2);
	  vec[0] = currX;
	  vec[1] = currY;
	  newPts->InsertNextPoint(vec);
	  length++;

	  // if there is a next pixel select it
	  if (forward[currY][currX])
	    {
	    newX = currX + xoffset[forward[currY][currX] - 1];
	    newY = currY + yoffset[forward[currY][currX] - 1];
	    }
	  // clear out this edgel now that were done with it
	  backward[newY][newX] = 0;
	  forward[currY][currX] = 0;
	  }
	while ((currX != newX || currY != newY));
	
	// build up the cell
	newLines->InsertNextCell(length);
	for (i = 0; i < length; i++)
	  {
	  newLines->InsertCellPoint(start);
	  start++;
	  }
	}
      }
    }

  // free up the memory
  for (i = 0; i < ydim; i++)
    {
    delete [] forward[i];
    delete [] backward[i];
    }
  delete [] forward;
  delete [] backward;
}

void vtkLinkEdgels::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "GradientThreshold:" << this->GradientThreshold << "\n";
  os << indent << "LinkThreshold:" << this->LinkThreshold << "\n";
  os << indent << "PhiThreshold:" << this->PhiThreshold << "\n";
}
