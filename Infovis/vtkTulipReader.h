/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTulipReader.h

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

#ifndef _vtkTulipReader_h
#define _vtkTulipReader_h

#include "vtkUndirectedGraphAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkTulipReader : public vtkUndirectedGraphAlgorithm
{
public:
  static vtkTulipReader *New();
  vtkTypeRevisionMacro(vtkTulipReader, vtkUndirectedGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The Chaco file name.
  vtkGetStringMacro(FileName);
  vtkSetStringMacro(FileName);

protected:
  vtkTulipReader();
  ~vtkTulipReader();

  virtual int RequestData(
    vtkInformation *, 
    vtkInformationVector **, 
    vtkInformationVector *);

private:
  char* FileName;

  vtkTulipReader(const vtkTulipReader&);  // Not implemented.
  void operator=(const vtkTulipReader&);  // Not implemented.
};

#endif // _vtkTulipReader_h

