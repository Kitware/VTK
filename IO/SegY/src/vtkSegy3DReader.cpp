/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlane.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSegy3DReader.h"
#include <vtkSmartPointer.h>
#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkSegy3DReader);

void vtkSegy3DReader::PrintSelf(ostream &os, vtkIndent indent)
{
    Superclass::PrintSelf(os, indent);
}

vtkImageData* vtkSegy3DReader::GetImage(int ImageNumber)
{
    reader.LoadFromFile(FileName);

    image = vtkSmartPointer<vtkImageData>::New();
    if(!reader.ExportData3D(image))
        cout << "Failed to export 3D image from reader" << endl;

    return image;
}
