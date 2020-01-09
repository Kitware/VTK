/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractVOI.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkmExtractVOI
 * @brief   select piece (e.g., volume of interest) and/or subsample structured points dataset
 *
 * vtkmExtractVOI is a filter that selects a portion of an input structured
 * points dataset, or subsamples an input dataset. (The selected portion of
 * interested is referred to as the Volume Of Interest, or VOI.) The output of
 * this filter is a structured points dataset. The filter treats input data
 * of any topological dimension (i.e., point, line, image, or volume) and can
 * generate output data of any topological dimension.
 *
 * To use this filter set the VOI ivar which are i-j-k min/max indices that
 * specify a rectangular region in the data. (Note that these are 0-offset.)
 * You can also specify a sampling rate to subsample the data.
 *
 * Typical applications of this filter are to extract a slice from a volume
 * for image processing, subsampling large volumes to reduce data size, or
 * extracting regions of a volume with interesting data.
 *
 */
#ifndef vtkmExtractVOI_h
#define vtkmExtractVOI_h

#include "vtkAcceleratorsVTKmModule.h" // for export macro
#include "vtkExtractVOI.h"

class VTKACCELERATORSVTKM_EXPORT vtkmExtractVOI : public vtkExtractVOI
{
public:
  vtkTypeMacro(vtkmExtractVOI, vtkExtractVOI);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkmExtractVOI* New();

protected:
  vtkmExtractVOI();
  ~vtkmExtractVOI() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkmExtractVOI(const vtkmExtractVOI&) = delete;
  void operator=(const vtkmExtractVOI&) = delete;
};

#endif // vtkmExtractVOI_h
// VTK-HeaderTest-Exclude: vtkmExtractVOI.h
