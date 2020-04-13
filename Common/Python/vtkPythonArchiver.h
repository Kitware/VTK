/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArchiver.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPythonArchiver
 * @brief   A version of vtkArchiver that can be implemented in Python
 *
 * vtkPythonArchiver is an implementation of vtkArchiver that calls a Python
 * object to do the actual work.
 * It defers the following methods to Python:
 * - OpenArchive()
 * - CloseArchive()
 * - InsertIntoArchive()
 * - Contains()
 *
 * Python signature of these methods is as follows:
 * - OpenArchive(self, vtkself) : vtkself is the vtk object
 * - CloseArchive(self, vtkself)
 * - InsertIntoArchive(self, vtkself, relativePath, data, size)
 * - Contains()
 *
 * @sa
 * vtkPythonArchiver
 */

#ifndef vtkPythonArchiver_h
#define vtkPythonArchiver_h
#if !defined(__VTK_WRAP__) || defined(__VTK_WRAP_HIERARCHY__) || defined(__VTK_WRAP_PYTHON__)

#include "vtkPython.h" // Must be first

#include "vtkArchiver.h"
#include "vtkCommonPythonModule.h" // For export macro

class vtkSmartPyObject;

class VTKCOMMONPYTHON_EXPORT vtkPythonArchiver : public vtkArchiver
{
public:
  static vtkPythonArchiver* New();
  vtkTypeMacro(vtkPythonArchiver, vtkArchiver);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Specify the Python object to use to perform the archiving. A reference will
   * be taken on the object.
   */
  void SetPythonObject(PyObject* obj);

  //@{
  /**
   * Open the arhive for writing.
   */
  void OpenArchive() override;
  //@}

  //@{
  /**
   * Close the arhive.
   */
  void CloseArchive() override;
  //@}

  //@{
  /**
   * Insert \p data of size \p size into the archive at \p relativePath.
   */
  void InsertIntoArchive(
    const std::string& relativePath, const char* data, std::size_t size) override;
  //@}

  //@{
  /**
   * Checks if \p relativePath represents an entry in the archive.
   */
  bool Contains(const std::string& relativePath) override;
  //@}

protected:
  vtkPythonArchiver();
  ~vtkPythonArchiver() override;

private:
  vtkPythonArchiver(const vtkPythonArchiver&) = delete;
  void operator=(const vtkPythonArchiver&) = delete;

  int CheckResult(const char* method, const vtkSmartPyObject& res);

  PyObject* Object;
};

#endif
#endif
