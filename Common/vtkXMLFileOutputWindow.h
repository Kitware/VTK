/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLFileOutputWindow.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
//   DisplayDebugText - <Debug.
// 
// The method DisplayTag outputs the text unprocessed.  To use this
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
