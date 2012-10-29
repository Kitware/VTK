/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLSDynaSummaryParser.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLSDynaSummaryParser
// .SECTION Description
// This is a helper class used by vtkLSDynaReader to read XML files.
// .SECTION See Also
// vtkLSDynaReader

#ifndef __vtkLSDynaSummaryParser_h
#define __vtkLSDynaSummaryParser_h

#include "vtkIOLSDynaModule.h" // For export macro
#include "vtkXMLParser.h"
#include "vtkStdString.h" //needed for vtkStdString

class LSDynaMetaData;
class VTKIOLSDYNA_EXPORT vtkLSDynaSummaryParser : public vtkXMLParser
{
public:
  vtkTypeMacro(vtkLSDynaSummaryParser,vtkXMLParser);
  static vtkLSDynaSummaryParser* New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);



  /// Must be set before calling Parse();
  LSDynaMetaData* MetaData;

protected:
  vtkLSDynaSummaryParser();
  virtual ~vtkLSDynaSummaryParser() { };

  virtual void StartElement(const char* name, const char** atts);
  virtual void EndElement(const char* name);
  virtual void CharacterDataHandler(const char* data, int length);

  vtkStdString PartName;
  int PartId;
  int PartStatus;
  int PartMaterial;
  int InPart;
  int InDyna;
  int InName;

private:
  vtkLSDynaSummaryParser( const vtkLSDynaSummaryParser& ); // Not implemented.
  void operator = ( const vtkLSDynaSummaryParser& ); // Not implemented.
};

#endif //__vtkLSDynaReader_h
