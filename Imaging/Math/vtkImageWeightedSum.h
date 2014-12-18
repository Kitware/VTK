/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageWeightedSum.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageWeightedSum -  adds any number of images, weighting
// each according to the weight set using this->SetWeights(i,w).
//
// .SECTION Description
// All weights are normalized so they will sum to 1.
// Images must have the same extents. Output is
//
// .SECTION Thanks
// The original author of this class is Lauren O'Donnell (MIT) for Slicer


#ifndef vtkImageWeightedSum_h
#define vtkImageWeightedSum_h

#include "vtkImagingMathModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class vtkDoubleArray;
class VTKIMAGINGMATH_EXPORT vtkImageWeightedSum : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageWeightedSum *New();
  vtkTypeMacro(vtkImageWeightedSum,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The weights control the contribution of each input to the sum.
  // They will be normalized to sum to 1 before filter execution.
  virtual void SetWeights(vtkDoubleArray*);
  vtkGetObjectMacro(Weights, vtkDoubleArray);

  // Description:
  // Change a specific weight. Reallocation is done
  virtual void SetWeight(vtkIdType id, double weight);

  // Description:
  // Setting NormalizeByWeight on will divide the
  // final result by the total weight of the component functions.
  // This process does not otherwise normalize the weighted sum
  // By default, NormalizeByWeight is on.
  vtkGetMacro(NormalizeByWeight, int);
  vtkSetClampMacro(NormalizeByWeight, int, 0, 1);
  vtkBooleanMacro(NormalizeByWeight, int);

  // Description:
  // Compute the total value of all the weight
  double CalculateTotalWeight();

protected:
  vtkImageWeightedSum();
  ~vtkImageWeightedSum();

  // Array to hold all the weights
  vtkDoubleArray *Weights;

  // Boolean flag to divide by sum or not
  int NormalizeByWeight;

  int RequestInformation (vtkInformation * vtkNotUsed(request),
    vtkInformationVector** vtkNotUsed( inputVector ),
    vtkInformationVector *outputVector);

  void ThreadedRequestData (vtkInformation* request,
                            vtkInformationVector** inputVector,
                            vtkInformationVector* outputVector,
                            vtkImageData ***inData, vtkImageData **outData,
                            int ext[6], int id);
  int FillInputPortInformation(int i, vtkInformation* info);

private:
  vtkImageWeightedSum(const vtkImageWeightedSum&);  // Not implemented.
  void operator=(const vtkImageWeightedSum&);  // Not implemented.
};

#endif

