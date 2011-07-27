/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRToUniformGrid.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "vtkAMRToGrid.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkUniformGrid.h"
#include "vtkIndent.h"


#include <cassert>

vtkStandardNewMacro( vtkAMRToGrid );

//-----------------------------------------------------------------------------
vtkAMRToGrid::vtkAMRToGrid()
{
  this->TransferToNodes   = 1;
  this->LevelOfResolution = 1;
  this->Controller        = vtkMultiProcessController::GetGlobalController();
  this->SetNumberOfInputPorts( 1 );
  this->SetNumberOfOutputPorts( 1 );
}

//-----------------------------------------------------------------------------
vtkAMRToGrid::~vtkAMRToGrid()
{

}

//-----------------------------------------------------------------------------
void vtkAMRToGrid::PrintSelf( std::ostream &oss, vtkIndent indent )
{
  this->Superclass::PrintSelf( oss, indent );
}

//-----------------------------------------------------------------------------
int vtkAMRToGrid::FillInputPortInformation( int port, vtkInformation *info )
{
  // TODO: implement this
  return 1;
}

//-----------------------------------------------------------------------------
int vtkAMRToGrid::FillOutputPortInformation(int port, vtkInformation *info )
{
  // TODO: implement this
  return 1;
}

//-----------------------------------------------------------------------------
int vtkAMRToGrid::RequestData(
    vtkInformation *rqst, vtkInformationVector** input,
    vtkInformationVector* output )
{
  // TODO: implement this
  return 1;
}
