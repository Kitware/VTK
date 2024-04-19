// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLFileOutputWindow
 * @brief   XML File Specific output window class
 *
 * Writes debug/warning/error output to an XML file. Uses prefined XML
 * tags for each text display method. The text is processed to replace
 * XML markup characters.
 *
 *   DisplayText - \<Text\>
 *
 *   DisplayErrorText - \<Error\>
 *
 *   DisplayWarningText - \<Warning\>
 *
 *   DisplayGenericWarningText - \<GenericWarning\>
 *
 *   DisplayDebugText - \<Debug\>
 *
 * The method DisplayTag outputs the text unprocessed. To use this
 * class, instantiate it and then call SetInstance(this).
 */

#ifndef vtkXMLFileOutputWindow_h
#define vtkXMLFileOutputWindow_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkFileOutputWindow.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkXMLFileOutputWindow : public vtkFileOutputWindow
{
public:
  vtkTypeMacro(vtkXMLFileOutputWindow, vtkFileOutputWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkXMLFileOutputWindow* New();

  ///@{
  /**
   * Put the text into the log file. The text is processed to
   * replace &, <, > with &amp, &lt, and &gt.
   * Each display method outputs a different XML tag.
   */
  void DisplayText(const char*) override;
  void DisplayErrorText(const char*) override;
  void DisplayWarningText(const char*) override;
  void DisplayGenericWarningText(const char*) override;
  void DisplayDebugText(const char*) override;
  ///@}

  /**
   * Put the text into the log file without processing it.
   */
  virtual void DisplayTag(const char*);

protected:
  vtkXMLFileOutputWindow() = default;
  ~vtkXMLFileOutputWindow() override = default;

  void Initialize();
  virtual void DisplayXML(const char*, const char*);

private:
  vtkXMLFileOutputWindow(const vtkXMLFileOutputWindow&) = delete;
  void operator=(const vtkXMLFileOutputWindow&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
