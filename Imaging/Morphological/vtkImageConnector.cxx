/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageConnector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageConnector.h"

#include "vtkImageData.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkImageConnector);

//----------------------------------------------------------------------------
vtkImageConnector::vtkImageConnector()
{
  this->Seeds = NULL;
  this->LastSeed = NULL;
  this->ConnectedValue = 255;
  this->UnconnectedValue = 128;
}

//----------------------------------------------------------------------------
vtkImageConnector::~vtkImageConnector()
{
  this->RemoveAllSeeds();
}

//----------------------------------------------------------------------------
void vtkImageConnector::RemoveAllSeeds()
{
  vtkImageConnectorSeed *temp;

  while (this->Seeds)
    {
    temp = this->Seeds;
    this->Seeds = temp->Next;
    delete temp;
    }
  this->LastSeed = NULL;
}


//----------------------------------------------------------------------------
vtkImageConnectorSeed *vtkImageConnector::NewSeed(int index[3], void *ptr)
{
  vtkImageConnectorSeed *seed = vtkImageConnectorSeed::New();
  int idx;

  for (idx = 0; idx < 3; ++idx)
    {
    seed->Index[idx] = index[idx];
    }
  seed->Pointer = ptr;
  seed->Next = NULL;

  return seed;
}

//----------------------------------------------------------------------------
// Add a new seed to the end of the seed list.
void vtkImageConnector::AddSeedToEnd(vtkImageConnectorSeed *seed)
{
  // Add the seed to the end of the list
  if (this->LastSeed == NULL)
    { // no seeds yet
    this->LastSeed = this->Seeds = seed;
    }
  else
    {
    this->LastSeed->Next = seed;
    this->LastSeed = seed;
    }
}

//----------------------------------------------------------------------------
// Add a new seed to the start of the seed list.
void vtkImageConnector::AddSeed(vtkImageConnectorSeed *seed)
{
  seed->Next = this->Seeds;
  this->Seeds = seed;
  if ( ! this->LastSeed)
    {
    this->LastSeed = seed;
    }
}

//----------------------------------------------------------------------------
// Removes a seed from the start of the seed list, and returns the seed.
vtkImageConnectorSeed *vtkImageConnector::PopSeed()
{
  vtkImageConnectorSeed *seed;

  seed = this->Seeds;
  this->Seeds = seed->Next;
  if (this->Seeds == NULL)
    {
    this->LastSeed = NULL;
    }
  return seed;
}

//----------------------------------------------------------------------------
// Input a data of 0's and "UnconnectedValue"s. Seeds of this object are
// used to find connected pixels.
// All pixels connected to seeds are set to ConnectedValue.
// The data has to be unsigned char.
void vtkImageConnector::MarkData(vtkImageData *data, int numberOfAxes, int extent[6])
{
  vtkIdType *incs, *pIncs;
  int *pExtent;
  vtkImageConnectorSeed *seed;
  unsigned char *ptr;
  int newIndex[3], *pIndex, idx;
  long count = 0;

  incs = data->GetIncrements();
  while (this->Seeds)
    {
    ++count;
    seed = this->PopSeed();
    // just in case the seed has not been marked visited.
    *(static_cast<unsigned char *>(seed->Pointer)) = this->ConnectedValue;
    // Add neighbors
    newIndex[0] = seed->Index[0];
    newIndex[1] = seed->Index[1];
    newIndex[2] = seed->Index[2];
    pExtent = extent;
    pIncs = incs;
    pIndex = newIndex;
    for (idx = 0; idx < numberOfAxes; ++idx)
      {
      // check pixel below
      if (*pExtent < *pIndex)
        {
        ptr = static_cast<unsigned char *>(seed->Pointer) - *pIncs;
        if (*ptr == this->UnconnectedValue)
          { // add a new seed
          --(*pIndex);
          *ptr = this->ConnectedValue;
          this->AddSeedToEnd(this->NewSeed(newIndex, ptr));
          ++(*pIndex);
          }
        }
      ++pExtent;
      // check above pixel
      if (*pExtent > *pIndex)
        {
        ptr = static_cast<unsigned char *>(seed->Pointer) + *pIncs;
        if (*ptr == this->UnconnectedValue)
          { // add a new seed
          ++(*pIndex);
          *ptr = this->ConnectedValue;
          this->AddSeedToEnd(this->NewSeed(newIndex, ptr));
          --(*pIndex);
          }
        }
      ++pExtent;
      // move to next axis
      ++pIncs;
      ++pIndex;
      }

    // Delete seed
    delete seed;
    }
  vtkDebugMacro("Marked " << count << " pixels");
}

void vtkImageConnector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ConnectedValue: " << this->ConnectedValue << "\n";
  os << indent << "UnconnectedValue: " << this->UnconnectedValue << "\n";

}

