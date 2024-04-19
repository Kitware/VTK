// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXYZMolReader2
 * @brief   read Molecular Data files
 *
 * vtkXYZMolReader2 is a source object that reads Molecule files
 * The reader will detect multiple timesteps in an XYZ molecule file.
 *
 * @par Thanks:
 * Dr. Jean M. Favre who developed and contributed this class
 */

#ifndef vtkXYZMolReader2_h
#define vtkXYZMolReader2_h

#include "vtkIOChemistryModule.h" // For export macro
#include "vtkMoleculeAlgorithm.h"

#include <istream> // for std::istream
#include <vector>  // for std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkMolecule;

class VTKIOCHEMISTRY_EXPORT vtkXYZMolReader2 : public vtkMoleculeAlgorithm
{
public:
  static vtkXYZMolReader2* New();
  vtkTypeMacro(vtkXYZMolReader2, vtkMoleculeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the output (vtkMolecule) that the reader will fill
   */
  vtkMolecule* GetOutput();
  void SetOutput(vtkMolecule*) override;
  ///@}

  ///@{
  /**
   * Get/Set the name of the XYZ Molecule file
   */
  vtkSetStdStringFromCharMacro(FileName);
  vtkGetCharFromStdStringMacro(FileName);
  ///@}

protected:
  vtkXYZMolReader2();
  ~vtkXYZMolReader2() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  std::string FileName;
  std::vector<istream::pos_type> FilePositions; // to store beginning of each step
  std::vector<double> TimeSteps;

private:
  vtkXYZMolReader2(const vtkXYZMolReader2&) = delete;
  void operator=(const vtkXYZMolReader2&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
