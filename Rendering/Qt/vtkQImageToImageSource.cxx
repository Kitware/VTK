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

#include <QApplication>
#include <QImage>
#include <cmath>

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkStandardNewMacro(vtkQImageToImageSource);

//----------------------------------------------------------------------------
vtkQImageToImageSource::vtkQImageToImageSource()
{
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
  if(!QApplication::instance())
  {
    vtkErrorMacro("You must initialize QApplication before using this filter.");
    return 0;
  }

  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *output = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if( this->QtImage == 0 )
  {
    vtkErrorMacro( "Qt Image was not set." );
    return 0;
  }

  QImage newImage = this->QtImage->convertToFormat( QImage::Format_ARGB32 );
  QSize size = newImage.size();
  int width = size.width();
  int height = size.height();
  const unsigned char* data2 = newImage.bits();

  unsigned char* data = new unsigned char[4*width*height];
  memcpy( data, data2, 4*width*height );

  output->SetExtent(this->DataExtent);
  output->AllocateScalars(VTK_UNSIGNED_CHAR, 4);

  vtkUnsignedCharArray* array = vtkArrayDownCast<vtkUnsignedCharArray>( output->GetPointData()->GetScalars() );

  int i, j;
  unsigned char temp[4];
  for( i = 0; i < height/2; i++ )
  {
    for( j = 0; j < width; j++ )
    {
      int bottom_address = ((((height-1)-i)*width)+j);
      int top_address = (i*width)+j;
      temp[0] = data[(4*bottom_address)+0];
      temp[1] = data[(4*bottom_address)+1];
      temp[2] = data[(4*bottom_address)+2];
      temp[3] = data[(4*bottom_address)+3];

      data[(4*bottom_address)+2] = data[(4*top_address)+0]; //B
      data[(4*bottom_address)+1] = data[(4*top_address)+1]; //G
      data[(4*bottom_address)+0] = data[(4*top_address)+2]; //R
      data[(4*bottom_address)+3] = data[(4*top_address)+3]; //A

      data[(4*top_address)+2] = temp[0]; //B
      data[(4*top_address)+1] = temp[1]; //G
      data[(4*top_address)+0] = temp[2]; //R
      data[(4*top_address)+3] = temp[3]; //A
    }
  }

  if( height%2 )
  {
    for( j = 0; j < width; j++ )
    {
      int address = (i*width)+j;
      unsigned char temp1 = data[(4*address)+0];
      data[(4*address)+0] = data[(4*address)+2];
      data[(4*address)+2] = temp1;
    }
  }

  array->SetVoidArray( data, 4*width*height, 0, vtkUnsignedCharArray::VTK_DATA_ARRAY_DELETE );
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
