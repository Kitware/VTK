/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAnnotate.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include <math.h>
#include "vtkImageAnnotate.h"

//----------------------------------------------------------------------------
vtkImageAnnotate::vtkImageAnnotate()
{
}


//----------------------------------------------------------------------------
vtkImageAnnotate::~vtkImageAnnotate()
{
}


//----------------------------------------------------------------------------
void vtkImageAnnotate::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImagePaint::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
// Draw the bounds of a region
template <class T>
void vtkImageAnnotateBounds(vtkImageAnnotate *self, T *ptr)
{
  int temp;
  T *ptr0, *ptr1;
  int idx0, idx1;
  int inc0, inc1;
  int min0, max0, min1, max1;
  // g0=(1,0), g1=(0,1), g01=(1,1), g10=(1,-1)
  int g0Min, g0Max, g1Min, g1Max, g01Min, g01Max, g10Min, g10Max;
  

  
  self->GetIncrements(inc0, inc1);
  self->GetExtent(min0, max0, min1, max1);
  // Initialize min max for each direction
  g0Min = max0; g0Max = min0;
  g1Min = max1; g1Max = min1;
  g01Min = max0+max1; g01Max = min0+min1;
  g10Min = max0-min1; g10Max = min0-max1;
  
  ptr1 = ptr;
  for (idx1 = min1; idx1 <= max1; ++idx1)
    {
    ptr0 = ptr1;
    for (idx0 = min0; idx0 <= max0; ++idx0)
      {
      if (*ptr0)
	{
	temp = idx0;
	g0Min = (g0Min > temp) ? temp : g0Min;
	g0Max = (g0Max < temp) ? temp : g0Max;
	temp = idx1;
	g1Min = (g1Min > temp) ? temp : g1Min;
	g1Max = (g1Max < temp) ? temp : g1Max;
	temp = idx0+idx1;
	g01Min = (g01Min > temp) ? temp : g01Min;
	g01Max = (g01Max < temp) ? temp : g01Max;
	temp = idx0-idx1;
	g10Min = (g10Min > temp) ? temp : g10Min;
	g10Max = (g10Max < temp) ? temp : g10Max;
	}
      
      ptr0 += inc0;
      }
    ptr1 += inc1;
    }

  g01Min = g01Min / 2;
  g01Max = g01Max / 2;
  g10Min = g10Min / 2;
  g10Max = g10Max / 2;

  self->SetMin0(g0Min);
  self->SetMax0(g0Max);
  self->SetMin1(g1Min);
  self->SetMax1(g1Max);
  self->SetMin01(g01Min);
  self->SetMax01(g01Max);
  self->SetMin10(g10Min);
  self->SetMax10(g10Max);

  // Average the center of the two bounding boxes.
  temp = ((g0Min + g0Max) + (g01Min + g01Max) + (g10Min + g10Max)) / 4;
  self->SetCenter0(temp);
  temp = ((g1Min + g1Max) + (g01Min + g01Max) - (g10Min + g10Max)) / 4;
  self->SetCenter1(temp);
}

//----------------------------------------------------------------------------
void vtkImageAnnotate::ComputeBounds()
{
  void *ptr;
  int ka, kb;
  int g0Min, g0Max, g1Min, g1Max, g01Min, g01Max, g10Min, g10Max;
  
  ptr = this->GetScalarPointer();
  switch (this->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageAnnotateBounds(this, (float *)(ptr));
      break;
    case VTK_INT:
      vtkImageAnnotateBounds(this, (int *)(ptr));
      break;
    case VTK_SHORT:
      vtkImageAnnotateBounds(this, (short *)(ptr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageAnnotateBounds(this, (unsigned short *)(ptr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageAnnotateBounds(this, (unsigned char *)(ptr));
      break;
    default:
      vtkErrorMacro(<< "Draw: Cannot handle ScalarType.");
      return;
    }   

  g0Min = this->GetMin0();
  g0Max = this->GetMax0();
  g1Min = this->GetMin1();
  g1Max = this->GetMax1();
  
  g01Min = this->GetMin01();
  g01Max = this->GetMax01();
  g10Min = this->GetMin10();
  g10Max = this->GetMax10();

  
  cerr << "g01Min: " << g01Min << ", g01Max: " << g01Max << "\n";
  
  // Draw the gem.
  this->SetDrawColor(255);
  // g0Min
  ka = g1Min;
  kb = g1Max;
  //this->DrawSegment(g0Min, ka, g0Min, kb);
  // g0Max
  ka = g1Min;
  kb = g1Max;
  //this->DrawSegment(g0Max, ka, g0Max, kb);
  // g1Min
  ka = g0Min;
  kb = g0Max;
  //this->DrawSegment(ka, g1Min, kb, g1Min);
  // g1Max
  ka = g0Min;
  kb = g0Max;
  //this->DrawSegment(ka, g1Max, kb, g1Max);
  
  // g01Min
  ka = g10Min;
  kb = g10Max;
  //this->DrawSegment(g01Min+ka, g01Min-ka, g01Min+kb, g01Min-kb);
  // g01Max
  ka = g10Min;
  kb = g10Max;
  //this->DrawSegment(g01Max+ka, g01Max-ka, g01Max+kb, g01Max-kb);
  
  // g10Min
  ka = g01Min;
  kb = g01Max;
  //this->DrawSegment(g10Min+ka, -g10Min+ka, g10Min+kb, -g10Min+kb);
  // g10Max
  ka = g01Min;
  kb = g01Max;
  //this->DrawSegment(g10Max+ka, -g10Max+ka, g10Max+kb, -g10Max+kb);

  this->DrawPoint(this->Center0, this->Center1);
}



//----------------------------------------------------------------------------
// Draw the bounds of a region
template <class T>
void vtkImageAnnotateFunction(vtkImageAnnotate *self, T *ptr, int partIdx)
{
  T *ptr0, *ptr1;
  int idx0, idx1;
  int inc0, inc1;
  int min0, max0, min1, max1;
  // g0=(1,0), g1=(0,1), g01=(1,1), g10=(1,-1)
  int g0Min, g0Max, g1Min, g1Max, g01Min, g01Max, g10Min, g10Max;
  // For finding the best bound.
  int bestSplit, bestIdx0, bestIdx1;
  int annotationIdx0, annotationIdx1;
  float bestDistance, d, d0, d1;

  g0Min = self->GetMin0();
  g0Max = self->GetMax0();
  g1Min = self->GetMin1();
  g1Max = self->GetMax1();
  g01Min = self->GetMin01();
  g01Max = self->GetMax01();
  g10Min = self->GetMin10();
  g10Max = self->GetMax10();

  bestSplit = -1;
  // Pick a big distance as initial value.
  bestDistance = 2.0 * ((g0Max-g0Min) + (g0Max-g0Min) + 
			(g01Max-g01Min) + (g10Max-g10Min));
  
  self->GetIncrements(inc0, inc1);
  self->GetExtent(min0, max0, min1, max1);
  
  ptr1 = ptr;
  for (idx1 = min1; idx1 <= max1; ++idx1)
    {
    ptr0 = ptr1;
    for (idx0 = min0; idx0 <= max0; ++idx0)
      {
      if ((int)(*ptr0) == partIdx)
	{
	// Compute the nearst bounding line.
	d = idx0 - g0Min;
	if (d < bestDistance)
	  {
	  bestDistance = d;
	  bestSplit = 0;
	  bestIdx0 = idx0;   bestIdx1 = idx1;
	  }
	d = g0Max - idx0;
	if (d < bestDistance)
	  {
	  bestDistance = d;
	  bestSplit = 1;
	  bestIdx0 = idx0;   bestIdx1 = idx1;
	  }

	d = idx1 - g1Min;
	if (d < bestDistance)
	  {
	  bestDistance = d;
	  bestSplit = 2;
	  bestIdx0 = idx0;   bestIdx1 = idx1;
	  }
	d = g1Max - idx1;
	if (d < bestDistance)
	  {
	  bestDistance = d;
	  bestSplit = 3;
	  bestIdx0 = idx0;   bestIdx1 = idx1;
	  }

	d = ((float)(idx0+idx1)*0.5 - g01Min) * 0.707;
	if (d < bestDistance)
	  {
	  cerr << "(" << idx0 << ", " << idx1 << "), d = " << d 
	       << ", g01Min = " << g01Min << "\n";
	  bestDistance = d;
	  bestSplit = 4;
	  bestIdx0 = idx0;   bestIdx1 = idx1;
	  }
	d = (g01Max - (float)(idx0+idx1)*0.5) * 0.707;
	if (d < bestDistance)
	  {
	  bestDistance = d;
	  bestSplit = 5;
	  bestIdx0 = idx0;   bestIdx1 = idx1;
	  }
	
	d = ((float)(idx0-idx1)*0.5 - g10Min) * 0.707;
	if (d < bestDistance)
	  {
	  bestDistance = d;
	  bestSplit = 6;
	  bestIdx0 = idx0;   bestIdx1 = idx1;
	  }
	d = (g10Max - (float)(idx0-idx1)*0.5) * 0.707;
	if (d < bestDistance)
	  {
	  bestDistance = d;
	  bestSplit = 7;
	  bestIdx0 = idx0;   bestIdx1 = idx1;
	  }
	
	}
      
      ptr0 += inc0;
      }
    ptr1 += inc1;
    }
  cerr << "g01Min: " << g01Min << ", g01Max: " << g01Max << "\n";

  cerr << "best Distance: " << bestDistance << ", bestSplit: " 
       << bestSplit << ", best point (" << bestIdx0 << ", " 
       << bestIdx1 << ")\n";
  
  
  // Now we know the closest point to the bounds.
  // Find the annotation location.
  /*
  switch (bestSplit)
    {
    case 0:
      annotationIdx0 = bestIdx0 - (int)(bestDistance) - 80;
      annotationIdx1 = bestIdx1;
      break;
    case 1:
      annotationIdx0 = bestIdx0 + (int)(bestDistance) + 80;
      annotationIdx1 = bestIdx1;
      break;
    case 2:
      annotationIdx0 = bestIdx0;
      annotationIdx1 = bestIdx1 - (int)(bestDistance) - 80;
      break;
    case 3:
      annotationIdx0 = bestIdx0;
      annotationIdx1 = bestIdx1 + (int)(bestDistance) + 80;
      break;
    case 4:
      annotationIdx0 = bestIdx0 - (int)(bestDistance) - 80;
      annotationIdx1 = bestIdx1 - (int)(bestDistance) - 80;
      break;
    case 5:
      annotationIdx0 = bestIdx0 + (int)(bestDistance) + 80;
      annotationIdx1 = bestIdx1 + (int)(bestDistance) + 80;
      break;
    case 6:
      annotationIdx0 = bestIdx0 - (int)(bestDistance) - 80;
      annotationIdx1 = bestIdx1 + (int)(bestDistance) + 80;
      break;
    case 7:
      annotationIdx0 = bestIdx0 + (int)(bestDistance) + 80;
      annotationIdx1 = bestIdx1 - (int)(bestDistance) - 80;
      break;
    }
    */
  
  // Compute leader lines radiating from center
  d0 = bestIdx0 - self->GetCenter0();
  d1 = bestIdx1 - self->GetCenter1();
  // Normalize
  d = sqrt(d0*d0 + d1*d1);
  d0 = d0 / d;
  d1 = d1 / d;
  // Make a small gap between leader line and object
  bestIdx0 += (int)(4.0 * d0);
  bestIdx1 += (int)(4.0 * d1);
  // compute the annotation position
  annotationIdx0 = bestIdx0 + (int)(d0 * (bestDistance + 80.0));
  annotationIdx1 = bestIdx1 + (int)(d1 * (bestDistance + 80.0));
  
  // Draw a line showing the annotation.
  self->SetDrawColor(255);
  self->DrawSegment(bestIdx0, bestIdx1, annotationIdx0, annotationIdx1);
}

//----------------------------------------------------------------------------
void vtkImageAnnotate::Annotate(int partIdx)
{
  void *ptr;
  
  ptr = this->GetScalarPointer();
  switch (this->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageAnnotateFunction(this, (float *)(ptr), partIdx);
      break;
    case VTK_INT:
      vtkImageAnnotateFunction(this, (int *)(ptr), partIdx);
      break;
    case VTK_SHORT:
      vtkImageAnnotateFunction(this, (short *)(ptr), partIdx);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageAnnotateFunction(this, (unsigned short *)(ptr), partIdx);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageAnnotateFunction(this, (unsigned char *)(ptr), partIdx);
      break;
    default:
      vtkErrorMacro(<< "Draw: Cannot handle ScalarType.");
      return;
    }   

  
}



