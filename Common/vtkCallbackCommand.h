/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCallbackCommand.h
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
// .NAME vtkCallbackCommand - generic callback

#ifndef __vtkCallbackCommand_h
#define __vtkCallbackCommand_h

#include "vtkCommand.h"

// a good command to use for generic function callbacks
// the function should have the format 
// void func(vtkObject *,void *clientdata, void *calldata)
class VTK_COMMON_EXPORT vtkCallbackCommand : public vtkCommand
{
public:
  static vtkCallbackCommand *New() 
    {return new vtkCallbackCommand;};

  void SetClientData(void *cd) 
    {this->ClientData = cd;};
  void* GetClientData()
    {return this->ClientData; }
  void SetCallback(void (*f)(vtkObject *, unsigned long, void *, void *)) 
    {this->Callback = f;};
  void SetClientDataDeleteCallback(void (*f)(void *))
    {this->ClientDataDeleteCallback = f;};
  
  void Execute(vtkObject *caller, unsigned long event, void *callData);

  void *ClientData;
  void (*Callback)(vtkObject *, unsigned long, void *, void *);
  void (*ClientDataDeleteCallback)(void *);
protected:
  vtkCallbackCommand();
  ~vtkCallbackCommand();
};



#endif /* __vtkCallbackCommand_h */
 
