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
/**
 * @class   vtkGaussianCubeReader
 * @brief   read ASCII Gaussian Cube Data files
 *
 * vtkGaussianCubeReader is a source object that reads ASCII files following
 * the description in http://www.gaussian.com/00000430.htm
 * The FileName must be specified.
 *
 * @par Thanks:
 * Dr. Jean M. Favre who developed and contributed this class.
*/

#ifndef vtkGaussianCubeReader_h
#define vtkGaussianCubeReader_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkMoleculeReaderBase.h"

class vtkImageData;
class vtkTransform;

class VTKIOGEOMETRY_EXPORT vtkGaussianCubeReader : public vtkMoleculeReaderBase
{
public:
  static vtkGaussianCubeReader *New();
  vtkTypeMacro(vtkGaussianCubeReader,vtkMoleculeReaderBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkGetObjectMacro(Transform,vtkTransform);
  vtkImageData *GetGridOutput();

protected:
  vtkGaussianCubeReader();
  ~vtkGaussianCubeReader() override;

  vtkTransform *Transform;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  void ReadSpecificMolecule(FILE* fp) override;

  int FillOutputPortInformation(int, vtkInformation*) override;
private:
  vtkGaussianCubeReader(const vtkGaussianCubeReader&) = delete;
  void operator=(const vtkGaussianCubeReader&) = delete;
};

#endif
