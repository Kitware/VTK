/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageInformation.h
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
// .NAME vtkImageInformation - Image specific info (like spacing).
// .SECTION Description
// Note:  This object is under development an might change in the future.


#ifndef __vtkImageInformation_h
#define __vtkImageInformation_h

#include "vtkStructuredInformation.h"


class VTK_EXPORT vtkImageInformation : public vtkStructuredInformation
{
public:
  static vtkImageInformation *New();

  vtkTypeMacro(vtkImageInformation,vtkStructuredInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Makes an empty similar type object.
  vtkDataInformation *MakeObject() 
    {return vtkImageInformation::New();}
  
  // Description:
  // Subclasses override this method, and try to be smart
  // if the types are different.
  void Copy(vtkDataInformation *in);
  
  // Description:
  // Set the spacing (width,height,length) of the cubical cells that
  // compose the data set.
  vtkSetVector3Macro(Spacing,float);
  vtkGetVectorMacro(Spacing,float,3);
  
  // Description:
  // Set the origin of the data. The origin plus spacing determine the
  // position in space of the points.
  vtkSetVector3Macro(Origin,float);
  vtkGetVectorMacro(Origin,float,3);
  
  // Description:
  // Set/Get the data scalar type (i.e VTK_FLOAT).
  vtkSetMacro(ScalarType, int);
  vtkGetMacro(ScalarType, int);

  // Description:
  // Set/Get the number of scalar components for points.
  vtkSetMacro(NumberOfScalarComponents,int);
  vtkGetMacro(NumberOfScalarComponents,int);
  
  // Description:
  // This method is passed a ClassName and returns 1 if the object is
  // a subclass of the class arg.  It is an attempt at making a smarter copy.
  int GetClassCheck(char *className);
  
  // Description:
  // Serialization provided for the multi-process ports.
  void ReadSelf(istream& is);
  void WriteSelf(ostream& os);

protected:
  vtkImageInformation();
  ~vtkImageInformation() {};
  vtkImageInformation(vtkImageInformation&) {};
  void operator=(vtkImageInformation&) {};

  float Origin[3];
  float Spacing[3];
  int ScalarType;
  int NumberOfScalarComponents;
};


#endif




