/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkImageReader2Factory.cxx
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
#include "vtkImageReader2Factory.h"
#include "vtkObjectFactory.h"
#include "vtkImageReader2.h"
#include "vtkGESignaReader.h"
#include "vtkPNGReader.h"
#include "vtkPNMReader.h"
#include "vtkJPEGReader.h"
#include "vtkTIFFReader.h"
#include "vtkBMPReader.h"
#include "vtkSLCReader.h"
#include "vtkImageReader2Collection.h"
#include "vtkObjectFactoryCollection.h"

vtkCxxRevisionMacro(vtkImageReader2Factory, "1.6");
vtkStandardNewMacro(vtkImageReader2Factory);

class vtkCleanUpImageReader2Factory
{
public:
  inline void Use() 
    {
    }
  ~vtkCleanUpImageReader2Factory()
    {
      if(vtkImageReader2Factory::AvailiableReaders)
        {
        vtkImageReader2Factory::AvailiableReaders->Delete();
        vtkImageReader2Factory::AvailiableReaders = 0;
        }
    }  
};
static vtkCleanUpImageReader2Factory vtkCleanUpImageReader2FactoryGlobal;

vtkImageReader2Collection* vtkImageReader2Factory::AvailiableReaders;

void vtkImageReader2Factory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Availiable Readers : ";
  if(AvailiableReaders)
    {
    AvailiableReaders->PrintSelf(os, indent);
    }
  else
    {
    os << "None.";
    }
}

vtkImageReader2Factory::vtkImageReader2Factory()
{
}

vtkImageReader2Factory::~vtkImageReader2Factory()
{
}

void vtkImageReader2Factory::RegisterReader(vtkImageReader2* r)
{
  vtkImageReader2Factory::InitializeReaders();
  AvailiableReaders->AddItem(r);
}


vtkImageReader2* vtkImageReader2Factory::CreateImageReader2(const char* path)
{ 
  vtkImageReader2Factory::InitializeReaders();
  vtkImageReader2* ret;
  vtkCollection* collection = vtkCollection::New();
  vtkObjectFactory::CreateAllInstance("vtkImageReaderObject",
                                      collection);
  vtkObject* o;
  // first try the current registered object factories to see
  // if one of them can 
  for(collection->InitTraversal(); (o = collection->GetNextItemAsObject()); )
    {
    if(o)
      {
      ret = vtkImageReader2::SafeDownCast(o);
      if(ret && ret->CanReadFile(path))
        {
        ret->Register(0);
        return ret;
        }
      }
    }
  // get rid of the collection
  collection->Delete();
  for(vtkImageReader2Factory::AvailiableReaders->InitTraversal();
      (ret = vtkImageReader2Factory::AvailiableReaders->GetNextItem());)
    {
    if(ret->CanReadFile(path))
      {
      ret->Register(ret); // up the reference count for return as this is
      // like a new call
      return ret;
      }
    }
  return 0;
}


void vtkImageReader2Factory::InitializeReaders()
{
  if(vtkImageReader2Factory::AvailiableReaders)
    {
    return;
    }
  vtkCleanUpImageReader2FactoryGlobal.Use();
  vtkImageReader2Factory::AvailiableReaders = vtkImageReader2Collection::New();
  vtkImageReader2* reader;

  vtkImageReader2Factory::AvailiableReaders->
    AddItem((reader = vtkPNGReader::New()));
  reader->Delete();
  vtkImageReader2Factory::AvailiableReaders->
    AddItem((reader = vtkPNMReader::New()));
  reader->Delete();
  vtkImageReader2Factory::AvailiableReaders->
    AddItem((reader = vtkTIFFReader::New()));
  reader->Delete();
  vtkImageReader2Factory::AvailiableReaders->
    AddItem((reader = vtkBMPReader::New()));
  reader->Delete();
  vtkImageReader2Factory::AvailiableReaders->
    AddItem((reader = vtkSLCReader::New()));
  reader->Delete();
  vtkImageReader2Factory::AvailiableReaders->
    AddItem((reader = vtkJPEGReader::New()));
  reader->Delete();
  vtkImageReader2Factory::AvailiableReaders->
    AddItem((reader = vtkGESignaReader::New()));
  reader->Delete();
}
