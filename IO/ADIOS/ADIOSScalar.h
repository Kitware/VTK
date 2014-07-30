/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ADIOSScalar.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME ADIOSScalar - The utility class wrapping the ADIOS_VARINFO struct

#ifndef _ADIOSScalar_h
#define _ADIOSScalar_h

#include <string>
#include <vector>

#include <vtkType.h>

#include <adios_read.h>

//----------------------------------------------------------------------------
class ADIOSScalar
{
public:
  ADIOSScalar(ADIOS_FILE *f, ADIOS_VARINFO *v);
  ~ADIOSScalar(void);

  std::string GetName(void) const { return this->Name; }
  int GetId(void) const { return this->Id; }
  int GetType(void) const { return this->Type; }
  int GetNumSteps(void) const { return this->NumSteps; }

  template<typename T>
  const std::vector<T>& GetValues(int step) const;

private:
  int Id;
  int Type;
  int NumSteps;
  const std::string Name;
  std::vector<void *> Values; // void* is actualy std::vector<T>
};

#endif // _ADIOSScalar_h
// VTK-HeaderTest-Exclude: ADIOSScalar.h
