/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageEuclideanDistance.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageEuclideanDistance - computes 3D Euclidean DT
// .SECTION Description
// vtkImageEuclideanDistance implements the Euclidean DT using
// Saito's algorithm. The distance map produced contains the square of the
// Euclidean distance values.
//
// The algorithm has a o(n^(D+1)) complexity over nxnx...xn images in D
// dimensions. It is very efficient on relatively small images. Cuisenaire's
// algorithms should be used instead if n >> 500. These are not implemented
// yet.
//
// For the special case of images where the slice-size is a multiple of
// 2^N with a large N (typically for 256x256 slices), Saito's algorithm
// encounters a lot of cache conflicts during the 3rd iteration which can
// slow it very significantly. In that case, one should use
// ::SetAlgorithmToSaitoCached() instead for better performance.
//
// References:
//
// T. Saito and J.I. Toriwaki. New algorithms for Euclidean distance
// transformations of an n-dimensional digitised picture with applications.
// Pattern Recognition, 27(11). pp. 1551--1565, 1994.
//
// O. Cuisenaire. Distance Transformation: fast algorithms and applications
// to medical image processing. PhD Thesis, Universite catholique de Louvain,
// October 1999. http://ltswww.epfl.ch/~cuisenai/papers/oc_thesis.pdf


#ifndef vtkImageEuclideanDistance_h
#define vtkImageEuclideanDistance_h

#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkImageDecomposeFilter.h"

#define VTK_EDT_SAITO_CACHED 0
#define VTK_EDT_SAITO 1

class VTKIMAGINGGENERAL_EXPORT vtkImageEuclideanDistance : public vtkImageDecomposeFilter
{
public:
  static vtkImageEuclideanDistance *New();
  vtkTypeMacro(vtkImageEuclideanDistance,vtkImageDecomposeFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Used internally for streaming and threads.
  // Splits output update extent into num pieces.
  // This method needs to be called num times.  Results must not overlap for
  // consistent starting extent.  Subclass can override this method.
  // This method returns the number of peices resulting from a
  // successful split.  This can be from 1 to "total".
  // If 1 is returned, the extent cannot be split.
  int SplitExtent(int splitExt[6], int startExt[6],
                  int num, int total);

  // Description:
  // Used to set all non-zero voxels to MaximumDistance before starting
  // the distance transformation. Setting Initialize off keeps the current
  // value in the input image as starting point. This allows to superimpose
  // several distance maps.
  vtkSetMacro(Initialize, int);
  vtkGetMacro(Initialize, int);
  vtkBooleanMacro(Initialize, int);

  // Description:
  // Used to define whether Spacing should be used in the computation of the
  // distances
  vtkSetMacro(ConsiderAnisotropy, int);
  vtkGetMacro(ConsiderAnisotropy, int);
  vtkBooleanMacro(ConsiderAnisotropy, int);

  // Description:
  // Any distance bigger than this->MaximumDistance will not ne computed but
  // set to this->MaximumDistance instead.
  vtkSetMacro(MaximumDistance, double);
  vtkGetMacro(MaximumDistance, double);

  // Description:
  // Selects a Euclidean DT algorithm.
  // 1. Saito
  // 2. Saito-cached
  // More algorithms will be added later on.
  vtkSetMacro(Algorithm, int);
  vtkGetMacro(Algorithm, int);
  void SetAlgorithmToSaito ()
    { this->SetAlgorithm(VTK_EDT_SAITO); }
  void SetAlgorithmToSaitoCached ()
    { this->SetAlgorithm(VTK_EDT_SAITO_CACHED); }

  virtual int IterativeRequestData(vtkInformation*,
                                   vtkInformationVector**,
                                   vtkInformationVector*);

protected:
  vtkImageEuclideanDistance();
  ~vtkImageEuclideanDistance() {}

  double MaximumDistance;
  int Initialize;
  int ConsiderAnisotropy;
  int Algorithm;

  // Replaces "EnlargeOutputUpdateExtent"
  virtual void AllocateOutputScalars(vtkImageData *outData,
                                     int outExt[6],
                                     vtkInformation* outInfo);

  virtual int IterativeRequestInformation(vtkInformation* in,
                                          vtkInformation* out);
  virtual int IterativeRequestUpdateExtent(vtkInformation* in,
                                           vtkInformation* out);

private:
  vtkImageEuclideanDistance(const vtkImageEuclideanDistance&);  // Not implemented.
  void operator=(const vtkImageEuclideanDistance&);  // Not implemented.
};

#endif










