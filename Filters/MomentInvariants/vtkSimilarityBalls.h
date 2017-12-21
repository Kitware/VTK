/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimilarityBalls.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

Copyright (c) 2017, Los Alamos National Security, LLC

All rights reserved.

Copyright 2017. Los Alamos National Security, LLC.
This software was produced under U.S. Government contract DE-AC52-06NA25396
for Los Alamos National Laboratory (LANL), which is operated by
Los Alamos National Security, LLC for the U.S. Department of Energy.
The U.S. Government has rights to use, reproduce, and distribute this software.
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
If software is modified to produce derivative works, such modified software
should be clearly marked, so as not to confuse it with the version available
from LANL.

Additionally, redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following conditions
are met:
-   Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
/**
 * @class   vtkSimilarityBalls
 * @brief   compute the local maxima in the similarity field and draw a
 * circle of radius around it
 *
 * vtkSimilarityBalls is a filter that takes the similarityData as produced
 * by vtkMomentInvariants. 0. It computes the local maxima in space plus
 * scale and produces the output localMaxSimilarity that contains the
 * similarity value together with the corresponding radius at the maxima.
 * All other points are zero.
 * If a grid is provided in the optional second input, it also produces
 * two output fields for further visualization that encode
 * the radius through drawing
 * 1. a solid ball or
 * 2. a hollow sphere around those places.
 * The second input steers the resolution of the balls. It is helpful if
 * its extent is a multiple of the first input's. Then, the circles are
 * centered nicely. The spheres/circles are good for 2D visualization The
 * balls are good for 3D volume rendering or steering seeding of visualization
 * elements The theory and the algorithm is described in Roxana Bujack, Jens
 * Kasten, Ingrid Hotz, Gerik Scheuermann, and Eckhard Hitzer: "Moment
 * Invariants for 2D Flow Fields via Normalization in Detail."
 * http://www.informatik.uni-leipzig.de/~bujack/bujackTVCGSmall.pdf
 * @par Thanks:
 * Developed by Roxana Bujack at Los Alamos National Laboratory.
 */

#ifndef vtkSimilarityBalls_h
#define vtkSimilarityBalls_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersMomentInvariantsModule.h" // For export macro

#include <vector> // Needed for internal API

class vtkImageData;

class VTKFILTERSMOMENTINVARIANTS_EXPORT vtkSimilarityBalls : public vtkDataSetAlgorithm
{
public:
  /**
   * standard vtk new operator
   */
  static vtkSimilarityBalls* New();

  /**
   * ?
   */
  vtkTypeMacro(vtkSimilarityBalls, vtkDataSetAlgorithm);

  /** standard vtk print function
   * @param os: the way how to print
   * @param indent: how far to the right the text shall appear
   */
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * standard pipeline input for port 1
   * This is the similarity data as produced by vtkMomentInvariants.
   */
  void SetSimilarityData(vtkDataObject* input) { this->SetInputData(0, input); };

  /**
   * standard pipeline input for port 1
   * This is the similarity data as produced by vtkMomentInvariants.
   */
  void SetSimilarityConnection(vtkAlgorithmOutput* algOutput)
  {
    this->SetInputConnection(0, algOutput);
  };

  /**
   * standard pipeline input for port 1
   * This vtkImageData input steers the resolution of the balls. It is helpful if its extent is a
   * multiple of the first input's. Then, the circles are centered nicely.
   */
  void SetGridData(vtkDataObject* input) { this->SetInputData(1, input); };

  /**
   * standard pipeline input for port 1
   * This vtkImageData input steers the resolution of the balls. It is helpful if its extent is a
   * multiple of the first input's. Then, the circles are centered nicely.
   */
  void SetGridConnection(vtkAlgorithmOutput* algOutput) { this->SetInputConnection(1, algOutput); };

  /**
   * Set/Get the maximal order up to which the moments are computed.
   */
  vtkSetMacro(KindOfMaxima, int);
  vtkGetMacro(KindOfMaxima, int);

protected:
  /**
   * constructor setting defaults
   */
  vtkSimilarityBalls();

  /**
   * destructor
   */
  ~vtkSimilarityBalls() VTK_OVERRIDE;

  /** main executive of the program, reads the input, calles the functions, and produces the utput.
   * @param request: ?
   * @param inputVector: the input information
   * @param outputVector: the output information
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  /**
   * standard vtk pipeline function?
   */
  int RequestUpdateExtent(vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) VTK_OVERRIDE;

private:
  vtkSimilarityBalls(const vtkSimilarityBalls&) = delete;
  void operator=(const vtkSimilarityBalls&) = delete;

  /**
   * Dimension of the source can be 2 or 3
   */
  int Dimension;

  /**
   * The local maxima are computed in one of the following ways:
   * 0: singular point: has to be greater than all its neighbors in space and scale
   * 1: ridge: the point has to be greater than all neighbors in scale and greater than almost all
   * (all but two) neighbors in space 2: everything: the point has to be greater than all neighbors
   * in scale
   */
  int KindOfMaxima;

  /**
   * the agorithm has two input ports
   * port 0 is the similarityData, which is a vtkImageData and the 0th output of vtkMomentInvariants
   * port 1 a finer grid that is used for the drawing of the circles and balls
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * the agorithm generates 3 outputs, all are vtkImageData
   * the first has the topology of the similarityData
   * it contains the similarity value together with the corresponding radius at the local maxima.
   * All other points are zero. the latter two have the topology of the second input the radius of
   * the local maxima is used to draw a circle around the location with the value of the similarity
   * the radius of the local maxima is used to draw a ball around the location with the value of the
   * similarity
   */
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  /**
   * Make sure that the user has not entered weird values.
   * @param similarityData: the similarity over the different radii
   * @param gridData: the grid for the balls
   */
  void CheckValidity(vtkImageData* similarityData, vtkImageData* gridData);

  /**
   * Find out the dimension and the date type of the source dataset.
   * @param similarityData: the similarity over the different radii
   */
  void InterpretSimilarityData(vtkImageData* similarityData);

  /** extraction of the local maxima of the similaity field. this avoids clutter in the
   * visualization.
   * @param similarityData: the output of this algorithm. it has the topology of moments and will
   * have a number of scalar fields euqal to NumberOfRadii. each point contains the similarity of
   * its surrounding (of size radius) to the pattern
   * @param localMaxData: contains the similariy value and the corresponding radius if the
   * similarity field had a local maximum in space plus scale at the given point. It also stored the
   * radius that caused the maximum
   */
  void LocalMaxSimilarity(vtkImageData* similarityData, vtkImageData* localMaxData);

  /**
   * extraction of the local maxima of the similaity field. this avoids clutter in the
   * visualization.
   * @param localMaxData: contains the similariy value and the corresponding radius if the
   * similarity field at a local maximum in space plus scale at the given point. It also stored the
   * radius that caused the maximum
   * @param gridData: the grid for the balls
   * @param ballsData: a solid ball drawn around the local maxima
   * @param spheresData: an emptz sphere drawn around the local maxima
   */
  void Balls(vtkImageData* localMaxData,
    vtkImageData* gridData,
    vtkImageData* ballsData,
    vtkImageData* spheresData);
};

#endif
