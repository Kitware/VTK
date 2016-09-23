/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLDataSetWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLDataSetWriter
 * @brief   Write any type of VTK XML file.
 *
 * vtkXMLDataSetWriter is a wrapper around the VTK XML file format
 * writers.  Given an input vtkDataSet, the correct writer is
 * automatically selected based on the type of input.
 *
 * @sa
 * vtkXMLImageDataWriter vtkXMLStructuredGridWriter
 * vtkXMLRectilinearGridWriter vtkXMLPolyDataWriter
 * vtkXMLUnstructuredGridWriter
*/

#ifndef vtkXMLDataSetWriter_h
#define vtkXMLDataSetWriter_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLWriter.h"

class vtkCallbackCommand;

class VTKIOXML_EXPORT vtkXMLDataSetWriter : public vtkXMLWriter
{
public:
  vtkTypeMacro(vtkXMLDataSetWriter,vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLDataSetWriter* New();

  /**
   * Get/Set the writer's input.
   */
  vtkDataSet* GetInput();

  /**
   * Creates a writer for the given dataset type. May return NULL for
   * unsupported/unrecognized dataset types. Returns a new instance. The caller
   * is responsible of calling vtkObject::Delete() or vtkObject::UnRegister() on
   * it when done.
   */
  static vtkXMLWriter* NewWriter(int dataset_type);

protected:
  vtkXMLDataSetWriter();
  ~vtkXMLDataSetWriter();

  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Override writing method from superclass.
  virtual int WriteInternal();

  // Dummies to satisfy pure virtuals from superclass.
  const char* GetDataSetName();
  const char* GetDefaultFileExtension();

  // Callback registered with the ProgressObserver.
  static void ProgressCallbackFunction(vtkObject*, unsigned long, void*,
                                       void*);
  // Progress callback from internal writer.
  virtual void ProgressCallback(vtkAlgorithm* w);

  // The observer to report progress from the internal writer.
  vtkCallbackCommand* ProgressObserver;


private:
  vtkXMLDataSetWriter(const vtkXMLDataSetWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLDataSetWriter&) VTK_DELETE_FUNCTION;
};

#endif
