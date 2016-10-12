/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractPolyDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAbstractPolyDataReader
 * @brief   Superclass for algorithms that read
 * models from a file.
 *
 * This class allows to use a single base class to manage AbstractPolyData
 * reader classes in a uniform manner without needing to know the actual
 * type of the reader.
 * i.e. makes it possible to create maps to associate filename extension
 * and vtkAbstractPolyDataReader object.
 *
 * @sa
 * vtkOBJReader vtkPLYReader vtkSTLReader
*/

#ifndef vtkAbstractPolyDataReader_h
#define vtkAbstractPolyDataReader_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKIOCORE_EXPORT vtkAbstractPolyDataReader : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkAbstractPolyDataReader, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Specify file name of AbstractPolyData file (obj / ply / stl).
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

protected:
  vtkAbstractPolyDataReader();
  ~vtkAbstractPolyDataReader();

  char *FileName;
private:
  vtkAbstractPolyDataReader(const vtkAbstractPolyDataReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkAbstractPolyDataReader&) VTK_DELETE_FUNCTION;
};

#endif
