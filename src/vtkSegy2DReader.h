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

#ifndef SEGYVISUALIZER2D_VTKSEGY2DREADER_H
#define SEGYVISUALIZER2D_VTKSEGY2DREADER_H

#include "vtkSegy2DReader.h"

#include <vtkPolyDataAlgorithm.h>
#include "SegyReader.h"

class vtkSegy2DReader : public vtkPolyDataAlgorithm
{
public:
    static vtkSegy2DReader* New();
    vtkTypeMacro(vtkSegy2DReader,vtkPolyDataAlgorithm);
    void PrintSelf(ostream& os, vtkIndent indent);

    vtkSetStringMacro(FileName);

    bool GetImageData(vtkImageData *imageData)
    {
      return reader.GetImageData(imageData);
    }

    vtkSegy2DReader();
    ~vtkSegy2DReader();
protected:
    int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
                    vtkInformationVector* outputVector);
private:
    char *FileName;
    SegyReader reader;

private:
    vtkSegy2DReader(const vtkSegy2DReader&);  // Not implemented.
    void operator=(const vtkSegy2DReader&);  // Not implemented.


};


#endif //SEGYVISUALIZER2D_VTKSEGY2DREADER_H
