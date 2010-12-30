/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageProjection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageProjection - project an image along the Z direction
// .SECTION Description
// vtkImageProjection will combine all of the slices of an image to
// create a single slice.  The slices can be combined with the
// following operations: averaging, summation, minimum, maximum.
// If you require an arbitrary angle of projection, you can use
// vtkImageReslice to rotate the image before applying this filter.
// .SECTION Thanks
// Thanks to David Gobbi for contributing this class to VTK.

#ifndef __vtkImageProjection_h
#define __vtkImageProjection_h

#include "vtkThreadedImageAlgorithm.h"


#define VTK_PROJECTION_AVERAGE  0
#define VTK_PROJECTION_SUM      1
#define VTK_PROJECTION_MINIMUM  2
#define VTK_PROJECTION_MAXIMUM  3

class VTK_IMAGING_EXPORT vtkImageProjection : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageProjection *New();
  vtkTypeMacro(vtkImageProjection, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the slice direction: zero for x, 1 for y, 2 for z.
  // The default is the Z direction.
  vtkSetClampMacro(SliceDirection, int, 0, 2);
  void SetSliceDirectionToX() {
    this->SetSliceDirection(0); };
  void SetSliceDirectionToY() {
    this->SetSliceDirection(1); };
  void SetSliceDirectionToZ() {
    this->SetSliceDirection(2); };
  vtkGetMacro(SliceDirection, int);

  // Description:
  // Set the range of slices to combine. The default is to project
  // through all slices.
  vtkSetVector2Macro(SliceRange, int);
  vtkGetVector2Macro(SliceRange, int);

  // Description:
  // Set the operation to use when combining slices.  The choices are
  // "Average", "Sum", "Maximum", "Minimum".  The default is "Average".
  vtkSetClampMacro(Operation, int, 0, 3);
  void SetOperationToAverage() {
    this->SetOperation(VTK_PROJECTION_AVERAGE); };
  void SetOperationToSum() {
    this->SetOperation(VTK_PROJECTION_SUM); };
  void SetOperationToMinimum() {
    this->SetOperation(VTK_PROJECTION_MINIMUM); };
  void SetOperationToMaximum() {
    this->SetOperation(VTK_PROJECTION_MAXIMUM); };
  vtkGetMacro(Operation, int);
  const char *GetOperationAsString();

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
  vtkImageProjection();
  ~vtkImageProjection();

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
  int SliceDirection;
  int SliceRange[2];
  int OutputScalarType;
  int MultiSliceOutput;

private:
  vtkImageProjection(const vtkImageProjection&);  // Not implemented.
  void operator=(const vtkImageProjection&);  // Not implemented.
};

#endif
