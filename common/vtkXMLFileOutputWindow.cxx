/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLFileOutputWindow.cxx
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
#include "vtkXMLFileOutputWindow.h"
#include "vtkObjectFactory.h"

vtkXMLFileOutputWindow* vtkXMLFileOutputWindow::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkXMLFileOutputWindow");
  if(ret)
    {
    return (vtkXMLFileOutputWindow*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkXMLFileOutputWindow;
}

void vtkXMLFileOutputWindow::Initialize() 
{
  if (!this->OStream)
    {
    if (!this->FileName)
      {
      char* fileName = (char *) "vtkMessageLog.xml";
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
      this->DisplayTag("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>");
      }
    }
}

void vtkXMLFileOutputWindow::DisplayTag(const char* text)
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

// Description:
// Process text to replace XML special characters with escape sequences
void vtkXMLFileOutputWindow:: DisplayXML(const char* tag, const char* text)
{
  char *xmlText;

  if(!text)
    {
    return;
    }

  // allocate enough room for the worst case
  xmlText = new char[strlen(text) * 6 + 1];

  const char *s = text;
  char *x = xmlText;
  *x = '\0';

  // replace all special characters
  while (*s)
    {
    switch (*s)
      {
      case '&':
	strcat(x, "&amp;"); x += 5;
	break;
      case '"':
	strcat(x, "&quot;"); x += 6;
	break;
      case '\'':
	strcat(x, "&apos;"); x += 6;
	break;
      case '<':
	strcat(x, "&lt;"); x += 4;
	break;
      case '>':
	strcat(x, "&gt;"); x += 4;
	break;
      default:
	*x = *s; x++;
	*x = '\0'; // explicitly terminate the new string
      }
    s++;
    }

  if (!this->OStream)
    {
    this->Initialize();
    }
  *this->OStream << "<" << tag << ">" << xmlText << "</" << tag << ">" << endl;
  
  if (this->Flush)
    {
    this->OStream->flush();
    }
  delete []xmlText;
}

void vtkXMLFileOutputWindow::DisplayText(const char* text)
{
  this->DisplayXML("Text", text);
}

void vtkXMLFileOutputWindow::DisplayErrorText(const char* text)
{
  this->DisplayXML("Error", text);
}

void vtkXMLFileOutputWindow::DisplayWarningText(const char* text)
{
  this->DisplayXML("Warning", text);
}

void vtkXMLFileOutputWindow::DisplayGenericWarningText(const char* text)
{
  this->DisplayXML("GenericWarning", text);
}

void vtkXMLFileOutputWindow::DisplayDebugText(const char* text)
{
  this->DisplayXML("Debug", text);
}
