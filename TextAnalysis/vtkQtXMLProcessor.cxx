/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtXMLProcessor.cxx

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

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkQtXMLProcessor.h"
#include "vtkUnicodeStringArray.h"
#include "vtkTable.h"

#include <QBuffer>
#include <QSimpleXmlNodeModel>
#include <QVector>
#include <QXmlQuery>
#include <QXmlSerializer>

#include <memory>
#include <stdexcept>
#include <algorithm>

////////////////////////////////////////////////////////////////////////////////////////
// vtkQtXMLProcessor::XMLAdapter

/// Adapter class that "maps" vtkFieldData into an XML-compatible structure, for use with QXmlQuery
class vtkQtXMLProcessor::XMLAdapter :
  public QSimpleXmlNodeModel
{
public:
  XMLAdapter(const QXmlNamePool& name_pool, vtkFieldData* const field_data, std::map<vtkStdString, vtkStdString> array_name_map, const vtkIdType row_begin = 0, const vtkIdType row_end = 0) :
    QSimpleXmlNodeModel(name_pool),
    FieldData(field_data),
    RowBegin(row_begin),
    RowEnd(std::max(row_begin, row_end))
  {
    for(vtkIdType i = 0; i != field_data->GetNumberOfArrays(); ++i)
      {
      vtkStdString array_name = field_data->GetAbstractArray(i)->GetName();

      if(array_name_map.count(array_name))
        array_name = array_name_map[array_name];

      std::replace(array_name.begin(), array_name.end(), ' ', '_');
      std::replace(array_name.begin(), array_name.end(), '(', '_');
      std::replace(array_name.begin(), array_name.end(), ')', '_');

      Arrays.push_back(field_data->GetAbstractArray(i));
      ArrayNames.push_back(array_name);
      }
  }

  void SetRange(const vtkIdType row_begin, const vtkIdType row_end)
  {
    this->RowBegin = row_begin;
    this->RowEnd = std::max(row_begin, row_end);
  }

  QVector<QXmlNodeModelIndex> attributes(const QXmlNodeModelIndex&) const
  {
    return QVector<QXmlNodeModelIndex>();
  }

  QXmlNodeModelIndex::DocumentOrder compareOrder(const QXmlNodeModelIndex& lhs, const QXmlNodeModelIndex& rhs) const
  {
    if(lhs.data() == rhs.data())
      {
      if(lhs.additionalData() == rhs.additionalData())
        return QXmlNodeModelIndex::Is;

      return lhs.additionalData() < rhs.additionalData() ? QXmlNodeModelIndex::Precedes : QXmlNodeModelIndex::Follows;
      }

    return lhs.data() < rhs.data() ? QXmlNodeModelIndex::Precedes : QXmlNodeModelIndex::Follows;
  }

  QUrl documentUri(const QXmlNodeModelIndex&) const
  {
    return QUrl();
  }

  QXmlNodeModelIndex::NodeKind kind(const QXmlNodeModelIndex& node) const
  {
    switch(node.data())
      {
      case DOCUMENT:
        return QXmlNodeModelIndex::Document;
      }

    return QXmlNodeModelIndex::Element;
  }
  
  QXmlName name(const QXmlNodeModelIndex& node) const
  {
    switch(node.data())
      {
      case TABLE:
        return QXmlName(this->namePool(), QLatin1String("table"));
      case ROWS:
        return QXmlName(this->namePool(), QLatin1String("rows"));
      case ROW:
        return QXmlName(this->namePool(), QLatin1String("row"));
      case CELL:
        return QXmlName(this->namePool(), QLatin1String(this->ArrayNames[this->get_column_index(node)].c_str()));
      }
    return QXmlName();
  }

  QXmlNodeModelIndex nextFromSimpleAxis(QAbstractXmlNodeModel::SimpleAxis axis, const QXmlNodeModelIndex& node) const
  {
    if(node.data() == DOCUMENT)
      {
      if(axis == FirstChild)
        {
        return createTableIndex();
        }
      }
    else if(node.data() == TABLE)
      {
      if(axis == Parent)
        {
        return createDocumentIndex();
        }
      else if(axis == FirstChild)
        {
        return createRowsIndex();
        }
      }
    else if(node.data() == ROWS)
      {
      if(axis == Parent)
        {
        return createTableIndex();
        }
      else if(axis == FirstChild && this->RowBegin != this->RowEnd)
        {
        return createRowIndex(this->RowBegin);
        }
      }
    else if(node.data() == ROW)
      {
      const vtkIdType row_index = this->get_row_index(node);

      if(axis == Parent)
        {
        return createRowsIndex();
        }
      else if(axis == FirstChild && this->FieldData->GetNumberOfArrays())
        { 
        return createCellIndex(row_index, 0);
        }
      else if(node.data() == ROW && axis == PreviousSibling && row_index != 0)
        {
        return createRowIndex(row_index - 1);
        }
      else if(node.data() == ROW && axis == NextSibling && row_index + 1 != this->RowEnd)
        {
        return createRowIndex(row_index + 1);
        }
      }
    else if(node.data() == CELL)
      {
      const vtkIdType row_index = this->get_row_index(node);
      const vtkIdType column_index = this->get_column_index(node);

      if(axis == Parent)
        {
        return createRowIndex(row_index);
        }
      else if(axis == PreviousSibling)
        {
        if(column_index != 0)
          return createCellIndex(row_index, column_index - 1);
        }
      else if(axis == NextSibling)
        {
        if(column_index + 1 != this->FieldData->GetNumberOfArrays())
          return createCellIndex(row_index, column_index + 1);
        }
      }

    return QXmlNodeModelIndex();
  }

  QXmlNodeModelIndex root(const QXmlNodeModelIndex& = QXmlNodeModelIndex()) const
  {
    return createDocumentIndex();
  }

  QVariant typedValue(const QXmlNodeModelIndex& node) const
  {
    switch(node.data())
      {
      case DOCUMENT:
      case TABLE:
      case ROWS:
      case ROW:
        return QVariant();
      case CELL:
        {
        const vtkVariant value = this->Arrays[this->get_column_index(node)]->GetVariantValue(this->get_row_index(node));

        if(value.IsInt())
          return value.ToInt();

        if(value.IsNumeric())
          return value.ToDouble();

        if(value.IsString())
          return value.ToString().c_str();

        if(value.IsUnicodeString())
          return QString::fromUtf8(value.ToUnicodeString().utf8_str());
        }
      }

    return QVariant();
  }

private:
  vtkFieldData* const FieldData;
  vtkIdType RowBegin;
  vtkIdType RowEnd;

  std::vector<vtkAbstractArray*> Arrays;
  std::vector<vtkStdString> ArrayNames;

  enum NodeType
  {
    DOCUMENT = 0,
    TABLE = 1,
    ROWS = 2,
    ROW = 3,
    CELL = 4
  };

  QXmlNodeModelIndex createDocumentIndex() const
  {
    return createIndex(DOCUMENT);
  }

  QXmlNodeModelIndex createTableIndex() const
  {
    return createIndex(TABLE);
  }

  QXmlNodeModelIndex createRowsIndex() const
  {
    return createIndex(ROWS);
  }

  QXmlNodeModelIndex createRowIndex(vtkIdType row_index) const
  {
    return createIndex(ROW, static_cast<qint64>(row_index));
  }

  QXmlNodeModelIndex createCellIndex(vtkIdType row_index, vtkIdType column_index) const
  {
    return createIndex(CELL, static_cast<qint64>(row_index) | (static_cast<qint64>(column_index) << 32));
  }

  vtkIdType get_column_index(const QXmlNodeModelIndex& node) const
  {
    return node.additionalData() >> 32;
  }

  vtkIdType get_row_index(const QXmlNodeModelIndex& node) const
  {
    return node.additionalData() & 0xffffffff;
  }
};

////////////////////////////////////////////////////////////////////////////////////////
// vtkQtXMLProcessor::Internals

class vtkQtXMLProcessor::Internals
{
public:
  std::map<vtkStdString, vtkStdString> ArrayNameMap;
};

////////////////////////////////////////////////////////////////////////////////////////
// vtkQtXMLProcessor

vtkStandardNewMacro(vtkQtXMLProcessor);

vtkQtXMLProcessor::vtkQtXMLProcessor() :
  Implementation(new Internals()),
  FieldType(vtkDataObject::VERTEX),
  InputDomain(ROW_DOMAIN),
  QueryType(XQUERY),
  Query(0),
  OutputArray(0)
{
  this->SetOutputArray("xslt");
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(2);

  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "xml");
}

vtkQtXMLProcessor::~vtkQtXMLProcessor()
{
  this->SetQuery(0);
  this->SetOutputArray(0);
  delete this->Implementation;
}

void vtkQtXMLProcessor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldType: " << this->FieldType << endl;
  os << indent << "InputDomain: " << this->InputDomain << endl;
  os << indent << "QueryType: " << this->QueryType << endl;
  os << indent << "Query: " << (this->Query ? this->Query : "(none)") << endl;
  os << indent << "OutputArray: " << (this->OutputArray ? this->OutputArray : "(none)") << endl;
}

void vtkQtXMLProcessor::MapArrayName(const vtkStdString& from, const vtkStdString& to)
{
  this->Implementation->ArrayNameMap.insert(std::make_pair(from, to));
  this->Modified();
}

void vtkQtXMLProcessor::ClearArrayNameMap()
{
  this->Implementation->ArrayNameMap.clear();
  this->Modified();
}

int vtkQtXMLProcessor::FillOutputPortInformation(int port, vtkInformation* info)
{
  switch(port)
    {
    case 1:
      info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
      return 1;
    }

  return vtkPassInputTypeAlgorithm::FillOutputPortInformation(port, info);
}

int vtkQtXMLProcessor::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  try
    {
    vtkDataObject* const input = inputVector[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT());
    if(!input)
      throw std::runtime_error("Missing input data object.");

    vtkDataObject* const output = outputVector->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT());
    if(!output)
      throw std::runtime_error("Missing output data object.");

    vtkTable* const output_table = vtkTable::GetData(outputVector, 1);
    if(!output_table)
      throw std::runtime_error("Missing output table.");

    if(!this->Query)
      throw std::runtime_error("Query not set.");

    if(!this->OutputArray)
      throw std::runtime_error("OutputArray not set.");

    output->ShallowCopy(input);

    std::auto_ptr<QXmlQuery> xml_query;
    switch(this->QueryType)
      {
      case vtkQtXMLProcessor::XQUERY:
        xml_query.reset(new QXmlQuery(QXmlQuery::XQuery10));
        break;
      case vtkQtXMLProcessor::XSLT:
        xml_query.reset(new QXmlQuery(QXmlQuery::XSLT20));
        break;
      default:
        throw std::runtime_error("Unknown QueryType.");
      }

    vtkUnicodeStringArray* const output_array = vtkUnicodeStringArray::New();
    output_array->SetName(this->OutputArray);
    output_array->SetNumberOfComponents(1);

    vtkFieldData* const field_data = output->GetAttributesAsFieldData(this->FieldType);
    if(!field_data)
      throw std::runtime_error("Missing field data.");

    switch(this->InputDomain)
      {
      case ROW_DOMAIN:
        {
        output_array->SetNumberOfTuples(field_data->GetNumberOfTuples());

        vtkQtXMLProcessor::XMLAdapter xml_input(xml_query->namePool(), field_data, this->Implementation->ArrayNameMap);
        xml_query->setFocus(QXmlItem(xml_input.root()));

        QByteArray xml_query_array(this->Query);
        QBuffer xml_query_buffer(&xml_query_array);
        xml_query_buffer.open(QIODevice::ReadOnly);
        xml_query->setQuery(&xml_query_buffer);

        for(vtkIdType row = 0; row != field_data->GetNumberOfTuples(); ++row)
          {
          xml_input.SetRange(row, row + 1);

          QBuffer xml_output;
          xml_output.open(QIODevice::ReadWrite);

          QXmlSerializer xml_serializer(*xml_query, &xml_output);
          xml_query->evaluateTo(&xml_serializer);

          output_array->SetValue(row, vtkUnicodeString::from_utf8(xml_output.data().data()));
          }
          
        field_data->AddArray(output_array);
        output_array->Delete();

        break;
        }

      case DATA_OBJECT_DOMAIN:
        {
        output_array->SetNumberOfTuples(1);

        vtkQtXMLProcessor::XMLAdapter xml_input(xml_query->namePool(), field_data, this->Implementation->ArrayNameMap, 0, field_data->GetNumberOfTuples());
        xml_query->setFocus(QXmlItem(xml_input.root()));

        QByteArray xml_query_array(this->Query);
        QBuffer xml_query_buffer(&xml_query_array);
        xml_query_buffer.open(QIODevice::ReadOnly);
        xml_query->setQuery(&xml_query_buffer);

        QBuffer xml_output;
        xml_output.open(QIODevice::ReadWrite);

        QXmlSerializer xml_serializer(*xml_query, &xml_output);
        xml_query->evaluateTo(&xml_serializer);

        output_array->SetValue(0, vtkUnicodeString::from_utf8(xml_output.data().data()));
        output_table->AddColumn(output_array);
        output_array->Delete();
        break;
        }

      case VALUE_DOMAIN:
        {
        vtkUnicodeStringArray* const xml = vtkUnicodeStringArray::SafeDownCast(this->GetInputAbstractArrayToProcess(0, inputVector));
        if(!xml)
          throw std::runtime_error("Missing input xml array.");

        output_array->SetNumberOfTuples(xml->GetNumberOfTuples());

        for(vtkIdType row = 0; row != xml->GetNumberOfTuples(); ++row)
          {
          QByteArray xml_input(xml->GetValue(row).utf8_str());
          QBuffer xml_input_buffer(&xml_input);
          xml_input_buffer.open(QIODevice::ReadOnly);
          xml_query->setFocus(&xml_input_buffer);

          QByteArray xml_query_array(this->Query);
          QBuffer xml_query_buffer(&xml_query_array);
          xml_query_buffer.open(QIODevice::ReadOnly);
          xml_query->setQuery(&xml_query_buffer); // Bizarre Qt Quirk: must be called AFTER setFocus().

          QBuffer xml_output;
          xml_output.open(QIODevice::ReadWrite);

          QXmlSerializer xml_serializer(*xml_query, &xml_output);
          xml_query->evaluateTo(&xml_serializer);

          output_array->SetValue(row, vtkUnicodeString::from_utf8(xml_output.data().data()));
          }
          
        field_data->AddArray(output_array);
        output_array->Delete();

        break;
        }

      default:
        {
        throw std::runtime_error("Unknown InputDomain.");
        }
      }
     
    return 1;
    }
  catch(std::exception& e)
    {
    vtkErrorMacro(<< "unhandled exception: " << e.what());
    return 0;
    }
  catch(...)
    {
    vtkErrorMacro(<< "unknown exception");
    return 0;
    }
}

