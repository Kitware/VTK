/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPoints.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
lorscThe following terms apply to all files associated with the software unless
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
// .NAME vtkStructuredPoints - A subclass of ImageData.
// .SECTION Description
// StructuredPoints is a subclass of ImageData that requires the data extent
// to exactly match the update extent. Normall image data allows that the
// data extent may be larger than the update extent.
// StructuredPoints also defines the origin differently that vtkImageData.
// For structured points the origin is the location of first point. 
// Whereas images define the origin as the location of point 0, 0, 0.
// Image Origin is stored in ivar, and structured points
// have special methods for setting/getting the origin/extents.


#ifndef __vtkStructuredPoints_h
#define __vtkStructuredPoints_h

#include "vtkImageData.h"

  
class VTK_EXPORT vtkStructuredPoints : public vtkImageData
{
public:
  vtkStructuredPoints();
  static vtkStructuredPoints *New() {return new vtkStructuredPoints;}
  const char *GetClassName() {return "vtkStructuredPoints";}
  
  // Description:
  // Create a similar type object
  vtkDataObject *MakeObject() {return new vtkStructuredPoints;}

  // Description:
  // Return what type of dataset this is.
  int GetDataObjectType() {return VTK_STRUCTURED_POINTS;}

  // Description:
  // Since vtkStructuredPointsToImage, put some translation
  // features in here.  UpdateInformation updates the source
  // and the sets WholeExtent and ScalarType variables.
  void UpdateInformation();
  
  
protected:
};

#endif



