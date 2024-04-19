// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkXMLDataWriterHelper
 * @brief helper for vtkXMLWriter2 and subclasses
 *
 * vtkXMLDataWriterHelper is intended to be a helper class used by vtkXMLWriter2
 * and subclasses to use to write the XML to a file. Currently, this is simply a
 * subclass of vtkXMLWriter with some minimal API exposed. The ultimate goal is
 * to move logic to write XML data to this class and remove the reliance on
 * vtkXMLWriter. To get things off the ground quickly, this is kept as a
 * subclass of vtkXMLWriter for now.
 */

#ifndef vtkXMLDataWriterHelper_h
#define vtkXMLDataWriterHelper_h

#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkXMLWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCompositeDataSet;
class vtkXMLDataElement;
class vtkXMLWriter2;

class VTKIOPARALLELXML_EXPORT vtkXMLDataWriterHelper : public vtkXMLWriter
{
public:
  static vtkXMLDataWriterHelper* New();
  vtkTypeMacro(vtkXMLDataWriterHelper, vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the writer using this helper. Attributes from the writer are used
   * to control how data is written out.
   */
  void SetWriter(vtkXMLWriter2*);
  vtkGetObjectMacro(Writer, vtkXMLWriter2);
  ///@}

  ///@{
  /**
   * The writing process is split into 4 parts:
   * 1. `OpenFile` opens the file for writing.
   * 2. `BeginWriting` begins writing by adding header.
   * 3. Write the contents. API calls like `AddGlobalFieldData`, `AddXML` can be used for this.
   * 4. `EndWriting` ends the writing, adding any footers as needed.
   */
  bool OpenFile();
  bool BeginWriting();
  bool EndWriting();
  ///@}

  /**
   * Adds global field data for composite dataset.
   */
  bool AddGlobalFieldData(vtkCompositeDataSet* dataset);

  /**
   * Serialize and add an XML dom described by the `xmlElement` to the file.
   */
  bool AddXML(vtkXMLDataElement* xmlElement);

  ///@{
  /**
   * These methods should be used by the caller to provide necessary information
   * to the helper to generate a valid header. This is currently necessary since
   * we are making this class reused parts of vtkXMLWriter. When than dependence
   * is severed, this API will likely change.
   */
  void SetDataSetName(const std::string& name) { this->DataSetName = name; }
  void SetDataSetVersion(int major, int minor)
  {
    this->DataSetVersion[0] = major;
    this->DataSetVersion[1] = minor;
  }
  ///@}

  const char* GetDefaultFileExtension() override { return nullptr; }

protected:
  vtkXMLDataWriterHelper();
  ~vtkXMLDataWriterHelper() override;

  const char* GetDataSetName() override { return this->DataSetName.c_str(); }
  int GetDataSetMajorVersion() override { return this->DataSetVersion[0]; }
  int GetDataSetMinorVersion() override { return this->DataSetVersion[1]; }

private:
  vtkXMLDataWriterHelper(const vtkXMLDataWriterHelper&) = delete;
  void operator=(const vtkXMLDataWriterHelper&) = delete;

  vtkXMLWriter2* Writer;
  std::string DataSetName;
  int DataSetVersion[2];
};

VTK_ABI_NAMESPACE_END
#endif
