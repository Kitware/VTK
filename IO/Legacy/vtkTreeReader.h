/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTreeReader
 * @brief   read vtkTree data file
 *
 * vtkTreeReader is a source object that reads ASCII or binary
 * vtkTree data files in vtk format. (see text for format details).
 * The output of this reader is a single vtkTree data object.
 * The superclass of this class, vtkDataReader, provides many methods for
 * controlling the reading of the data file, see vtkDataReader for more
 * information.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * @sa
 * vtkTree vtkDataReader vtkTreeWriter
*/

#ifndef vtkTreeReader_h
#define vtkTreeReader_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataReader.h"

class vtkTree;

class VTKIOLEGACY_EXPORT vtkTreeReader : public vtkDataReader
{
public:
  static vtkTreeReader *New();
  vtkTypeMacro(vtkTreeReader,vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get the output of this reader.
   */
  vtkTree *GetOutput();
  vtkTree *GetOutput(int idx);
  void SetOutput(vtkTree *output);
  //@}

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(const std::string& fname,
                     vtkDataObject* output) override;

protected:
  vtkTreeReader();
  ~vtkTreeReader() override;

  int FillOutputPortInformation(int, vtkInformation*) override;
private:
  vtkTreeReader(const vtkTreeReader&) = delete;
  void operator=(const vtkTreeReader&) = delete;
};

#endif
