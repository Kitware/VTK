/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGaussianCubeReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGaussianCubeReader - read ASCII Gaussian Cube Data files
// .SECTION Description
// vtkGaussianCubeReader is a source object that reads ASCII files following
// the description in http://www.gaussian.com/00000430.htm
// The FileName must be specified.
//
// .SECTION Thanks
// Dr. Jean M. Favre who developed and contributed this class.

#ifndef __vtkGaussianCubeReader_h
#define __vtkGaussianCubeReader_h

#include "vtkMoleculeReaderBase.h"

class vtkImageData;
class vtkTransform;

class VTK_IO_EXPORT vtkGaussianCubeReader : public vtkMoleculeReaderBase 
{
public:
  static vtkGaussianCubeReader *New();
  vtkTypeRevisionMacro(vtkGaussianCubeReader,vtkMoleculeReaderBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkGetObjectMacro(Transform,vtkTransform);
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  vtkImageData *GetGridOutput();

protected:
  vtkGaussianCubeReader();
  ~vtkGaussianCubeReader();

  char *FileName;
  vtkTransform *Transform;

  void Execute();
  
  void ReadSpecificMolecule(FILE* fp);

private:
  vtkGaussianCubeReader(const vtkGaussianCubeReader&);  // Not implemented.
  void operator=(const vtkGaussianCubeReader&);  // Not implemented.
};

#endif
