/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageQuantizeRGBToIndex.h
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
// .NAME vtkImageQuantizeRGBToIndex - generalized histograms upto 4 dimensions
// .SECTION Description
// vtkImageQuantizeRGBToIndex takes a 3 component RGB image as
// input and produces a one component index image as output, along with
// a lookup table that contains the color definitions for the index values.
// This filter works on the entire input extent - it does not perform
// streaming, and it does not supported threaded execution (because it has
// to process the entire image).
//
// To use this filter, you typically set the number of colors 
// (between 2 and 65536), execute it, and then retrieve the lookup table.
// The colors can then be using the lookup table and the image index.

#ifndef __vtkImageQuantizeRGBToIndex_h
#define __vtkImageQuantizeRGBToIndex_h

#include "vtkImageToImageFilter.h"
#include "vtkLookupTable.h"

class VTK_EXPORT vtkImageQuantizeRGBToIndex : public vtkImageToImageFilter
{
public:
  vtkImageQuantizeRGBToIndex();
  ~vtkImageQuantizeRGBToIndex();
  static vtkImageQuantizeRGBToIndex *New() {return new vtkImageQuantizeRGBToIndex;};
  const char *GetClassName() {return "vtkImageQuantizeRGBToIndex";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / Get the number of color index values to produce - must be 
  // a number between 2 and 65536.
  vtkSetClampMacro( NumberOfColors, int, 2, 65536 );
  vtkGetMacro( NumberOfColors, int );

  // Description:
  // Get the resulting lookup table that contains the color definitions
  // corresponding to the index values in the output image.
  vtkGetObjectMacro( LookupTable, vtkLookupTable );

  vtkGetMacro( InitializeExecuteTime, float );
  vtkGetMacro( BuildTreeExecuteTime, float );
  vtkGetMacro( LookupIndexExecuteTime, float );

//BTX
  // Description: 
  // For internal use only - get the type of the image
  vtkGetMacro( InputType, int );

  // Description: 
  // For internal use only - set the times for execution
  vtkSetMacro( InitializeExecuteTime, float );
  vtkSetMacro( BuildTreeExecuteTime, float );
  vtkSetMacro( LookupIndexExecuteTime, float );
//ETX

protected:
  vtkLookupTable  *LookupTable;
  int             NumberOfColors;
  int             InputType;

  float           InitializeExecuteTime;
  float           BuildTreeExecuteTime;
  float           LookupIndexExecuteTime;

  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ComputeRequiredInputUpdateExtent(int inExt[6], int outExt[6]);
  void Execute(vtkImageData *inData, vtkImageData *outData);

  // Description:
  // Generate more than requested.  Called by the superclass before
  // an execute, and before output memory is allocated.
  void ModifyOutputUpdateExtent();

};

#endif








