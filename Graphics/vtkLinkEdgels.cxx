/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinkEdgels.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
#include <stdlib.h>
#include "vtkLinkEdgels.h"
#include "vtkMath.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkLinkEdgels* vtkLinkEdgels::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkLinkEdgels");
  if(ret)
    {
    return (vtkLinkEdgels*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkLinkEdgels;
}




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
  vtkPoints *newPts=0;
  vtkCellArray *newLines=0;
  vtkFloatArray *inScalars;
  vtkFloatArray *outScalars;
  vtkImageData *input = this->GetInput();
  vtkFloatArray *outVectors;
  vtkPolyData *output = this->GetOutput();
  int *dimensions;
  float *CurrMap, *inDataPtr;
  vtkDataArray *inVectors;
  int ptId;
  
  vtkDebugMacro(<< "Extracting structured points geometry");

  pd = input->GetPointData();
  dimensions = input->GetDimensions();
  inScalars = vtkFloatArray::SafeDownCast(pd->GetActiveScalars());
  inVectors = pd->GetActiveVectors();
  if ((input->GetNumberOfPoints()) < 2 || inScalars == NULL)
    {
    vtkErrorMacro(<<"No data to transform (or wrong data type)!");
    return;
    }

  // set up the input
  inDataPtr = inScalars->GetPointer(0);

  // Finally do edge following to extract the edge data from the Thin image
  newPts = vtkPoints::New();
  newLines = vtkCellArray::New();
  outScalars = vtkFloatArray::New();
  outVectors = vtkFloatArray::New();
  outVectors->SetNumberOfComponents(3);

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
//  outScalars->ComputeRange();
  output->GetPointData()->SetScalars(outScalars);
  output->GetPointData()->SetVectors(outVectors);
  
  newPts->Delete();
  newLines->Delete();
  outScalars->Delete();
  outVectors->Delete();
}

// This method links the edges for one image. 
void vtkLinkEdgels::LinkEdgels(int xdim, int ydim, float *image, 
			       vtkDataArray *inVectors,
			       vtkCellArray *newLines, 
			       vtkPoints *newPts,
			       vtkFloatArray *outScalars, 
			       vtkFloatArray *outVectors,
			       int z)
{
  int **forward;
  int **backward;
  int x,y,ypos,zpos;
  int currX, currY, i;
  int newX, newY;
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
  int bestDirection = 0;
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
	inVectors->GetTuple(x+ypos+zpos,vec1); 
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
	      inVectors->GetTuple(x + xoffset[i] + 
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
		inVectors->GetTuple(x + xoffset[i] + 
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
	start = outScalars->GetNumberOfTuples();
	newX = currX;
	newY = currY;
	do
	  {
	  currX = newX;
	  currY = newY;
	  outScalars->InsertNextTuple(&(image[currX + currY*xdim]));
	  inVectors->GetTuple(currX+currY*xdim+zpos,vec2); 
	  vtkMath::Normalize(vec2); 
          outVectors->InsertNextTuple(vec2);
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
