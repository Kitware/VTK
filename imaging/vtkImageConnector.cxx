/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageConnector.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkImageRegion.h"
#include "vtkImageConnector.h"


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
  vtkImageConnectorSeed *seed = new vtkImageConnectorSeed;
  int idx;

  for (idx = 0; idx < 3; ++idx)
    {
    seed->Index[idx] = index[idx];
    }
  seed->Pointer = ptr;

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
// Description:
// Input a region of 0's and "UnconnectedValue"s. Seeds of this object are 
// used to find connected pixels.
// All pixels connected to seeds are set to ConnectedValue.  
// The region has to be unsigned char.
void vtkImageConnector::MarkRegion(vtkImageRegion *region, int numberOfAxes)
{
  int *incs, *pIncs, *extent, *pExtent;
  vtkImageConnectorSeed *seed;
  unsigned char *ptr;
  int newIndex[3], *pIndex, idx;

  incs = region->GetIncrements();
  extent = region->GetExtent();
  while (this->Seeds)
    {
    seed = this->PopSeed();
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
        ptr = (unsigned char *)(seed->Pointer) - pIncs[idx];
        if (*ptr == this->UnconnectedValue)
          { // add a new seed
          --pIndex;
          this->AddSeedToEnd(this->NewSeed(newIndex, ptr));
          ++pIndex;
          }
        }
      ++pExtent;
      // check above pixel
      if (*pExtent > *pIndex)
        {
        ptr = (unsigned char *)(seed->Pointer) + pIncs[idx];
        if (*ptr == this->UnconnectedValue)
          { // add a new seed
          ++pIndex;
          this->AddSeedToEnd(this->NewSeed(newIndex, ptr));
          --pIndex;
          }
        }
      // Delete seed, and mark seed position as part of the connected region
      *((unsigned char *)(seed->Pointer)) = this->ConnectedValue;
      delete seed;
      }
    }
}

  
