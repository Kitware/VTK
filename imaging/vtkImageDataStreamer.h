/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataStreamer.h
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
// .NAME vtkImageDataStreamer - Initiates streaming on image data.
// .SECTION Description
// To statisfy a request, this filter calls update on its input
// many times with smaller update extents.  All processing up stream
// streams smaller pieces.

#ifndef __vtkImageDataStreamer_h
#define __vtkImageDataStreamer_h

#include "vtkImageSource.h"

class VTK_EXPORT vtkImageDataStreamer : public vtkImageSource
{
public:
  static vtkImageDataStreamer *New();
  const char *GetClassName() {return "vtkImageDataStreamer";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the Input of a filter. 
  void SetInput(vtkImageData *input);
  vtkImageData *GetInput();
  
  // Description:
  // Pipeline calls made by vtkImageData.
  void PreUpdate(vtkDataObject *data);
  void InternalUpdate(vtkDataObject *data);

  // Description:
  // Until Lisa finishes, we will use a very brute force method
  // of streaming (determining how many pieces to use).
  vtkSetMacro(NumberOfDivisions, int);
  vtkGetMacro(NumberOfDivisions, int);
  
protected:
  vtkImageDataStreamer();
  ~vtkImageDataStreamer() {};
  vtkImageDataStreamer(const vtkImageDataStreamer&) {};
  void operator=(const vtkImageDataStreamer&) {};

  int NumberOfDivisions;

  int SplitExtent(int piece, int numPieces, int *ext);
};




#endif



