/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArrayMetaData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// .NAME vtkArrayMetaData - abstract class for data associated with vtkDataArray
//
// .SECTION Description
//
// vtkArrayMetaData is an abstract class for meta data pertaining
// to data array objects.
//
#ifndef vtkDataArrayMetaData_h
#define vtkDataArrayMetaData_h

#include "vtkObject.h"

class VTK_COMMON_EXPORT vtkDataArrayMetaData : public vtkObject
{
  public:
    //
    vtkTypeRevisionMacro(vtkDataArrayMetaData,vtkObject);
    void PrintSelf(ostream &os, vtkIndent indent);
    static vtkDataArrayMetaData *New();
    //
    virtual int DeepCopy(const vtkDataArrayMetaData *){return 0;}
    virtual int ShallowCopy(const vtkDataArrayMetaData *){return 0;}
  protected:
    vtkDataArrayMetaData(){};
    virtual ~vtkDataArrayMetaData(){};
  private:
    vtkDataArrayMetaData(const vtkDataArrayMetaData&); // Not implemented
    void operator=(const vtkDataArrayMetaData&); // Not implemented
};

#endif
