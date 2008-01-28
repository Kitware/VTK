/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChacoGraphReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#ifndef _vtkChacoGraphReader_h
#define _vtkChacoGraphReader_h

#include "vtkUndirectedGraphAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkChacoGraphReader : public vtkUndirectedGraphAlgorithm
{
public:
  static vtkChacoGraphReader *New();
  vtkTypeRevisionMacro(vtkChacoGraphReader, vtkUndirectedGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The Chaco file name.
  vtkGetStringMacro(FileName);
  vtkSetStringMacro(FileName);

protected:
  vtkChacoGraphReader();
  ~vtkChacoGraphReader();

  virtual int RequestData(
    vtkInformation *, 
    vtkInformationVector **, 
    vtkInformationVector *);

private:
  char* FileName;

  vtkChacoGraphReader(const vtkChacoGraphReader&);  // Not implemented.
  void operator=(const vtkChacoGraphReader&);  // Not implemented.
};

#endif // _vtkChacoGraphReader_h

