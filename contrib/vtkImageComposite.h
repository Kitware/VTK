/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageComposite.h
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
// .NAME vtkImageComposite - Composites multiple images.
// .SECTION Description
// vtkImageComposite Takes a number of inputs of structured points with
// pixel data and zbuffer data, and composites them into one.  The pixel
// data should be stored in point scalars, and the z buffer data should be 
// stored in a point field called ZBuffer.  This is the format produced by 
// vtkRendererSource.  

// .SECTION Notes
// Although this fitlter processes structured points, future plans are to
// have it produce vtkImageData and have it render select pieces of the 
// image. Also, this filter ignores alpha (for now).

// .SECTION See Also
// vtkRendererSource

#ifndef __vtkImageComposite_h
#define __vtkImageComposite_h

#include "vtkStructuredPointsToStructuredPointsFilter.h"

class VTK_EXPORT vtkImageComposite : public vtkStructuredPointsToStructuredPointsFilter
{
public:
  static vtkImageComposite *New();

  const char *GetClassName() {return "vtkImageComposite";}
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a dataset to the list of data to append.
  void AddInput(vtkImageData *);

  // Description:
  // Remove a dataset from the list of data to append.
  void RemoveInput(vtkImageData *);

  // Description:
  // Get any input of this filter.
  vtkImageData *GetInput(int idx);

protected:
  vtkImageComposite();
  ~vtkImageComposite();
  vtkImageComposite(const vtkImageComposite&) {};
  void operator=(const vtkImageComposite&) {};

  // Usual data generation method
  void Execute();
  void ComputeInputUpdateExtents(vtkDataObject *data);
};

#endif


