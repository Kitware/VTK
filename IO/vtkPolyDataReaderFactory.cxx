/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataReaderFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyDataReaderFactory.h"

#include "vtkAbstractPolyDataReader.h"
#include "vtkPolyDataReaderCollection.h"
#include "vtkPolyDataReader.h"
#include "vtkObjectFactory.h"
#include "vtkObjectFactoryCollection.h"
#include "vtkBYUReader.h"
#include "vtkFacetReader.h"
#include "vtkOBJReader.h"
#include "vtkPLYReader.h"
#include "vtkSTLReader.h"
#include "vtkUGFacetReader.h"
#include "vtkLegacyPolyDataReader.h"

// Destroying the prototype readers requires information keys.
// Include the manager here to make sure the keys are not destroyed
// until after the AvailableReaders singleton has been destroyed.
#include "vtkFilteringInformationKeyManager.h"

vtkCxxRevisionMacro(vtkPolyDataReaderFactory, "1.1");
vtkStandardNewMacro(vtkPolyDataReaderFactory);

class vtkPolyDataReaderFactoryCleanup
{
public:
  inline void Use()
    {
    }
  ~vtkPolyDataReaderFactoryCleanup()
    {
    if(vtkPolyDataReaderFactory::AvailableReaders)
      {
      vtkPolyDataReaderFactory::AvailableReaders->Delete();
      vtkPolyDataReaderFactory::AvailableReaders = 0;
      }
    }
};
static vtkPolyDataReaderFactoryCleanup vtkPolyDataReaderFactoryCleanupGlobal;

vtkPolyDataReaderCollection* vtkPolyDataReaderFactory::AvailableReaders;

void vtkPolyDataReaderFactory::PrintSelf(ostream& os, vtkIndent indent)
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

vtkPolyDataReaderFactory::vtkPolyDataReaderFactory()
{
}

vtkPolyDataReaderFactory::~vtkPolyDataReaderFactory()
{
}

void vtkPolyDataReaderFactory::RegisterReader(vtkAbstractPolyDataReader* r)
{
  vtkPolyDataReaderFactory::InitializeReaders();
  AvailableReaders->AddItem(r);
}


vtkAbstractPolyDataReader* vtkPolyDataReaderFactory::CreatePolyDataReader(const char* path)
{ 
  vtkPolyDataReaderFactory::InitializeReaders();
  vtkAbstractPolyDataReader* ret;
  vtkCollection* collection = vtkCollection::New();
  vtkObjectFactory::CreateAllInstance("vtkPolyDataReaderObject",
                                      collection);
  vtkObject* o;
  // first try the current registered object factories to see
  // if one of them can 
  for(collection->InitTraversal(); (o = collection->GetNextItemAsObject()); )
    {
    if(o)
      {
      ret = vtkAbstractPolyDataReader::SafeDownCast(o);
      if(ret && ret->CanReadFile(path))
        {
        return ret;
        }
      }
    }
  // get rid of the collection
  collection->Delete();
  vtkCollectionSimpleIterator sit;
  for(vtkPolyDataReaderFactory::AvailableReaders->InitTraversal(sit);
      (ret = vtkPolyDataReaderFactory::AvailableReaders->GetNextPolyDataReader(sit));)
    {
    if(ret->CanReadFile(path))
      {
      // like a new call
      return ret->NewInstance();
      }
    }
  return 0;
}


void vtkPolyDataReaderFactory::InitializeReaders()
{
  if(vtkPolyDataReaderFactory::AvailableReaders)
    {
    return;
    }
  vtkPolyDataReaderFactoryCleanupGlobal.Use();
  vtkPolyDataReaderFactory::AvailableReaders = vtkPolyDataReaderCollection::New();
  vtkAbstractPolyDataReader* reader;
  
  vtkPolyDataReaderFactory::AvailableReaders->
    AddItem((reader = vtkBYUReader::New()));
  reader->Delete();
  vtkPolyDataReaderFactory::AvailableReaders->
    AddItem((reader = vtkSTLReader::New()));
  reader->Delete();
  vtkPolyDataReaderFactory::AvailableReaders->
    AddItem((reader = vtkLegacyPolyDataReader::New()));
  reader->Delete();
  //Test 2 for now. More to follow... 
}


void vtkPolyDataReaderFactory::GetRegisteredReaders(vtkPolyDataReaderCollection* collection)
{
  vtkPolyDataReaderFactory::InitializeReaders();
  // get all dynamic readers
  vtkObjectFactory::CreateAllInstance("vtkPolyDataReaderObject",
                                      collection);
  // get the current registered readers
  vtkAbstractPolyDataReader* ret;
  vtkCollectionSimpleIterator sit;
  for(vtkPolyDataReaderFactory::AvailableReaders->InitTraversal(sit);
      (ret = vtkPolyDataReaderFactory::AvailableReaders->GetNextPolyDataReader(sit));)
    {
    collection->AddItem(ret);
    }
}


