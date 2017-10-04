/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGaussianCubeReader2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGaussianCubeReader2
 * @brief   Read a Gaussian Cube file and output a
 * vtkMolecule object and a vtkImageData
 *
 *
 * @par Thanks:
 * Dr. Jean M. Favre who developed and contributed this class.
*/

#ifndef vtkGaussianCubeReader2_h
#define vtkGaussianCubeReader2_h

#include "vtkDomainsChemistryModule.h" // For export macro
#include "vtkMoleculeAlgorithm.h"

class vtkMolecule;
class vtkImageData;

class VTKDOMAINSCHEMISTRY_EXPORT vtkGaussianCubeReader2 : public vtkMoleculeAlgorithm
{
public:
  static vtkGaussianCubeReader2 *New();
  vtkTypeMacro(vtkGaussianCubeReader2,vtkMoleculeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the output (vtkMolecule) that the reader will fill
   */
  vtkMolecule *GetOutput();
  void SetOutput(vtkMolecule *) override;
  //@}

  /**
   * Get/Set the output (vtkImageData) that the reader will fill
   */
  vtkImageData *GetGridOutput();

  //@{
  /**
   * Get/Set the name of the CML file
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

protected:
  vtkGaussianCubeReader2();
  ~vtkGaussianCubeReader2() override;

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;
  int RequestInformation(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;
  int FillOutputPortInformation(int, vtkInformation*) override;

  char *FileName;
private:
  vtkGaussianCubeReader2(const vtkGaussianCubeReader2&) = delete;
  void operator=(const vtkGaussianCubeReader2&) = delete;
};

#endif
