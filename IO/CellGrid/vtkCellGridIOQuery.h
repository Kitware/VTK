// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridIOQuery
 * @brief   Serialize/deserialize vtkCellMetadata records.
 *
 * vtkCellGridIOQuery is a concrete subclass of vtkCellGridQuery that helps
 * serialize/deserialize vtkCellGrid objects to/from JSON.
 * Specifically, it reads/writes data specific to subclasses of vtkCellMetadata.
 *
 * @sa
 * vtkCellGridQuery
 */

#ifndef vtkCellGridIOQuery_h
#define vtkCellGridIOQuery_h

#include "vtkCellGridQuery.h"
#include "vtkIOCellGridModule.h" // For export macro

#include <vtk_nlohmannjson.h>        // For API.
#include VTK_NLOHMANN_JSON(json.hpp) // For API.

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArraySelection;
class vtkFieldData;
class vtkImageData;
class vtkStringArray;

class VTKIOCELLGRID_EXPORT vtkCellGridIOQuery : public vtkCellGridQuery
{
public:
  static vtkCellGridIOQuery* New();
  vtkTypeMacro(vtkCellGridIOQuery, vtkCellGridQuery);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Prepare to deserialize cell metadata from the given \a parent JSON object.
   */
  void PrepareToDeserialize(nlohmann::json& sourceData)
  {
    this->Data = &sourceData;
    this->Serializing = false;
  }
  ///@}

  ///@{
  /**
   * Prepare to serialize cell metadata to the given \a parent JSON object.
   */
  void PrepareToSerialize(nlohmann::json& destination)
  {
    this->Data = &destination;
    this->Serializing = true;
  }
  ///@}

  /// Return the JSON object that is our source or target.
  nlohmann::json* GetData() { return this->Data; }

  bool IsSerializing() const { return this->Serializing; }

protected:
  vtkCellGridIOQuery() = default;
  ~vtkCellGridIOQuery() override = default;

  nlohmann::json* Data{ nullptr };
  bool Serializing{ true };

private:
  vtkCellGridIOQuery(const vtkCellGridIOQuery&) = delete;
  void operator=(const vtkCellGridIOQuery&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
