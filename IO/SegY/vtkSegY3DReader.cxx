/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSegY3DReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSegY3DReader.h"

#include "vtkImageData.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSegY3DReader);

//-----------------------------------------------------------------------------
vtkSegY3DReader::vtkSegY3DReader()
{
  this->image = nullptr;
  this->SetNumberOfInputPorts(0);
}

//-----------------------------------------------------------------------------
vtkSegY3DReader::~vtkSegY3DReader()
{
  if (this->image)
  {
    this->image = nullptr;
  }
}

//-----------------------------------------------------------------------------
void vtkSegY3DReader::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)")
     << "\n";
  Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
vtkImageData* vtkSegY3DReader::GetImage()
{
  this->reader.LoadFromFile(FileName);

  image = vtkSmartPointer<vtkImageData>::New();
  if (!reader.ExportData3D(image))
    cout << "Failed to export 3D image from reader" << endl;

  return image;
}
