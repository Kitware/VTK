/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridClip.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStructuredGridClip - Reduces the image extent of the input.
// .SECTION Description
// vtkStructuredGridClip  will make an image smaller.  The output must have
// an image extent which is the subset of the input.  The filter has two 
// modes of operation: 
// 1: By default, the data is not copied in this filter. 
// Only the whole extent is modified.  
// 2: If ClipDataOn is set, then you will get no more that the clipped
// extent.
#ifndef __vtkStructuredGridClip_h
#define __vtkStructuredGridClip_h

// I did not make this a subclass of in place filter because
// the references on the data do not matter. I make no modifications
// to the data.
#include "vtkStructuredGridAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkStructuredGridClip : public vtkStructuredGridAlgorithm
{
public:
  static vtkStructuredGridClip *New();
  vtkTypeMacro(vtkStructuredGridClip,vtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The whole extent of the output has to be set explicitly.
  void SetOutputWholeExtent(int extent[6], vtkInformation *outInfo=0);
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
  vtkStructuredGridClip();
  ~vtkStructuredGridClip() {};

  // Time when OutputImageExtent was computed.
  vtkTimeStamp CTime;
  int Initialized; // Set the OutputImageExtent for the first time.
  int OutputWholeExtent[6];

  int ClipData;
  
  virtual int RequestInformation (vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *);

  void CopyData(vtkStructuredGrid *inData, vtkStructuredGrid *outData, int *ext);

  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *);

private:
  vtkStructuredGridClip(const vtkStructuredGridClip&);  // Not implemented.
  void operator=(const vtkStructuredGridClip&);  // Not implemented.
};



#endif



