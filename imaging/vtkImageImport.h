/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageImport.h
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
// .NAME vtkImageImport - Feed generic memory into image pipeline.
// .SECTION Description vtkImageImport allows the user to feed any image
// into the image pipeline.



#ifndef __vtkImageImport_h
#define __vtkImageImport_h

#include "vtkImageSource.h"
#include "vtkImageRegion.h"
#include "vtkImageExport.h"


class VTK_EXPORT vtkImageImport : public vtkImageSource
{
public:
  vtkImageImport();
  ~vtkImageImport();
  static vtkImageImport *New() {return new vtkImageImport;};
  char *GetClassName() {return "vtkImageImport";};
  void PrintSelf(ostream& os, vtkIndent indent);   
  
  // Description:
  // The output is an image source that can be hooked to image filters.
  vtkImageSource *GetOutput(){return this->Region->GetOutput();};
  
  // Description:
  // Set/Get the data type of the imported memory.
  vtkSetMacro(ScalarType, int);
  vtkGetMacro(ScalarType, int);

  // Description:
  // Set/Get the data order of the imported memory.  The first axis is the
  // inner most loop.
  void SetAxes(int dim, int *axes);
  vtkImageSetMacro(Axes, int);
  void GetAxes(int dim, int *axes){this->Region->GetAxes(dim, axes);};
  vtkImageGetMacro(Axes, int);
  
  // Description:
  // Set/Get the extent of the data.
  void SetExtent(int dim, int *extent);
  vtkImageSetExtentMacro(Extent);
  void GetExtent(int dim, int *extent){this->Region->GetExtent(dim, extent);};
  vtkImageGetExtentMacro(Extent);
  
  // Description:
  // This passes the pipeline the data.  It must be called after "SetAxes",
  // "SetExtent" and "SetScalarType".
  void SetPointer(void *ptr);
  
  // Description:
  // This method is used for testing the import/export fitlers.
  // It gets the pointer from the exporter, and calls SetPointer.
  // The two filters must be setup properly before this method is called.
  void TestExport(vtkImageExport *export)
  {this->SetPointer(export->GetPointer());}

  // Description:
  // For debugging.
  vtkGetObjectMacro(Region,vtkImageRegion);
  
protected:
  vtkImageRegion *Region;
  int Axes[VTK_IMAGE_DIMENSIONS];
  int Extent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int ScalarType;
  
  // Dummy methods
  void UpdateRegion(vtkImageRegion *){};
  void UpdateImageInformation(vtkImageRegion *){};
};


#endif


