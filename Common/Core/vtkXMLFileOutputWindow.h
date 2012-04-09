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
// .NAME vtkXMLFileOutputWindow - XML File Specific output window class
// .SECTION Description
// Writes debug/warning/error output to an XML file. Uses prefined XML
// tags for each text display method. The text is processed to replace
// XML markup characters.
// 
//   DisplayText - <Text>
// 
//   DisplayErrorText - <Error>
// 
//   DisplayWarningText - <Warning>
// 
//   DisplayGenericWarningText - <GenericWarning>
// 
//   DisplayDebugText - <Debug>
// 
// The method DisplayTag outputs the text unprocessed. To use this
// class, instantiate it and then call SetInstance(this).


#ifndef __vtkXMLFileOutputWindow_h
#define __vtkXMLFileOutputWindow_h

#include "vtkFileOutputWindow.h"


class VTK_COMMON_EXPORT vtkXMLFileOutputWindow : public vtkFileOutputWindow
{
public:
  vtkTypeMacro(vtkXMLFileOutputWindow, vtkFileOutputWindow);

  static vtkXMLFileOutputWindow* New();

  // Description:
  // Put the text into the log file. The text is processed to
  // replace &, <, > with &amp, &lt, and &gt.
  // Each display method outputs a different XML tag.
  virtual void DisplayText(const char*);
  virtual void DisplayErrorText(const char*);
  virtual void DisplayWarningText(const char*);
  virtual void DisplayGenericWarningText(const char*);
  virtual void DisplayDebugText(const char*);

  // Description:
  // Put the text into the log file without processing it.
  virtual void DisplayTag(const char*);

protected:
  vtkXMLFileOutputWindow() {}; 
  virtual ~vtkXMLFileOutputWindow() {}; 

  void Initialize();
  virtual void DisplayXML(const char*, const char*);

private:
  vtkXMLFileOutputWindow(const vtkXMLFileOutputWindow&);  // Not implemented.
  void operator=(const vtkXMLFileOutputWindow&);  // Not implemented.
};



#endif
