/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageConnector.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

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
#include "vtkImageConnector.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageConnector* vtkImageConnector::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageConnector");
  if(ret)
    {
    return (vtkImageConnector*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageConnector;
}





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
  int *incs, *pIncs, *pExtent;
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
    *((unsigned char *)(seed->Pointer)) = this->ConnectedValue;
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
        ptr = (unsigned char *)(seed->Pointer) - *pIncs;
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
        ptr = (unsigned char *)(seed->Pointer) + *pIncs;
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
  vtkObject::PrintSelf(os,indent);

  os << indent << "ConnectedValue: " << this->ConnectedValue << "\n";
  os << indent << "UnconnectedValue: " << this->UnconnectedValue << "\n";

}
 
