/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageResample.cxx
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
#include "vtkImageResample.h"


// Macro for trilinear interpolation - do four linear interpolations on
// edges, two linear interpolations between pairs of edges, then a final
// interpolation between faces
#define vtkTrilinFuncMacro(v,x,y,z,a,b,c,d,e,f,g,h)         \
        t00 =   a + (x)*(b-a);      \
        t01 =   c + (x)*(d-c);      \
        t10 =   e + (x)*(f-e);      \
        t11 =   g + (x)*(h-g);      \
        t0  = t00 + (y)*(t01-t00);  \
        t1  = t10 + (y)*(t11-t10);  \
        v   = (T)( t0 + (z)*(t1-t0));

#define vtkBilinFuncMacro(v,x,y,a,b,c,d)         \
        t00 =   a + (x)*(b-a);      \
        t01 =   c + (x)*(d-c);      \
        v  = (T)(t00 + (y)*(t01-t00)); 

//----------------------------------------------------------------------------
// Constructor: Sets default filter to be identity.
vtkImageResample::vtkImageResample()
{
  this->MagnificationFactors[0] = 1.0;
  this->MagnificationFactors[1] = 1.0;
  this->MagnificationFactors[2] = 1.0;
  this->OutputSpacing[0] = 0.0; // not specified
  this->OutputSpacing[1] = 0.0; // not specified
  this->OutputSpacing[2] = 0.0; // not specified
  this->Interpolate = 1;
  this->Dimensionality = 3;
}

//----------------------------------------------------------------------------
void vtkImageResample::SetAxisOutputSpacing(int axis, float spacing)
{
  if (axis < 0 || axis > 2)
    {
    vtkErrorMacro("Bad axis: " << axis);
    return;
    }
  
  if (this->OutputSpacing[axis] != spacing)
    {
    this->OutputSpacing[axis] = spacing;
    this->Modified();
    if (spacing != 0.0)
      {
      // Delay computing the magnification factor.
      // Input might not be set yet.
      this->MagnificationFactors[axis] = 0.0; // Not computed yet.
      }
    }
}

//----------------------------------------------------------------------------
void vtkImageResample::SetAxisMagnificationFactor(int axis, float factor)
{
  if (axis < 0 || axis > 2)
    {
    vtkErrorMacro("Bad axis: " << axis);
    return;
    }
  
  if (this->MagnificationFactors[axis] == factor)
    {
    return;
    }
  this->Modified();
  this->MagnificationFactors[axis] = factor;
  // Spacing is no longer valid.
  this->OutputSpacing[axis] = 0.0; // Not computed yet.
}

//----------------------------------------------------------------------------
float vtkImageResample::GetAxisMagnificationFactor(int axis)
{
  if (axis < 0 || axis > 2)
    {
    vtkErrorMacro("Bad axis: " << axis);
    return 0.0;
    }
  
  if (this->MagnificationFactors[axis] == 0.0)
    {
    float *inputSpacing;
    if ( ! this->GetInput())
      {
      vtkErrorMacro("GetMagnificationFactor: Input not set.");
      return 0.0;
      }
    this->GetInput()->UpdateInformation();
    inputSpacing = this->GetInput()->GetSpacing();
    this->MagnificationFactors[axis] = 
      inputSpacing[axis] / this->OutputSpacing[axis];
    
    }

  vtkDebugMacro("Returning magnification factor " 
		<<  this->MagnificationFactors[axis] << " for axis "
		<< axis);
  
  return this->MagnificationFactors[axis];
}




//----------------------------------------------------------------------------
// This method computes the Region of input necessary to generate outRegion.
// It assumes offset and size are multiples of Magnify Factors.
void vtkImageResample::ComputeRequiredInputUpdateExtent(int inExt[6], 
                                                        int outExt[6])
{
  int min, max, axis;
  float factor;

  memcpy(inExt, outExt, 6 * sizeof(int));

  for (axis = 0; axis < 3; axis++)
    {
    factor = this->GetAxisMagnificationFactor(axis);
    
    vtkDebugMacro("ComputeInputUpdateExtent (axis " << axis 
                  << ") factor " << factor);    
    
    min = outExt[axis*2];
    max = outExt[axis*2+1];
    
    min = (int)(floor((float)(min) / factor));
    max = (int)(ceil((float)(max) / factor));
    
    inExt[axis*2] = min;
    inExt[axis*2+1] = max;
    }
}


//----------------------------------------------------------------------------
// Computes any global image information associated with regions.
void vtkImageResample::ExecuteInformation(vtkImageData *inData, 
					  vtkImageData *outData) 
{
  int wholeMin, wholeMax, axis, ext[6];
  float spacing[3], factor;

  inData->GetWholeExtent(ext);
  inData->GetSpacing(spacing);
  
  for (axis = 0; axis < 3; axis++)
    {
    wholeMin = ext[axis*2];
    wholeMax = ext[axis*2+1];
    
    // Scale the output extent
    factor = this->GetAxisMagnificationFactor(axis);
    wholeMin = (int)(ceil((float)(wholeMin) * factor));
    wholeMax = (int)(floor((float)(wholeMax) * factor));
    
    // Change the data spacing
    spacing[axis] /= factor;
    
    ext[axis*2] = wholeMin;
    ext[axis*2+1] = wholeMax;
    
    // just in case  the input spacing has changed.
    if (this->OutputSpacing[axis] != 0.0)
      {
      // Cause MagnificationFactor to recompute.
      this->MagnificationFactors[axis] = 0.0;
      }
    }

  outData->SetWholeExtent(ext);
  outData->SetSpacing(spacing);
  
}


//----------------------------------------------------------------------------
template <class T>
static void vtkImageResampleExecuteNI(vtkImageResample *self,
                                      vtkImageData *inData, 
                                      T *inPtr, int inExt[6],
                                      vtkImageData *outData, T *outPtr,
                                      int outExt[6], int id)
{
  float magX = self->GetAxisMagnificationFactor(0);
  float magY = self->GetAxisMagnificationFactor(1);
  float magZ = self->GetAxisMagnificationFactor(2);

  float zPos, yPos, xPos;
  int idxC, idxX, idxY, idxZ;
  int inMaxX, inMaxY, inMaxZ;
  int maxC, maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  T *inPtrZ, *inPtrY, *inPtrX, *outPtrC;
  
  // find the region to loop over
  maxC = outData->GetNumberOfScalarComponents();
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4]; 
  target = (unsigned long)(maxC*(maxZ+1)*(maxY+1)/50.0);
  target++;
  
  // Get increments to march through data 
  inData->GetIncrements(inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  inData->GetExtent(idxC, inMaxX, idxC, inMaxY, idxC, inMaxZ);
  int *wInExt = inData->GetWholeExtent();

  // find the starting sample locations
  float xStart = outExt[0]/magX;
  float yStart = outExt[2]/magY;
  float zStart = outExt[4]/magZ;
  xStart = xStart - ((int)xStart);
  yStart = yStart - ((int)yStart);
  zStart = zStart - ((int)zStart);

  float xFloatInc = 1.0/magX;
  float yFloatInc = 1.0/magY;  
  float zFloatInc = 1.0/magZ;
  
  // Loop through ouput pixels
  for (idxC = 0; idxC < maxC; idxC++)
    {
    outPtrC = outPtr + idxC;
    inPtrZ = inPtr + idxC;
    zPos = zStart;
    for (idxZ = 0; idxZ <= maxZ; idxZ++)
      {
      inPtrY = inPtrZ;
      yPos = yStart;
      for (idxY = 0; !self->AbortExecute && idxY <= maxY; idxY++)
        {
        if (!id) 
          {
          if (!(count%target))
            {
            self->UpdateProgress(count/(50.0*target));
            }
          count++;
          }
        
        inPtrX = inPtrY;
        xPos = xStart;
	for (idxX = 0; idxX <= maxX; idxX++)
	  {
          *outPtrC = (T)*inPtrX;
          outPtrC += maxC;
          xPos += xFloatInc;
          while (xPos >= 1.0 ) 
            {
            inPtrX += inIncX;
            xPos -= 1.0;
            }
          }
        outPtrC += outIncY;
        yPos += yFloatInc;
        while (yPos >= 1.0 ) 
          {
          inPtrY += inIncY;
          yPos -= 1.0;
          }
        }
      outPtrC += outIncZ;
      zPos += zFloatInc;
      while (zPos >= 1.0) 
        {
        inPtrZ += inIncZ;
        zPos -= 1.0;
        }
      }
    }
}

//----------------------------------------------------------------------------
template <class T>
static void vtkImageResampleExecute2D(vtkImageResample *self,
                                      vtkImageData *inData, 
                                      T *inPtr, int inExt[6],
                                      vtkImageData *outData, T *outPtr,
                                      int outExt[6], int id)
{
  float magX = self->GetAxisMagnificationFactor(0);
  float magY = self->GetAxisMagnificationFactor(1);

  float t00, t01;
  float yPos, xPos;
  int idxC, idxX, idxY, idxZ;
  int inMaxX, inMaxY, inMaxZ;
  int maxC, maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  int xMaxIdx, yMaxIdx;
  T *inPtrZ, *inPtrY, *inPtrX, *outPtrC;
  
  // find the region to loop over
  maxC = outData->GetNumberOfScalarComponents();
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4]; 
  target = (unsigned long)(maxC*(maxZ+1)*(maxY+1)/50.0);
  target++;
  
  // Get increments to march through data 
  inData->GetIncrements(inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  inData->GetExtent(idxC, inMaxX, idxC, inMaxY, idxC, inMaxZ);
  int *wInExt = inData->GetWholeExtent();

  // find the starting sample locations
  float xStart = outExt[0]/magX;
  float yStart = outExt[2]/magY;
  xStart = xStart - ((int)xStart);
  yStart = yStart - ((int)yStart);

  float xFloatInc = 1.0/magX;
  float yFloatInc = 1.0/magY;
  
  // compute the offsets to the other verts
  int off1 = inIncX;
  int off2 = inIncY;
  int off3 = inIncX + inIncY;
  float A,B,C,D;

  // we need to know what the last x will be without
  // going out of memory. We use a loop here so
  // that the same calculations are done. same
  // accumulated errors etc.
  xPos = xStart;
  xMaxIdx = maxX;
  int itmp = inExt[0];

  // we create these arrays to store some value we'll
  // use when incrementing along the x axis
  float *XRatios = new float [maxX+1];
  int *XSteps = new int [maxX+1];
  float *pXRatios = XRatios;
  int   *pXSteps = XSteps;
  
  for (idxX = 0; idxX <= maxX; idxX++)
    {
    xPos += xFloatInc;
    *pXSteps = 0;
    while (xPos >= 1.0 ) 
      {
      xPos -= 1.0;
      itmp++;
      *pXSteps = *pXSteps + 1;
      }
    if (itmp >= inMaxX && idxX <= xMaxIdx)
      {
      xMaxIdx = idxX - 1;
      }
    *pXRatios = xPos;
    pXRatios++;
    pXSteps++;
    }
  yPos = yStart;
  yMaxIdx = maxY;
  itmp = inExt[2];
  for (idxY = 0; idxY <= maxY; idxY++)
    {
    yPos += yFloatInc;
    while (yPos >= 1.0 ) 
      {
      yPos -= 1.0;
      itmp++;
      }
    if (itmp >= inMaxY && idxY <= yMaxIdx)
      {
      yMaxIdx = idxY - 1;
      }
    }
  
  // Loop through ouput pixels
  for (idxC = 0; idxC < maxC; idxC++)
    {
    outPtrC = outPtr + idxC;
    inPtrZ = inPtr + idxC;
    for (idxZ = 0; idxZ <= maxZ; idxZ++)
      {
      inPtrY = inPtrZ;
      yPos = yStart;
      off2 = inIncY;
      off3 = inIncX + inIncY;
      for (idxY = 0; !self->AbortExecute && idxY <= maxY; idxY++)
        {
        if (idxY > yMaxIdx)
          {
          off2 = 0;
          off3 = inIncX;
          }
        if (!id) 
          {
          if (!(count%target))
            {
            self->UpdateProgress(count/(50.0*target));
            }
          count++;
          }
        
        inPtrX = inPtrY;
        xPos = xStart;
        A = *(inPtrX);
        B = *(inPtrX + off1);
        C = *(inPtrX + off2);
        D = *(inPtrX + off3);
        pXRatios = XRatios;
        pXSteps = XSteps;
	for (idxX = 0; idxX <= xMaxIdx; idxX++)
	  {
          vtkBilinFuncMacro( *outPtrC, xPos, yPos, A, B, C, D);
          outPtrC += maxC;
          xPos = *pXRatios++;
          if (*pXSteps) 
            {
            inPtrX = inPtrX + *pXSteps*inIncX;
            A = *(inPtrX);
            B = *(inPtrX + off1);
            C = *(inPtrX + off2);
            D = *(inPtrX + off3);
            }
          pXSteps++;
          }
	for (;idxX <= maxX; idxX++)
	  {
          *outPtrC = (T)(B + (yPos)*(D-B));
          outPtrC += maxC;
          }
        outPtrC += outIncY;
        yPos += yFloatInc;
        while (yPos >= 1.0 ) 
          {
          inPtrY += inIncY;
          yPos -= 1.0;
          }
        }
      outPtrC += outIncZ;
      inPtrZ += inIncZ;
      }
    }
  delete [] XRatios;
  delete [] XSteps;
}


//----------------------------------------------------------------------------
template <class T>
static void vtkImageResampleExecute3D(vtkImageResample *self,
                                      vtkImageData *inData, T *inPtr, 
                                      int inExt[6],
                                      vtkImageData *outData, T *outPtr,
                                      int outExt[6], int id)
{
  float magZ = self->GetAxisMagnificationFactor(2);
  float magX = self->GetAxisMagnificationFactor(0);
  float magY = self->GetAxisMagnificationFactor(1);

  float t00, t01, t10, t11, t0, t1;
  float zPos, yPos, xPos;
  int idxC, idxX, idxY, idxZ;
  int inMaxX, inMaxY, inMaxZ;
  int maxC, maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  int xMaxIdx, yMaxIdx, zMaxIdx;
  T *inPtrZ, *inPtrY, *inPtrX, *outPtrC;
  
  // find the region to loop over
  maxC = outData->GetNumberOfScalarComponents();
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];

  target = (unsigned long)(maxC*(maxZ+1)*(maxY+1)/50.0);
  target++;
  
  // Get increments to march through data 
  inData->GetIncrements(inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  inData->GetExtent(idxC, inMaxX, idxC, inMaxY, idxC, inMaxZ);
  int *wInExt = inData->GetWholeExtent();

  // find the starting sample locations
  float xStart = outExt[0]/magX;
  float yStart = outExt[2]/magY;
  float zStart = outExt[4]/magZ;
  xStart = xStart - ((int)xStart);
  yStart = yStart - ((int)yStart);
  zStart = zStart - ((int)zStart);

  float xFloatInc = 1.0/magX;
  float yFloatInc = 1.0/magY;
  float zFloatInc = 1.0/magZ;

  // we create these arrays to store some value we'll
  // use when incrementing along the x axis
  float *XRatios = new float [maxX+1];
  int *XSteps = new int [maxX+1];
  float *pXRatios = XRatios;
  int   *pXSteps = XSteps;

  // we need to know what the last x will be without
  // going out of memory. We use a loop here so
  // that the same calculations are done. same
  // accumulated errors etc.
  xPos = xStart;
  xMaxIdx = maxX;
  int itmp = inExt[0];
  for (idxX = 0; idxX <= maxX; idxX++)
    {
    xPos += xFloatInc;
    *pXSteps = 0;
    while (xPos >= 1.0 ) 
      {
      xPos -= 1.0;
      itmp++;
      *pXSteps = *pXSteps + 1;
      }
    if (itmp >= inMaxX && idxX <= xMaxIdx)
      {
      xMaxIdx = idxX - 1;
      }
    *pXRatios = xPos;
    pXRatios++;
    pXSteps++;
    }

  yPos = yStart;
  yMaxIdx = maxY;
  itmp = inExt[2];
  for (idxY = 0; idxY <= maxY; idxY++)
    {
    yPos += yFloatInc;
    while (yPos >= 1.0 ) 
      {
      yPos -= 1.0;
      itmp++;
      }
    if (itmp >= inMaxY && idxY <= yMaxIdx)
      {
      yMaxIdx = idxY - 1;
      }
    }
  
  zPos = zStart;
  zMaxIdx = maxZ;
  itmp = inExt[4];
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    zPos += zFloatInc;
    while (zPos >= 1.0 ) 
      {
      zPos -= 1.0;
      itmp++;
      }
    if (itmp >= inMaxZ && idxZ <= zMaxIdx)
      {
      zMaxIdx = idxZ - 1;
      }
    }
  
  // compute the offsets to the other verts
  int off1 = inIncX;
  int off2 = inIncY;
  int off3 = inIncX + inIncY;
  int off4 = inIncZ;
  int off5 = inIncZ + inIncX;
  int off6 = inIncZ + inIncY;
  int off7 = inIncZ + inIncY + inIncX;
  float A,B,C,D,E,F,G,H;
  
  // Loop through ouput pixels
  for (idxC = 0; idxC < maxC; idxC++)
    {
    zPos = zStart;
    inPtrZ = inPtr + idxC;
    outPtrC = outPtr + idxC;
    off4 = inIncZ;
    off5 = inIncZ + inIncX;
    off6 = inIncZ + inIncY;
    off7 = inIncZ + inIncY + inIncX;
    for (idxZ = 0; idxZ <= maxZ; idxZ++)
      {
      inPtrY = inPtrZ;
      yPos = yStart;
      off2 = inIncY;
      off3 = inIncY + inIncX;
      if (idxZ > zMaxIdx)
        {
        off4 = 0;
        off5 = inIncX;
        off6 = inIncY;
        off7 = inIncY + inIncX;
        }
      for (idxY = 0; !self->AbortExecute && idxY <= maxY; idxY++)
	{
        if (idxY > yMaxIdx)
          {
          off2 = 0;
          off3 = inIncX;
          if (idxZ > zMaxIdx)
            {
            off6 = 0;
            off7 = inIncX;
            }
          else
            {
            off6 = inIncZ;
            off7 = inIncZ + inIncX;
            }
          }
	if (!id) 
	  {
	  if (!(count%target))
	    {
	    self->UpdateProgress(count/(50.0*target));
	    }
	  count++;
	  }
	
	inPtrX = inPtrY;
        xPos = xStart;
        A = *(inPtrX);
        B = *(inPtrX + off1);
        C = *(inPtrX + off2);
        D = *(inPtrX + off3);
        E = *(inPtrX + off4);
        F = *(inPtrX + off5);
        G = *(inPtrX + off6);
        H = *(inPtrX + off7);
        pXRatios = XRatios;
        pXSteps = XSteps;
	for (idxX = 0; idxX <= xMaxIdx; idxX++)
	  {
          vtkTrilinFuncMacro( *outPtrC, xPos, yPos, zPos, 
                              A, B, C, D, E, F, G, H );
          outPtrC += maxC;
          xPos = *pXRatios++;
          if (*pXSteps) 
            {
            inPtrX = inPtrX + *pXSteps*inIncX;
            A = *(inPtrX);
            B = *(inPtrX + off1);
            C = *(inPtrX + off2);
            D = *(inPtrX + off3);
            E = *(inPtrX + off4);
            F = *(inPtrX + off5);
            G = *(inPtrX + off6);
            H = *(inPtrX + off7);
            }
          pXSteps++;
          }
	for (;idxX <= maxX; idxX++)
	  {
          vtkBilinFuncMacro( *outPtrC, yPos, zPos, B, D, F, H );
          outPtrC += maxC;
          }
        outPtrC += outIncY;
        yPos += yFloatInc;
        while (yPos >= 1.0 ) 
          {
          inPtrY += inIncY;
          yPos -= 1.0;
          }
	}
      outPtrC += outIncZ;
      zPos += zFloatInc;
      while (zPos >= 1.0) 
        {
        inPtrZ += inIncZ;
        zPos -= 1.0;
        }
      }
    }
  delete [] XRatios;
  delete [] XSteps;
}

template <class T>
static void vtkImageResampleExecute(vtkImageResample *self,
				  vtkImageData *inData, T *inPtr, int inExt[6],
				  vtkImageData *outData, T *outPtr,
				  int outExt[6], int id)
{
  float magZ = self->GetAxisMagnificationFactor(2);

  if (!self->GetInterpolate())
    {
    vtkImageResampleExecuteNI(self, inData, inPtr, inExt,
                              outData, outPtr, outExt, id);
    }
  else
    {
    if (magZ == 1.0 || self->GetDimensionality() < 3)
      {
      vtkImageResampleExecute2D(self, inData, inPtr, inExt,
                                outData, outPtr, outExt, id);
      return;
      }
    else
      {
      vtkImageResampleExecute3D(self, inData, inPtr, inExt,
                                outData, outPtr, outExt, id);
      return;
      }      
    }
  
}

void vtkImageResample::ThreadedExecute(vtkImageData *inData, 
				      vtkImageData *outData,
				      int outExt[6], int id)
{
  int inExt[6];
  this->ComputeRequiredInputUpdateExtent(inExt,outExt);
  void *inPtr = inData->GetScalarPointerForExtent(inExt);
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
  << ", outData = " << outData);
  
  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()
    << ", must match out ScalarType " << outData->GetScalarType());
    return;
    }
  
  switch (inData->GetScalarType())
    {
    case VTK_DOUBLE:
      vtkImageResampleExecute(this, 
			     inData, (double *)(inPtr), inExt,
			     outData, (double *)(outPtr), outExt, id);
      break;
    case VTK_FLOAT:
      vtkImageResampleExecute(this, 
			     inData, (float *)(inPtr), inExt,
			     outData, (float *)(outPtr), outExt, id);
      break;
    case VTK_LONG:
      vtkImageResampleExecute(this, 
			     inData, (long *)(inPtr), inExt,
			     outData, (long *)(outPtr), outExt, id);
      break;
    case VTK_UNSIGNED_LONG:
      vtkImageResampleExecute(this, 
			     inData, (unsigned long *)(inPtr), inExt,
			     outData, (unsigned long *)(outPtr), outExt, id);
      break;
    case VTK_INT:
      vtkImageResampleExecute(this, 
			     inData, (int *)(inPtr), inExt,
			     outData, (int *)(outPtr), outExt, id);
      break;
    case VTK_UNSIGNED_INT:
      vtkImageResampleExecute(this, 
			     inData, (unsigned int *)(inPtr), inExt,
			     outData, (unsigned int *)(outPtr), outExt, id);
      break;
    case VTK_SHORT:
      vtkImageResampleExecute(this, 
			     inData, (short *)(inPtr), inExt,
			     outData, (short *)(outPtr), outExt, id);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageResampleExecute(this, 
			     inData, (unsigned short *)(inPtr), inExt,
			     outData, (unsigned short *)(outPtr), outExt, id);
      break;
    case VTK_CHAR:
      vtkImageResampleExecute(this, 
			     inData, (char *)(inPtr), inExt,
			     outData, (char *)(outPtr), outExt, id);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageResampleExecute(this, 
			     inData, (unsigned char *)(inPtr), inExt,
			     outData, (unsigned char *)(outPtr), outExt, id);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}

void vtkImageResample::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageToImageFilter::PrintSelf(os,indent);
  os << indent << "Dimensionality: " << this->Dimensionality << "\n";
  os << indent << "Interpolate: " << (this->Interpolate ? "On\n" : "Off\n");
}

