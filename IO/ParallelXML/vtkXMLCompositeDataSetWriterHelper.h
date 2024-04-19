// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkXMLCompositeDataSetWriterHelper
 * @brief a helper class used by vtkXMLWriter2 subclasses that write composite
 *        datasets.
 *
 * vtkXMLCompositeDataSetWriterHelper is a helper class intended to be used by
 * subclasses of vtkXMLWriter2 that want to write composite datasets. It
 * consolidates the logic to write individual datasets for leaf nodes into
 * separate files.
 */

#ifndef vtkXMLCompositeDataSetWriterHelper_h
#define vtkXMLCompositeDataSetWriterHelper_h

#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkObject.h"
#include "vtkSmartPointer.h" // for vtkSmartPointer

#include <map>    // for std::map
#include <string> // for std::string

VTK_ABI_NAMESPACE_BEGIN
class vtkXMLWriterBase;
class vtkDataObject;

class VTKIOPARALLELXML_EXPORT vtkXMLCompositeDataSetWriterHelper : public vtkObject
{
public:
  static vtkXMLCompositeDataSetWriterHelper* New();
  vtkTypeMacro(vtkXMLCompositeDataSetWriterHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the writer using this helper. This is reference counted. So caller
   * should avoid cycles explicitly.
   */
  void SetWriter(vtkXMLWriterBase* writer);
  vtkGetObjectMacro(Writer, vtkXMLWriterBase);
  ///@}

  /**
   * Write a specific dataset to a file. The dataset cannot be a composite
   * dataset. The implementation uses `vtkXMLDataObjectWriter` to find a writer
   * to use. Internally, writers are cached and will be reused when same type of
   * data is being written out multiple times.
   *
   * The filename is created using the `path` and `prefix`. The prefix is
   * extended with a `.<ext>` where the `<ext>` is dictated by the writer used.
   * `vtkXMLWriterBase::GetDefaultFileExtension` is used to obtain the
   * extension to use for the file written out.
   *
   * On success, returns `<prefix>.<ext>`, otherwise an empty string is
   * returned.
   */
  std::string WriteDataSet(const std::string& path, const std::string& prefix, vtkDataObject* data);

protected:
  vtkXMLCompositeDataSetWriterHelper();
  ~vtkXMLCompositeDataSetWriterHelper() override;

  /**
   * Method to obtain a writer for the given data type. Either a new writer is
   * created or one from the cache may be used.
   */
  vtkXMLWriterBase* GetWriter(int dataType);

private:
  vtkXMLCompositeDataSetWriterHelper(const vtkXMLCompositeDataSetWriterHelper&) = delete;
  void operator=(const vtkXMLCompositeDataSetWriterHelper&) = delete;

  std::map<int, vtkSmartPointer<vtkXMLWriterBase>> WriterCache;
  vtkXMLWriterBase* Writer;
};

VTK_ABI_NAMESPACE_END
#endif
