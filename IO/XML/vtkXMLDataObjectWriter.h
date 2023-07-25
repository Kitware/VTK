// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLDataObjectWriter
 * @brief   Write any type of VTK XML file.
 *
 * vtkXMLDataObjectWriter is a wrapper around the VTK XML file format
 * writers.  Given an input vtkDataSet, the correct writer is
 * automatically selected based on the type of input.
 *
 * @sa
 * vtkXMLImageDataWriter vtkXMLStructuredGridWriter
 * vtkXMLRectilinearGridWriter vtkXMLPolyDataWriter
 * vtkXMLUnstructuredGridWriter
 */

#ifndef vtkXMLDataObjectWriter_h
#define vtkXMLDataObjectWriter_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCallbackCommand;

class VTKIOXML_EXPORT vtkXMLDataObjectWriter : public vtkXMLWriter
{
public:
  vtkTypeMacro(vtkXMLDataObjectWriter, vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLDataObjectWriter* New();

  /**
   * Get/Set the writer's input.
   */
  vtkDataSet* GetInput();

  /**
   * Creates a writer for the given dataset type. May return nullptr for
   * unsupported/unrecognized dataset types. Returns a new instance. The caller
   * is responsible of calling vtkObject::Delete() or vtkObject::UnRegister() on
   * it when done.
   */
  static vtkXMLWriter* NewWriter(int dataset_type);

protected:
  vtkXMLDataObjectWriter();
  ~vtkXMLDataObjectWriter() override;

  // see algorithm for more info
  int FillInputPortInformation(int port, vtkInformation* info) override;

  // Override writing method from superclass.
  int WriteInternal() override;

  // Dummies to satisfy pure virtuals from superclass.
  const char* GetDataSetName() override;
  const char* GetDefaultFileExtension() override;

  // Callback registered with the InternalProgressObserver.
  static void ProgressCallbackFunction(vtkObject*, unsigned long, void*, void*);
  // Progress callback from internal writer.
  virtual void ProgressCallback(vtkAlgorithm* w);

  // The observer to report progress from the internal writer.
  vtkCallbackCommand* InternalProgressObserver;

private:
  vtkXMLDataObjectWriter(const vtkXMLDataObjectWriter&) = delete;
  void operator=(const vtkXMLDataObjectWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
