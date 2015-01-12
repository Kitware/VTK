/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIVWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkIVWriter - export polydata into OpenInventor 2.0 format.
// .SECTION Description
// vtkIVWriter is a concrete subclass of vtkWriter that writes OpenInventor 2.0
// files.
//
// .SECTION See Also
// vtkPolyDataWriter


#ifndef vtkIVWriter_h
#define vtkIVWriter_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkWriter.h"

class vtkPolyData;

class VTKIOGEOMETRY_EXPORT vtkIVWriter : public vtkWriter
{
public:
  static vtkIVWriter *New();
  vtkTypeMacro(vtkIVWriter,vtkWriter);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the input to this writer.
  vtkPolyData* GetInput();
  vtkPolyData* GetInput(int port);

  // Description:
  // Specify file name of vtk polygon data file to write.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkIVWriter()
    {
    this->FileName = NULL;
    }

  ~vtkIVWriter()
    {
    delete[] this->FileName;
    }

  void WriteData();
  void WritePolyData(vtkPolyData *polyData, FILE *fp);

  char *FileName;

  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkIVWriter(const vtkIVWriter&);  // Not implemented.
  void operator=(const vtkIVWriter&);  // Not implemented.
};

#endif

