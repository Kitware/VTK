/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractParticlesOverTime.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExtractParticlesOverTime
 * @brief   Extract particles that goes through a given volumic data set.
 *
 * vtkExtractParticlesOverTime extracts particles (points) from the first input that goes through
 * the volume of the second input by iterating over time. Both inputs should be vtkDataSet objects.
 * The first input should be temporal (i.e contains time steps), and the second one should be a
 * volumic dataset (i.e contains 3D cells).
 *
 * The output is a vtkDataSet that contains points which are subsets of the first input. The points
 * move over time the same way the first input does.
 */

#ifndef vtkExtractParticlesOverTime_h
#define vtkExtractParticlesOverTime_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersExtractionModule.h" // For export macro
#include <memory>                       // For smart pointers
#include <string>                       // For channel array name

class vtkExtractParticlesOverTimeInternals;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractParticlesOverTime : public vtkDataSetAlgorithm
{
public:
  ///@{
  /**
   * Standard Type-Macro
   */
  static vtkExtractParticlesOverTime* New();
  vtkTypeMacro(vtkExtractParticlesOverTime, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify the name of a scalar array which will be used to fetch
   * the index of each point. This is necessary only if the particles
   * change position (Id order) on each time step. The Id can be used
   * to identify particles at each step and hence track them properly.
   * If this array is nullptr, the global point ids are used.  If an Id
   * array cannot otherwise be found, the point index is used as the ID.
   */
  vtkSetStdStringFromCharMacro(IdChannelArray);
  vtkGetCharFromStdStringMacro(IdChannelArray);
  ///@}

protected:
  vtkExtractParticlesOverTime();
  ~vtkExtractParticlesOverTime() override = default;

  ///@{
  /**
   * The necessary parts of the standard pipeline update mechanism
   */
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  ///@}

private:
  vtkExtractParticlesOverTime(const vtkExtractParticlesOverTime&) = delete;
  void operator=(const vtkExtractParticlesOverTime&) = delete;

  std::string IdChannelArray;
  std::shared_ptr<vtkExtractParticlesOverTimeInternals> Internals;
};

#endif
