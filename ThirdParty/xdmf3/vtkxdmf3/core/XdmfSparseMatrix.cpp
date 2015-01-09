/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfSparseMatrix.cpp                                                */
/*                                                                           */
/*  Author:                                                                  */
/*     Kenneth Leiter                                                        */
/*     kenneth.leiter@arl.army.mil                                           */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2011 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/

#include <iostream>

#include <sstream>
#include "XdmfSparseMatrix.hpp"
#include "XdmfError.hpp"

shared_ptr<XdmfSparseMatrix>
XdmfSparseMatrix::New(const unsigned int numberRows,
                      const unsigned int numberColumns)
{
  shared_ptr<XdmfSparseMatrix> p(new XdmfSparseMatrix(numberRows,
                                                      numberColumns));
  return p;
}

XdmfSparseMatrix::XdmfSparseMatrix(const unsigned int numberRows,
                                   const unsigned int numberColumns) :
  mColumnIndex(XdmfArray::New()),
  mName(""),
  mNumberColumns(numberColumns),
  mNumberRows(numberRows),
  mRowPointer(XdmfArray::New()),
  mValues(XdmfArray::New())
{
  mRowPointer->resize<unsigned int>(mNumberRows + 1, 0);
}

XdmfSparseMatrix::~XdmfSparseMatrix()
{
}

const std::string XdmfSparseMatrix::ItemTag = "SparseMatrix";

shared_ptr<XdmfArray>
XdmfSparseMatrix::getColumnIndex()
{
  return mColumnIndex;
}

std::map<std::string, std::string>
XdmfSparseMatrix::getItemProperties() const
{
  std::map<std::string, std::string> sparseMatrixProperties;
  sparseMatrixProperties.insert(std::make_pair("Name", mName));
  std::stringstream numberRowsString;
  numberRowsString << mNumberRows;
  sparseMatrixProperties.insert(std::make_pair("NumberRows",
                                               numberRowsString.str()));
  std::stringstream numberColumnsString;
  numberColumnsString << mNumberColumns;
  sparseMatrixProperties.insert(std::make_pair("NumberColumns",
                                               numberColumnsString.str()));
  return sparseMatrixProperties;
}

std::string
XdmfSparseMatrix::getItemTag() const
{
  return ItemTag;
}

std::string
XdmfSparseMatrix::getName() const
{
  if (mName.c_str() == NULL) {
    return "";
  }
  else {
    return mName;
  }
}

unsigned int
XdmfSparseMatrix::getNumberColumns() const
{
  return mNumberColumns;
}

unsigned int
XdmfSparseMatrix::getNumberRows() const
{
  return mNumberRows;
}

shared_ptr<XdmfArray>
XdmfSparseMatrix::getRowPointer()
{
  return mRowPointer;
}

shared_ptr<XdmfArray>
XdmfSparseMatrix::getValues()
{
  return mValues;
}

std::string
XdmfSparseMatrix::getValuesString() const
{

  std::stringstream toReturn;
  for(unsigned int i=0; i<mNumberRows; ++i) {
    if (i + 1 < mNumberRows) {
      if (mRowPointer->getValue<unsigned int>(i) > mRowPointer->getValue<unsigned int>(i+1)) {
        XdmfError::message(XdmfError::FATAL,
                           "Error: getValuesString(), Sparse Matrix Row Pointer is not sorted.");
      }
    }
    unsigned int index = 0;
    for(unsigned int j=mRowPointer->getValue<unsigned int>(i);
        j<mRowPointer->getValue<unsigned int>(i+1);
        ++j) {
      const unsigned int k = mColumnIndex->getValue<unsigned int>(j);
      while(index++ < k) {
        toReturn << "0.0, ";
      }
      toReturn << mValues->getValue<double>(j) << ", ";
    }
    while(index++ < mNumberColumns) {
      toReturn << "0.0, ";
    }
    toReturn << std::endl;
  }

  return toReturn.str();

}

void
XdmfSparseMatrix::populateItem(const std::map<std::string, std::string> & itemProperties,
                               const std::vector<shared_ptr<XdmfItem> > & childItems,
                               const XdmfCoreReader * const reader)
{
  XdmfItem::populateItem(itemProperties, childItems, reader);
  std::map<std::string, std::string>::const_iterator name =
    itemProperties.find("Name");
  if(name != itemProperties.end()) {
    mName = name->second;
  }
  else  {
    XdmfError::message(XdmfError::FATAL,
                       "'Name' not found in itemProperties in "
                       "XdmfSparseMatrix::populateItem");
  }
  std::map<std::string, std::string>::const_iterator numberRows =
    itemProperties.find("NumberRows");
  if(numberRows != itemProperties.end()) {
    mNumberRows = std::atoi(numberRows->second.c_str());
  }
  else  {
    XdmfError::message(XdmfError::FATAL,
                       "'NumberRows' not found in itemProperties in "
                       "XdmfSparseMatrix::populateItem");
  }
  std::map<std::string, std::string>::const_iterator numberColumns =
    itemProperties.find("NumberColumns");
  if(numberColumns != itemProperties.end()) {
    mNumberColumns = std::atoi(numberColumns->second.c_str());
  }
  else  {
    XdmfError::message(XdmfError::FATAL,
                       "'NumberColumns' not found in itemProperties in "
                       "XdmfSparseMatrix::populateItem");
  }

  std::vector<shared_ptr<XdmfArray> > arrayVector;
  arrayVector.reserve(3);
  for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter =
        childItems.begin();
      iter != childItems.end();
      ++iter) {
    if(shared_ptr<XdmfArray> array = shared_dynamic_cast<XdmfArray>(*iter)) {
      arrayVector.push_back(array);
    }
  }

  if(arrayVector.size() < 3) {
    // The three required arrays are for
    // the row pointer, column index, and the contained values.
    // Without these arrays the object can't be properly built.
    XdmfError::message(XdmfError::FATAL,
                       "Expected 3 arrays attached to "
                       "XdmfSparseMatrix::populateItem");
  }
  mRowPointer = arrayVector[0];
  mColumnIndex = arrayVector[1];
  mValues = arrayVector[2];
}

void
XdmfSparseMatrix::setColumnIndex(const shared_ptr<XdmfArray> columnIndex)
{
  mColumnIndex = columnIndex;
}

void
XdmfSparseMatrix::setName(const std::string & name)
{
  mName = name;
}

void
XdmfSparseMatrix::setRowPointer(const shared_ptr<XdmfArray> rowPointer)
{
  mRowPointer = rowPointer;
}

void
XdmfSparseMatrix::setValues(const shared_ptr<XdmfArray> values)
{
  mValues = values;
}

void
XdmfSparseMatrix::traverse(const shared_ptr<XdmfBaseVisitor> visitor)
{
  XdmfItem::traverse(visitor);
  mRowPointer->accept(visitor);
  mColumnIndex->accept(visitor);
  mValues->accept(visitor);
}
