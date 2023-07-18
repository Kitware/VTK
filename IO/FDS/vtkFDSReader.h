/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFDSReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkFDSReader_h
#define vtkFDSReader_h

#include "vtkIOFDSModule.h" // For export macro
#include "vtkPartitionedDataSetCollectionAlgorithm.h"

#include <string>

VTK_ABI_NAMESPACE_BEGIN

/**
 * @class vtkFDSReader
 *
 * TODO
 */
class VTKIOFDS_EXPORT vtkFDSReader : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkFDSReader* New();
  vtkTypeMacro(vtkFDSReader, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specifies the name of the .smv file to be loaded.
   */
  vtkSetMacro(FileName, std::string);
  vtkGetMacro(FileName, std::string);
  ///@}

protected:
  vtkFDSReader();
  ~vtkFDSReader() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkFDSReader(const vtkFDSReader&) = delete;
  void operator=(const vtkFDSReader&) = delete;

  std::string FileName;
};

VTK_ABI_NAMESPACE_END

#endif
