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
/**
 * @class   vtkLSDynaSummaryParser
 *
 * This is a helper class used by vtkLSDynaReader to read XML files.
 * @sa
 * vtkLSDynaReader
*/

#ifndef vtkLSDynaSummaryParser_h
#define vtkLSDynaSummaryParser_h

#include "vtkIOLSDynaModule.h" // For export macro
#include "vtkXMLParser.h"
#include "vtkStdString.h" //needed for vtkStdString

class LSDynaMetaData;
class VTKIOLSDYNA_EXPORT vtkLSDynaSummaryParser : public vtkXMLParser
{
public:
  vtkTypeMacro(vtkLSDynaSummaryParser,vtkXMLParser);
  static vtkLSDynaSummaryParser* New();
  void PrintSelf(ostream &os, vtkIndent indent) override;



  /// Must be set before calling Parse();
  LSDynaMetaData* MetaData;

protected:
  vtkLSDynaSummaryParser();
  ~vtkLSDynaSummaryParser() override { };

  void StartElement(const char* name, const char** atts) override;
  void EndElement(const char* name) override;
  void CharacterDataHandler(const char* data, int length) override;

  vtkStdString PartName;
  int PartId;
  int PartStatus;
  int PartMaterial;
  int InPart;
  int InDyna;
  int InName;

private:
  vtkLSDynaSummaryParser( const vtkLSDynaSummaryParser& ) = delete;
  void operator = ( const vtkLSDynaSummaryParser& ) = delete;
};

#endif //vtkLSDynaReader_h
