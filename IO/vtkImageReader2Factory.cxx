/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageReader2Factory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageReader2Factory.h"

#include "vtkToolkits.h" // VTK_USE_METAIO
#include "vtkBMPReader.h"
#include "vtkGESignaReader.h"
#include "vtkImageReader2.h"
#include "vtkImageReader2Collection.h"
#include "vtkJPEGReader.h"
#include "vtkMINCImageReader.h"
#include "vtkObjectFactory.h"
#include "vtkObjectFactoryCollection.h"
#include "vtkPNGReader.h"
#include "vtkPNMReader.h"
#include "vtkSLCReader.h"
#include "vtkTIFFReader.h"
#ifdef VTK_USE_METAIO
#include "vtkMetaImageReader.h"
#endif

// Destroying the prototype readers requires information keys.
// Include the manager here to make sure the keys are not destroyed
// until after the AvailableReaders singleton has been destroyed.
#include "vtkFilteringInformationKeyManager.h"

vtkStandardNewMacro(vtkImageReader2Factory);

class vtkImageReader2FactoryCleanup
{
public:
  inline void Use()
    {
    }
  ~vtkImageReader2FactoryCleanup()
    {
    if(vtkImageReader2Factory::AvailableReaders)
      {
      vtkImageReader2Factory::AvailableReaders->Delete();
      vtkImageReader2Factory::AvailableReaders = 0;
      }
    }
};
static vtkImageReader2FactoryCleanup vtkImageReader2FactoryCleanupGlobal;

vtkImageReader2Collection* vtkImageReader2Factory::AvailableReaders;

void vtkImageReader2Factory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Available Readers : ";
  if(AvailableReaders)
    {
    AvailableReaders->PrintSelf(os, indent);
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
  AvailableReaders->AddItem(r);
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
        return ret;
        }
      }
    }
  // get rid of the collection
  collection->Delete();
  vtkCollectionSimpleIterator sit;
  for(vtkImageReader2Factory::AvailableReaders->InitTraversal(sit);
      (ret = vtkImageReader2Factory::AvailableReaders->GetNextImageReader2(sit));)
    {
    if(ret->CanReadFile(path))
      {
      // like a new call
      return ret->NewInstance();
      }
    }
  return 0;
}


void vtkImageReader2Factory::InitializeReaders()
{
  if(vtkImageReader2Factory::AvailableReaders)
    {
    return;
    }
  vtkImageReader2FactoryCleanupGlobal.Use();
  vtkImageReader2Factory::AvailableReaders = vtkImageReader2Collection::New();
  vtkImageReader2* reader;

  vtkImageReader2Factory::AvailableReaders->
    AddItem((reader = vtkPNGReader::New()));
  reader->Delete();
  vtkImageReader2Factory::AvailableReaders->
    AddItem((reader = vtkPNMReader::New()));
  reader->Delete();
  vtkImageReader2Factory::AvailableReaders->
    AddItem((reader = vtkTIFFReader::New()));
  reader->Delete();
  vtkImageReader2Factory::AvailableReaders->
    AddItem((reader = vtkBMPReader::New()));
  reader->Delete();
  vtkImageReader2Factory::AvailableReaders->
    AddItem((reader = vtkSLCReader::New()));
  reader->Delete();
  vtkImageReader2Factory::AvailableReaders->
    AddItem((reader = vtkJPEGReader::New()));
  reader->Delete();
  vtkImageReader2Factory::AvailableReaders->
    AddItem((reader = vtkGESignaReader::New()));
  reader->Delete();
  vtkImageReader2Factory::AvailableReaders->
    AddItem((reader = vtkMINCImageReader::New()));
  reader->Delete();
#ifdef VTK_USE_METAIO
  vtkImageReader2Factory::AvailableReaders->
    AddItem((reader = vtkMetaImageReader::New()));
  reader->Delete();
#endif
}


void vtkImageReader2Factory::GetRegisteredReaders(vtkImageReader2Collection* collection)
{
  vtkImageReader2Factory::InitializeReaders();
  // get all dynamic readers
  vtkObjectFactory::CreateAllInstance("vtkImageReaderObject",
                                      collection);
  // get the current registered readers
  vtkImageReader2* ret;
  vtkCollectionSimpleIterator sit;
  for(vtkImageReader2Factory::AvailableReaders->InitTraversal(sit);
      (ret = vtkImageReader2Factory::AvailableReaders->GetNextImageReader2(sit));)
    {
    collection->AddItem(ret);
    }
}


