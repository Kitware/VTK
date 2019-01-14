/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCMLMoleculeReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCMLMoleculeReader
 * @brief   Read a CML file and output a
 * vtkMolecule object
 *
*/

#ifndef vtkCMLMoleculeReader_h
#define vtkCMLMoleculeReader_h

#include "vtkDomainsChemistryModule.h" // For export macro
#include "vtkMoleculeAlgorithm.h"

class vtkMolecule;

class VTKDOMAINSCHEMISTRY_EXPORT vtkCMLMoleculeReader : public vtkMoleculeAlgorithm
{
public:
  static vtkCMLMoleculeReader *New();
  vtkTypeMacro(vtkCMLMoleculeReader,vtkMoleculeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the output (vtkMolecule) that the reader will fill
   */
  vtkMolecule *GetOutput();
  void SetOutput(vtkMolecule *) override;
  //@}

  //@{
  /**
   * Get/Set the name of the CML file
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

protected:
  vtkCMLMoleculeReader();
  ~vtkCMLMoleculeReader() override;

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;
  int FillOutputPortInformation(int, vtkInformation*) override;

  char *FileName;

private:
  vtkCMLMoleculeReader(const vtkCMLMoleculeReader&) = delete;
  void operator=(const vtkCMLMoleculeReader&) = delete;
};

#endif
