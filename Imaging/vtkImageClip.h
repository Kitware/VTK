/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageClip.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageClip - Reduces the image extent of the input.
// .SECTION Description
// vtkImageClip  will make an image smaller.  The output must have
// an image extent which is the subset of the input.  The filter has two 
// modes of operation: 
// 1: By default, the data is not copied in this filter. 
// Only the whole extent is modified.  
// 2: If ClipDataOn is set, then you will get no more that the clipped
// extent.
#ifndef __vtkImageClip_h
#define __vtkImageClip_h

// I did not make this a subclass of in place filter because
// the references on the data do not matter. I make no modifications
// to the data.
#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageClip : public vtkImageToImageFilter
{
public:
  static vtkImageClip *New();
  vtkTypeRevisionMacro(vtkImageClip,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The whole extent of the output has to be set explicitly.
  void SetOutputWholeExtent(int extent[6]);
  void SetOutputWholeExtent(int minX, int maxX, int minY, int maxY, 
                            int minZ, int maxZ);
  void GetOutputWholeExtent(int extent[6]);
  int *GetOutputWholeExtent() {return this->OutputWholeExtent;}

  void ResetOutputWholeExtent();

  // Description:
  // By default, ClipData is off, and only the WholeExtent is modified.
  // the data's extent may actually be larger.  When this flag is on,
  // the data extent will be no more than the OutputWholeExtent.
  vtkSetMacro(ClipData, int);
  vtkGetMacro(ClipData, int);
  vtkBooleanMacro(ClipData, int);

  // Description:
  // Hack set output by piece
  void SetOutputWholeExtent(int piece, int numPieces);

protected:
  vtkImageClip();
  ~vtkImageClip() {};

  // Time when OutputImageExtent was computed.
  vtkTimeStamp CTime;
  int Initialized; // Set the OutputImageExtent for the first time.
  int OutputWholeExtent[6];

  int ClipData;
  
  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
  void CopyData(vtkImageData *inData, vtkImageData *outData, int *ext);

  int SplitExtentTmp(int piece, int numPieces, int *ext);

  virtual void ExecuteData(vtkDataObject *out);
private:
  vtkImageClip(const vtkImageClip&);  // Not implemented.
  void operator=(const vtkImageClip&);  // Not implemented.
};



#endif



