/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageExport.h
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
// .NAME vtkImageExport - Gets data out of the pipeline as generic memory.
// .SECTION Description
// vtkImageExport will terminate the image pipeline and return a pointer
// to memory formated to the users specifications.  The exporter will 
// put the data in memory allocated by the user is UseOwnMemory is on.


#ifndef __vtkImageExport_h
#define __vtkImageExport_h

#include "vtkObject.h"
#include "vtkImageCache.h"
#include "vtkImageRegion.h"


class VTK_EXPORT vtkImageExport : public vtkObject
{
public:
  vtkImageExport();
  ~vtkImageExport();
  static vtkImageExport *New() {return new vtkImageExport;};
  const char *GetClassName() {return "vtkImageExport";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the scalar input object from the image pipeline.
  vtkSetObjectMacro(Input,vtkImageCache);
  vtkGetObjectMacro(Input,vtkImageCache);

  // Description:
  // This method will specify the data type the memory wanted.
  void SetScalarTypeToFloat(){this->SetScalarType(VTK_FLOAT);}
  void SetScalarTypeToInt(){this->SetScalarType(VTK_INT);}
  void SetScalarTypeToShort(){this->SetScalarType(VTK_SHORT);}
  void SetScalarTypeToUnsignedShort(){this->SetScalarType(VTK_UNSIGNED_SHORT);}
  void SetScalarTypeToUnsignedChar(){this->SetScalarType(VTK_UNSIGNED_CHAR);}
  vtkSetMacro(ScalarType, int);
  vtkGetMacro(ScalarType, int);
  
  // Description:
  // Set/Get the extent to translate explicitely.
  void SetExtent(int dim, int *extent);
  vtkImageSetExtentMacro(Extent);
  void GetExtent(int dim, int *extent);
  vtkImageGetExtentMacro(Extent);

  // Description:
  // Set/Get the coordinate system which determines how extent are interpreted,
  // and the order of the axes in memory.  Inner loop is the first axis.
  void SetAxes(int dim, int *axes);
  vtkImageSetMacro(Axes,int);
  void GetAxes(int dim, int *axes);
  vtkImageGetMacro(Axes,int);
  int *GetAxes() {return this->Axes;};  

  // Description:
  // This method will execute the image pipeline and return a pointer
  // to resulting memory.  The contents of memory will not change in 
  // the future, and the user is responsible for eventually deleting
  // it.
  void *GetPointer();
  
  // Description
  // This method executes the pipeline and puts the results int the memory
  // supplied by the user.  IT IS THE USERS RESPONSIBILITY TO MAKE SURE
  // THE MEMORY IS LARGE ENOUGH TO HOLD THE EXTENT REQUESTED, AND IS OF THE
  // CORRECT DATA TYPE.
  void UpdateMemory(void *ptr);
  
  
protected:
  vtkImageCache *Input;
  int ScalarType;
  int Extent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int Axes[VTK_IMAGE_DIMENSIONS];
};


#endif


