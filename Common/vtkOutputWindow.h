/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutputWindow.h
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
// .NAME vtkOutputWindow - base class for writing debug output to a console
// .SECTION Description
// This class is used to encapsulate all text output, so that it will work
// with operating systems that have a stdout and stderr, and ones that
// do not.  (i.e windows does not).  Sub-classes can be provided which can
// redirect the output to a window.

#ifndef __vtkOutputWindow_h
#define __vtkOutputWindow_h

#include "vtkObject.h"

//BTX

class VTK_EXPORT vtkOutputWindow;

class VTK_EXPORT vtkOutputWindowSmartPointer
{
public:
  vtkOutputWindowSmartPointer(vtkOutputWindow* p) { this->Pointer=p; };
  void SetPointer(vtkOutputWindow* obj)
    {
      this->Pointer = obj;
    }
  ~vtkOutputWindowSmartPointer();
private:
  vtkOutputWindow* Pointer;
};
//ETX

class VTK_EXPORT vtkOutputWindow : public vtkObject
{
public:
// Methods from vtkObject
  vtkTypeMacro(vtkOutputWindow,vtkObject);
  // Description:
  // Print ObjectFactor to stream.
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is a singleton pattern New.  There will only be ONE
  // reference to a vtkOutputWindow object per process.  Clients that
  // call this must call Delete on the object so that the reference
  // counting will work.   The single instance will be unreferenced when
  // the program exits.
  static vtkOutputWindow* New();
  // Description:
  // Return the singleton instance with no reference counting.
  static vtkOutputWindow* GetInstance();
  // Description:
  // Supply a user defined output window. Call ->Delete() on the supplied
  // instance after setting it.
  static void SetInstance(vtkOutputWindow *instance);
  // Description:
  // Display the text. Four virtual methods exist, depending on the type of
  // message to display. This allows redirection or reformatting of the
  // messages. The default implementation uses DisplayText for all.
  virtual void DisplayText(const char*);
  virtual void DisplayErrorText(const char*);
  virtual void DisplayWarningText(const char*);
  virtual void DisplayGenericWarningText(const char*);

  virtual void DisplayDebugText(const char*);
  // Description:
  // If PromptUser is set to true then each time a line of text
  // is displayed, the user is asked if they want to keep getting
  // messages.
  vtkBooleanMacro(PromptUser,int);
  vtkSetMacro(PromptUser, int);
//BTX
  // use this as a way of memory management when the
  // program exits the SmartPointer will be deleted which
  // will delete the Instance singleton
  static vtkOutputWindowSmartPointer SmartPointer;
//ETX
protected:
  vtkOutputWindow();
  virtual ~vtkOutputWindow();
  vtkOutputWindow(const vtkOutputWindow&);
  void operator=(const vtkOutputWindow&);
  int PromptUser;
private:
  static vtkOutputWindow* Instance;
};

#endif
