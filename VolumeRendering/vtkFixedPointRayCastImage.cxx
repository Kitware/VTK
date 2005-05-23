/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFixedPointRayCastImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFixedPointRayCastImage.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkFixedPointRayCastImage, "1.1");
vtkStandardNewMacro(vtkFixedPointRayCastImage);

// Construct a new vtkFixedPointRayCastImage with default values
vtkFixedPointRayCastImage::vtkFixedPointRayCastImage()
{
  this->ImageViewportSize[0] = 0;
  this->ImageViewportSize[1] = 0;
  
  this->ImageMemorySize[0]   = 0;
  this->ImageMemorySize[1]   = 0;
  
  this->ImageInUseSize[0]    = 0;
  this->ImageInUseSize[1]    = 0;
  
  this->ImageOrigin[0]       = 0;
  this->ImageOrigin[1]       = 0;
  
  this->Image                = NULL;
}

// Destruct a vtkFixedPointRayCastImage - clean up any memory used
vtkFixedPointRayCastImage::~vtkFixedPointRayCastImage()
{
  delete [] this->Image;
}

// Allocate memory
void vtkFixedPointRayCastImage::AllocateImage()
{
  delete [] this->Image;
  this->Image = NULL;
  
  if ( this->ImageMemorySize[0]*this->ImageMemorySize[1] > 0 )
    {
    this->Image = new unsigned short [ ( 4 * 
                                         this->ImageMemorySize[0] *
                                         this->ImageMemorySize[1] )];
    }
}

// Clear image to 0
void vtkFixedPointRayCastImage::ClearImage()
{
  unsigned short *ucptr = this->Image;
    
  for ( int i = 0; i < this->ImageMemorySize[0]*this->ImageMemorySize[1]; i++ )
    {
    *(ucptr++) = 0;
    *(ucptr++) = 0;
    *(ucptr++) = 0;
    *(ucptr++) = 0;      
    }
}

void vtkFixedPointRayCastImage::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Image Viewport Size: "
     << this->ImageViewportSize[0] << " " 
     << this->ImageViewportSize[1] << endl;

  os << indent << "Image Memory Size: "
     << this->ImageMemorySize[0] << " " 
     << this->ImageMemorySize[1] << endl;

  os << indent << "Image In Use Size: "
     << this->ImageInUseSize[0] << " " 
     << this->ImageInUseSize[1] << endl;
  
  os << indent << "Image Origin: "
     << this->ImageOrigin[0] << " " 
     << this->ImageOrigin[1] << endl;
  
}

