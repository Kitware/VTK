/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOTFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOTFilter
 * @brief
 * A generic VTK Filter to process vtkTable
 * using openturns algorithm.
 * It expects a vtkTable as first input,
 * converts it to a openturns structure and then process it
 * The inherited classes are responsible of filling up the output
 * table in the Process() method.
 */

#ifndef vtkOTFilter_h
#define vtkOTFilter_h

#include "vtkFiltersOpenTURNSModule.h" // For export macro
#include "vtkTableAlgorithm.h"

namespace OT
{
class Sample;
}

class VTKFILTERSOPENTURNS_EXPORT vtkOTFilter : public vtkTableAlgorithm
{
public:
  vtkTypeMacro(vtkOTFilter, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkOTFilter();
  ~vtkOTFilter() override;

  /**
   * Set the input of this filter, a vtkTable
   */
  virtual int FillInputPortInformation(int port, vtkInformation* info) override;

  virtual int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Abstract method to process openturns data
   */
  virtual int Process(OT::Sample* input) = 0;

  /**
   * Method to add a openturns data to a table as a named column
   */
  virtual void AddToOutput(OT::Sample* ns, const std::string& name);

  vtkTable* Output;

private:
  void operator=(const vtkOTFilter&) = delete;
  vtkOTFilter(const vtkOTFilter&) = delete;
};

#endif
