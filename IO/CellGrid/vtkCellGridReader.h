/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellGridReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkCellGridReader
 * @brief   Read a cell-grid file.
 *
 * vtkCellGridReader is a concrete subclass of vtkCellGridAlgorithm that reads data
 * into a vtkCellGrid instance (which is really just a collection of array groups).
 *
 * @sa
 * vtkCellGridAlgorithm
 * vtkCellGridImporter
 */

#ifndef vtkCellGridReader_h
#define vtkCellGridReader_h

#include "vtkCellGridAlgorithm.h"
#include "vtkIOCellGridModule.h" // For export macro
#include "vtkSmartPointer.h"     // For SmartPointer

#include <string> // For std::string
#include <vector> // For std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArraySelection;
class vtkFieldData;
class vtkCellGridDocumentLoader;
class vtkImageData;
class vtkStringArray;

class VTKIOCELLGRID_EXPORT vtkCellGridReader : public vtkCellGridAlgorithm
{
public:
  static vtkCellGridReader* New();
  vtkTypeMacro(vtkCellGridReader, vtkCellGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the name of the file from which to read data.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

protected:
  vtkCellGridReader();
  ~vtkCellGridReader() override;

  char* FileName = nullptr;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkCellGridReader(const vtkCellGridReader&) = delete;
  void operator=(const vtkCellGridReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
