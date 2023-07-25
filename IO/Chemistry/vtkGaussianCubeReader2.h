// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkIOChemistryModule.h" // For export macro
#include "vtkMoleculeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMolecule;
class vtkImageData;

class VTKIOCHEMISTRY_EXPORT vtkGaussianCubeReader2 : public vtkMoleculeAlgorithm
{
public:
  static vtkGaussianCubeReader2* New();
  vtkTypeMacro(vtkGaussianCubeReader2, vtkMoleculeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the output (vtkMolecule) that the reader will fill
   */
  vtkMolecule* GetOutput();
  void SetOutput(vtkMolecule*) override;
  ///@}

  /**
   * Get/Set the output (vtkImageData) that the reader will fill
   */
  vtkImageData* GetGridOutput();

  ///@{
  /**
   * Get/Set the name of the CML file
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

protected:
  vtkGaussianCubeReader2();
  ~vtkGaussianCubeReader2() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillOutputPortInformation(int, vtkInformation*) override;

  char* FileName;

private:
  vtkGaussianCubeReader2(const vtkGaussianCubeReader2&) = delete;
  void operator=(const vtkGaussianCubeReader2&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
