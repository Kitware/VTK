/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkImageDataToStructuredGridFilter.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkImageDataToStructuredGridFilter.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkDataObject.h"
#include "vtkAssertUtils.hpp"

//
// Standard methods
//
vtkStandardNewMacro(vtkImageDataToStructuredGridFilter)

vtkImageDataToStructuredGridFilter::vtkImageDataToStructuredGridFilter()
{
  // TODO Auto-generated constructor stub

}

//------------------------------------------------------------------------------
vtkImageDataToStructuredGridFilter::~vtkImageDataToStructuredGridFilter()
{
  // TODO Auto-generated destructor stub
}

//------------------------------------------------------------------------------
void vtkImageDataToStructuredGridFilter::PrintSelf( std::ostream &oss,
                                                    vtkIndent indent )
{
  this->Superclass::PrintSelf( oss, indent );
}

//------------------------------------------------------------------------------
int vtkImageDataToStructuredGridFilter::FillInputPortInformation(
                                                int port, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkImageData");
  return 1;
}

//------------------------------------------------------------------------------
int vtkImageDataToStructuredGridFilter::FillOutputPortInformation(
                                                int port, vtkInformation* info )
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(),"vtkStructuredGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkImageDataToStructuredGridFilter::RequestData(
                                  vtkInformation* request,
                                  vtkInformationVector** inputVector,
                                  vtkInformationVector* outputVector )
{
  vtkInformation* inInfo  = inputVector[0]->GetInformationObject( 0 );
  vtkInformation* outInfo = outputVector->GetInformationObject( 0 );
  vtkAssertUtils::assertNotNull( inInfo,__FILE__, __LINE__);
  vtkAssertUtils::assertNotNull( outInfo,__FILE__,__LINE__);

   // TODO: implement this
}
