/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinkSurfels.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkTriangle.h"
#include "vtkMergePoints.h"
#include "vtkLinkSurfels.h"


//----------------------------------------------------------------------------
// Description:
// Construct instance of vtkImageLinkSurfels with GradientThreshold set to 
// 0.1, PhiThreshold set to 90 degrees and LinkThreshold set to 90 degrees.
vtkLinkSurfels::vtkLinkSurfels()
{
  this->GradientThreshold = 0.1;
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
  float *spacing, *origin;
  
  vtkDebugMacro(<< "Extracting structured points geometry");

  pd = input->GetPointData();
  dimensions = input->GetDimensions();
  spacing = input->GetSpacing();
  origin = input->GetOrigin();
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

  vtkDebugMacro("doing surfel linking\n");

  this->LinkSurfels(dimensions[0], dimensions[1], dimensions[2],
		    inDataPtr, inVectors, newLines, newPts, 
		    outScalars, outVectors, spacing, origin);
  
  output->SetPoints(newPts);
  output->SetPolys(newLines);

  // Update ourselves
  outScalars->ComputeRange();
  output->GetPointData()->SetScalars(outScalars);
  output->GetPointData()->SetVectors(outVectors);
  
  newPts->Delete();
  newLines->Delete();
  outScalars->Delete();
  outVectors->Delete();
}


//
// vertex indicies:
// (xyz): index
// ------------
// (000): 0
// (100): 1
// (010): 2
// (110): 3
// (001): 4
// (101): 5
// (011): 6
// (111): 7
//  none: 8
//

// Description:
// This method links the edges for one image. 
void vtkLinkSurfels::LinkSurfels(int xdim, int ydim, int zdim,
				 float *image,
				 vtkVectors *inVectors,
				 vtkCellArray *newLines, 
				 vtkFloatPoints *newPts,
				 vtkFloatScalars *outScalars, 
				 vtkFloatVectors *outVectors,
				 float *spacing, float *origin)
{

//  at most 3 polygons (each 4 numbers make a polygon.)
//  fourth index for triangles is 8, 9 indicates end of list.
  static char polygonCases[256*13] = { 
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0000 0000*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0000 0001*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0000 0010*/ 
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0000 0011*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0000 0100*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0000 0101*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0000 0110*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0000 0111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0000 1000*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0000 1001*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0000 1010*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0000 1011*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0000 1100*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0000 1101*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0000 1110*/
    4, 5, 7, 6, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0000 1111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0001 0000*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0001 0001*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0001 0010*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0001 0011*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0001 0100*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0001 0101*/
    3, 5, 6, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0001 0110*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0001 0111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0001 1000*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0001 1001*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0001 1010*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0001 1011*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0001 1100*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0001 1101*/
    3, 4, 5, 8, 3, 4, 6, 8, 9, 9, 9, 9, 9, /*0001 1110*/
    4, 5, 7, 6, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0001 1111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0010 0000*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0010 0001*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0010 0010*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0010 0011*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0010 0100*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0010 0101*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0010 0110*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0010 0111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0010 1000*/
    2, 4, 7, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0010 1001*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0010 1010*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0010 1011*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0010 1100*/
    2, 4, 5, 8, 2, 5, 7, 8, 9, 9, 9, 9, 9, /*0010 1101*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0010 1110*/
    4, 5, 7, 6, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0010 1111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0011 0000*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0011 0001*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0011 0010*/
    2, 3, 7, 6, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0011 0011*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0011 0100*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0011 0101*/
    2, 3, 5, 8, 2, 5, 6, 8, 9, 9, 9, 9, 9, /*0011 0110*/
    2, 3, 7, 6, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0011 0111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0011 1000*/
    2, 3, 4, 8, 3, 4, 7, 8, 9, 9, 9, 9, 9, /*0011 1001*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0011 1010*/
    2, 3, 7, 6, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0011 1011*/
    2, 3, 5, 4, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0011 1100*/
    2, 3, 5, 4, 3, 5, 7, 8, 9, 9, 9, 9, 9, /*0011 1101*/
    2, 3, 5, 4, 2, 4, 6, 8, 9, 9, 9, 9, 9, /*0011 1110*/
    2, 3, 7, 6, 4, 5, 7, 6, 9, 9, 9, 9, 9, /*0011 1111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0100 0000*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0100 0001*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0100 0010*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0100 0011*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0100 0100*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0100 0101*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0100 0110*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0100 0111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0100 1000*/
    1, 4, 7, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0100 1001*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0100 1010*/
    1, 4, 6, 8, 1, 6, 7, 8, 9, 9, 9, 9, 9, /*0100 1011*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0100 1100*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0100 1101*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0100 1110*/
    4, 5, 7, 6, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0100 1111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0101 0000*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0101 0001*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0101 0010*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0101 0011*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0101 0100*/
    1, 3, 7, 5, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0101 0101*/
    1, 3, 6, 8, 1, 5, 6, 8, 9, 9, 9, 9, 9, /*0101 0110*/
    1, 3, 7, 5, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0101 0111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0101 1000*/
    1, 3, 4, 8, 3, 4, 7, 8, 9, 9, 9, 9, 9, /*0101 1001*/
    1, 3, 6, 4, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0101 1010*/
    1, 3, 6, 4, 3, 6, 7, 8, 9, 9, 9, 9, 9, /*0101 1011*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0101 1100*/
    1, 3, 7, 5, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0101 1101*/
    1, 3, 6, 4, 1, 4, 5, 8, 9, 9, 9, 9, 9, /*0101 1110*/
    1, 3, 7, 5, 4, 6, 7, 5, 9, 9, 9, 9, 9, /*0101 1111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0110 0000*/
    1, 2, 7, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0110 0001*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0110 0010*/
    1, 2, 6, 8, 1, 6, 7, 8, 9, 9, 9, 9, 9, /*0110 0011*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0110 0100*/
    1, 2, 5, 8, 2, 5, 7, 8, 9, 9, 9, 9, 9, /*0110 0101*/
    1, 2, 6, 5, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0110 0110*/
    1, 2, 6, 5, 5, 6, 7, 8, 9, 9, 9, 9, 9, /*0110 0111*/
    1, 2, 4, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0110 1000*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0110 1001*/
    1, 2, 6, 8, 1, 4, 6, 8, 9, 9, 9, 9, 9, /*0110 1010*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0110 1011*/
    1, 2, 5, 8, 2, 4, 5, 8, 9, 9, 9, 9, 9, /*0110 1100*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0110 1101*/
    1, 5, 6, 2, 4, 5, 6, 8, 9, 9, 9, 9, 9, /*0110 1110*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0110 1111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0111 0000*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0111 0001*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0111 0010*/
    2, 3, 7, 6, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0111 0011*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0111 0100*/
    1, 3, 7, 5, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0111 0101*/
    1, 5, 6, 2, 1, 2, 3, 8, 9, 9, 9, 9, 9, /*0111 0110*/
    1, 3, 7, 5, 2, 3, 7, 6, 9, 9, 9, 9, 9, /*0111 0111*/
    2, 3, 4, 8, 1, 3, 4, 8, 9, 9, 9, 9, 9, /*0111 1000*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0111 1001*/
    1, 3, 6, 4, 2, 3, 6, 8, 9, 9, 9, 9, 9, /*0111 1010*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0111 1011*/
    2, 3, 5, 4, 1, 3, 5, 8, 9, 9, 9, 9, 9, /*0111 1100*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*0111 1101*/
    1, 2, 3, 8, 4, 5, 6, 8, 1, 2, 6, 5, 9, /*0111 1110*/
    2, 3, 7, 6, 4, 5, 7, 6, 1, 3, 7, 5, 9, /*0111 1111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1000 0000*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1000 0001*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1000 0010*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1000 0011*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1000 0100*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1000 0101*/
    0, 5, 6, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1000 0110*/
    0, 5, 7, 8, 0, 6, 7, 8, 9, 9, 9, 9, 9, /*1000 0111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1000 1000*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1000 1001*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1000 1010*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1000 1011*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1000 1100*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1000 1101*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1000 1110*/
    4, 5, 7, 6, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1000 1111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1001 0000*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1001 0001*/
    0, 3, 6, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1001 0010*/
    0, 3, 7, 8, 0, 6, 7, 8, 9, 9, 9, 9, 9, /*1001 0011*/
    0, 3, 5, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1001 0100*/
    0, 3, 7, 8, 0, 5, 7, 8, 9, 9, 9, 9, 9, /*1001 0101*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1001 0110*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1001 0111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1001 1000*/
    0, 4, 7, 3, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1001 1001*/
    0, 3, 4, 8, 3, 4, 6, 8, 9, 9, 9, 9, 9, /*1001 1010*/
    0, 4, 7, 3, 4, 6, 7, 8, 9, 9, 9, 9, 9, /*1001 1011*/
    0, 3, 4, 8, 3, 4, 5, 8, 9, 9, 9, 9, 9, /*1001 1100*/
    0, 4, 7, 3, 4, 5, 7, 8, 9, 9, 9, 9, 9, /*1001 1101*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1001 1110*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1001 1111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1010 0000*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1010 0001*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1010 0010*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1010 0011*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1010 0100*/
    0, 2, 7, 5, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1010 0101*/
    0, 2, 5, 8, 2, 5, 6, 8, 9, 9, 9, 9, 9, /*1010 0110*/
    0, 2, 7, 5, 2, 6, 7, 8, 9, 9, 9, 9, 9, /*1010 0111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1010 1000*/
    0, 2, 7, 8, 0, 4, 7, 8, 9, 9, 9, 9, 9, /*1010 1001*/
    64,2, 6, 4, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1010 1010*/
    64,2, 6, 4, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1010 1011*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1010 1100*/
    0, 2, 7, 5, 0, 4, 5, 8, 9, 9, 9, 9, 9, /*1010 1101*/
    64,2, 6, 4, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1010 1110*/
    64,2, 6, 4, 4, 5, 7, 6, 9, 9, 9, 9, 9, /*1010 1111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1011 0000*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1011 0001*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1011 0010*/
    2, 3, 7, 6, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1011 0011*/
    0, 2, 5, 8, 2, 3, 5, 8, 9, 9, 9, 9, 9, /*1011 0100*/
    0, 2, 7, 5, 2, 3, 7, 8, 9, 9, 9, 9, 9, /*1011 0101*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1011 0110*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1011 0111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1011 1000*/
    0, 4, 7, 3, 0, 2, 3, 8, 9, 9, 9, 9, 9, /*1011 1001*/
    64,2, 6, 4, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1011 1010*/
    64,2, 6, 4, 2, 3, 7, 6, 9, 9, 9, 9, 9, /*1011 1011*/
    2, 3, 5, 4, 0, 2, 4, 8, 9, 9, 9, 9, 9, /*1011 1100*/
    0, 2, 3, 8, 4, 5, 7, 8, 0, 3, 7, 4, 9, /*1011 1101*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1011 1110*/
    64,2, 6, 4, 2, 3, 7, 6, 4, 5, 7, 6, 9, /*1011 1111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1100 0000*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1100 0001*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1100 0010*/
    0, 1, 7, 6, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1100 0011*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1100 0100*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1100 0101*/
    0, 1, 6, 8, 1, 5, 6, 8, 9, 9, 9, 9, 9, /*1100 0110*/
    0, 1, 7, 6, 1, 5, 7, 8, 9, 9, 9, 9, 9, /*1100 0111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1100 1000*/
    0, 1, 7, 8, 0, 4, 7, 8, 9, 9, 9, 9, 9, /*1100 1001*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1100 1010*/
    0, 1, 7, 6, 0, 4, 6, 8, 9, 9, 9, 9, 9, /*1100 1011*/
    32,1, 5, 4, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1100 1100*/
    32,1, 5, 4, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1100 1101*/
    32,1, 5, 4, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1100 1110*/
    32,1, 5, 4, 4, 5, 7, 6, 9, 9, 9, 9, 9, /*1100 1111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1101 0000*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1101 0001*/
    0, 1, 6, 8, 1, 3, 6, 8, 9, 9, 9, 9, 9, /*1101 0010*/
    0, 1, 7, 6, 1, 3, 7, 8, 9, 9, 9, 9, 9, /*1101 0011*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1101 0100*/
    1, 3, 7, 5, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1101 0101*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1101 0110*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1101 0111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1101 1000*/
    0, 4, 7, 3, 0, 1, 3, 8, 9, 9, 9, 9, 9, /*1101 1001*/
    1, 3, 6, 4, 0, 1, 4, 8, 9, 9, 9, 9, 9, /*1101 1010*/
    0, 1, 3, 8, 4, 6, 7, 8, 0, 3, 7, 4, 9, /*1101 1011*/
    32,1, 5, 4, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1101 1100*/
    32,1, 5, 4, 1, 3, 7, 5, 9, 9, 9, 9, 9, /*1101 1101*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1101 1110*/
    32,1, 5, 4, 1, 3, 7, 5, 4, 5, 7, 6, 9, /*1101 1111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1110 0000*/
    0, 1, 7, 8, 0, 2, 7, 8, 9, 9, 9, 9, 9, /*1110 0001*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1110 0010*/
    0, 1, 7, 6, 0, 2, 6, 8, 9, 9, 9, 9, 9, /*1110 0011*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1110 0100*/
    0, 2, 7, 5, 0, 1, 5, 8, 9, 9, 9, 9, 9, /*1110 0101*/
    1, 5, 6, 2, 0, 1, 2, 8, 9, 9, 9, 9, 9, /*1110 0110*/
    0, 1, 2, 8, 5, 6, 7, 8, 1, 2, 6, 5, 9, /*1110 0111*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1110 1000*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1110 1001*/
    64,2, 6, 4, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1110 1010*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1110 1011*/
    32,1, 5, 4, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1110 1100*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1110 1101*/
    32,1, 5, 4, 64,2, 6, 4, 9, 9, 9, 9, 9, /*1110 1110*/
    32,1, 5, 4, 64,2, 6, 4, 4, 5, 7, 6, 9, /*1110 1111*/
    16,1, 3, 2, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1111 0000*/
    16,1, 3, 2, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1111 0001*/
    16,1, 3, 2, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1111 0010*/
    16,1, 3, 2, 2, 3, 7, 6, 9, 9, 9, 9, 9, /*1111 0011*/
    16,1, 3, 2, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1111 0100*/
    16,1, 3, 2, 1, 3, 7, 5, 9, 9, 9, 9, 9, /*1111 0101*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1111 0110*/
    16,1, 3, 2, 1, 3, 7, 5, 2, 3, 7, 6, 9, /*1111 0111*/
    16,1, 3, 2, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1111 1000*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, /*1111 1001*/
    16,1, 3, 2, 64,2, 6, 4, 9, 9, 9, 9, 9, /*1111 1010*/
    16,1, 3, 2, 64,2, 6, 4, 2, 3, 7, 6, 9, /*1111 1011*/
    16,1, 3, 2, 32,1, 5, 4, 9, 9, 9, 9, 9, /*1111 1100*/
    16,1, 3, 2, 32,1, 5, 4, 1, 3, 7, 5, 9, /*1111 1101*/
    16,1, 3, 2, 32,1, 5, 4, 64,2, 6, 4, 9, /*1111 1110*/
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9  /*1111 1111*/
  };

  int offset[8];
  unsigned char *Primary;
  int x,y,z,index;
  int xo,yo,zo;
  int ypos, zpos;
  int pointNum;
  vtkMergePoints *locator;
  float vec[3], bounds[6];
  int Id[4];
  int caseVal;
  float norm[3];
  
  offset[0] = 0;
  offset[1] = 1;
  offset[2] = xdim;
  offset[3] = xdim + 1;
  offset[4] = xdim*ydim;
  offset[5] = xdim*ydim + 1;
  offset[6] = xdim*ydim + xdim;
  offset[7] = xdim*ydim + xdim + 1;

  // first allocate memory
  Primary = new unsigned char [zdim*ydim*xdim];
  memset(Primary,0,xdim*ydim*zdim);
  
  // allocate the locator
  locator = vtkMergePoints::New();
  bounds[0] = origin[0]; bounds[1] = xdim*spacing[0] + origin[0]; 
  bounds[2] = origin[1]; bounds[3] = ydim*spacing[1] + origin[1]; 
  bounds[4] = origin[2]; bounds[5] = zdim*spacing[2] + origin[2]; 
  locator->InitPointInsertion(newPts, bounds);

  // then do the threshold
  for (z = 0; z < zdim; z++)
    {
    zpos = z*ydim*xdim;
    for (y = 0; y < ydim; y++)
      {
      ypos = y*xdim;
      for (x = 0; x < xdim; x++)
	{
	// if its value is less than threshold then ignore it
	if (image[zpos + ypos + x] >= this->GradientThreshold)
	  {
	  Primary[zpos + ypos + x] = 128;
	  }
	}
      }
    }
    
  // now extract the polygons
  for (z = 0; z < (zdim-1); z++)
    {
    zpos = z*ydim*xdim;
    for (y = 0; y < (ydim-1); y++)
      {
      ypos = y*xdim;
      for (x = 0; x < (xdim-1); x++)
	{
	// compute the index
	if (Primary[zpos +ypos +x]) index = 128;
	else index = 0;
	if (Primary[zpos +ypos +x + offset[1]]) index += 64;
	if (Primary[zpos +ypos +x + offset[2]]) index += 32;
	if (Primary[zpos +ypos +x + offset[3]]) index += 16;
	if (Primary[zpos +ypos +x + offset[4]]) index +=  8;
	if (Primary[zpos +ypos +x + offset[5]]) index +=  4;
	if (Primary[zpos +ypos +x + offset[6]]) index +=  2;
	if (Primary[zpos +ypos +x + offset[7]]) index +=  1;

	index = index*13;
	while (polygonCases[index] != 9)
	  {
	  // add the points if necc
	  if ((polygonCases[index] < 16)||
	      ((polygonCases[index]/64) && (x == 0)) ||
	      (((polygonCases[index]/32)%2) && (y == 0)) ||
	      (((polygonCases[index]/16)%2) && (z == 0)))
	    {
	    for (pointNum = 0; pointNum < 4; pointNum++)
	      {
	      caseVal = polygonCases[index + pointNum]%16;
	      if (caseVal < 8)
		{
		xo = x + caseVal%2;
		yo = y + (caseVal/2)%2;
		zo = z + (caseVal/4)%2;
		vec[0] = xo*spacing[0] + origin[0];
		vec[1] = yo*spacing[1] + origin[1];
		vec[2] = zo*spacing[2] + origin[2];
		if ((Id[pointNum] = locator->IsInsertedPoint(vec)) < 0)
		  {
		  Id[pointNum] = locator->InsertNextPoint(vec);
		  outScalars->InsertNextScalar(image[xo +
						    xdim*(yo + zo*ydim)]);
		  inVectors->GetVector(xo+xdim*(yo+zo*ydim),vec);
		  vtkMath::Normalize(vec);
		  outVectors->InsertNextVector(vec);
		  }
		}
	      }
	    // add the polygon
	    vtkTriangle::ComputeNormal(newPts->GetPoint(Id[0]),
				       newPts->GetPoint(Id[1]), 
				       newPts->GetPoint(Id[2]),norm);
	    inVectors->GetVector(xo+xdim*(yo+zo*ydim),vec);
	    if (vtkMath::Dot(norm,vec) > 0)
	      {
	      if (polygonCases[index + 3] < 8)
		{
		newLines->InsertNextCell(4);
		newLines->InsertCellPoint(Id[0]);
		newLines->InsertCellPoint(Id[1]);
		newLines->InsertCellPoint(Id[2]);
		newLines->InsertCellPoint(Id[3]);
		}
	      else
		{
		newLines->InsertNextCell(3);
		newLines->InsertCellPoint(Id[0]);
		newLines->InsertCellPoint(Id[1]);
		newLines->InsertCellPoint(Id[2]);
		}
	      }
	    else
	      {
	      if (polygonCases[index + 3] < 8)
		{
		newLines->InsertNextCell(4);
		newLines->InsertCellPoint(Id[3]);
		newLines->InsertCellPoint(Id[2]);
		newLines->InsertCellPoint(Id[1]);
		newLines->InsertCellPoint(Id[0]);
		}
	      else
		{
		newLines->InsertNextCell(3);
		newLines->InsertCellPoint(Id[2]);
		newLines->InsertCellPoint(Id[1]);
		newLines->InsertCellPoint(Id[0]);
		}
	      }
	    }
	  index += 4;
	  }
	
	} // end of for x loop
      } // end of for y loop
    } // end of for z loop
  
  // free memory
  delete [] Primary;
  locator->Delete();
}

void vtkLinkSurfels::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "GradientThreshold:" << this->GradientThreshold << "\n";
}

