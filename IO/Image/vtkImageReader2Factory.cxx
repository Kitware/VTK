// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImageReader2Factory.h"

#include "vtkBMPReader.h"
#include "vtkGESignaReader.h"
#include "vtkHDRReader.h"
#include "vtkImageReader2.h"
#include "vtkImageReader2Collection.h"
#include "vtkJPEGReader.h"
#include "vtkMetaImageReader.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkObjectFactoryCollection.h"
#include "vtkPNGReader.h"
#include "vtkPNMReader.h"
#include "vtkSLCReader.h"
#include "vtkTGAReader.h"
#include "vtkTIFFReader.h"

// Destroying the prototype readers requires information keys.
// Include the manager here to make sure the keys are not destroyed
// until after the AvailableReaders singleton has been destroyed.
#include "vtkFilteringInformationKeyManager.h"

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImageReader2Factory);

//----------------------------------------------------------------------------
class vtkImageReader2FactoryCleanup
{
public:
  void Use() {}
  ~vtkImageReader2FactoryCleanup()
  {
    if (vtkImageReader2Factory::AvailableReaders)
    {
      vtkImageReader2Factory::AvailableReaders->Delete();
      vtkImageReader2Factory::AvailableReaders = nullptr;
    }
  }
};
static vtkImageReader2FactoryCleanup vtkImageReader2FactoryCleanupGlobal;

//----------------------------------------------------------------------------
vtkImageReader2Collection* vtkImageReader2Factory::AvailableReaders;

//----------------------------------------------------------------------------
void vtkImageReader2Factory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Available Readers : ";
  if (AvailableReaders)
  {
    AvailableReaders->PrintSelf(os, indent);
  }
  else
  {
    os << "None.";
  }
}

//----------------------------------------------------------------------------
void vtkImageReader2Factory::RegisterReader(vtkImageReader2* r)
{
  vtkImageReader2Factory::InitializeReaders();
  AvailableReaders->AddItem(r);
}

//----------------------------------------------------------------------------
vtkImageReader2* vtkImageReader2Factory::CreateImageReader2(const char* path)
{
  vtkImageReader2Factory::InitializeReaders();
  vtkImageReader2* ret;
  vtkNew<vtkCollection> collection;
  vtkObjectFactory::CreateAllInstance("vtkImageReaderObject", collection);
  // first try the current registered object factories to see
  // if one of them can
  for (collection->InitTraversal(); vtkObject* object = collection->GetNextItemAsObject();)
  {
    ret = vtkImageReader2::SafeDownCast(object);
    if (ret && ret->CanReadFile(path))
    {
      return ret;
    }
  }

  // Then try all available readers
  vtkCollectionSimpleIterator sit;
  for (vtkImageReader2Factory::AvailableReaders->InitTraversal(sit);
       (ret = vtkImageReader2Factory::AvailableReaders->GetNextImageReader2(sit));)
  {
    if (ret->CanReadFile(path))
    {
      // like a new call
      return ret->NewInstance();
    }
  }
  return nullptr;
}

//----------------------------------------------------------------------------
vtkImageReader2* vtkImageReader2Factory::CreateImageReader2FromExtension(const char* extension)
{
  vtkImageReader2Factory::InitializeReaders();
  vtkImageReader2* ret;
  vtkNew<vtkCollection> collection;
  vtkObjectFactory::CreateAllInstance("vtkImageReaderObject", collection);

  // first try the current registered object factories to see
  // if one of them can
  for (collection->InitTraversal(); vtkObject* object = collection->GetNextItemAsObject();)
  {
    ret = vtkImageReader2::SafeDownCast(object);
    if (ret)
    {
      const char* extensions = ret->GetFileExtensions();
      if (vtkImageReader2Factory::CheckExtensionIsInExtensions(extension, extensions))
      {
        return ret;
      }
    }
  }

  // Then try all available readers
  vtkCollectionSimpleIterator sit;
  for (vtkImageReader2Factory::AvailableReaders->InitTraversal(sit);
       (ret = vtkImageReader2Factory::AvailableReaders->GetNextImageReader2(sit));)
  {
    const char* extensions = ret->GetFileExtensions();
    if (vtkImageReader2Factory::CheckExtensionIsInExtensions(extension, extensions))
    {
      return ret->NewInstance();
    }
  }
  return nullptr;
}

//----------------------------------------------------------------------------
bool vtkImageReader2Factory::CheckExtensionIsInExtensions(
  const char* extension, const char* extensions)
{
  std::istringstream iss(extensions);
  std::string localExtension;
  while (iss >> localExtension)
  {
    if (localExtension == std::string(extension) || localExtension == "." + std::string(extension))
    {
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkImageReader2Factory::InitializeReaders()
{
  if (vtkImageReader2Factory::AvailableReaders)
  {
    return;
  }
  vtkImageReader2FactoryCleanupGlobal.Use();
  vtkImageReader2Factory::AvailableReaders = vtkImageReader2Collection::New();
  vtkImageReader2* reader;

  vtkImageReader2Factory::AvailableReaders->AddItem((reader = vtkPNGReader::New()));
  reader->Delete();
  vtkImageReader2Factory::AvailableReaders->AddItem((reader = vtkPNMReader::New()));
  reader->Delete();
  vtkImageReader2Factory::AvailableReaders->AddItem((reader = vtkTIFFReader::New()));
  reader->Delete();
  vtkImageReader2Factory::AvailableReaders->AddItem((reader = vtkBMPReader::New()));
  reader->Delete();
  vtkImageReader2Factory::AvailableReaders->AddItem((reader = vtkSLCReader::New()));
  reader->Delete();
  vtkImageReader2Factory::AvailableReaders->AddItem((reader = vtkHDRReader::New()));
  reader->Delete();
  vtkImageReader2Factory::AvailableReaders->AddItem((reader = vtkJPEGReader::New()));
  reader->Delete();
  vtkImageReader2Factory::AvailableReaders->AddItem((reader = vtkGESignaReader::New()));
  reader->Delete();
  vtkImageReader2Factory::AvailableReaders->AddItem((reader = vtkMetaImageReader::New()));
  reader->Delete();
  vtkImageReader2Factory::AvailableReaders->AddItem((reader = vtkTGAReader::New()));
  reader->Delete();
}

//----------------------------------------------------------------------------
void vtkImageReader2Factory::GetRegisteredReaders(vtkImageReader2Collection* collection)
{
  vtkImageReader2Factory::InitializeReaders();
  // get all dynamic readers
  vtkObjectFactory::CreateAllInstance("vtkImageReaderObject", collection);
  // get the current registered readers
  vtkImageReader2* ret;
  vtkCollectionSimpleIterator sit;
  for (vtkImageReader2Factory::AvailableReaders->InitTraversal(sit);
       (ret = vtkImageReader2Factory::AvailableReaders->GetNextImageReader2(sit));)
  {
    collection->AddItem(ret);
  }
}
VTK_ABI_NAMESPACE_END
