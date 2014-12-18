/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSlab.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageSlab - combine image slices to form a slab image
// .SECTION Description
// vtkImageSlab will combine all of the slices of an image to
// create a single slice.  The slices can be combined with the
// following operations: averaging, summation, minimum, maximum.
// If you require an arbitrary angle of projection, you can use
// vtkImageReslice.
// .SECTION Thanks
// Thanks to David Gobbi for contributing this class to VTK.

#ifndef vtkImageSlab_h
#define vtkImageSlab_h

#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGGENERAL_EXPORT vtkImageSlab : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageSlab *New();
  vtkTypeMacro(vtkImageSlab, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the slice direction: zero for x, 1 for y, 2 for z.
  // The default is the Z direction.
  vtkSetClampMacro(Orientation, int, 0, 2);
  void SetOrientationToX() {
    this->SetOrientation(0); };
  void SetOrientationToY() {
    this->SetOrientation(1); };
  void SetOrientationToZ() {
    this->SetOrientation(2); };
  vtkGetMacro(Orientation, int);

  // Description:
  // Set the range of slices to combine. The default is to project
  // through all slices.
  vtkSetVector2Macro(SliceRange, int);
  vtkGetVector2Macro(SliceRange, int);

  // Description:
  // Set the operation to use when combining slices.  The choices are
  // "Mean", "Sum", "Min", "Max".  The default is "Mean".
  vtkSetClampMacro(Operation, int, VTK_IMAGE_SLAB_MIN, VTK_IMAGE_SLAB_SUM);
  void SetOperationToMin() {
    this->SetOperation(VTK_IMAGE_SLAB_MIN); };
  void SetOperationToMax() {
    this->SetOperation(VTK_IMAGE_SLAB_MAX); };
  void SetOperationToMean() {
    this->SetOperation(VTK_IMAGE_SLAB_MEAN); };
  void SetOperationToSum() {
    this->SetOperation(VTK_IMAGE_SLAB_SUM); };
  vtkGetMacro(Operation, int);
  const char *GetOperationAsString();

  // Description:
  // Use trapezoid integration for slab computation.  This weighs the
  // first and last slices by half when doing sum and mean, as compared
  // to the default midpoint integration that weighs all slices equally.
  // It is off by default.
  vtkSetMacro(TrapezoidIntegration, int);
  vtkBooleanMacro(TrapezoidIntegration, int);
  vtkGetMacro(TrapezoidIntegration, int);

  // Description:
  // Turn on multi-slice output.  Each slice of the output will be
  // a projection through the specified range of input slices, e.g.
  // if the SliceRange is [0,3] then slice 'i' of the output will
  // be a projection through slices 'i' through '3+i' of the input.
  // This flag is off by default.
  vtkSetMacro(MultiSliceOutput, int);
  vtkBooleanMacro(MultiSliceOutput, int);
  vtkGetMacro(MultiSliceOutput, int);

  // Description:
  // Set the output scalar type to float or double, to avoid
  // potential overflow when doing a summation operation.
  // The default is to use the scalar type of the input data,
  // and clamp the output to the range of the input scalar type.
  void SetOutputScalarTypeToFloat() {
    this->SetOutputScalarType(VTK_FLOAT); };
  void SetOutputScalarTypeToDouble() {
    this->SetOutputScalarType(VTK_DOUBLE); };
  void SetOutputScalarTypeToInputScalarType() {
    this->SetOutputScalarType(0); };
  vtkGetMacro(OutputScalarType, int);

protected:
  vtkImageSlab();
  ~vtkImageSlab();

  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *);
  virtual void ThreadedRequestData(vtkInformation *request,
                                   vtkInformationVector **inputVector,
                                   vtkInformationVector *outputVector,
                                   vtkImageData ***inData,
                                   vtkImageData **outData, int ext[6], int id);

  vtkSetMacro(OutputScalarType, int);

  int Operation;
  int Orientation;
  int SliceRange[2];
  int OutputScalarType;
  int MultiSliceOutput;
  int TrapezoidIntegration;

private:
  vtkImageSlab(const vtkImageSlab&);  // Not implemented.
  void operator=(const vtkImageSlab&);  // Not implemented.
};

#endif
