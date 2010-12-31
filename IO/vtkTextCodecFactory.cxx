/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkSQLDatabase.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include <vtkTextCodecFactory.h>

#include <vtkObjectFactory.h>
#include <vtkTextCodec.h>

#include <algorithm>
#include <vector>

vtkStandardNewMacro(vtkTextCodecFactory);

class vtkTextCodecFactory::CallbackVector :
     public std::vector <vtkTextCodecFactory::CreateFunction>
{
};

vtkTextCodecFactory::CallbackVector* vtkTextCodecFactory::Callbacks = NULL;


// Ensures that there are no leaks when the application exits.
class vtkTextCodecCleanup
{
public:
  void Use()
    {
    }
  ~vtkTextCodecCleanup()
    {
    vtkTextCodecFactory::UnRegisterAllCreateCallbacks();
    }
};

// Used to clean up the Callbacks
static vtkTextCodecCleanup vtkCleanupTextCodecGlobal;


void vtkTextCodecFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "vtkTextCodecFactory (" << this << ") \n";
  indent = indent.GetNextIndent();
  if(NULL != vtkTextCodecFactory::Callbacks)
    {
    os << vtkTextCodecFactory::Callbacks->size() << " Callbacks registered\n";
    }
  else
    {
    os << "No Callbacks registered.\n";
    }
  this->Superclass::PrintSelf(os, indent.GetNextIndent());
}


void vtkTextCodecFactory::RegisterCreateCallback(
  vtkTextCodecFactory::CreateFunction callback)
{
  if (!vtkTextCodecFactory::Callbacks)
    {
    vtkCleanupTextCodecGlobal.Use();
    Callbacks = new vtkTextCodecFactory::CallbackVector();
    }

  if (find(vtkTextCodecFactory::Callbacks->begin(),
           vtkTextCodecFactory::Callbacks->end(), callback) ==
      vtkTextCodecFactory::Callbacks->end())
    {
    vtkTextCodecFactory::Callbacks->push_back(callback);
    }
}


void vtkTextCodecFactory::UnRegisterCreateCallback(
  vtkTextCodecFactory::CreateFunction callback)
{
// we don't know for sure what order we are called in so if the global ones goes first this is NULL
  if (vtkTextCodecFactory::Callbacks)
    {
    for (std::vector <vtkTextCodecFactory::CreateFunction>::iterator i =
         vtkTextCodecFactory::Callbacks->begin();
         i != vtkTextCodecFactory::Callbacks->end(); ++i)
      {
      if (*i == callback)
        {
        vtkTextCodecFactory::Callbacks->erase(i);
        break;
        }
      }

    if (vtkTextCodecFactory::Callbacks->empty())
      {
      delete vtkTextCodecFactory::Callbacks;
      vtkTextCodecFactory::Callbacks = NULL;
      }
    }
}


void vtkTextCodecFactory::UnRegisterAllCreateCallbacks()
{
  if (NULL != vtkTextCodecFactory::Callbacks)
    {
    vtkTextCodecFactory::Callbacks->clear();

    if (vtkTextCodecFactory::Callbacks->empty())
      {
      delete vtkTextCodecFactory::Callbacks;
      vtkTextCodecFactory::Callbacks = NULL;
      }
    }
}


vtkTextCodec* vtkTextCodecFactory::CodecForName(const char* codecName)
{
  std::vector <vtkTextCodecFactory::CreateFunction>::iterator CF_i;
  for (CF_i = Callbacks->begin(); CF_i != Callbacks->end(); ++CF_i)
    {
    vtkTextCodec* outCodec = (*CF_i)();
    if (NULL != outCodec)
      {
      if (outCodec->CanHandle(codecName))
        {
        return outCodec;
        }
      else
        {
        outCodec->Delete();
        }
      }
    }

  return NULL;
}


vtkTextCodec* vtkTextCodecFactory::CodecToHandle(istream& SampleData)
{
  std::vector <vtkTextCodecFactory::CreateFunction>::iterator CF_i;
  for (CF_i = Callbacks->begin(); CF_i != Callbacks->end(); ++CF_i)
    {
    vtkTextCodec* outCodec = (*CF_i)();
    if (NULL != outCodec)
      {
      if (outCodec->IsValid(SampleData))
        {
        return outCodec;
        }
      else
        {
        outCodec->Delete();
        }
      }
    }

  return NULL;
}


vtkTextCodecFactory::vtkTextCodecFactory()
{
}


vtkTextCodecFactory::~vtkTextCodecFactory()
{
}
