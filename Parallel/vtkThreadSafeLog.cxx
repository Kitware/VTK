/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreadSafeLog.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkThreadSafeLog.h"
#include "vtkObjectFactory.h"
#include <iomanip.h>

vtkCxxRevisionMacro(vtkThreadSafeLog, "1.3");
vtkStandardNewMacro(vtkThreadSafeLog);

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

  





