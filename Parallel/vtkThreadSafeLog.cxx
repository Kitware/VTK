/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreadSafeLog.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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

#include "vtkThreadSafeLog.h"
#include "vtkObjectFactory.h"
#include <iomanip.h>


//----------------------------------------------------------------------------
vtkThreadSafeLog* vtkThreadSafeLog::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkThreadSafeLog");
  if(ret)
    {
    return (vtkThreadSafeLog*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkThreadSafeLog;
}


//----------------------------------------------------------------------------
vtkThreadSafeLog::vtkThreadSafeLog()
{
  this->Timer = vtkTimerLog::New();
  this->NumberOfEntries = 0;
}

//----------------------------------------------------------------------------
vtkThreadSafeLog::~vtkThreadSafeLog()
{
  int idx;

  this->Timer->Delete();
  this->Timer = NULL;

  for (idx = 0; idx < this->NumberOfEntries; ++idx)
    {
    if (this->Tags[idx])
      {
      delete [] this->Tags[idx];
      }
    }
  this->NumberOfEntries = 0;
}

//----------------------------------------------------------------------------
void vtkThreadSafeLog::AddEntry(char *tag, float value)
{
  int idx = this->NumberOfEntries;

  if (idx >= VTK_THREAD_SAFE_LOG_MAX)
    {
    vtkErrorMacro("Too many entries");
    }

  if (tag == NULL)
    {
    this->Tags[idx] = NULL;
    }
  else
    {
    this->Tags[idx] = new char[strlen(tag)+1];
    strcpy(this->Tags[idx], tag);
    }
  this->Values[idx] = value;
  this->NumberOfEntries = idx+1;
}

//----------------------------------------------------------------------------
void vtkThreadSafeLog::DumpLog(char *filename, int nMode)
{
  ofstream os(filename, nMode);
  int idx;
  
  if (nMode == ios::out)
    {
    for (idx = 0; idx < this->NumberOfEntries; ++idx)
      {
      os << setw(10) << this->Tags[idx] << " ";
      }
    os << endl;
    }

  for (idx = 0; idx < this->NumberOfEntries; ++idx)
    {
    os << setw(10) << this->Values[idx] << " ";
    }
  os << endl;
}

  





