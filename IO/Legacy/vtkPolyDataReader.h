/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPolyDataReader
 * @brief   read vtk polygonal data file
 *
 * vtkPolyDataReader is a source object that reads ASCII or binary
 * polygonal data files in vtk format (see text for format details).
 * The output of this reader is a single vtkPolyData data object.
 * The superclass of this class, vtkDataReader, provides many methods for
 * controlling the reading of the data file, see vtkDataReader for more
 * information.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * @sa
 * vtkPolyData vtkDataReader
*/

#ifndef vtkPolyDataReader_h
#define vtkPolyDataReader_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataReader.h"

class vtkPolyData;

class VTKIOLEGACY_EXPORT vtkPolyDataReader : public vtkDataReader
{
public:
  static vtkPolyDataReader *New();
  vtkTypeMacro(vtkPolyDataReader,vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get the output of this reader.
   */
  vtkPolyData *GetOutput();
  vtkPolyData *GetOutput(int idx);
  void SetOutput(vtkPolyData *output);
  //@}

protected:
  vtkPolyDataReader();
  ~vtkPolyDataReader() VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) VTK_OVERRIDE;

  // Update extent of PolyData is specified in pieces.
  // Since all DataObjects should be able to set UpdateExent as pieces,
  // just copy output->UpdateExtent  all Inputs.
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;

  int FillOutputPortInformation(int, vtkInformation*) VTK_OVERRIDE;

private:
  vtkPolyDataReader(const vtkPolyDataReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPolyDataReader&) VTK_DELETE_FUNCTION;
};

#endif
