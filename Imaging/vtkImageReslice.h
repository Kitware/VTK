/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageReslice.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageReslice - Reslices a volume along a new set of axes.
// .SECTION Description
// vtkImageReslice is the swiss-army-knife of image geometry filters:  
// It can permute, rotate, flip, scale, resample, deform, and pad image
// data in any combination with reasonably high efficiency.  Simple
// operations such as permutation, resampling and padding are done
// with similar efficiently to the specialized vtkImagePermute,
// vtkImageResample, and vtkImagePad filters.  There are a number of
// tasks that vtkImageReslice is well suited for:
// <p>1) Application of simple rotations, scales, and translations to
// an image. It is often a good idea to use vtkImageChangeInformation
// to center the image first, so that scales and rotations occur around
// the center rather than around the lower-left corner of the image.
// <p>2) Resampling of one data set to match the voxel sampling of 
// a second data set via the SetInformationInput() method, e.g. for
// the purpose of comparing two images or combining two images.
// A transformation, either linear or nonlinear, can be applied 
// at the same time via the SetResliceTransform method if the two
// images are not in the same coordinate space.
// <p>3) Extraction of slices from an image volume.  The most convenient
// way to do this is to use SetResliceAxesDirectionCosines() to
// specify the orientation of the slice.  The direction cosines give
// the x, y, and z axes for the output volume.  The method 
// SetOutputDimensionality(2) is used to specify that want to output a
// slice rather than a volume.  The SetResliceAxesOrigin() command is
// used to provide an (x,y,z) point that the slice will pass through.
// You can use both the ResliceAxes and the ResliceTransform at the
// same time, in order to extract slices from a volume that you have
// applied a transformation to.
// .SECTION Caveats
// This filter is very inefficient if the output X dimension is 1.
// .SECTION see also
// vtkAbstractTransform vtkMatrix4x4


#ifndef __vtkImageReslice_h
#define __vtkImageReslice_h


#include "vtkImageResliceBase.h"

class VTK_IMAGING_EXPORT vtkImageReslice : public vtkImageResliceBase
{
public:
  static vtkImageReslice *New();
  vtkTypeMacro(vtkImageReslice, vtkImageResliceBase);

  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set a vtkImageData from which the default Spacing, Origin,
  // and WholeExtent of the output will be copied.  The spacing,
  // origin, and extent will be permuted according to the 
  // ResliceAxes.  Any values set via SetOutputSpacing, 
  // SetOutputOrigin, and SetOutputExtent will override these
  // values.  By default, the Spacing, Origin, and WholeExtent
  // of the Input are used.
  virtual void SetInformationInput(vtkImageData*);
  vtkGetObjectMacro(InformationInput, vtkImageData);

  // Description:
  // Turn on and off optimizations (default on, they should only be
  // turned off for testing purposes). 
  vtkSetMacro(Optimization, int);
  vtkGetMacro(Optimization, int);
  vtkBooleanMacro(Optimization, int);

  // Description:
  // Force the dimensionality of the output to either 1, 2,
  // 3 or 0 (default: 3).  If the dimensionality is 2D, then
  // the Z extent of the output is forced to (0,0) and the Z
  // origin of the output is forced to 0.0 (i.e. the output
  // extent is confined to the xy plane).  If the dimensionality
  // is 1D, the output extent is confined to the x axis.  
  // For 0D, the output extent consists of a single voxel at 
  // (0,0,0).
  vtkSetMacro(OutputDimensionality, int);
  vtkGetMacro(OutputDimensionality, int);

  // Description:
  // Report object referenced by instances of this class.
  virtual void ReportReferences(vtkGarbageCollector*);

  // Description:
  // Use a stencil to limit the calculations to a specific region of
  // the output.  Portions of the output that are 'outside' the stencil
  // will be cleared to the background color.  
  void SetStencil(vtkImageStencilData *stencil);
  vtkImageStencilData *GetStencil();

  // Description:
  // Generate an output stencil that defines which pixels were
  // interpolated and which pixels were out-of-bounds of the input.
  vtkSetMacro(GenerateStencilOutput, int);
  vtkGetMacro(GenerateStencilOutput, int);
  vtkBooleanMacro(GenerateStencilOutput, int);

  // Description:
  // Get the output stencil.
  vtkAlgorithmOutput *GetStencilOutputPort() {
    return this->GetOutputPort(1); }
  vtkImageStencilData *GetStencilOutput();
  void SetStencilOutput(vtkImageStencilData *stencil);

protected:
  vtkImageReslice();
  ~vtkImageReslice();

  vtkImageData *InformationInput;
  int Optimization;
  int GenerateStencilOutput;

  virtual void AllocateOutputData(vtkImageData *output, int *uExtent);
  virtual vtkImageData *AllocateOutputData(vtkDataObject *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *);
  virtual void InternalThreadedRequestData(vtkInformation *request,
                                   vtkInformationVector **inputVector,
                                   vtkInformationVector *outputVector,
                                   vtkImageData ***inData,
                                   vtkImageData **outData, int ext[6], int id);
  virtual int FillInputPortInformation(int port, vtkInformation *info);
  virtual int FillOutputPortInformation(int port, vtkInformation *info);

  virtual void BuildInterpolationTables();

private:
  vtkImageReslice(const vtkImageReslice&);  // Not implemented.
  void operator=(const vtkImageReslice&);  // Not implemented.
};

#endif
