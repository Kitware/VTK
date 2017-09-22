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
#include <climits>
#include <set>
#include "XdmfArray.hpp"
#include "XdmfBinaryController.hpp"
#include "XdmfError.hpp"
#include "XdmfHDF5Controller.hpp"
#include "XdmfItem.hpp"
#include "XdmfItemFactory.hpp"
#include "XdmfReader.hpp"
#include "XdmfTemplate.hpp"
#include "XdmfStringUtils.hpp"
#include "XdmfSystemUtils.hpp"
#include "XdmfVisitor.hpp"
#include "XdmfWriter.hpp"

#include <stdio.h>

std::vector<shared_ptr<XdmfHeavyDataController> >
getStepControllers(unsigned int stepId,
                   std::vector<unsigned int> stepDims,
                   std::vector<shared_ptr<XdmfHeavyDataController> > datasetControllers)
{
  std::vector<shared_ptr<XdmfHeavyDataController> > returnVector;
  if (datasetControllers.size() > 0)
  {
    unsigned int sizePerStep = 1;
    for (unsigned int i = 0; i < stepDims.size(); ++i)
    {
      sizePerStep *= stepDims[i];
    }
//    unsigned int offset = (sizePerStep * stepId);
//    unsigned int offsetStepsRemaining = 0;
    unsigned int offset = 0;
    unsigned int offsetStepsRemaining = stepId;
    // grabbing the subset is a little different for each type
    // Right now we assume controllers are of the same type
    unsigned int controllerIndex = 0;
    unsigned int sizeRemaining = sizePerStep;
    unsigned int arrayoffset = 0;
    while (sizeRemaining > 0)
    {
//printf("sizeRemaining = %u\n", sizeRemaining);
      // We don't reset the controller index between runs of the while loop
      // On iterations after the first it should only execute the loop once
      // because offset is set to 0
      while (controllerIndex < datasetControllers.size())
      {
//printf("offset = %u\n", offset);
//printf("%u >= %u\n", offset, datasetControllers[controllerIndex]->getSize());
        //Iterate until we find the controller that the step starts in
        if (offset >= datasetControllers[controllerIndex]->getSize())
        {
          offset -= datasetControllers[controllerIndex]->getSize();
          ++controllerIndex;
        }
        else
        {
          if (offsetStepsRemaining == 0)
          {
            // offset is within the current controller
            break;
          }
          else
          {
            // There are steps left to offset
//printf("accounting for step %d\n", offsetStepsRemaining);
            offset += sizePerStep;
            --offsetStepsRemaining;
          }
        }
      }
//printf("final offset = %u\n", offset);
      std::vector<unsigned int> newDimVector;
      std::vector<unsigned int> newStarts;
//printf("after creating dim vector but before filling it\n");
//printf("%d < %d\n", controllerIndex, datasetControllers.size());
//printf("size left %d\n", sizeRemaining);
      if (offset + sizeRemaining <= datasetControllers[controllerIndex]->getSize())
      {
//printf("step is entirely in the controller\n");
        // step is entirely within this controller
        newStarts.push_back(offset + datasetControllers[controllerIndex]->getStart()[0]); // TODO multidim version
        newDimVector.push_back(sizeRemaining);
        sizeRemaining = 0;
      }
      else
      {
//printf("step is partially in the controller\n");
        if (controllerIndex + 1 >= datasetControllers.size()) {
          // Error, step doesn't fit in the data set provided
          XdmfError::message(XdmfError::FATAL, "Error: Step does not fit in data step provided");
        }
        // step is partially in this controller
        newDimVector.push_back(sizeRemaining -
                               (sizeRemaining - (datasetControllers[controllerIndex]->getSize() - offset)));
        newStarts.push_back(offset+datasetControllers[controllerIndex]->getStart()[0]); // TODO multidim version
        sizeRemaining -= newDimVector[0];
      }
//printf("size for other controllers %d\n", sizeRemaining);
//printf("before creating the new controller\n");
      // Using the remaining space in the controller
      // Slightly differen creation method for each controller
      if (datasetControllers[0]->getName().compare("Binary") == 0) {
        shared_ptr<XdmfBinaryController> createdController =
          XdmfBinaryController::New(datasetControllers[0]->getFilePath(),
                                    datasetControllers[0]->getType(),
                                    shared_dynamic_cast<XdmfBinaryController>(
                                      datasetControllers[0])->getEndian(),
                                    newStarts[0],
                                    newDimVector);
        returnVector.push_back(createdController);
      }
      else if (datasetControllers[0]->getName().compare("HDF") == 0) {
        // TODO
        // The writer should only write to contiguous sets when in this mode.
        // A user would need to do something custom to foul this up.
        std::vector<unsigned int> newStrides;
        newStrides.push_back(1);
        shared_ptr<XdmfHDF5Controller> createdController =
          XdmfHDF5Controller::New(datasetControllers[controllerIndex]->getFilePath(),
                                  shared_dynamic_cast<XdmfHDF5Controller>(
                                    datasetControllers[controllerIndex])->getDataSetPath(),
                                  datasetControllers[0]->getType(),
                                  newStarts,
                                  newStrides,
                                  newDimVector,
                                  shared_dynamic_cast<XdmfHDF5Controller>(
                                    datasetControllers[controllerIndex])->getDataspaceDimensions());
        returnVector.push_back(createdController);
      }
//printf("after creating the new controller\n");
      returnVector[returnVector.size()-1]->setArrayOffset(arrayoffset);
      arrayoffset += returnVector[returnVector.size()-1]->getSize();
      offset = 0;
      ++controllerIndex;
      // Starts at the beggining of the next controller
    }
  }
/*
printf("size of return vector = %d\n", returnVector.size());
for (unsigned int i = 0; i < returnVector.size(); ++i)
{
  shared_ptr<XdmfHDF5Controller> currentController = shared_dynamic_cast<XdmfHDF5Controller>(returnVector[i]);
assert(currentController);
  printf("file = %s\n", currentController->getFilePath().c_str());
  printf("dataset = %s\n", currentController->getDataSetPath().c_str());
  printf("start = %u\n", currentController->getStart()[0]);
  printf("dimension = %u\n", currentController->getDimensions()[0]);
  printf("dataspace = %u\n", currentController->getDataspaceDimensions()[0]);
}
*/
  return returnVector;
}

std::vector<shared_ptr<XdmfHeavyDataController> >
getControllersExcludingStep(unsigned int stepId,
                            std::vector<unsigned int> stepDims,
                            std::vector<shared_ptr<XdmfHeavyDataController> > datasetControllers)
{
  std::vector<shared_ptr<XdmfHeavyDataController> > returnVector;
  if (datasetControllers.size() > 0)
  {
    unsigned int sizePerStep = 1;
    for (unsigned int i = 0; i < stepDims.size(); ++i)
    {
      sizePerStep *= stepDims[i];
    }
    unsigned int offset = sizePerStep * stepId;
    unsigned int sizeRemaining = sizePerStep;
//printf("base offset = %u\nstarting size remaining = %u\ncutting from %u controllers\n", offset, sizeRemaining, datasetControllers.size());
    // grabbing the subset is a little different for each type
    // Right now we assume controllers are of the same type
    for (unsigned int controllerIndex = 0; controllerIndex < datasetControllers.size(); ++controllerIndex)
    {
//printf("offset = %u out of controller size %u\n", offset, datasetControllers[controllerIndex]->getSize());
      if (offset >= datasetControllers[controllerIndex]->getSize())
      {
        // The removed step isn't in the controller provided
        // Simply add it back into the return set
        returnVector.push_back(datasetControllers[controllerIndex]);
        // then subtract the size from the offset
        offset -= datasetControllers[controllerIndex]->getSize();
      }
      else
      {
        // The removed step is inside the controller provided
        if (offset > 0)
        {
//printf("removed step is inside this controller\n");
          // If offset is greater than zero the controller has a section chopped off the front
          std::vector<unsigned int> newDim;
          newDim.push_back(offset);
          // Dataspace is the same
          // stride is the same
          // start is the same
          // TODO dims is reduced to just cover the offset size
          if (datasetControllers[controllerIndex]->getName().compare("Binary") == 0) {
            shared_ptr<XdmfBinaryController> createdController =
              XdmfBinaryController::New(datasetControllers[controllerIndex]->getFilePath(),
                                        datasetControllers[controllerIndex]->getType(),
                                        shared_dynamic_cast<XdmfBinaryController>(
                                          datasetControllers[controllerIndex])->getEndian(),
                                        shared_dynamic_cast<XdmfBinaryController>(
                                          datasetControllers[controllerIndex])->getSeek(),
                                        datasetControllers[controllerIndex]->getStart(),
                                        datasetControllers[controllerIndex]->getStride(),
                                        newDim,
                                        datasetControllers[controllerIndex]->getDataspaceDimensions());
            returnVector.push_back(createdController);
          }
          else if (datasetControllers[controllerIndex]->getName().compare("HDF") == 0) {
            // TODO
            // The writer should only write to contiguous sets when in this mode.
            // A user would need to do something custom to foul this up.
            shared_ptr<XdmfHDF5Controller> createdController =
              XdmfHDF5Controller::New(datasetControllers[controllerIndex]->getFilePath(),
                                      shared_dynamic_cast<XdmfHDF5Controller>(
                                        datasetControllers[controllerIndex])->getDataSetPath(),
                                      datasetControllers[controllerIndex]->getType(),
                                      datasetControllers[controllerIndex]->getStart(),
                                      datasetControllers[controllerIndex]->getStride(),
                                      newDim,
                                      shared_dynamic_cast<XdmfHDF5Controller>(
                                        datasetControllers[controllerIndex])->getDataspaceDimensions());
            returnVector.push_back(createdController);
          }
          // These are the stats for the first half of the dataset
          if (sizeRemaining <= datasetControllers[controllerIndex]->getSize() - offset)
          {
            // The controller is large enough to need to be split into two controllers
            std::vector<unsigned int> newStart; //TODO we're assuming one dim for now
            newStart.push_back(datasetControllers[controllerIndex]->getStart()[0] +sizeRemaining + offset);
            std::vector<unsigned int> newDim;
            newDim.push_back(datasetControllers[controllerIndex]->getSize() - (sizeRemaining + offset));
            // These are the stats of the second controller
            sizeRemaining = 0;
            if (datasetControllers[controllerIndex]->getName().compare("Binary") == 0) {
              shared_ptr<XdmfBinaryController> createdController =
                XdmfBinaryController::New(datasetControllers[controllerIndex]->getFilePath(),
                                          datasetControllers[controllerIndex]->getType(),
                                          shared_dynamic_cast<XdmfBinaryController>(
                                            datasetControllers[controllerIndex])->getEndian(),
                                          shared_dynamic_cast<XdmfBinaryController>(
                                            datasetControllers[controllerIndex])->getSeek(),
                                          newStart,
                                          datasetControllers[controllerIndex]->getStride(),
                                          newDim,
                                          datasetControllers[controllerIndex]->getDataspaceDimensions());
              returnVector.push_back(createdController);
            }
            else if (datasetControllers[controllerIndex]->getName().compare("HDF") == 0) {
              // TODO
              // The writer should only write to contiguous sets when in this mode.
              // A user would need to do something custom to foul this up.
              shared_ptr<XdmfHDF5Controller> createdController =
                XdmfHDF5Controller::New(datasetControllers[controllerIndex]->getFilePath(),
                                        shared_dynamic_cast<XdmfHDF5Controller>(
                                          datasetControllers[controllerIndex])->getDataSetPath(),
                                        datasetControllers[controllerIndex]->getType(),
                                        newStart,
                                        datasetControllers[controllerIndex]->getStride(),
                                        newDim,
                                        shared_dynamic_cast<XdmfHDF5Controller>(
                                          datasetControllers[controllerIndex])->getDataspaceDimensions());
              returnVector.push_back(createdController);
            }
          }
          else {
            // The controller only contains part of the dataset
            sizeRemaining -= (datasetControllers[controllerIndex]->getSize() - offset);
          }
          offset = 0;
        }
        else
        {
          // in the case of 0 offset, we either need to trim from the front or just use the whole controller
          if (sizeRemaining > 0)
          {
            if (sizeRemaining < datasetControllers[controllerIndex]->getSize())
            {
              std::vector<unsigned int> newStart;
              newStart.push_back(sizeRemaining);
              std::vector<unsigned int> newDim;
              newDim.push_back(datasetControllers[controllerIndex]->getSize() - sizeRemaining);
              sizeRemaining = 0;
              if (datasetControllers[controllerIndex]->getName().compare("Binary") == 0) {
                shared_ptr<XdmfBinaryController> createdController =
                  XdmfBinaryController::New(datasetControllers[controllerIndex]->getFilePath(),
                                            datasetControllers[controllerIndex]->getType(),
                                            shared_dynamic_cast<XdmfBinaryController>(
                                            datasetControllers[controllerIndex])->getEndian(),
                                            shared_dynamic_cast<XdmfBinaryController>(
                                              datasetControllers[controllerIndex])->getSeek(),
                                            newStart,
                                            datasetControllers[controllerIndex]->getStride(),
                                            newDim,
                                            datasetControllers[controllerIndex]->getDataspaceDimensions());
                returnVector.push_back(createdController);
              }
              else if (datasetControllers[controllerIndex]->getName().compare("HDF") == 0) {
                // TODO
                // The writer should only write to contiguous sets when in this mode.
                // A user would need to do something custom to foul this up.
                shared_ptr<XdmfHDF5Controller> createdController =
                  XdmfHDF5Controller::New(datasetControllers[controllerIndex]->getFilePath(),
                                          shared_dynamic_cast<XdmfHDF5Controller>(
                                            datasetControllers[controllerIndex])->getDataSetPath(),
                                          datasetControllers[controllerIndex]->getType(),
                                          newStart,
                                          datasetControllers[controllerIndex]->getStride(),
                                          newDim,
                                          shared_dynamic_cast<XdmfHDF5Controller>(
                                            datasetControllers[controllerIndex])->getDataspaceDimensions());
                returnVector.push_back(createdController);
              }
            }
            else {
              sizeRemaining -= datasetControllers[controllerIndex]->getSize();
            }
          }
          else
          {
            // Just use the current controller
            returnVector.push_back(datasetControllers[controllerIndex]);
          }
        }
      }
    }
  }
  return returnVector;
}

class XdmfArrayGatherer : public XdmfVisitor, public Loki::Visitor<XdmfArray>
{
  public:

    static shared_ptr<XdmfArrayGatherer>
    New(std::vector<XdmfArray *> * storageVector)
    {
       shared_ptr<XdmfArrayGatherer> p(new XdmfArrayGatherer(storageVector));
       return p;
    }

    ~XdmfArrayGatherer()
    {
    }

    virtual void
    visit(XdmfArray & array,
          const shared_ptr<XdmfBaseVisitor> visitor)
    {
      ++mDepth;
      if (!array.isInitialized())
      {
//      mStorage->push_back(&array);
        mArrayCollection.insert(&array);
      }
      array.traverse(visitor);
      --mDepth;
      if (mDepth == 0)
      {
        moveToStorage();
      }
    }

    virtual void
    visit(XdmfItem & item,
          const shared_ptr<XdmfBaseVisitor> visitor)
    {
      ++mDepth;
      item.traverse(visitor);
      --mDepth;
      if (mDepth == 0)
      {
        moveToStorage();
      }
    }

    void
    moveToStorage()
    {
      for (std::set<XdmfArray *>::iterator iter = mArrayCollection.begin();
           iter != mArrayCollection.end();
           ++iter)
      {
        mStorage->push_back(*iter);
      }
    }

  private: 

    XdmfArrayGatherer(std::vector<XdmfArray *> * storageVector) :
      mDepth(0),
      mStorage(storageVector)
    {
    }

  unsigned int mDepth;
  std::set<XdmfArray *> mArrayCollection;
  std::vector<XdmfArray *> * mStorage;
};

shared_ptr<XdmfTemplate>
XdmfTemplate::New()
{
  shared_ptr<XdmfTemplate> p(new XdmfTemplate());
  return p;
}


XdmfTemplate::XdmfTemplate() :
  mHeavyWriter(shared_ptr<XdmfHeavyDataWriter>()),
  mBase(shared_ptr<XdmfItem>()),
  mCurrentStep(-1),
  mNumSteps(0),
  mItemFactory(shared_ptr<XdmfItemFactory>())
{
}

XdmfTemplate::~XdmfTemplate()
{
}

const std::string XdmfTemplate::ItemTag = "Template";

unsigned int
XdmfTemplate::addStep()
{
  mCurrentStep = this->getNumberSteps();
  std::stringstream datastream;
  if (mTrackedArrays.size() < 1) {
    XdmfError::message(XdmfError::FATAL,
                       "Error: XdmfTemplate attempting to add a step when no arrays are tracked");
  }
  for (unsigned int arrayIndex = 0; arrayIndex < mTrackedArrays.size(); ++arrayIndex) {
    if (mTrackedArrayTypes.size() < mTrackedArrays.size()){
      mTrackedArrayTypes.resize(mTrackedArrays.size());
    }
    if (mTrackedArrayDims.size() < mTrackedArrays.size()){
      mTrackedArrayDims.resize(mTrackedArrays.size());
    }
    if (!mTrackedArrayTypes[arrayIndex]) {
      mTrackedArrayTypes[arrayIndex] = mTrackedArrays[arrayIndex]->getArrayType();
    }
    if (mTrackedArrayDims[arrayIndex].size() == 0) {
      mTrackedArrayDims[arrayIndex] = mTrackedArrays[arrayIndex]->getDimensions();
    }
    // Write the tracked arrays to heavy data if they aren't already
    if (mHeavyWriter) {
      bool revertToAppend = false;
      if (mHeavyWriter->getMode() == XdmfHeavyDataWriter::Append) {
        // Set to original heavy data controllers for append
        if (mDataControllers.size() > arrayIndex)
        {
          if (mDataControllers[arrayIndex].size() > 0)
          {
            while (mTrackedArrays[arrayIndex]->getNumberHeavyDataControllers() > 0) {
              mTrackedArrays[arrayIndex]->removeHeavyDataController(0);
            }
            for (unsigned int i = 0; i < mDataControllers[arrayIndex].size(); ++i)
            {
              mTrackedArrays[arrayIndex]->insert(mDataControllers[arrayIndex][i]);
            }
          }
        }
        else
        {
          // Creating new Dataset
          // Set to default mode so that it doesn't overlap
          mHeavyWriter->setMode(XdmfHeavyDataWriter::Default);
          revertToAppend = true;
        }
      }
      else if (mHeavyWriter->getMode() == XdmfHeavyDataWriter::Hyperslab) {
        // Use the controller that references the subset that will be overwritten
        if (!(arrayIndex < mDataControllers.size()))
        {
          // When in overwrite mode the dataset must be preallocated
          XdmfError::message(XdmfError::FATAL, "Error: Heavy Data dataset must be preallocated "
                                               "to use Hyperslab mode Templates");
        }
        std::vector<shared_ptr<XdmfHeavyDataController> > overwriteControllers =
          getStepControllers(mCurrentStep, mTrackedArrayDims[arrayIndex], mDataControllers[arrayIndex]);
        mTrackedArrays[arrayIndex]->setHeavyDataController(overwriteControllers);
      }
      mTrackedArrays[arrayIndex]->accept(mHeavyWriter);
      if (revertToAppend)
      {
        mHeavyWriter->setMode(XdmfHeavyDataWriter::Append);
      }
    }
    datastream.str(std::string());
    for (unsigned int controllerIndex = 0; controllerIndex < mTrackedArrays[arrayIndex]->getNumberHeavyDataControllers(); ++controllerIndex) {
      // TODO throw error if controller types don't match
      // For each heavy data controller
      std::string writerPath = XdmfSystemUtils::getRealPath(mHeavyWriter->getFilePath());
      std::string heavyDataPath =
        mTrackedArrays[arrayIndex]->getHeavyDataController(controllerIndex)->getFilePath();
      size_t index = heavyDataPath.find_last_of("/\\");
      if(index != std::string::npos) {
        // If path is not a folder
        // put the directory path into this variable
        const std::string heavyDataDir = heavyDataPath.substr(0, index + 1);
        // If the directory is in the XML File Path
        if(writerPath.find(heavyDataDir) == 0) {
          heavyDataPath =
            heavyDataPath.substr(heavyDataDir.size(),
                                 heavyDataPath.size() - heavyDataDir.size());
          // Pull the file off of the end and place it in the DataPath
        }
        // Otherwise the full path is required
      }
      datastream << heavyDataPath;
      datastream << mTrackedArrays[arrayIndex]->getHeavyDataController(controllerIndex)->getDescriptor();
        datastream << "|";
        for (unsigned int i = 0; i < mTrackedArrays[arrayIndex]->getHeavyDataController(controllerIndex)->getDimensions().size(); ++i) {
          datastream << mTrackedArrays[arrayIndex]->getHeavyDataController(controllerIndex)->getDimensions()[i];
          if (i < mTrackedArrays[arrayIndex]->getHeavyDataController(controllerIndex)->getDimensions().size() - 1) {
            datastream << " ";
          }
        }
        if (controllerIndex + 1 < mTrackedArrays[arrayIndex]->getNumberHeavyDataControllers()) {
          datastream << "|";
        }
    }
    if (mHeavyWriter) {
      if (mHeavyWriter->getMode() == XdmfHeavyDataWriter::Append) {
        if (mDataControllers.size() > arrayIndex)
        {
          // If controllers already exist
          // Store the overarching controllers again
          mDataControllers[arrayIndex].clear();
          for (unsigned int i = 0; i < mTrackedArrays[arrayIndex]->getNumberHeavyDataControllers(); ++i)
          {
            mDataControllers[arrayIndex].push_back(mTrackedArrays[arrayIndex]->getHeavyDataController(i));
          }
          // Clear controllers from the array
          while (mTrackedArrays[arrayIndex]->getNumberHeavyDataControllers() > 0) {
            mTrackedArrays[arrayIndex]->removeHeavyDataController(0);
          }
          // If append set controller to the correct subsection of the whole
          std::vector<shared_ptr<XdmfHeavyDataController> > readControllers = getStepControllers(mCurrentStep, mTrackedArrayDims[arrayIndex], mDataControllers[arrayIndex]);
          mTrackedArrays[arrayIndex]->setHeavyDataController(readControllers);
          // Replace with updated description
          mDataDescriptions[arrayIndex] = datastream.str();
        }
        else
        {
          // If a new dataset, as normal
          mDataControllers.push_back(std::vector<shared_ptr<XdmfHeavyDataController> >());
          for (unsigned int i = 0; i < mTrackedArrays[arrayIndex]->getNumberHeavyDataControllers(); ++i) {
            mDataControllers[mDataControllers.size()-1].push_back((mTrackedArrays[arrayIndex]->getHeavyDataController(i)));
          }
          if (mTrackedArrays[arrayIndex]->getNumberHeavyDataControllers() > 0) {
            mDataTypes.push_back(mTrackedArrays[arrayIndex]->getHeavyDataController(0)->getName());
            mDataDescriptions.push_back(datastream.str());
          }
        }
      }
      else if (mHeavyWriter->getMode() == XdmfHeavyDataWriter::Hyperslab) {
        // Hyperslab is already storing the base controller
        // So nothing is done here, the controller should already be pointing to the correct location
        // TODO, to what the file index was before the add, as opposed to 0
        mHeavyWriter->setFileIndex(0);
      }
      else {
        mDataControllers.push_back(std::vector<shared_ptr<XdmfHeavyDataController> >());
        for (unsigned int i = 0; i < mTrackedArrays[arrayIndex]->getNumberHeavyDataControllers(); ++i) {
          mDataControllers[mDataControllers.size()-1].push_back((mTrackedArrays[arrayIndex]->getHeavyDataController(i)));
        }
        if (mTrackedArrays[arrayIndex]->getNumberHeavyDataControllers() > 0) {
          mDataTypes.push_back(mTrackedArrays[arrayIndex]->getHeavyDataController(0)->getName());
          mDataDescriptions.push_back(datastream.str());
        }
      }
    }
    else {
      mDataControllers.push_back(std::vector<shared_ptr<XdmfHeavyDataController> >());
      mDataTypes.push_back("XML");
      mDataDescriptions.push_back(mTrackedArrays[arrayIndex]->getValuesString());
    }
  }
  ++mNumSteps;
  this->setIsChanged(true);
  return mCurrentStep;
}

void
XdmfTemplate::clearStep()
{
  for (unsigned int i = 0; i < mTrackedArrays.size(); ++i) {
    mTrackedArrays[i]->release();
    while (mTrackedArrays[i]->getNumberHeavyDataControllers() > 0) {
      mTrackedArrays[i]->removeHeavyDataController(0);
    }
  }
  mCurrentStep = -1;
}

shared_ptr<XdmfItem>
XdmfTemplate::getBase()
{
  return mBase;
}

shared_ptr<XdmfHeavyDataWriter>
XdmfTemplate::getHeavyDataWriter()
{
  return mHeavyWriter;
}

std::map<std::string, std::string>
XdmfTemplate::getItemProperties() const
{
  std::map<std::string, std::string> templateProperties;
/*
  std::stringstream value;
  value << mValue;
  timeProperties.insert(std::make_pair("Value", value.str()));
*/
  return templateProperties;
}

std::string
XdmfTemplate::getItemTag() const
{
  return ItemTag;
}

unsigned int
XdmfTemplate::getNumberSteps() const
{
  return mNumSteps;
}

unsigned int
XdmfTemplate::getNumberTrackedArrays() const
{
  return static_cast<unsigned int>(mTrackedArrays.size());
}

XdmfArray *
XdmfTemplate::getTrackedArray(unsigned int index)
{
  return mTrackedArrays[index];
}


void
XdmfTemplate::populateItem(const std::map<std::string, std::string> & itemProperties,
                           const std::vector<shared_ptr<XdmfItem> > & childItems,
                           const XdmfCoreReader * const reader)
{
  XdmfItem::populateItem(itemProperties, childItems, reader);

  // The first child item is the base
  mBase = childItems[0];
  mCurrentStep = 0;

  std::string referenceHDF5File = "";

  if (childItems.size() > 1) {
    for(std::vector<shared_ptr<XdmfItem> >::const_iterator iter =
          childItems.begin() + 1;
        iter != childItems.end();
        ++iter) {
      if(shared_ptr<XdmfArray> array = shared_dynamic_cast<XdmfArray>(*iter)) {
        // Pull hdf5 reference data from the first provided array
        if (array->getNumberHeavyDataControllers() > 0 && !mHeavyWriter) {
          mHeavyWriter = reader->generateHeavyDataWriter(array->getHeavyDataController(0)->getName(), array->getHeavyDataController(0)->getFilePath());
        }
        if (array->getName().compare("Data Description") == 0) {
          // Split description into substrings based on the " character
          array->read();

          std::string descriptionString;
          if (array->getArrayType() == XdmfArrayType::Int8())
          {
            descriptionString = std::string((char *)array->getValuesInternal());
          }
          else if (array->getArrayType() == XdmfArrayType::String())
          {
            std::stringstream descriptionstream;
            for (unsigned int i = 0; i < array->getSize(); ++i)
            {
              descriptionstream << array->getValue<std::string>(i);
              if (i < array->getSize() - 1)
              {
                descriptionstream << '|';
              }
            }
            descriptionString = descriptionstream.str();
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
        else {
          mTrackedArrays.push_back(array.get());
          mTrackedArrayDims.push_back(array->getDimensions());
          mTrackedArrayTypes.push_back(array->getArrayType());
        }
      }
    }
  }
  for (unsigned int i = 0; i < mDataTypes.size(); ++i)
  {
    mDataControllers.push_back(std::vector<shared_ptr<XdmfHeavyDataController> >());
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
  else
  {
    // Error because a writer is required? TODO
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
    unsigned int previousTotal = controllerTotal;
    controllerTotal += mDataControllers[0][i]->getSize();
    if (previousTotal != controllerTotal - mDataControllers[0][i]->getSize())
    {
      controllerTotal = UINT_MAX;
      break;
    }
  }
  // If the array is smaller, set the writer to append.
  if (controllerTotal > mTrackedArrays[0]->getSize())
  {
    mHeavyWriter->setMode(XdmfHeavyDataWriter::Append);
    mNumSteps = 0;
    unsigned int currentTotal = 0;
    for (unsigned int controllerIndex = 0; controllerIndex < mDataControllers[0].size(); ++controllerIndex)
    {
      currentTotal += mDataControllers[0][controllerIndex]->getSize();
      while (currentTotal >= mTrackedArrays[0]->getSize())
      {
        currentTotal -= mTrackedArrays[0]->getSize();
        ++mNumSteps;
      }
    }
//    mNumSteps = controllerTotal / mTrackedArrays[0]->getSize();
  }
  else {
    mNumSteps = static_cast<unsigned int>(mDataControllers.size() / mTrackedArrays.size());
  }
  this->setStep(0);
}

void
XdmfTemplate::preallocateSteps(unsigned int numSteps)
{
  // Preallocate steps based on the current size of the arrays
  // Use a temporary array to write data to hdf5
  shared_ptr<XdmfArray> tempArray = XdmfArray::New();
  // Set to Default mode so that the new allocations are in new locations
  mHeavyWriter->setMode(XdmfHeavyDataWriter::Default);
  int preallocatedSize = 0;
  unsigned int numberSetsPreallocated = 0;
  std::stringstream datastream;
  for (unsigned int i = 0; i < mTrackedArrays.size(); ++i) {
    preallocatedSize = mTrackedArrays[i]->getSize() * numSteps;
///*
    numberSetsPreallocated = 1;

    int adjustment = 1;
    while (preallocatedSize / (numSteps/adjustment) != mTrackedArrays[i]->getSize() || 0 > (int)preallocatedSize) {
//      XdmfError::message(XdmfError::WARNING, "Overflow error");
      ++adjustment;
      while (numSteps % adjustment != 0) {
//printf("%d / %d remainder %d\n", numSteps, adjustment, (numSteps % adjustment));
        ++adjustment;
      }
      numberSetsPreallocated = numberSetsPreallocated * adjustment;
      preallocatedSize = mTrackedArrays[i]->getSize() * (numSteps / adjustment);
//printf("%d / %d = %d ?= %d\n", preallocatedSize , (numSteps/adjustment), preallocatedSize / (numSteps/adjustment), mTrackedArrays[i]->getSize());
    }

    // If adjusted, split one more time, to ensure that the dataset fits.
    if (adjustment > 1) {
      ++adjustment;
      while (numSteps % adjustment != 0) {
        ++adjustment;
      }
      numberSetsPreallocated = numberSetsPreallocated * adjustment;
      preallocatedSize = mTrackedArrays[i]->getSize() * (numSteps / adjustment);
    }

    bool allocateSucceeded = false;
    while (!allocateSucceeded)
    {
      try {
        mHeavyWriter->openFile();
//printf("now size %d allocated %d times\n", preallocatedSize, numberSetsPreallocated);
        for (unsigned int allocateIteration = 0;
             allocateIteration < numberSetsPreallocated;
             ++allocateIteration)
        {
//printf("allocating subsection %u\n", allocateIteration);
//*/
//printf("initializing base array\n");
          tempArray->initialize(mTrackedArrays[i]->getArrayType(), preallocatedSize);
//printf("writing subsection");
          tempArray->accept(mHeavyWriter);
//printf("subsection written\n");
//          mHeavyWriter->clearCache();
///*
          if (mDataControllers.size() <= i) {
            mDataControllers.push_back(std::vector<shared_ptr<XdmfHeavyDataController> >());
          }
          // Clean Array for the next iteration
          while (tempArray->getNumberHeavyDataControllers() > 0) {
            mDataControllers[i].push_back(tempArray->getHeavyDataController(0));
            if (mDataTypes.size() <= i) {
              mDataTypes.push_back(tempArray->getHeavyDataController(0)->getName());
            }
            tempArray->removeHeavyDataController(0);
          }
          tempArray->release();
//*/
///*
//printf("moving to next allocation\n");
        }
        mHeavyWriter->closeFile();
        allocateSucceeded = true;
//*/
//TODO catch the controllers created by this.
///*
      }
      catch (...)
      {
        while (tempArray->getNumberHeavyDataControllers() > 0) {
          tempArray->removeHeavyDataController(0);
        }
        tempArray->release();
//        XdmfError::message(XdmfError::WARNING, "Array Allocation failed");
        int factor = 2;
        while (preallocatedSize % factor != 0) {
//printf("%d / %d remainder %d\n", preallocatedSize, factor, (preallocatedSize % factor));
          factor = factor + 1;
        }
//printf("adjusted factor %d\n", factor);
        numberSetsPreallocated = numberSetsPreallocated * factor;
        preallocatedSize = (preallocatedSize) / factor;
//printf("now size %d allocated %d times\n", preallocatedSize, numberSetsPreallocated);
      }
    }
//printf("Done writing to hdf5\n");
//*/
/*
    // Transfer controllers to the appropriate slot before clearing them
    if (mDataControllers.size() <= i) {
      mDataControllers.push_back(std::vector<shared_ptr<XdmfHeavyDataController> >());
    }
    // Clean Array for the next iteration
    while (tempArray->getNumberHeavyDataControllers() > 0) {
      mDataControllers[i].push_back(tempArray->getHeavyDataController(0));
      if (mDataTypes.size() <= i) {
        mDataTypes.push_back(tempArray->getHeavyDataController(0)->getName());
      }
      tempArray->removeHeavyDataController(0);
    }
    tempArray->release();
*/
    datastream.str(std::string());
    for (unsigned int controllerIndex = 0; controllerIndex < mDataControllers[i].size(); ++controllerIndex) {
      // TODO throw error if controller types don't match
      // For each heavy data controller
      std::string writerPath = XdmfSystemUtils::getRealPath(mHeavyWriter->getFilePath());
      std::string heavyDataPath =
        mDataControllers[i][controllerIndex]->getFilePath();
      size_t index = heavyDataPath.find_last_of("/\\");
      if(index != std::string::npos) {
        // If path is not a folder
        // put the directory path into this variable
        const std::string heavyDataDir = heavyDataPath.substr(0, index + 1);
        // If the directory is in the XML File Path
        if(writerPath.find(heavyDataDir) == 0) {
          heavyDataPath =
            heavyDataPath.substr(heavyDataDir.size(),
                                 heavyDataPath.size() - heavyDataDir.size());
          // Pull the file off of the end and place it in the DataPath
        }
        // Otherwise the full path is required
      }
      datastream << heavyDataPath;
      datastream << mDataControllers[i][controllerIndex]->getDescriptor();
      datastream << "|";
      for (unsigned int j = 0; j < mDataControllers[i][controllerIndex]->getDimensions().size(); ++j) {
        datastream << mDataControllers[i][controllerIndex]->getDimensions()[j];
        if (j < mDataControllers[i][controllerIndex]->getDimensions().size() - 1) {
          datastream << " ";
        }
      }
      if (controllerIndex + 1 < mDataControllers[i].size()) {
        datastream << "|";
      }
    }
    mDataDescriptions.push_back(datastream.str());
  }
  // To end set the heavy writer to overwrite mode
  mHeavyWriter->setMode(XdmfHeavyDataWriter::Hyperslab);
}


void
XdmfTemplate::removeStep(unsigned int stepId)
{
  if (stepId < this->getNumberSteps()) {
    for (unsigned int i = 0; i < mTrackedArrays.size(); ++i) {
      if (mHeavyWriter->getMode() == XdmfHeavyDataWriter::Append ||
          mHeavyWriter->getMode() == XdmfHeavyDataWriter::Hyperslab) {
        std::vector<shared_ptr<XdmfHeavyDataController> > replacementControllers = getControllersExcludingStep(stepId, mTrackedArrayDims[i], mDataControllers[i]);
        for (unsigned int j = 0; j < replacementControllers.size(); ++j)
        {
          if (mDataControllers[i].size() > j) {
            mDataControllers[i][j] = replacementControllers[j];
          }
          else {
            mDataControllers[i].push_back(replacementControllers[j]);
          }
        }
      }
      else {
        mDataTypes.erase(mDataTypes.begin() + (stepId*mTrackedArrays.size()));
        mDataDescriptions.erase(mDataDescriptions.begin() + (stepId*mTrackedArrays.size()));
        mDataControllers.erase(mDataControllers.begin() + (stepId*mTrackedArrays.size()));
      }
    }
    --mNumSteps;
  }
  mCurrentStep = -1;
  this->setIsChanged(true);
}

void
XdmfTemplate::setBase(shared_ptr<XdmfItem> newBase)
{
  shared_ptr<XdmfArrayGatherer> accumulator = XdmfArrayGatherer::New(&mTrackedArrays);
  newBase->accept(accumulator);
  mBase = newBase;
  this->setIsChanged(true);
}

void
XdmfTemplate::setHeavyDataWriter(shared_ptr<XdmfHeavyDataWriter> writer)
{
  mHeavyWriter = writer;
}

void
XdmfTemplate::setStep(unsigned int stepId)
{
  if (stepId != mCurrentStep) {
    if (!mItemFactory) {
      mItemFactory = XdmfItemFactory::New();
    }
    if (stepId < this->getNumberSteps()) {
      for (unsigned int i = 0; i < mTrackedArrays.size(); ++i) {
        unsigned int arrayIndex = 0;
        if (mHeavyWriter) {
            if (mHeavyWriter->getMode() == XdmfHeavyDataWriter::Append ||
                mHeavyWriter->getMode() == XdmfHeavyDataWriter::Hyperslab) {
              arrayIndex = i;
            }
            else {
              arrayIndex = i+(stepId*static_cast<unsigned int>(mTrackedArrays.size()));
            }
          }
          else {
            arrayIndex = i+(stepId*static_cast<unsigned int>(mTrackedArrays.size()));
        }
        if (mDataControllers[arrayIndex].size() > 0) {
          if(mHeavyWriter) {
            if (mHeavyWriter->getMode() == XdmfHeavyDataWriter::Append ||
                mHeavyWriter->getMode() == XdmfHeavyDataWriter::Hyperslab) {
              std::vector<shared_ptr<XdmfHeavyDataController> > insertVector =
                getStepControllers(stepId, mTrackedArrayDims[i], mDataControllers[i]);
               mTrackedArrays[i]->setHeavyDataController(insertVector);
            }
            else {
              mTrackedArrays[i]->setHeavyDataController(mDataControllers[i+(stepId*mTrackedArrays.size())]);
            }
          }
          else {
            mTrackedArrays[i]->setHeavyDataController(mDataControllers[i+(stepId*mTrackedArrays.size())]);
          }
        }
        else {
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
          populateProperties["Content"] = mDataDescriptions[arrayIndex];
          std::vector<shared_ptr<XdmfHeavyDataController> > readControllers;
          if (mHeavyWriter) {
            if (mHeavyWriter->getMode() == XdmfHeavyDataWriter::Append ||
                mHeavyWriter->getMode() == XdmfHeavyDataWriter::Hyperslab) {
              std::vector<shared_ptr<XdmfHeavyDataController> > totalControllers =
                mItemFactory->generateHeavyDataControllers(populateProperties, mTrackedArrayDims[i], mTrackedArrayTypes[i], mDataTypes[i+(stepId*mTrackedArrays.size())]);
              readControllers = getStepControllers(stepId, mTrackedArrayDims[i], totalControllers);
            }
            else {
              readControllers = mItemFactory->generateHeavyDataControllers(populateProperties, mTrackedArrayDims[i], mTrackedArrayTypes[i], mDataTypes[i+(stepId*mTrackedArrays.size())]);
            }
          }
          else {
            readControllers = mItemFactory->generateHeavyDataControllers(populateProperties, mTrackedArrayDims[i], mTrackedArrayTypes[i], mDataTypes[i+(stepId*mTrackedArrays.size())]);
          }
          if (readControllers.size() > 0) {
            // Heavy data controllers reference the data
            mTrackedArrays[i]->setHeavyDataController(readControllers);
            mDataControllers[arrayIndex] = readControllers; 
          }
          else {
            // Data is contained in the content
            std::string content = mDataDescriptions[i+(stepId*mTrackedArrays.size())];

            mTrackedArrays[i]->initialize(mTrackedArrayTypes[i], 
					  mTrackedArrayDims[i]);

            if(mTrackedArrayTypes[i] == XdmfArrayType::String()) {
	      std::vector<std::string> tokens;
	      XdmfStringUtils::split(content, tokens);
	      mTrackedArrays[i]->insert(0, &(tokens[0]), static_cast<unsigned int>(tokens.size()));
	    }
            else {
	      std::vector<double> tokens;
	      XdmfStringUtils::split(content, tokens);
	      mTrackedArrays[i]->insert(0, &(tokens[0]), static_cast<unsigned int>(tokens.size()));
            }
          }
        }
      }
    }
    else {
      XdmfError::message(XdmfError::FATAL, 
			 "Error: Template attempting to load invalid step");
    }
    mCurrentStep = stepId;
  }
}

void
XdmfTemplate::trackArray(shared_ptr<XdmfArray> newArray)
{
  bool found = false;

  for (unsigned int i = 0; i < mTrackedArrays.size() && !found; ++i) {
    if (mTrackedArrays[i] == newArray.get()) {
      found = true;
    }
  }

  if (!found) {
    mTrackedArrays.push_back(newArray.get());
  }
  this->setIsChanged(true);
}

void
XdmfTemplate::traverse(const shared_ptr<XdmfBaseVisitor> visitor)
{
  // Set to the first step when writing, as the first step is the model for the rest of the template
  // Will fail if there are no steps
  if (this->getNumberSteps() == 0) {
    XdmfError::message(XdmfError::FATAL, "Error: No steps in template in XdmfTemplate::traverse");
  }
  this->clearStep();

  unsigned int arraysize = 1;
  for (unsigned int i = 0; i < mTrackedArrayDims[0].size(); ++i)
  {
    arraysize *= mTrackedArrayDims[0][i];
  }

  unsigned int controllersize = 0;
  for (unsigned int i = 0; i < mDataControllers[0].size(); ++i)
  {
    controllersize += mDataControllers[0][i]->getSize();
  }

  XdmfHeavyDataWriter::Mode originalMode;

  if (mHeavyWriter)
  {
    originalMode = mHeavyWriter->getMode();
    if (controllersize > arraysize) {
      mHeavyWriter->setMode(XdmfHeavyDataWriter::Append);
    }
  }

  this->setStep(0);

  if (mHeavyWriter)
  {
    mHeavyWriter->setMode(originalMode);
  }

  // Sending visitor to the base first so that it appears first when reading.
  mBase->accept(visitor);

  for (unsigned int i = 0; i < mTrackedArrays.size(); ++i) {
    mTrackedArrays[i]->release();
    mTrackedArrays[i]->accept(visitor);
  }

  // Create an array to hold all of the data information strings

  bool originalXPath;

  if (shared_ptr<XdmfWriter> writer =
        shared_dynamic_cast<XdmfWriter>(visitor)) {
    originalXPath = writer->getWriteXPaths();
    writer->setWriteXPaths(false);
  }

  shared_ptr<XdmfArray> dataInfoArray = XdmfArray::New();

  dataInfoArray->setName("Data Description");

  unsigned int i = 0;

  std::stringstream arrayInfo;
  while (i < mDataTypes.size()) {
    arrayInfo << "\"" << mDataTypes[i] << "\"" << mDataDescriptions[i];
    ++i;
  }
  dataInfoArray->insert(0, arrayInfo.str().c_str(), static_cast<unsigned int>(arrayInfo.str().length()));
  dataInfoArray->insert(dataInfoArray->getSize(), 0);

  dataInfoArray->accept(visitor);

  if (shared_ptr<XdmfWriter> writer =
        shared_dynamic_cast<XdmfWriter>(visitor)) {
    writer->setWriteXPaths(originalXPath);
  }

  XdmfItem::traverse(visitor);
}

XDMF_ITEM_C_CHILD_WRAPPER(XdmfTemplate, XDMFTEMPLATE)
