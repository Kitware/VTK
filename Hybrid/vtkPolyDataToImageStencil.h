/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataToImageStencil.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPolyDataToImageStencil - clip an image with polydata
// .SECTION Description
// vtkPolyDataToImageStencil will convert a vtkPolyData into an image
// that can be used with vtkImageStencil or other vtk classes that apply
// a stencil to an image.
// .SECTION see also
// vtkPolyData vtkImageStencil vtkImplicitFunctionToImageStencil

#ifndef __vtkPolyDataToImageStencil_h
#define __vtkPolyDataToImageStencil_h


#include "vtkImageStencilSource.h"

class vtkPolyData;
class vtkOBBTree;
class vtkImageData;

class VTK_HYBRID_EXPORT vtkPolyDataToImageStencil : public vtkImageStencilSource
{
public:
  static vtkPolyDataToImageStencil *New();
  vtkTypeRevisionMacro(vtkPolyDataToImageStencil, vtkImageStencilSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the polydata to convert into a stencil.
  virtual void SetInput(vtkPolyData *input);
  virtual vtkPolyData *GetInput();

  // Description:
  // Set a vtkImageData that has the Spacing, Origin, and
  // WholeExtent that will be used for the stencil.  This
  // input should be set to the image that you wish to
  // apply the stencil to.  If you use this method, then
  // any values set with the SetOutputSpacing, SetOutputOrigin,
  // and SetOutputWholeExtent methods will be ignored.
  virtual void SetInformationInput(vtkImageData*);
  vtkGetObjectMacro(InformationInput, vtkImageData);

  // Description:
  // Set the Origin to be used for the stencil.  It should be
  // set to the Origin of the image you intend to apply the
  // stencil to. The default value is (0,0,0).
  vtkSetVector3Macro(OutputOrigin, double);
  vtkGetVector3Macro(OutputOrigin, double);

  // Description:
  // Set the Spacing to be used for the stencil. It should be
  // set to the Spacing of the image you intend to apply the
  // stencil to. The default value is (1,1,1)
  vtkSetVector3Macro(OutputSpacing, double);
  vtkGetVector3Macro(OutputSpacing, double);

  // Description:
  // Set the whole extent for the stencil (anything outside
  // this extent will be considered to be "outside" the stencil).
  // If this is not set, then the stencil will always use
  // the requested UpdateExtent as the stencil extent.
  vtkSetVector6Macro(OutputWholeExtent, int);
  vtkGetVector6Macro(OutputWholeExtent, int);  

  // Description:
  // Set the tolerance for doing spatial searches of the polydata.
  vtkSetMacro(Tolerance, double);
  vtkGetMacro(Tolerance, double);

protected:
  vtkPolyDataToImageStencil();
  ~vtkPolyDataToImageStencil();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *);

  double Tolerance;
  vtkOBBTree *OBBTree;

  // Description:
  // Set in subclasses where the primary input is not a vtkImageData.
  vtkImageData *InformationInput;

  // Description:
  // Set in subclasses where the primary input is not a vtkImageData.
  int OutputWholeExtent[6];
  double OutputOrigin[3];
  double OutputSpacing[3];

  virtual void ReportReferences(vtkGarbageCollector*);
  virtual int FillInputPortInformation(int, vtkInformation*);
private:
  vtkPolyDataToImageStencil(const vtkPolyDataToImageStencil&);  // Not implemented.
  void operator=(const vtkPolyDataToImageStencil&);  // Not implemented.
};

#endif
