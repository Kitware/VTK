/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPComputeHistogram2DOutliers.h

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
 * @class   vtkPComputeHistogram2DOutliers
 * @brief   extract outlier rows from
 *  a vtkTable based on input 2D histograms, in parallel.
 *
 *
 *  This class does exactly the same this as vtkComputeHistogram2DOutliers,
 *  but does it in a multi-process environment.  After each node
 *  computes their own local outliers, class does an AllGather
 *  that distributes the outliers to every node.  This could probably just
 *  be a Gather onto the root node instead.
 *
 *  After this operation, the row selection will only contain local row ids,
 *  since I'm not sure how to deal with distributed ids.
 *
 * @sa
 *  vtkComputeHistogram2DOutliers
 *
 * @par Thanks:
 *  Developed by David Feng at Sandia National Laboratories
 *------------------------------------------------------------------------------
*/

#ifndef vtkPComputeHistogram2DOutliers_h
#define vtkPComputeHistogram2DOutliers_h
//------------------------------------------------------------------------------
#include "vtkFiltersParallelImagingModule.h" // For export macro
#include "vtkComputeHistogram2DOutliers.h"
//------------------------------------------------------------------------------
class vtkMultiProcessController;
//------------------------------------------------------------------------------
class VTKFILTERSPARALLELIMAGING_EXPORT vtkPComputeHistogram2DOutliers : public vtkComputeHistogram2DOutliers
{
public:
  static vtkPComputeHistogram2DOutliers* New();
  vtkTypeMacro(vtkPComputeHistogram2DOutliers, vtkComputeHistogram2DOutliers);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller,vtkMultiProcessController);
protected:
  vtkPComputeHistogram2DOutliers();
  ~vtkPComputeHistogram2DOutliers();

  virtual int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

  vtkMultiProcessController* Controller;
private:
  vtkPComputeHistogram2DOutliers(const vtkPComputeHistogram2DOutliers&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPComputeHistogram2DOutliers&) VTK_DELETE_FUNCTION;
};

#endif
