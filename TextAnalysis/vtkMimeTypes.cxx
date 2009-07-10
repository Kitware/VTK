
/*
* Copyright (c) 2007, Sandia Corporation
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the Sandia Corporation nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY Sandia Corporation ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Sandia Corporation BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <vtkFileExtensionMimeTypeStrategy.h>
#include <vtkMimeTypes.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

#include <vector>

////////////////////////////////////////////////////////////////
// vtkMimeTypes::implementation

class vtkMimeTypes::implementation
{
public:
  implementation()
  {
    // Add more sophisticated platform-specific strategies here ...
    
    // Last-but-not-least, our fallback strategy is to identify MIME type using file extensions
    Strategies.push_back(vtkFileExtensionMimeTypeStrategy::New());
  }

  ~implementation()
  {
    for(unsigned int i = 0; i != this->Strategies.size(); ++i)
      this->Strategies[i]->Delete();
  }

  vtkstd::vector<vtkMimeTypeStrategy*> Strategies;
};

////////////////////////////////////////////////////////////////
// vtkMimeTypes

vtkCxxRevisionMacro(vtkMimeTypes, "1.1");
vtkStandardNewMacro(vtkMimeTypes);

vtkMimeTypes::vtkMimeTypes() :
  Implementation(new implementation())
{
}

vtkMimeTypes::~vtkMimeTypes()
{
  delete this->Implementation;
}

void vtkMimeTypes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  for(unsigned int i = 0; i != this->Implementation->Strategies.size(); ++i)
    {
    os << indent << "Strategy: " << endl;
    this->Implementation->Strategies[i]->PrintSelf(os, indent.GetNextIndent());
    }
}

vtkStdString vtkMimeTypes::Lookup(const vtkStdString& path)
{
  for(unsigned int i = 0; i != this->Implementation->Strategies.size(); ++i)
    {
    const vtkStdString mime_type = this->Implementation->Strategies[i]->Lookup(path);
    if(mime_type.size())
      return mime_type;
    }
  return vtkStdString();
}

