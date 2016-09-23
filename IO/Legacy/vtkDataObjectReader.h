/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDataObjectReader
 * @brief   read vtk field data file
 *
 * vtkDataObjectReader is a source object that reads ASCII or binary field
 * data files in vtk format. Fields are general matrix structures used
 * represent complex data. (See text for format details).  The output of this
 * reader is a single vtkDataObject.  The superclass of this class,
 * vtkDataReader, provides many methods for controlling the reading of the
 * data file, see vtkDataReader for more information.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * @sa
 * vtkFieldData vtkDataObjectWriter
*/

#ifndef vtkDataObjectReader_h
#define vtkDataObjectReader_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataReader.h"

class vtkDataObject;

class VTKIOLEGACY_EXPORT vtkDataObjectReader : public vtkDataReader
{
public:
  static vtkDataObjectReader *New();
  vtkTypeMacro(vtkDataObjectReader,vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Get the output field of this reader.
   */
  vtkDataObject *GetOutput();
  vtkDataObject *GetOutput(int idx);
  void SetOutput(vtkDataObject *);
  //@}

protected:
  vtkDataObjectReader();
  ~vtkDataObjectReader();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  virtual int FillOutputPortInformation(int, vtkInformation*);
private:
  vtkDataObjectReader(const vtkDataObjectReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDataObjectReader&) VTK_DELETE_FUNCTION;
};

#endif
