/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageEuclideanDistance.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Olivier Cuisenaire who developed this class
             URL: http://ltswww.epfl.ch/~cuisenai
	     Email: Olivier.Cuisenaire@epfl.ch

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
 

#ifndef __vtkImageEuclideanDistance_h
#define __vtkImageEuclideanDistance_h

#include "vtkImageDecomposeFilter.h"

#define VTK_EDT_SAITO_CACHED 0
#define VTK_EDT_SAITO 1 

class VTK_IMAGING_EXPORT vtkImageEuclideanDistance : public vtkImageDecomposeFilter
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
  vtkSetMacro(MaximumDistance, float);
  vtkGetMacro(MaximumDistance, float);

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

  virtual void IterativeExecuteData(vtkImageData *in, vtkImageData *out)
    { this->MultiThread(in,out); };

protected:
  vtkImageEuclideanDistance();
  ~vtkImageEuclideanDistance() {}
  vtkImageEuclideanDistance(const vtkImageEuclideanDistance&);
  void operator=(const vtkImageEuclideanDistance&);

  float MaximumDistance;
  int Initialize;
  int ConsiderAnisotropy;
  int Algorithm;

  void ExecuteInformation(vtkImageData *input, vtkImageData *output);
  void ExecuteInformation()
    {this->vtkImageIterateFilter::ExecuteInformation();}
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
		       int outExt[6], int threadId);
};

#endif










