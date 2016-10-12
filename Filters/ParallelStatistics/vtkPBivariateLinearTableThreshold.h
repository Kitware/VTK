/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPBivariateLinearTableThreshold.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   vtkPBivariateLinearTableThreshold
 * @brief   performs line-based thresholding
 * for vtkTable data in parallel.
 *
 *
 * Perform the table filtering operations provided by
 * vtkBivariateLinearTableThreshold in parallel.
*/

#ifndef vtkPBivariateLinearTableThreshold_h
#define vtkPBivariateLinearTableThreshold_h

#include "vtkFiltersParallelStatisticsModule.h" // For export macro
#include "vtkBivariateLinearTableThreshold.h"

class vtkIdTypeArray;
class vtkMultiProcessController;

class VTKFILTERSPARALLELSTATISTICS_EXPORT vtkPBivariateLinearTableThreshold : public vtkBivariateLinearTableThreshold
{
public:
  static vtkPBivariateLinearTableThreshold* New();
  vtkTypeMacro(vtkPBivariateLinearTableThreshold, vtkBivariateLinearTableThreshold);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Set the vtkMultiProcessController to be used for combining filter
   * results from the individual nodes.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller,vtkMultiProcessController);
  //@}

protected:
  vtkPBivariateLinearTableThreshold();
  virtual ~vtkPBivariateLinearTableThreshold();

  virtual int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

  vtkMultiProcessController* Controller;
private:
  vtkPBivariateLinearTableThreshold(const vtkPBivariateLinearTableThreshold&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPBivariateLinearTableThreshold&) VTK_DELETE_FUNCTION;
};

#endif
