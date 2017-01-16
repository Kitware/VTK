/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimplePointsReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSimplePointsReader
 * @brief   Read a list of points from a file.
 *
 * vtkSimplePointsReader is a source object that reads a list of
 * points from a file.  Each point is specified by three
 * floating-point values in ASCII format.  There is one point per line
 * of the file.  A vertex cell is created for each point in the
 * output.  This reader is meant as an example of how to write a
 * reader in VTK.
*/

#ifndef vtkSimplePointsReader_h
#define vtkSimplePointsReader_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKIOLEGACY_EXPORT vtkSimplePointsReader : public vtkPolyDataAlgorithm
{
public:
  static vtkSimplePointsReader* New();
  vtkTypeMacro(vtkSimplePointsReader,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the name of the file from which to read points.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

protected:
  vtkSimplePointsReader();
  ~vtkSimplePointsReader() VTK_OVERRIDE;

  char* FileName;

  int RequestData(vtkInformation*,
                  vtkInformationVector**,
                  vtkInformationVector*) VTK_OVERRIDE;
private:
  vtkSimplePointsReader(const vtkSimplePointsReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSimplePointsReader&) VTK_DELETE_FUNCTION;
};

#endif
