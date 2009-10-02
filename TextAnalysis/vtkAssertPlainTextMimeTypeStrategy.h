/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssertPlainTextMimeTypeStrategy.h

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

#ifndef _vtkAssertPlainTextMimeTypeStrategy_h
#define _vtkAssertPlainTextMimeTypeStrategy_h

#include <vtkMimeTypeStrategy.h>

// .NAME vtkAssertPlainTextMimeTypeStrategy - Claim a text/plain mime type no matter what
//
// .SECTION See Also
// vtkMimeTypeStrategy, vtkMimeTypes.
//
// .SECTION Thanks 
// Developed by Andy Wilson (atwilso@sandia.gov) at Sandia National Laboratories.

class VTK_TEXT_ANALYSIS_EXPORT vtkAssertPlainTextMimeTypeStrategy :
  public vtkMimeTypeStrategy
{
public:
  static vtkAssertPlainTextMimeTypeStrategy* New();
  vtkTypeRevisionMacro(vtkAssertPlainTextMimeTypeStrategy, vtkMimeTypeStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);


  vtkStdString Lookup(const vtkStdString& uri, const vtkTypeUInt8* begin, const vtkTypeUInt8* end);

protected:
  vtkAssertPlainTextMimeTypeStrategy();
  virtual ~vtkAssertPlainTextMimeTypeStrategy();
   
private:
  vtkAssertPlainTextMimeTypeStrategy(const vtkAssertPlainTextMimeTypeStrategy&); //Not implemented.
  void operator=(const vtkAssertPlainTextMimeTypeStrategy&); //Not implemented.
};

#endif // !_vtkAssertPlainTextMimeTypeStrategy_h

