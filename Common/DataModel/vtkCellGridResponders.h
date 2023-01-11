/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellGridResponders.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCellGridResponders
 * @brief   A container that holds objects able to respond to queries
 *          specialized for particular vtkCellMetadata types.
 *
 * vtkCellGridResponders holds a list of objects statically registered to
 * the vtkCellMetadata subclass. These objects respond to
 * queries for information (e.g., a bounding box) or processing
 * (e.g., rendering, picking, generating isocontours) for one cell
 * type (and subclasses of that cell type if no more specific
 * responder is registered).
 *
 * Application code (such as a plugin) can register subclasses of
 * vtkCellGridResponse which accept the API of a particular
 * vtkCellGridQuery for that cell type.
 * Then, when a query is passed to the cell, this collection will
 * identify matching responders for the query and invoke them until
 * one returns true (indicating success).
 * Multiple matches can exist as a responder can be registered to a
 * common base cell class and/or to handle common base query classes.
 *
 * If a given cell type cannot respond to a query, its superclasses
 * are asked to respond. If no superclass can respond to the query,
 * then query's superclasses are searched for responders.
 *
 * Because queries can be registered to cell types at any time,
 * existing cell types can be extended to support new features
 * by additional libraries.
 *
 * @sa vtkCellMetadata vtkCellGrid
 */

#ifndef vtkCellGridResponders_h
#define vtkCellGridResponders_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"
#include "vtkSmartPointer.h" // For return values
#include "vtkStringToken.h"  // For API.
#include "vtkTypeName.h"     // For RegisterQueryResponder.

#include <unordered_map>

VTK_ABI_NAMESPACE_BEGIN
class vtkCellGridQuery;
class vtkCellGridResponderBase;
class vtkCellMetadata;

class VTKCOMMONDATAMODEL_EXPORT vtkCellGridResponders : public vtkObject
{
public:
  static vtkCellGridResponders* New();

  vtkTypeMacro(vtkCellGridResponders, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Register \a responder for processing a cell's data.
   */
  template <typename CellType, typename QueryType, typename ResponderType>
  void RegisterQueryResponder(ResponderType* responder)
  {
    vtkStringToken queryTypeKey = vtk::TypeName<QueryType>();
    vtkStringToken cellTypeKey = vtk::TypeName<CellType>();
    this->Responders[queryTypeKey][cellTypeKey] = responder;
  }

  /**
   * Invoke a responder for the given query and cell type.
   *
   * If none exists, return false.
   */
  bool Query(vtkCellMetadata* cellType, vtkCellGridQuery* query);

protected:
  vtkCellGridResponders() = default;
  ~vtkCellGridResponders() override = default;

  std::unordered_map<vtkStringToken,
    std::unordered_map<vtkStringToken, vtkSmartPointer<vtkCellGridResponderBase>>>
    Responders;
  // std::unordered_multimap<vtkStringToken, vtkSmartPointer<vtkCellGridCache>> Caches;

private:
  vtkCellGridResponders(const vtkCellGridResponders&) = delete;
  void operator=(const vtkCellGridResponders&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
