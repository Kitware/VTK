/*=========================================================================
  
Program:   Visualization Toolkit
Module:    vtkPPairwiseExtractHistogram2D.cxx

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
#include "vtkPPairwiseExtractHistogram2D.h"

#include "vtkArrayData.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkStatisticsAlgorithmPrivate.h"
#include "vtkCollection.h"
#include "vtkPExtractHistogram2D.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkImageMedian3D.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedIntArray.h"

#include <set>
#include <vector>
#include <string>

vtkStandardNewMacro(vtkPPairwiseExtractHistogram2D);
vtkCxxSetObjectMacro(vtkPPairwiseExtractHistogram2D, Controller, vtkMultiProcessController);

vtkPPairwiseExtractHistogram2D::vtkPPairwiseExtractHistogram2D()
{
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}


vtkPPairwiseExtractHistogram2D::~vtkPPairwiseExtractHistogram2D()
{
  this->SetController(0);
}

void vtkPPairwiseExtractHistogram2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller: " << this->Controller << endl;
}

vtkExtractHistogram2D* vtkPPairwiseExtractHistogram2D::NewHistogramFilter()
{
  vtkPExtractHistogram2D* ph = vtkPExtractHistogram2D::New();
  ph->SetController(this->Controller);
  return ph;
}
