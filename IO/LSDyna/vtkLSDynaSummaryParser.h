// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLSDynaSummaryParser
 * @brief   This is a helper class used by vtkLSDynaReader to read XML files.
 *
 * @sa
 * vtkLSDynaReader
 */

#ifndef vtkLSDynaSummaryParser_h
#define vtkLSDynaSummaryParser_h

#include "vtkIOLSDynaModule.h" // For export macro
#include "vtkStdString.h"      //needed for vtkStdString
#include "vtkXMLParser.h"

VTK_ABI_NAMESPACE_BEGIN
class LSDynaMetaData;
class VTKIOLSDYNA_EXPORT vtkLSDynaSummaryParser : public vtkXMLParser
{
public:
  vtkTypeMacro(vtkLSDynaSummaryParser, vtkXMLParser);
  static vtkLSDynaSummaryParser* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Must be set before calling Parse();
  LSDynaMetaData* MetaData;

protected:
  vtkLSDynaSummaryParser();
  ~vtkLSDynaSummaryParser() override = default;

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
  vtkLSDynaSummaryParser(const vtkLSDynaSummaryParser&) = delete;
  void operator=(const vtkLSDynaSummaryParser&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkLSDynaReader_h
