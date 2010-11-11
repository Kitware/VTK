/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamingStatistics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkStreamingStatistics.h"
#include "vtkStatisticsAlgorithm.h"

#include "vtkDataObjectCollection.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkTable.h"

vtkStandardNewMacro(vtkStreamingStatistics);

vtkCxxSetObjectMacro(vtkStreamingStatistics,
                     StatisticsAlgorithm,
                     vtkStatisticsAlgorithm);

// ----------------------------------------------------------------------
vtkStreamingStatistics::vtkStreamingStatistics()
{
  // Initialize internal stats algorithm to NULL
  this->StatisticsAlgorithm = 0;
  this->SetStatisticsAlgorithm(0);

  // Initialize internal model
  this->internalModel = vtkMultiBlockDataSet::New();
}

// ----------------------------------------------------------------------
vtkStreamingStatistics::~vtkStreamingStatistics()
{
  // Release/delete internal stats algorithm
  this->SetStatisticsAlgorithm(0);
  this->StatisticsAlgorithm = 0;

  // Release/delete internal model to NULL
  this->internalModel->Delete();
  this->internalModel = 0;
}

// ----------------------------------------------------------------------
int vtkStreamingStatistics::RequestData( vtkInformation*,
                                         vtkInformationVector** inputVector,
                                         vtkInformationVector* outputVector )
{
  // Input handles
  vtkTable*             inData       = vtkTable::GetData( inputVector[INPUT_DATA], 0 );
  vtkMultiBlockDataSet* inModel      = vtkMultiBlockDataSet::GetData( inputVector[INPUT_MODEL], 0 );
  vtkTable*             inParameters = vtkTable::GetData( inputVector[LEARN_PARAMETERS], 0 );

  // Output handles
  vtkTable*             outData  = vtkTable::GetData( outputVector, OUTPUT_DATA );
  vtkMultiBlockDataSet* outModel = vtkMultiBlockDataSet::GetData( outputVector, OUTPUT_MODEL );
  vtkTable*             outTest  = vtkTable::GetData( outputVector, OUTPUT_TEST );


  // Note: Experiment code. Lots of use case are currently not handled in
  // any way and there are a lot of assumptions made about this or that

  // Make sure the statistics algorithm is set
  if ( !this->StatisticsAlgorithm )
    {
    vtkErrorMacro("StatisticsAlgorithm not set! Punting!")
    cerr << "StatisticsAlgorithm not set! Punting!" << endl;
    return 0;
    }

  // Set the input into my stats algorithms
  this->StatisticsAlgorithm->SetInput(inData);
  this->StatisticsAlgorithm->SetInputModel(this->internalModel);

  // Force an update
  this->StatisticsAlgorithm->Update();

  // Grab (DeepCopy) the model for next time
  this->internalModel->DeepCopy(this->StatisticsAlgorithm->GetOutputDataObject(OUTPUT_MODEL));

  // Shallow copy the internal output to external output
  outData->ShallowCopy(this->StatisticsAlgorithm->GetOutput(OUTPUT_DATA));
  outModel->ShallowCopy(this->StatisticsAlgorithm->GetOutputDataObject(OUTPUT_MODEL));

  return 1;
}




// ----------------------------------------------------------------------
void vtkStreamingStatistics::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  if (this->StatisticsAlgorithm)
    {
    os << indent << "StatisticsAlgorithm: ";
    this->StatisticsAlgorithm->PrintSelf(os,indent);
    }
}