/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenSlideReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenSlideReader.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkToolkits.h"

vtkStandardNewMacro(vtkOpenSlideReader);

void vtkOpenSlideReader::ExecuteInformation()
{
  std::cout << this->GetFileName() << std::endl;

  this->openslide_handle = openslide_open(this->GetFileName());

  if(this->openslide_handle == NULL || openslide_get_error(this->openslide_handle) != NULL)
    {
    vtkErrorWithObjectMacro(this,
                            "Trying to read a JPEG image from "
                            "a zero-length memory buffer!");
    return;
    }


  if (this->GetFileName() == NULL)
    {
    }
}


//----------------------------------------------------------------------------
// This function reads a data from a file.  The datas extent/axes
// are assumed to be the same as the file extent/order.
void vtkOpenSlideReader::ExecuteDataWithInformation(vtkDataObject *output,
                                               vtkInformation *outInfo)
{
  vtkImageData *data = this->AllocateOutputData(output, outInfo);

  //// Leverage openslide to read the region
  //openslide_read_region(this->openslide_handle, data->GetPointer(), x, y, 0, w, h);
  //assert(openslide_get_error(osr) == NULL);
  //openslide_close(osr);


  //if (this->InternalFileName == NULL)
    //{
    //vtkErrorMacro(<< "Either a FileName or FilePrefix must be specified.");
    //return;
    //}

  //this->ComputeDataIncrements();

  data->GetPointData()->GetScalars()->SetName("OpenSlideImage");

  // No updating anything right now
}



int vtkOpenSlideReader::CanReadFile(const char* fname)
{
  return 3;
}


//----------------------------------------------------------------------------
void vtkOpenSlideReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
