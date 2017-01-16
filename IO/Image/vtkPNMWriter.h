/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNMWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPNMWriter
 * @brief   Writes PNM (portable any map)  files.
 *
 * vtkPNMWriter writes PNM file. The data type
 * of the file is unsigned char regardless of the input type.
*/

#ifndef vtkPNMWriter_h
#define vtkPNMWriter_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageWriter.h"

class VTKIOIMAGE_EXPORT vtkPNMWriter : public vtkImageWriter
{
public:
  static vtkPNMWriter *New();
  vtkTypeMacro(vtkPNMWriter,vtkImageWriter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkPNMWriter() {}
  ~vtkPNMWriter() VTK_OVERRIDE {}

  void WriteFile(
    ofstream *file, vtkImageData *data, int extent[6], int wExt[6]) VTK_OVERRIDE;
  void WriteFileHeader(
    ofstream *, vtkImageData *, int wExt[6]) VTK_OVERRIDE;
private:
  vtkPNMWriter(const vtkPNMWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPNMWriter&) VTK_DELETE_FUNCTION;
};

#endif


