/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMedianFilter.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
// .NAME vtkImageMedianFilter - Median Filter
// .SECTION Description
// vtkImageMedianFilter a Median filter that replaces each pixel with the 
// median value from a square neighborhood around that pixel.


#ifndef __vtkImageMedianFilter_h
#define __vtkImageMedianFilter_h


#include "vtkImageFilter.hh"

class vtkImageMedianFilter : public vtkImageFilter
{
public:
  vtkImageMedianFilter();
  char *GetClassName() {return "vtkImageMedianFilter";};
  void GetBoundary(int *offset, int *size);
  
  void SetRadius(int rad0, int rad1, int rad2);
  // Description:
  // Get the convolution axis.
  vtkGetVector3Macro(Radius,int);

protected:
  int Radius[3];

  // stuff for sorting the pixels
  int NumNeighborhood;
  float *Sort;
  float *Median;
  int UpMax;
  int DownMax;
  int UpNum;
  int DownNum;

  void RequiredRegion(int *outOffset, int *outSize, 
		      int *inOffset, int *inSize);
  void Execute(vtkImageRegion *inTile, vtkImageRegion *outTile);
  float NeighborhoodMedian(float *inPtr, int inc0, int inc1, int inc2);
  void ClearMedian();
  void AccumulateMedian(float val);
};

#endif



