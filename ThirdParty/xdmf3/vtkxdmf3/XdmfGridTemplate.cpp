/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfTemplate.cpp                                                    */
/*                                                                           */
/*  Author:                                                                  */
/*     Andrew Burns                                                          */
/*     andrew.j.burns2@us.army.mil                                           */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2013 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/

#include <sstream>
#include <utility>
#include "XdmfArray.hpp"
#include "XdmfCurvilinearGrid.hpp"
#include "XdmfItem.hpp"
#include "XdmfItemFactory.hpp"
#include "XdmfReader.hpp"
#include "XdmfGridCollection.hpp"
#include "XdmfGridCollectionType.hpp"
#include "XdmfGridTemplate.hpp"
#include "XdmfRectilinearGrid.hpp"
#include "XdmfRegularGrid.hpp"
#include "XdmfTemplate.hpp"
#include "XdmfTopology.hpp"
#include "XdmfError.hpp"
#include "XdmfUnstructuredGrid.hpp"
#include "XdmfVisitor.hpp"
#include "XdmfWriter.hpp"

#include "XdmfSystemUtils.hpp"

#include <stdio.h>

shared_ptr<XdmfGridTemplate>
XdmfGridTemplate::New()
{
  shared_ptr<XdmfGridTemplate> p(new XdmfGridTemplate());
  return p;
}


XdmfGridTemplate::XdmfGridTemplate() :
  XdmfTemplate(),
  XdmfGridCollection(),
  mTimeCollection(XdmfArray::New())
{
  mTimeCollection->setName("Time Collection");
}

XdmfGridTemplate::~XdmfGridTemplate()
{
}

const std::string XdmfGridTemplate::ItemTag = "Template";

unsigned int
XdmfGridTemplate::addStep()
{
  XdmfTemplate::addStep();
  if (shared_dynamic_cast<XdmfGrid>(mBase)->getTime()) {
    if (!mTimeCollection->isInitialized()) {
      mTimeCollection->read();
    }
    mTimeCollection->pushBack(shared_dynamic_cast<XdmfGrid>(mBase)->getTime()->getValue());
  }
  return mCurrentStep;
}

std::map<std::string, std::string>
XdmfGridTemplate::getItemProperties() const
{
  std::map<std::string, std::string> templateProperties = XdmfGridCollection::getItemProperties();

  templateProperties["BaseType"] = "Grid";
  return templateProperties;
}

std::string
XdmfGridTemplate::getItemTag() const
{
  return ItemTag;
}

shared_ptr<XdmfArray>
XdmfGridTemplate::getTimes()
{
  return mTimeCollection;
}

shared_ptr<XdmfGridCollection>
XdmfGridTemplate::getGridCollection(const unsigned int index)
{
  if (mBase) {
    if (index < getNumberSteps()) {
      this->clearStep();
      this->setStep(index);
      if (shared_ptr<XdmfGridCollection> grid =
            shared_dynamic_cast<XdmfGridCollection>(mBase)) {
        return grid;
      }
      else {
        return shared_ptr<XdmfGridCollection>();
      }
    }
    else {
      return shared_ptr<XdmfGridCollection>();
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Attempting to get GridCollection from template without a base");
    return shared_ptr<XdmfGridCollection>();
  }
}

shared_ptr<const XdmfGridCollection>
XdmfGridTemplate::getGridCollection(const unsigned int index) const
{
  if (shared_ptr<XdmfGridCollection> grid =
      shared_dynamic_cast<XdmfGridCollection>(mBase)) {
    if (index != mCurrentStep)
    {
      XdmfError::message(XdmfError::FATAL, "Error: GridTemplates can not return a constant reference to its base on an index other than the currently loaded one.");
      return shared_ptr<XdmfGridCollection>();
    }
    else
    {
      return grid;
    }
  }
  else {
    return shared_ptr<XdmfGridCollection>();
  }
}

shared_ptr<XdmfGridCollection>
XdmfGridTemplate::getGridCollection(const std::string & Name)
{
  if (mBase) {
   if (shared_ptr<XdmfGridCollection> grid =
          shared_dynamic_cast<XdmfGridCollection>(mBase)) {
      if (grid->getName().compare(Name) == 0) {
        return grid;
      }
      else {
        return shared_ptr<XdmfGridCollection>();
      }
    }
    else {
      return shared_ptr<XdmfGridCollection>();
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Attempting to get GridCollection from template without a base");
    return shared_ptr<XdmfGridCollection>();
  }
}

shared_ptr<const XdmfGridCollection>
XdmfGridTemplate::getGridCollection(const std::string & Name) const
{
  if (mBase) {
   if (shared_ptr<XdmfGridCollection> grid =
          shared_dynamic_cast<XdmfGridCollection>(mBase)) {
      if (grid->getName().compare(Name) == 0) {
        return grid;
      }
      else {
        return shared_ptr<XdmfGridCollection>();
      }
    }
    else {
      return shared_ptr<XdmfGridCollection>();
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Attempting to get GridCollection from template without a base");
    return shared_ptr<XdmfGridCollection>();
  }
}

unsigned int
XdmfGridTemplate::getNumberGridCollections() const
{
  if (shared_ptr<XdmfGridCollection> grid =
        shared_dynamic_cast<XdmfGridCollection>(mBase)) {
    return this->getNumberSteps();
  }
  else {
    return 0;
  }
}

void
XdmfGridTemplate::insert(const shared_ptr<XdmfGridCollection> GridCollection)
{
  XdmfError::message(XdmfError::FATAL, "Error: Attempting to use insert to add an XdmfGridCollection to an XdmfGridTemplate. "
                                       "Use addStep instead of insert to add to an XdmfGridTemplate");
}

void
XdmfGridTemplate::removeGridCollection(const unsigned int index)
{
  if (mBase) {
    if (index < getNumberSteps()) {
      if (shared_ptr<XdmfGridCollection> grid =
            shared_dynamic_cast<XdmfGridCollection>(mBase)) {
        this->removeStep(index);
      }
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Attempting to get GridCollection from template without a base");
  }
}

void
XdmfGridTemplate::removeGridCollection(const std::string & Name)
{
  XdmfError::message(XdmfError::FATAL, "Error: Removing Grids by name from XdmfGridTemplate is not supported");
}

shared_ptr<XdmfCurvilinearGrid>
XdmfGridTemplate::getCurvilinearGrid(const unsigned int index)
{
  if (mBase) {
    if (index < getNumberSteps()) {
      this->clearStep();
      this->setStep(index);
      if (shared_ptr<XdmfCurvilinearGrid> grid =
            shared_dynamic_cast<XdmfCurvilinearGrid>(mBase)) {
        return grid;
      }
      else {
        return shared_ptr<XdmfCurvilinearGrid>();
      }
    }
    else {
      return shared_ptr<XdmfCurvilinearGrid>();
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Attempting to get CurvilinearGrid from template without a base");
    return shared_ptr<XdmfCurvilinearGrid>();
  }
}

shared_ptr<const XdmfCurvilinearGrid>
XdmfGridTemplate::getCurvilinearGrid(const unsigned int index) const
{
  if (shared_ptr<XdmfCurvilinearGrid> grid =
      shared_dynamic_cast<XdmfCurvilinearGrid>(mBase)) {
    if (index != mCurrentStep)
    {
      XdmfError::message(XdmfError::FATAL, "Error: GridTemplates can not return a constant reference to its base on an index other than the currently loaded one.");
      return shared_ptr<XdmfCurvilinearGrid>();
    }
    else
    {
      return grid;
    }
  }
  else {
    return shared_ptr<XdmfCurvilinearGrid>();
  }
}

shared_ptr<XdmfCurvilinearGrid>
XdmfGridTemplate::getCurvilinearGrid(const std::string & Name)
{
  if (mBase) {
   if (shared_ptr<XdmfCurvilinearGrid> grid =
          shared_dynamic_cast<XdmfCurvilinearGrid>(mBase)) {
      if (grid->getName().compare(Name) == 0) {
        return grid;
      }
      else {
        return shared_ptr<XdmfCurvilinearGrid>();
      }
    }
    else {
      return shared_ptr<XdmfCurvilinearGrid>();
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Attempting to get CurvilinearGrid from template without a base");
    return shared_ptr<XdmfCurvilinearGrid>();
  }
}

shared_ptr<const XdmfCurvilinearGrid>
XdmfGridTemplate::getCurvilinearGrid(const std::string & Name) const
{
  if (mBase) {
   if (shared_ptr<XdmfCurvilinearGrid> grid =
          shared_dynamic_cast<XdmfCurvilinearGrid>(mBase)) {
      if (grid->getName().compare(Name) == 0) {
        return grid;
      }
      else {
        return shared_ptr<XdmfCurvilinearGrid>();
      }
    }
    else {
      return shared_ptr<XdmfCurvilinearGrid>();
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Attempting to get CurvilinearGrid from template without a base");
    return shared_ptr<XdmfCurvilinearGrid>();
  }
}

unsigned int
XdmfGridTemplate::getNumberCurvilinearGrids() const
{
  if (shared_ptr<XdmfCurvilinearGrid> grid =
        shared_dynamic_cast<XdmfCurvilinearGrid>(mBase)) {
    return this->getNumberSteps();
  }
  else {
    return 0;
  }
}

void
XdmfGridTemplate::insert(const shared_ptr<XdmfCurvilinearGrid> CurvilinearGrid)
{
  XdmfError::message(XdmfError::FATAL, "Error: Attempting to use insert to add an XdmfCurvilinearGrid to an XdmfGridTemplate. "
                                       "Use addStep instead of insert to add to an XdmfGridTemplate");
}

void
XdmfGridTemplate::removeCurvilinearGrid(const unsigned int index)
{
  if (mBase) {
    if (index < getNumberSteps()) {
      if (shared_ptr<XdmfCurvilinearGrid> grid =
            shared_dynamic_cast<XdmfCurvilinearGrid>(mBase)) {
        this->removeStep(index);
      }
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Attempting to get CurvilinearGrid from template without a base");
  }
}

void
XdmfGridTemplate::removeCurvilinearGrid(const std::string & Name)
{
  XdmfError::message(XdmfError::FATAL, "Error: Removing Grids by name from XdmfGridTemplate is not supported");
}

shared_ptr<XdmfRectilinearGrid>
XdmfGridTemplate::getRectilinearGrid(const unsigned int index)
{
  if (mBase) {
    if (index < getNumberSteps()) {
      this->clearStep();
      this->setStep(index);
      if (shared_ptr<XdmfRectilinearGrid> grid =
            shared_dynamic_cast<XdmfRectilinearGrid>(mBase)) {
        return grid;
      }
      else {
        return shared_ptr<XdmfRectilinearGrid>();
      }
    }
    else {
      return shared_ptr<XdmfRectilinearGrid>();
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Attempting to get RectilinearGrid from template without a base");
    return shared_ptr<XdmfRectilinearGrid>();
  }
}

shared_ptr<const XdmfRectilinearGrid>
XdmfGridTemplate::getRectilinearGrid(const unsigned int index) const
{
  if (shared_ptr<XdmfRectilinearGrid> grid =
      shared_dynamic_cast<XdmfRectilinearGrid>(mBase)) {
    if (index != mCurrentStep)
    {
      XdmfError::message(XdmfError::FATAL, "Error: GridTemplates can not return a constant reference to its base on an index other than the currently loaded one.");
      return shared_ptr<XdmfRectilinearGrid>();
    }
    else
    {
      return grid;
    }
  }
  else {
    return shared_ptr<XdmfRectilinearGrid>();
  }
}

shared_ptr<XdmfRectilinearGrid>
XdmfGridTemplate::getRectilinearGrid(const std::string & Name)
{
  if (mBase) {
   if (shared_ptr<XdmfRectilinearGrid> grid =
          shared_dynamic_cast<XdmfRectilinearGrid>(mBase)) {
      if (grid->getName().compare(Name) == 0) {
        return grid;
      }
      else {
        return shared_ptr<XdmfRectilinearGrid>();
      }
    }
    else {
      return shared_ptr<XdmfRectilinearGrid>();
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Attempting to get RectilinearGrid from template without a base");
    return shared_ptr<XdmfRectilinearGrid>();
  }
}

shared_ptr<const XdmfRectilinearGrid>
XdmfGridTemplate::getRectilinearGrid(const std::string & Name) const
{
  if (mBase) {
   if (shared_ptr<XdmfRectilinearGrid> grid =
          shared_dynamic_cast<XdmfRectilinearGrid>(mBase)) {
      if (grid->getName().compare(Name) == 0) {
        return grid;
      }
      else {
        return shared_ptr<XdmfRectilinearGrid>();
      }
    }
    else {
      return shared_ptr<XdmfRectilinearGrid>();
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Attempting to get RectilinearGrid from template without a base");
    return shared_ptr<XdmfRectilinearGrid>();
  }
}

unsigned int
XdmfGridTemplate::getNumberRectilinearGrids() const
{
  if (shared_ptr<XdmfRectilinearGrid> grid =
        shared_dynamic_cast<XdmfRectilinearGrid>(mBase)) {
    return this->getNumberSteps();
  }
  else {
    return 0;
  }
}

void
XdmfGridTemplate::insert(const shared_ptr<XdmfRectilinearGrid> RectilinearGrid)
{
  XdmfError::message(XdmfError::FATAL, "Error: Attempting to use insert to add a XdmfRectilinearGrid to an XdmfGridTemplate."
                                       "Use addStep instead of insert to add to an XdmfGridTemplate");
}

void
XdmfGridTemplate::removeRectilinearGrid(const unsigned int index)
{
  if (mBase) {
    if (index < getNumberSteps()) {
      if (shared_ptr<XdmfRectilinearGrid> grid =
            shared_dynamic_cast<XdmfRectilinearGrid>(mBase)) {
        this->removeStep(index);
      }
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Attempting to get RectilinearGrid from template without a base");
  }
}

void
XdmfGridTemplate::removeRectilinearGrid(const std::string & Name)
{
  XdmfError::message(XdmfError::FATAL, "Error: Removing Grids by name from XdmfGridTemplate is not supported");
}

shared_ptr<XdmfRegularGrid>
XdmfGridTemplate::getRegularGrid(const unsigned int index)
{
  if (mBase) {
    if (index < getNumberSteps()) {
      this->clearStep();
      this->setStep(index);
      if (shared_ptr<XdmfRegularGrid> grid =
            shared_dynamic_cast<XdmfRegularGrid>(mBase)) {
        return grid;
      }
      else {
        return shared_ptr<XdmfRegularGrid>();
      }
    }
    else {
      return shared_ptr<XdmfRegularGrid>();
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Attempting to get RegularGrid from template without a base");
    return shared_ptr<XdmfRegularGrid>();
  }
}

shared_ptr<const XdmfRegularGrid>
XdmfGridTemplate::getRegularGrid(const unsigned int index) const
{
  if (shared_ptr<XdmfRegularGrid> grid =
      shared_dynamic_cast<XdmfRegularGrid>(mBase)) {
    if (index != mCurrentStep)
    {
      XdmfError::message(XdmfError::FATAL, "Error: GridTemplates can not return a constant reference to its base on an index other than the currently loaded one.");
      return shared_ptr<XdmfRegularGrid>();
    }
    else
    {
      return grid;
    }
  }
  else {
    return shared_ptr<XdmfRegularGrid>();
  }
}

shared_ptr<XdmfRegularGrid>
XdmfGridTemplate::getRegularGrid(const std::string & Name)
{
  if (mBase) {
   if (shared_ptr<XdmfRegularGrid> grid =
          shared_dynamic_cast<XdmfRegularGrid>(mBase)) {
      if (grid->getName().compare(Name) == 0) {
        return grid;
      }
      else {
        return shared_ptr<XdmfRegularGrid>();
      }
    }
    else {
      return shared_ptr<XdmfRegularGrid>();
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Attempting to get RegularGrid from template without a base");
    return shared_ptr<XdmfRegularGrid>();
  }
}

shared_ptr<const XdmfRegularGrid>
XdmfGridTemplate::getRegularGrid(const std::string & Name) const
{
  if (mBase) {
   if (shared_ptr<XdmfRegularGrid> grid =
          shared_dynamic_cast<XdmfRegularGrid>(mBase)) {
      if (grid->getName().compare(Name) == 0) {
        return grid;
      }
      else {
        return shared_ptr<XdmfRegularGrid>();
      }
    }
    else {
      return shared_ptr<XdmfRegularGrid>();
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Attempting to get RegularGrid from template without a base");
    return shared_ptr<XdmfRegularGrid>();
  }
}

unsigned int
XdmfGridTemplate::getNumberRegularGrids() const
{
  if (shared_ptr<XdmfRegularGrid> grid =
        shared_dynamic_cast<XdmfRegularGrid>(mBase)) {
    return this->getNumberSteps();
  }
  else {
    return 0;
  }
}

void
XdmfGridTemplate::insert(const shared_ptr<XdmfRegularGrid> RegularGrid)
{
  XdmfError::message(XdmfError::FATAL, "Error: Attempting to use insert to add an XdmfRegularGrid to an XdmfGridTemplate."
                                       "Use addStep instead of insert to add to an XdmfGridTemplate");
}

void
XdmfGridTemplate::removeRegularGrid(const unsigned int index)
{
  if (mBase) {
    if (index < getNumberSteps()) {
      if (shared_ptr<XdmfRegularGrid> grid =
            shared_dynamic_cast<XdmfRegularGrid>(mBase)) {
        this->removeStep(index);
      }
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Attempting to get RegularGrid from template without a base");
  }
}

void
XdmfGridTemplate::removeRegularGrid(const std::string & Name)
{
  XdmfError::message(XdmfError::FATAL, "Error: Removing Grids by name from XdmfGridTemplate is not supported");
}

shared_ptr<XdmfUnstructuredGrid>
XdmfGridTemplate::getUnstructuredGrid(const unsigned int index)
{
  if (mBase) {
    if (index < getNumberSteps()) {
      this->clearStep();
      this->setStep(index);
      if (shared_ptr<XdmfUnstructuredGrid> grid =
            shared_dynamic_cast<XdmfUnstructuredGrid>(mBase)) {
        return grid;
      }
      else {
        return shared_ptr<XdmfUnstructuredGrid>();
      }
    }
    else {
      return shared_ptr<XdmfUnstructuredGrid>();
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Attempting to get UnstructuredGrid from template without a base");
    return shared_ptr<XdmfUnstructuredGrid>();
  }
}

shared_ptr<const XdmfUnstructuredGrid>
XdmfGridTemplate::getUnstructuredGrid(const unsigned int index) const
{
  if (shared_ptr<XdmfUnstructuredGrid> grid =
      shared_dynamic_cast<XdmfUnstructuredGrid>(mBase)) {
    if (index != mCurrentStep)
    {
      XdmfError::message(XdmfError::FATAL, "Error: GridTemplates can not return a constant reference to its base on an index other than the currently loaded one.");
      return shared_ptr<XdmfUnstructuredGrid>();
    }
    else
    {
      return grid;
    }
  }
  else {
    return shared_ptr<XdmfUnstructuredGrid>();
  }
}

shared_ptr<XdmfUnstructuredGrid>
XdmfGridTemplate::getUnstructuredGrid(const std::string & Name)
{
  if (mBase) {
   if (shared_ptr<XdmfUnstructuredGrid> grid =
          shared_dynamic_cast<XdmfUnstructuredGrid>(mBase)) {
      if (grid->getName().compare(Name) == 0) {
        return grid;
      }
      else {
        return shared_ptr<XdmfUnstructuredGrid>();
      }
    }
    else {
      return shared_ptr<XdmfUnstructuredGrid>();
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Attempting to get UnstructuredGrid from template without a base");
    return shared_ptr<XdmfUnstructuredGrid>();
  }
}

shared_ptr<const XdmfUnstructuredGrid>
XdmfGridTemplate::getUnstructuredGrid(const std::string & Name) const
{
  if (mBase) {
   if (shared_ptr<XdmfUnstructuredGrid> grid =
          shared_dynamic_cast<XdmfUnstructuredGrid>(mBase)) {
      if (grid->getName().compare(Name) == 0) {
        return grid;
      }
      else {
        return shared_ptr<XdmfUnstructuredGrid>();
      }
    }
    else {
      return shared_ptr<XdmfUnstructuredGrid>();
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Attempting to get UnstructuredGrid from template without a base");
    return shared_ptr<XdmfUnstructuredGrid>();
  }
}

unsigned int
XdmfGridTemplate::getNumberUnstructuredGrids() const
{
  if (shared_ptr<XdmfUnstructuredGrid> grid =
        shared_dynamic_cast<XdmfUnstructuredGrid>(mBase)) {
    return this->getNumberSteps();
  }
  else {
    return 0;
  }
}


void
XdmfGridTemplate::insert(const shared_ptr<XdmfUnstructuredGrid> UnstructuredGrid)
{
  XdmfError::message(XdmfError::FATAL, "Error: Attempting to use insert to add an XdmfUnstructuredGrid to an XdmfGridTemplate."
                                       "Use addStep instead of insert to add to an XdmfGridTemplate");
}

void
XdmfGridTemplate::removeUnstructuredGrid(const unsigned int index)
{
  if (mBase) {
    if (index < getNumberSteps()) {
      if (shared_ptr<XdmfUnstructuredGrid> grid =
            shared_dynamic_cast<XdmfUnstructuredGrid>(mBase)) {
        this->removeStep(index);
      }
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Attempting to get UnstructuredGrid from template without a base");
  }
}


void
XdmfGridTemplate::removeUnstructuredGrid(const std::string & Name)
{
  XdmfError::message(XdmfError::FATAL, "Error: Removing Grids by name from XdmfGridTemplate is not supported");
}

void
XdmfGridTemplate::populateItem(const std::map<std::string, std::string> & itemProperties,
                               const std::vector<shared_ptr<XdmfItem> > & childItems,
                               const XdmfCoreReader * const reader)
{
  // We are overrriding the populate item of the template and grid collection here
  // The template functions internally different from either. 

  this->setType(XdmfGridCollectionType::New(itemProperties));

  // The first child item is the base
  mBase = childItems[0];
  mCurrentStep = 0;

  if (childItems.size() > 1) {
    for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter =
          childItems.begin() + 1;
        iter != childItems.end();
        ++iter) {
      if(shared_ptr<XdmfArray> array = shared_dynamic_cast<XdmfArray>(*iter)) {
        if (array->getName().compare("Data Description") == 0) {
          // Split description into substrings based on the " character

          if (array->getNumberHeavyDataControllers() > 0 && !mHeavyWriter) {
            mHeavyWriter = reader->generateHeavyDataWriter(array->getHeavyDataController(0)->getName(), array->getHeavyDataController(0)->getFilePath());
          }

          array->read();

          // If a character array, create std::string version? TODO
          std::string descriptionString;
          if (array->getArrayType() == XdmfArrayType::Int8())
          {
            descriptionString = std::string((char *)array->getValuesInternal());
          }
          else if (array->getArrayType() == XdmfArrayType::String())
          {
            descriptionString = array->getValue<std::string>(0);
          }

          size_t index = descriptionString.find_first_of("\"");
          size_t previousIndex = 0;

          if (index != std::string::npos) {
            // Removing the prepended "
            previousIndex = index + 1;
            index = descriptionString.find_first_of("\"", previousIndex);
          }

          while (index != std::string::npos) {
            std::string type = descriptionString.substr(previousIndex, index - previousIndex);
            mDataTypes.push_back(type);
            previousIndex = index + 1;
            index = descriptionString.find_first_of("\"", previousIndex);
            if (index - previousIndex > 0) {
              std::string description;
              description = descriptionString.substr(previousIndex, index - previousIndex);
              mDataDescriptions.push_back(description);
              // create controllers here based on the type/description?
              // Is array type already filled?
              // Potentially call "fillControllers" after populating?
              if (index != std::string::npos) {
                previousIndex = index + 1;
                index = descriptionString.find_first_of("\"", previousIndex);
              }
            }
            else {
              XdmfError::message(XdmfError::FATAL, "Error: Type without a description in XdmfTemplate::populateItem");
            }
          }
        }
        else if (array->getName().compare("Time Collection") == 0) {
          mTimeCollection = array;
        }
        else {
          mTrackedArrays.push_back(array.get());
          mTrackedArrayDims.push_back(array->getDimensions());
          mTrackedArrayTypes.push_back(array->getArrayType());
        }
      }
    }
  }
  mDataControllers.resize(mDataTypes.size());
  if (!mItemFactory) {
    mItemFactory = XdmfItemFactory::New();
  }
  std::map<std::string, std::string> populateProperties;
  if (mHeavyWriter) {
    // The heavy writer provides the XMLDir, which is used to get full paths for the controllers
    // It is assumed that the files that the controllers reference are in the same directory
    // as the file that the writer references
    std::string filepath = XdmfSystemUtils::getRealPath(mHeavyWriter->getFilePath());
    size_t index = filepath.find_last_of("/\\");
    filepath = filepath.substr(0, index + 1);
    populateProperties["XMLDir"] = filepath;
  }
  for (unsigned int i = 0;  i < mDataDescriptions.size(); ++i) {
    populateProperties["Content"] = mDataDescriptions[i];
    std::vector<shared_ptr<XdmfHeavyDataController> > readControllers =
      reader->generateHeavyDataControllers(populateProperties, mTrackedArrayDims[i % mTrackedArrays.size()], mTrackedArrayTypes[i % mTrackedArrays.size()], mDataTypes[i]);
    if (readControllers.size() > 0) {
      // Heavy data controllers reference the data
      for (unsigned int j = 0; j < readControllers.size(); ++j) {
        mDataControllers[i].push_back(readControllers[j]);
      }
    }
  }
  // Compare the first set of controllers to the size of the first array
  unsigned int controllerTotal = 0;
  for (unsigned int i = 0; i < mDataControllers[0].size(); ++i)
  {
    controllerTotal += mDataControllers[0][i]->getSize();
  }
  // If the array is smaller, set the writer to append.
  if (controllerTotal > mTrackedArrays[0]->getSize())
  {
    mHeavyWriter->setMode(XdmfHeavyDataWriter::Append);
    mNumSteps = controllerTotal / mTrackedArrays[0]->getSize();
  }
  else {
    mNumSteps = mDataControllers.size() / mTrackedArrays.size();
  }
}

void
XdmfGridTemplate::removeStep(unsigned int stepId)
{
  if (stepId < this->getNumberSteps()) {
    XdmfTemplate::removeStep(stepId);
    mTimeCollection->erase(stepId);
  }
  this->setIsChanged(true);
}

void
XdmfGridTemplate::setBase(shared_ptr<XdmfItem> newBase)
{
  if (shared_ptr<XdmfGrid> grid = shared_dynamic_cast<XdmfGrid>(newBase)) {
    XdmfTemplate::setBase(newBase);
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: XdmfGridTemplate::setBase,"
                                         " attempting to set a Base that is not grid type.");
  }
}

void
XdmfGridTemplate::setStep(unsigned int stepId)
{
  XdmfTemplate::setStep(stepId);
  if (mTimeCollection->getSize() >= stepId) {
    if (!mTimeCollection->isInitialized()) {
      mTimeCollection->read();
    }
    if (shared_dynamic_cast<XdmfGrid>(mBase)->getTime()) {
      shared_dynamic_cast<XdmfGrid>(mBase)->getTime()->setValue(mTimeCollection->getValue<double>(stepId));
    }
    else {
      shared_dynamic_cast<XdmfGrid>(mBase)->setTime(XdmfTime::New(mTimeCollection->getValue<double>(stepId)));
    }
  }
}

void
XdmfGridTemplate::setStep(shared_ptr<XdmfTime> time)
{
  if (mTimeCollection->getSize() > 0)
  {
    if (!mTimeCollection->isInitialized()) {
      mTimeCollection->read();
    }
    unsigned int index = 0;
    while (index < mTimeCollection->getSize() &&
           time->getValue() != mTimeCollection->getValue<double>(index))
    {
      ++index;
    }
    if (index < mTimeCollection->getSize())
    {
      this->setStep(index);
    }
  }
}

void
XdmfGridTemplate::traverse(const shared_ptr<XdmfBaseVisitor> visitor)
{
  // We are only using the template traverse
  // since the grid data is only held in the Base
  if (mTimeCollection->getSize() > 0)
  {
    this->setType(XdmfGridCollectionType::Temporal());
  }
  else
  {
    this->setType(XdmfGridCollectionType::Spatial());
  }
  XdmfTemplate::traverse(visitor);
  mTimeCollection->accept(visitor);
}

XDMF_ITEM_C_CHILD_WRAPPER(XdmfGridTemplate, XDMFGRIDTEMPLATE)
