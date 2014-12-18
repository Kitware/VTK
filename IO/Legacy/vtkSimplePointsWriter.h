/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSimplePointsWriter.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSimplePointsWriter - write a file of xyz coordinates
// .SECTION Description
// vtkSimplePointsWriter writes a simple file of xyz coordinates

// .SECTION See Also
// vtkSimplePointsReader

#ifndef vtkSimplePointsWriter_h
#define vtkSimplePointsWriter_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataSetWriter.h"

class VTKIOLEGACY_EXPORT vtkSimplePointsWriter : public vtkDataSetWriter
{
public:
  static vtkSimplePointsWriter *New();
  vtkTypeMacro(vtkSimplePointsWriter,vtkDataSetWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkGetMacro(DecimalPrecision, int);
  vtkSetMacro(DecimalPrecision, int);

protected:
  vtkSimplePointsWriter();
  ~vtkSimplePointsWriter(){}

  void WriteData();

  int DecimalPrecision;

private:
  vtkSimplePointsWriter(const vtkSimplePointsWriter&);  // Not implemented.
  void operator=(const vtkSimplePointsWriter&);  // Not implemented.
};

#endif
