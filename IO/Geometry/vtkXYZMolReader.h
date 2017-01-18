/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXYZMolReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXYZMolReader
 * @brief   read Molecular Data files
 *
 * vtkXYZMolReader is a source object that reads Molecule files
 * The FileName must be specified
 *
 * @par Thanks:
 * Dr. Jean M. Favre who developed and contributed this class
*/

#ifndef vtkXYZMolReader_h
#define vtkXYZMolReader_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkMoleculeReaderBase.h"


class VTKIOGEOMETRY_EXPORT vtkXYZMolReader : public vtkMoleculeReaderBase
{
public:
  vtkTypeMacro(vtkXYZMolReader,vtkMoleculeReaderBase);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static vtkXYZMolReader *New();

  /**
   * Test whether the file with the given name can be read by this
   * reader.
   */
  virtual int CanReadFile(const char* name);

  //@{
  /**
   * Set the current time step. It should be greater than 0 and smaller than
   * MaxTimeStep.
   */
  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);
  //@}

  //@{
  /**
   * Get the maximum time step.
   */
  vtkGetMacro(MaxTimeStep, int);
  //@}

protected:
  vtkXYZMolReader();
  ~vtkXYZMolReader() VTK_OVERRIDE;

  void ReadSpecificMolecule(FILE* fp) VTK_OVERRIDE;

  /**
   * Get next line that is not a comment. It returns the beginning of data on
   * line (skips empty spaces)
   */
  char* GetNextLine(FILE* fp, char* line, int maxlen);

  int GetLine1(const char* line, int *cnt);
  int GetLine2(const char* line, char *name);
  int GetAtom(const char* line, char* atom, float *x);

  void InsertAtom(const char* atom, float *pos);

  vtkSetMacro(MaxTimeStep, int);

  int TimeStep;
  int MaxTimeStep;

private:
  vtkXYZMolReader(const vtkXYZMolReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXYZMolReader&) VTK_DELETE_FUNCTION;
};

#endif
