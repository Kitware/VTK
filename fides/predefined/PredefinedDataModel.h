//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_datamodel_PredefinedDataModel_H
#define fides_datamodel_PredefinedDataModel_H

#include <fides_rapidjson.h>
#include FIDES_RAPIDJSON(rapidjson/document.h)

#include <memory>
#include <string>

namespace fides
{
namespace predefined
{

// fwd declaration
class InternalMetadataSource;

/// PredefinedDataModel enables Fides to generate data models
/// based on attributes contained in the InternalMetadataSource.
/// The source file should contain an attribute named Fides_Data_Model.
class PredefinedDataModel
{
public:
  PredefinedDataModel(std::shared_ptr<InternalMetadataSource> source);
  virtual ~PredefinedDataModel() = default;

  /// Generate and return the DOM. The optional print argument
  /// prints the JSON to stdout if true. Specific data models can
  /// override this if they need to add additional objects to the DOM
  /// (e.g., XGC adds a 'number_of_planes' object).
  virtual rapidjson::Document& GetDOM(bool print = false);

  /// prints the JSON
  void PrintJSON();

protected:
  /// creates a single data source called source
  virtual void CreateDataSources(rapidjson::Value& parent);

  /// create a generalized data source
  virtual void CreateDataSource(rapidjson::Value& parent,
                                const std::string& name,
                                const std::string& mode,
                                const std::string& filename = "");

  /// Adds step information to the DOM
  virtual void AddStepInformation(rapidjson::Value& parent);

  /// creates a wildcard field with info on grabbing Fides_Variable_List
  /// and Fides_Variable_Association
  virtual void CreateFields(rapidjson::Value& parent);

  /// Must be overridden by the derived class to create the coordinate system DOM
  virtual void CreateCoordinateSystem(rapidjson::Value& parent) = 0;

  /// Must be overridden by the derived class to create the cell set DOM
  virtual void CreateCellSet(rapidjson::Value& parent) = 0;

  /// Must be overridden by the derived class to add the root object to the
  /// DOM with the name of the data model
  virtual void AddRootToDocument(rapidjson::Value& root) = 0;

  rapidjson::Document Doc;
  std::string DataSourceName = "source";
  std::shared_ptr<InternalMetadataSource> MetadataSource;
};

/// Creates a uniform data model with uniform point coordinates for the
/// coordinate system and a structured cell set.
/// Fides_Data_Model should be set to 'uniform'.
/// The origin and spacing of the coordinate system should be specified in
/// attributes in the source file with attribute names Fides_Origin and
/// Fides_Spacing.
class UniformDataModel : public PredefinedDataModel
{
public:
  UniformDataModel(std::shared_ptr<InternalMetadataSource> source);

protected:
  void CreateCoordinateSystem(rapidjson::Value& parent);
  void CreateCellSet(rapidjson::Value& parent);
  void AddRootToDocument(rapidjson::Value& root);
};

/// Creates a rectilinear data model using a cartesian product for the
/// array for the coordinate system (contains 3 arrays for the x, y, and
/// z coordinates) and a structured cell set.
/// Fides_Data_Model should be set to 'rectilinear'.
class RectilinearDataModel : public PredefinedDataModel
{
public:
  RectilinearDataModel(std::shared_ptr<InternalMetadataSource> source);

protected:
  void CreateCoordinateSystem(rapidjson::Value& parent);
  void CreateCellSet(rapidjson::Value& parent);
  void AddRootToDocument(rapidjson::Value& root);
};

/// Creates a data model for an unstructured grid where the coordinate system
/// is stored in a basic array and the cell set is explicitly defined through 3
/// arrays giving the connectivity, cell types, and number of vertices.
/// Fides_Data_Model should be set to 'unstructured'.
class UnstructuredDataModel : public PredefinedDataModel
{
public:
  UnstructuredDataModel(std::shared_ptr<InternalMetadataSource> source);

protected:
  void CreateCoordinateSystem(rapidjson::Value& parent);
  void CreateCellSet(rapidjson::Value& parent);
  void AddRootToDocument(rapidjson::Value& root);
};

/// Similar to UnstructuredDataModel, except for data sets with only
/// a single cell type, so only the connectivity is needed to be explicitly
/// stored.
/// Fides_Data_Model should be set to 'unstructured_single'.
/// This data model also requires the source file to define an
/// attribute with the name Fides_Cell_Type to specify the  cell type.
class UnstructuredSingleTypeDataModel : public UnstructuredDataModel
{
public:
  UnstructuredSingleTypeDataModel(std::shared_ptr<InternalMetadataSource> source);

protected:
  void CreateCellSet(rapidjson::Value& parent) override;
  void AddRootToDocument(rapidjson::Value& root) override;
};

/// Creates an XGC data model with data sources 'mesh' and '3d'.
/// Fides_Data_Model should be set to 'xgc'.
class XGCDataModel : public PredefinedDataModel
{
public:
  XGCDataModel(std::shared_ptr<InternalMetadataSource> source);
  rapidjson::Document& GetDOM(bool print = false) override;

protected:
  void CreateDataSources(rapidjson::Value& parent) override;
  void AddStepInformation(rapidjson::Value& parent) override;
  void CreateCoordinateSystem(rapidjson::Value& parent) override;
  void CreateCellSet(rapidjson::Value& parent) override;
  void CreateFields(rapidjson::Value& parent) override;
  void AddRootToDocument(rapidjson::Value& root) override;
};

}
}

#endif
