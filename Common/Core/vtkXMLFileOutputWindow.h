/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLFileOutputWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLFileOutputWindow
 * @brief   XML File Specific output window class
 *
 * Writes debug/warning/error output to an XML file. Uses prefined XML
 * tags for each text display method. The text is processed to replace
 * XML markup characters.
 *
 *   DisplayText - <Text>
 *
 *   DisplayErrorText - <Error>
 *
 *   DisplayWarningText - <Warning>
 *
 *   DisplayGenericWarningText - <GenericWarning>
 *
 *   DisplayDebugText - <Debug>
 *
 * The method DisplayTag outputs the text unprocessed. To use this
 * class, instantiate it and then call SetInstance(this).
*/

#ifndef vtkXMLFileOutputWindow_h
#define vtkXMLFileOutputWindow_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkFileOutputWindow.h"


class VTKCOMMONCORE_EXPORT vtkXMLFileOutputWindow : public vtkFileOutputWindow
{
public:
  vtkTypeMacro(vtkXMLFileOutputWindow, vtkFileOutputWindow);

  static vtkXMLFileOutputWindow* New();

  //@{
  /**
   * Put the text into the log file. The text is processed to
   * replace &, <, > with &amp, &lt, and &gt.
   * Each display method outputs a different XML tag.
   */
  void DisplayText(const char*) VTK_OVERRIDE;
  void DisplayErrorText(const char*) VTK_OVERRIDE;
  void DisplayWarningText(const char*) VTK_OVERRIDE;
  void DisplayGenericWarningText(const char*) VTK_OVERRIDE;
  void DisplayDebugText(const char*) VTK_OVERRIDE;
  //@}

  /**
   * Put the text into the log file without processing it.
   */
  virtual void DisplayTag(const char*);

protected:
  vtkXMLFileOutputWindow() {}
  ~vtkXMLFileOutputWindow() VTK_OVERRIDE {}

  void Initialize();
  virtual void DisplayXML(const char*, const char*);

private:
  vtkXMLFileOutputWindow(const vtkXMLFileOutputWindow&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLFileOutputWindow&) VTK_DELETE_FUNCTION;
};



#endif
// VTK-HeaderTest-Exclude: vtkXMLFileOutputWindow.h
