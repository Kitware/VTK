/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQImageToImageSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkQImageToImageSource.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkPointData.h"

#include "vtkQtInitialization.h"
#include <QImage>
#include <math.h>

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkCxxRevisionMacro(vtkQImageToImageSource, "1.1");
vtkStandardNewMacro(vtkQImageToImageSource);

//----------------------------------------------------------------------------
vtkQImageToImageSource::vtkQImageToImageSource()
{
  VTK_CREATE( vtkQtInitialization, initApp );

  this->QtImage = 0;
  this->SetNumberOfInputPorts(0);
  this->DataExtent[0] = 0;
  this->DataExtent[1] = 0;
  this->DataExtent[2] = 0;
  this->DataExtent[3] = 0;
  this->DataExtent[4] = 0;
  this->DataExtent[5] = 0;
}

//----------------------------------------------------------------------------
int vtkQImageToImageSource::RequestData( vtkInformation *vtkNotUsed(request),
                                         vtkInformationVector **vtkNotUsed(inputVector), 
                                         vtkInformationVector *outputVector)
{ 
  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *output = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  if( this->QtImage == 0 )
    {
    vtkErrorMacro( "Qt Image was not set." );
    return 0;
    }

  this->QtImage->convertToFormat( QImage::Format_ARGB32 );
  unsigned char* data = this->QtImage->bits();

  output->SetNumberOfScalarComponents( 4 );
  output->SetScalarTypeToUnsignedChar();
  output->SetExtent(this->DataExtent);
  output->AllocateScalars();
  
  unsigned char temp[4];
  vtkUnsignedCharArray* array = vtkUnsignedCharArray::SafeDownCast( output->GetPointData()->GetScalars() );
  
  QSize size = this->QtImage->size();
  int width = size.width();
  int height = size.height();

  int range = width*height;
  for( int i = 0; i < range; i++ )
    {
    temp[2] = data[4*i+0]; //B
    temp[1] = data[4*i+1]; //G
    temp[0] = data[4*i+2]; //R
    temp[3] = data[4*i+3]; //A
    
    array->SetTupleValue( range - (((i/width)+1)*width) + (i%width), temp );
    }
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkQImageToImageSource::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector ** vtkNotUsed( inputVector ),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if( this->QtImage == 0 )
    {
    vtkErrorMacro( "Qt Image was not set." );
    return 0;
    }

  QSize size = this->QtImage->size();
  this->DataExtent[1] = size.width() - 1;
  this->DataExtent[3] = size.height() - 1;
    
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               this->DataExtent,6);
  return 1;
}

//----------------------------------------------------------------------------
void vtkQImageToImageSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
