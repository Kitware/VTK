/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFileOutputWindow.cxx
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
#include "vtkFileOutputWindow.h"
#include "vtkObjectFactory.h"

vtkFileOutputWindow* vtkFileOutputWindow::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkFileOutputWindow");
  if(ret)
    {
    return (vtkFileOutputWindow*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkFileOutputWindow;
}

vtkFileOutputWindow::vtkFileOutputWindow()
{
  this->OStream = NULL;
  this->FileName = NULL;
  this->Append = 0;
  this->Flush = 0;
}

vtkFileOutputWindow::~vtkFileOutputWindow()
{
  if (this->FileName) 
    {
    delete[] this->FileName;
    }
  if (this->OStream)
    {
    delete this->OStream;
    }
}

void vtkFileOutputWindow::Initialize() 
{
  if (!this->OStream)
    {
    if (!this->FileName)
      {
      char* fileName = (char *) "vtkMessageLog.log";
      this->FileName = new char[strlen(fileName)+1];
      strcpy(this->FileName, fileName);
      }
    if (this->Append)
      {
      this->OStream = new ofstream(this->FileName, ios::app);
      }
    else
      {
      this->OStream = new ofstream(this->FileName);
      }
    }
}

void vtkFileOutputWindow::DisplayText(const char* text)
{
  if(!text)
    {
    return;
    }

  if (!this->OStream)
    {
    this->Initialize();
    }
  *this->OStream << text << endl;
  
  if (this->Flush)
    {
    this->OStream->flush();
    }
}

void vtkFileOutputWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkOutputWindow::PrintSelf(os, indent);
  os << indent << "OStream: " << this->OStream << endl;
  os << indent << "File Name: " 
     << (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "Append: " << (this->Append ? "On" : "Off") << endl;
  os << indent << "Flush: " << (this->Flush ? "On" : "Off") << endl;
}
