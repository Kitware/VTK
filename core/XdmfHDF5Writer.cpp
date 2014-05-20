/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfHDF5Writer.cpp                                                  */
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

#include <hdf5.h>
#include <sstream>
#include <cstdio>
#include <cmath>
#include <set>
#include <list>
#include "XdmfItem.hpp"
#include "XdmfArray.hpp"
#include "XdmfArrayType.hpp"
#include "XdmfError.hpp"
#include "XdmfHDF5Controller.hpp"
#include "XdmfHDF5Writer.hpp"

namespace {

  const static unsigned int DEFAULT_CHUNK_SIZE = 1000;

}

/**
 * PIMPL
 */
class XdmfHDF5Writer::XdmfHDF5WriterImpl  {

public:

  XdmfHDF5WriterImpl():
    mHDF5Handle(-1),
    mChunkSize(DEFAULT_CHUNK_SIZE),
    mOpenFile(""),
    mDepth(0)
  {
  };

  ~XdmfHDF5WriterImpl()
  {
    closeFile();
  };

  void
  closeFile()
  {
    if(mHDF5Handle >= 0) {
      /*herr_t status =*/H5Fclose(mHDF5Handle);
      mHDF5Handle = -1;
    }
    mOpenFile = "";
  };  

  int
  openFile(const std::string & filePath,
           const int fapl,
           const int mDataSetId)
  {
    if(mHDF5Handle >= 0) {
      // Perhaps we should throw a warning.
      closeFile();
    }
    // Save old error handler and turn off error handling for now
    H5E_auto_t old_func;
    void * old_client_data;
    H5Eget_auto(0, &old_func, &old_client_data);
    H5Eset_auto2(0, NULL, NULL);
  
    int toReturn = 0;

    mOpenFile.assign(filePath);


    if(H5Fis_hdf5(filePath.c_str()) > 0) {
      mHDF5Handle = H5Fopen(filePath.c_str(), 
                            H5F_ACC_RDWR, 
                            fapl);
      if(mDataSetId == 0) {
        hsize_t numObjects;
        /*herr_t status = */H5Gget_num_objs(mHDF5Handle,
                                            &numObjects);
        toReturn = numObjects;
      }
      else {
        toReturn = mDataSetId;
      }
    }
    else {
      // This is where it currently fails
      mHDF5Handle = H5Fcreate(filePath.c_str(),
                              H5F_ACC_TRUNC,
                              H5P_DEFAULT,
                              fapl);
    }

    // Restore previous error handler
    H5Eset_auto2(0, old_func, old_client_data);

    return toReturn;

  }

  hid_t mHDF5Handle;
  unsigned int mChunkSize;
  std::string mOpenFile;
  int mDepth;
  std::set<const XdmfItem *> mWrittenItems;

};

shared_ptr<XdmfHDF5Writer>
XdmfHDF5Writer::New(const std::string & filePath,
                    const bool clobberFile)
{
  if(clobberFile) {
    std::remove(filePath.c_str());
  }
  shared_ptr<XdmfHDF5Writer> p(new XdmfHDF5Writer(filePath));
  return p;
}

XdmfHDF5Writer::XdmfHDF5Writer(const std::string & filePath) :
  XdmfHeavyDataWriter(filePath, 1, 800),
  mImpl(new XdmfHDF5WriterImpl())
{
}

XdmfHDF5Writer::~XdmfHDF5Writer()
{
  delete mImpl;
}

shared_ptr<XdmfHeavyDataController>
XdmfHDF5Writer::createController(const std::string & hdf5FilePath,
                                     const std::string & dataSetPath,
                                     const shared_ptr<const XdmfArrayType> type,
                                     const std::vector<unsigned int> & start,
                                     const std::vector<unsigned int> & stride,
                                     const std::vector<unsigned int> & dimensions,
                                     const std::vector<unsigned int> & dataspaceDimensions)
{
  return XdmfHDF5Controller::New(hdf5FilePath,
                                 dataSetPath,
                                 type,
                                 start,
                                 stride,
                                 dimensions,
                                 dataspaceDimensions);
}

unsigned int
XdmfHDF5Writer::getChunkSize() const
{
  return mImpl->mChunkSize;
}

int
XdmfHDF5Writer::getDataSetSize(std::string fileName, std::string dataSetName, const int fapl)
{
  hid_t handle = -1;
  H5E_auto_t old_func;
  void * old_client_data;
  H5Eget_auto(0, &old_func, &old_client_data);
  H5Eset_auto2(0, NULL, NULL);
  if (fileName !=  mImpl->mOpenFile) {
    // Save old error handler and turn off error handling for now

    if(H5Fis_hdf5(fileName.c_str()) > 0) {
      handle = H5Fopen(fileName.c_str(),
                       H5F_ACC_RDWR,
                       fapl);
    }
    else {
      // This is where it currently fails
      handle = H5Fcreate(fileName.c_str(),
                         H5F_ACC_TRUNC,
                         H5P_DEFAULT,
                         fapl);
    }
  }
  else {
    handle = mImpl->mHDF5Handle;
  }

  // Restore previous error handler
  H5Eset_auto2(0, old_func, old_client_data);

  hid_t checkset = H5Dopen(handle,
                           dataSetName.c_str(),
                           H5P_DEFAULT);
  hid_t checkspace = H5S_ALL;
  checkspace = H5Dget_space(checkset);
  hssize_t checksize = H5Sget_simple_extent_npoints(checkspace);
  herr_t status = H5Dclose(checkset);
  if(checkspace != H5S_ALL) {
    status = H5Sclose(checkspace);
  }
  if (handle != mImpl->mHDF5Handle) {
    H5Fclose(handle);
  }
  return checksize;
}

void 
XdmfHDF5Writer::closeFile()
{
  mImpl->closeFile();
}

void 
XdmfHDF5Writer::openFile()
{
  this->openFile(H5P_DEFAULT);
}

void
XdmfHDF5Writer::openFile(const int fapl)
{
  mDataSetId = mImpl->openFile(mFilePath,
                               fapl,
                               mDataSetId);
}

void
XdmfHDF5Writer::setChunkSize(const unsigned int chunkSize)
{
  mImpl->mChunkSize = chunkSize;
}

void
XdmfHDF5Writer::visit(XdmfArray & array,
                      const shared_ptr<XdmfBaseVisitor> visitor)
{
  mImpl->mDepth++;
  std::set<const XdmfItem *>::iterator checkWritten = mImpl->mWrittenItems.find(&array);
  if (checkWritten == mImpl->mWrittenItems.end() || array.getItemTag() == "DataItem") {
    // If it has children send the writer to them too.
    array.traverse(visitor);
    if (array.isInitialized()) {
      // Only do this if the object has not already been written
      this->write(array, H5P_DEFAULT);
      mImpl->mWrittenItems.insert(&array);
    }
  }
  // If the object has already been written, just end, it already has the data
  mImpl->mDepth--;
  if(mImpl->mDepth <= 0) {
    mImpl->mWrittenItems.clear();
  }
}

void
XdmfHDF5Writer::visit(XdmfItem & item,
                      const shared_ptr<XdmfBaseVisitor> visitor)
{
  mImpl->mDepth++;
  // This is similar to the algorithm for writing XPaths
  // shouldn't be a problem if XPaths are turned off because all this does is avoid writing an object twice
  // if it was written once then all instances of the object should have the controller
  std::set<const XdmfItem *>::iterator checkWritten = mImpl->mWrittenItems.find(&item);
  if (checkWritten == mImpl->mWrittenItems.end()) {
    mImpl->mWrittenItems.insert(&item);
    item.traverse(visitor);
  }
  mImpl->mDepth--;
  if(mImpl->mDepth <= 0) {
    mImpl->mWrittenItems.clear();
  }
}


void
XdmfHDF5Writer::write(XdmfArray & array,
                      const int fapl)
{
  hid_t datatype = -1;
  bool closeDatatype = false;

  // Determining data type
  if(array.isInitialized()) {
    if(array.getArrayType() == XdmfArrayType::Int8()) {
      datatype = H5T_NATIVE_CHAR;
    }
    else if(array.getArrayType() == XdmfArrayType::Int16()) {
      datatype = H5T_NATIVE_SHORT;
    }
    else if(array.getArrayType() == XdmfArrayType::Int32()) {
      datatype = H5T_NATIVE_INT;
    }
    else if(array.getArrayType() == XdmfArrayType::Int64()) {
      datatype = H5T_NATIVE_LONG;
    }
    else if(array.getArrayType() == XdmfArrayType::Float32()) {
      datatype = H5T_NATIVE_FLOAT;
    }
    else if(array.getArrayType() == XdmfArrayType::Float64()) {
      datatype = H5T_NATIVE_DOUBLE;
    }
    else if(array.getArrayType() == XdmfArrayType::UInt8()) {
      datatype = H5T_NATIVE_UCHAR;
    }
    else if(array.getArrayType() == XdmfArrayType::UInt16()) {
      datatype = H5T_NATIVE_USHORT;
    }
    else if(array.getArrayType() == XdmfArrayType::UInt32()) {
      datatype = H5T_NATIVE_UINT;
    }
    else if(array.getArrayType() == XdmfArrayType::String()) {
      // Strings are a special case as they have mutable size
      datatype = H5Tcopy(H5T_C_S1);
      H5Tset_size(datatype, H5T_VARIABLE);
      closeDatatype = true;
    }
    else {
      XdmfError::message(XdmfError::FATAL,
                         "Array of unsupported type in "
                         "XdmfHDF5Writer::write");
    }
  }

  herr_t status;

  if(datatype != -1) {
    std::string hdf5FilePath = mFilePath;

    size_t extIndex;
    std::string checkFileName;
    std::string checkFileExt;
    extIndex = hdf5FilePath.find_last_of(".");
    if (extIndex == std::string::npos) {
      checkFileName = hdf5FilePath;
      checkFileExt = "";
    }
    else {
      checkFileName = hdf5FilePath.substr(0, extIndex);
      checkFileExt = hdf5FilePath.substr(extIndex+1);
    }

    std::stringstream dataSetPath;

    std::vector<shared_ptr<XdmfHeavyDataController> > previousControllers;

    // Hold the controllers in order to base the new controllers on them
    for(unsigned int i = 0; i < array.getNumberHeavyDataControllers(); ++i) {
      previousControllers.push_back(array.getHeavyDataController(i));
    }

    // Remove controllers from the array
    // they will be replaced by the controllers created by this function.
    while(array.getNumberHeavyDataControllers() != 0) {
      array.removeHeavyDataController(array.getNumberHeavyDataControllers() -1);
    }



    if (previousControllers.size() == 0) {
      // Create a temporary controller if the array doesn't have one
      shared_ptr<XdmfHeavyDataController> tempDataController =
        this->createController(hdf5FilePath,
                               "Data",
                               array.getArrayType(),
                               std::vector<unsigned int>(1, 0),
                               std::vector<unsigned int>(1, 1),
                               std::vector<unsigned int>(1, array.getSize()),
                               std::vector<unsigned int>(1, array.getSize()));
      previousControllers.push_back(tempDataController);
    }

    int controllerIndexOffset = 0;

    // It is assumed that the array will have at least one controller
    // if it didn't have one a temporary one was generated
    for(unsigned int i = 0; i < previousControllers.size(); ++i)
    {
      if (mMode == Append) {
        // Append only cares about the last controller, so add the rest back in
	for (; i < previousControllers.size() - 1; ++i) {
          array.insert(previousControllers[i]);
	}
      }

      std::list<std::string> filesWritten;
      std::list<shared_ptr<XdmfArray> > arraysWritten;
      std::list<std::vector<unsigned int> > startsWritten;
      std::list<std::vector<unsigned int> > stridesWritten;
      std::list<std::vector<unsigned int> > dimensionsWritten;
      std::list<std::vector<unsigned int> > dataSizesWritten;
      std::list<unsigned int> arrayOffsetsWritten;

      // Open a hdf5 dataset and write to it on disk.
      hsize_t size = array.getSize();

      // Save old error handler and turn off error handling for now
      H5E_auto_t old_func;
      void * old_client_data;
      H5Eget_auto(0, &old_func, &old_client_data);
      H5Eset_auto2(0, NULL, NULL);

      bool startedloop = false;
      unsigned int origFileIndex = getFileIndex();
      while ((mMode == Hyperslab
              && i < previousControllers.size())
             || !startedloop) {
        // Hyperslab mode wants to assign all data using the current location
        // without writing until all data sets are determined

        startedloop = true;

        shared_ptr<XdmfHeavyDataController> heavyDataController =
          previousControllers[i];
        // Stats for the data currently stored in the array
    
        std::vector<unsigned int> dimensions;
        if (mMode != Hyperslab) {
          dimensions = array.getDimensions();
        }
        else {
	  dimensions = heavyDataController->getDimensions();
        }
        std::vector<unsigned int> dataspaceDimensions = dimensions;
        std::vector<unsigned int> start(dimensions.size(), 0);
        std::vector<unsigned int> stride(dimensions.size(), 1);

        if((mMode == Overwrite || mMode == Append || mMode == Hyperslab)
          && heavyDataController) {

          // If overwriting, appending, or writing to a hyperslab this
          // should be an hdf5 controller - cast it and pull info of
          // dataset we are writing to.
          shared_ptr<XdmfHDF5Controller> hdf5Controller = 
            shared_dynamic_cast<XdmfHDF5Controller>(heavyDataController);

          if(hdf5Controller) {
            // Write to the previous dataset
            dataSetPath.str(std::string());
            dataSetPath << hdf5Controller->getDataSetPath();
            hdf5FilePath = hdf5Controller->getFilePath();
            if(mMode == Hyperslab) {
              // Start, stride, and dataspace dimensions only matter
              // for hyperslab mode
              dataspaceDimensions = hdf5Controller->getDataspaceDimensions();
              start = hdf5Controller->getStart();
              stride = hdf5Controller->getStride();
            }
          }
          else {
            XdmfError::message(XdmfError::FATAL,
                               "Can only overwrite, append, or write a "
                               "hyperslab to a dataset of the same type");
          }
        }
        else {
          dataSetPath.str(std::string());
          dataSetPath << "Data" << mDataSetId;
        }

        // Check here for if the file would become
        // larger than the limit after the addition.
        // Then check subsequent files for the same limitation
        controllerSplitting(array,
                            fapl,
                            controllerIndexOffset,
                            heavyDataController,
                            checkFileName,
                            checkFileExt,
                            dataSetPath.str(),
                            dimensions,
                            dataspaceDimensions,
                            start,
                            stride,
                            filesWritten,
                            arraysWritten,
                            startsWritten,
                            stridesWritten,
                            dimensionsWritten,
                            dataSizesWritten,
                            arrayOffsetsWritten);

        if (mMode == Hyperslab)
        {
          i++;
          setFileIndex(origFileIndex);
        }

      }

      std::list<std::string>::iterator fileNameWalker = filesWritten.begin();
      std::list<shared_ptr<XdmfArray> >::iterator arrayWalker = arraysWritten.begin();
      std::list<std::vector<unsigned int> >::iterator startWalker = startsWritten.begin();
      std::list<std::vector<unsigned int> >::iterator strideWalker = stridesWritten.begin();
      std::list<std::vector<unsigned int> >::iterator dimensionWalker = dimensionsWritten.begin();
      std::list<std::vector<unsigned int> >::iterator dataSizeWalker = dataSizesWritten.begin();
      std::list<unsigned int>::iterator arrayOffsetWalker = arrayOffsetsWritten.begin();

      // Loop based on the amount of blocks split from the array.
      for (unsigned int writeIndex = 0; writeIndex < arraysWritten.size(); ++writeIndex) {

	// This is the section where the data is written to hdf5
	// If you want to change the writer to write to a different data format, do it here

        std::string curFileName = *fileNameWalker;
        shared_ptr<XdmfArray> curArray = *arrayWalker;
        std::vector<unsigned int> curStart = *startWalker;
        std::vector<unsigned int> curStride = *strideWalker;
        std::vector<unsigned int> curDimensions = *dimensionWalker;
        std::vector<unsigned int> curDataSize = *dataSizeWalker;
        unsigned int curArrayOffset = *arrayOffsetWalker;


	bool closeFile = false;
        // This is meant to open files if it isn't already opened by the write prior
        // If it wasn't open prior to writing it will be closed after writing
        if (mImpl->mOpenFile.compare(curFileName) != 0) {
          if(mImpl->mHDF5Handle < 0) {
            closeFile = true;
          }
          mImpl->openFile(curFileName,
                          fapl, mDataSetId);
        }

	htri_t testingSet = H5Lexists(mImpl->mHDF5Handle,
                                      dataSetPath.str().c_str(),
                                      H5P_DEFAULT);

        hid_t dataset = 0;

        if (testingSet == 0) {
          dataset = -1;
        }
        else {
          dataset = H5Dopen(mImpl->mHDF5Handle,
                            dataSetPath.str().c_str(),
                            H5P_DEFAULT);
        }

        // If default mode find a new data set to write to (keep
        // incrementing dataSetId)
        if(dataset >=0 && mMode == Default) {
          while(true) {
            dataSetPath.str(std::string());
            dataSetPath << "Data" << ++mDataSetId;
            if(!H5Lexists(mImpl->mHDF5Handle,
                          dataSetPath.str().c_str(),
                          H5P_DEFAULT)) {
              dataset = H5Dopen(mImpl->mHDF5Handle,
                                dataSetPath.str().c_str(),
                                H5P_DEFAULT);
              break;
            }
          }
        }

        // Restore previous error handler
        H5Eset_auto2(0, old_func, old_client_data);

        hid_t dataspace = H5S_ALL;
        hid_t memspace = H5S_ALL;

        std::vector<hsize_t> current_dims(curDataSize.begin(),
                                          curDataSize.end());

        if(dataset < 0) {
          // If the dataset doesn't contain anything

          std::vector<hsize_t> maximum_dims(curDimensions.size(), H5S_UNLIMITED);
          // Create a new dataspace
          dataspace = H5Screate_simple(current_dims.size(),
                                       &current_dims[0],
                                       &maximum_dims[0]);
          hid_t property = H5Pcreate(H5P_DATASET_CREATE);

          const hsize_t totalDimensionsSize =
            std::accumulate(current_dims.begin(),
                            current_dims.end(),
                            1,
                            std::multiplies<hsize_t>());
          // The Nth root of the chunk size divided by the dimensions added together
          const double factor =
            std::pow(((double)mImpl->mChunkSize / totalDimensionsSize),
                     1.0 / current_dims.size());
          // The end result is the amount of slots alloted per unit of dimension
          std::vector<hsize_t> chunk_size(current_dims.begin(),
                                          current_dims.end());
	  if (mImpl->mChunkSize > 0) {
            // The chunk size won't do anything unless it's positive
            for(std::vector<hsize_t>::iterator iter = chunk_size.begin();
                iter != chunk_size.end(); ++iter) {
              *iter = (hsize_t)(*iter * factor);
              if(*iter == 0) {
                *iter = 1;
              }
            }
          }

          status = H5Pset_chunk(property, current_dims.size(), &chunk_size[0]);
          // Use that dataspace to create a new dataset
          dataset = H5Dcreate(mImpl->mHDF5Handle,
                              dataSetPath.str().c_str(),
                              datatype,
                              dataspace,
                              H5P_DEFAULT,
                              property,
                              H5P_DEFAULT);
          status = H5Pclose(property);
        }

        if(mMode == Append) {
          // Need to resize dataset to fit new data

          // Get size of old dataset
          dataspace = H5Dget_space(dataset);
          hssize_t datasize = H5Sget_simple_extent_npoints(dataspace);
          status = H5Sclose(dataspace);

          // Reset the datasize if the file or set is different
          if (curFileName != previousControllers[i]->getFilePath()) {
            datasize = 0;
          }
          if (dataSetPath.str() != previousControllers[i]->getDataSetPath()) {
            datasize = 0;
          }

          // Resize to fit size of old and new data.
          hsize_t newSize = curArray->getSize() + datasize;
          status = H5Dset_extent(dataset, &newSize);
          
          // Select hyperslab to write to.
          memspace = H5Screate_simple(1, &size, NULL);
          dataspace = H5Dget_space(dataset);
          hsize_t dataStart = datasize;
          status = H5Sselect_hyperslab(dataspace,
                                       H5S_SELECT_SET,
                                       &dataStart,
                                       NULL,
                                       &size,
                                       NULL);
        }
        else if(mMode == Overwrite) {
          // Overwriting - dataset rank must remain the same (hdf5 constraint)
          dataspace = H5Dget_space(dataset);

          const unsigned int ndims = H5Sget_simple_extent_ndims(dataspace);
          if(ndims != current_dims.size()) {
            XdmfError::message(XdmfError::FATAL,
                               "Data set rank different -- ndims != "
                               "current_dims.size() -- in "
                               "XdmfHDF5Writer::write");
          }

          status = H5Dset_extent(dataset, &current_dims[0]);
          dataspace = H5Dget_space(dataset);
        }
        else if(mMode == Hyperslab) {
          // Hyperslab - dataset rank must remain the same (hdf5 constraint)
          dataspace = H5Dget_space(dataset);

          const unsigned int ndims = H5Sget_simple_extent_ndims(dataspace);
          if(ndims != current_dims.size()) {
            XdmfError::message(XdmfError::FATAL,
                               "Data set rank different -- ndims != "
                               "current_dims.size() -- in "
                               "XdmfHDF5Writer::write");
          }
          status = H5Dset_extent(dataset, &current_dims[0]);
          dataspace = H5Dget_space(dataset);




          std::vector<hsize_t> count(curDimensions.begin(),
                                     curDimensions.end());
          std::vector<hsize_t> currStride(curStride.begin(),
                                          curStride.end());
          std::vector<hsize_t> currStart(curStart.begin(),
                                         curStart.end());

          memspace = H5Screate_simple(count.size(),
                                      &(count[0]),
                                      NULL);
          status = H5Sselect_hyperslab(dataspace,
                                       H5S_SELECT_SET,
                                       &currStart[0],
                                       &currStride[0],
                                       &count[0],
                                       NULL) ;

          if(status < 0) {
            XdmfError::message(XdmfError::FATAL,
                               "H5Dset_extent returned failure in "
                               "XdmfHDF5Writer::write -- status: " + status);
          }
        }

        status = H5Dwrite(dataset,
                          datatype,
                          memspace,
                          dataspace,
                          H5P_DEFAULT,
                          curArray->getValuesInternal());

        if(status < 0) {
          XdmfError::message(XdmfError::FATAL,
                             "H5Dwrite returned failure in XdmfHDF5Writer::write "
                             "-- status: " + status);
        }

        if(dataspace != H5S_ALL) {
          status = H5Sclose(dataspace);
        }

        if(memspace != H5S_ALL) {
          status = H5Sclose(memspace);
        }

        status = H5Dclose(dataset);

	// This is causing a lot of overhead
        if(closeFile) {
          mImpl->closeFile();
        }

        if(mMode == Default) {
          ++mDataSetId;
        }

        // Attach a new controller to the array
        shared_ptr<XdmfHDF5Controller> newDataController =
          shared_ptr<XdmfHDF5Controller>();
        //This generates an empty pointer

        unsigned int newSize;
        if(mMode == Append) {
          // Find data size
          mImpl->openFile(curFileName,
                          fapl, mDataSetId);
          hid_t checkset = H5Dopen(mImpl->mHDF5Handle,
                                   dataSetPath.str().c_str(),
                                   H5P_DEFAULT);
          hid_t checkspace = H5S_ALL;
          checkspace = H5Dget_space(checkset);
          newSize = H5Sget_simple_extent_npoints(checkspace);
          status = H5Dclose(checkset);
	  if(checkspace != H5S_ALL) {
	    status = H5Sclose(checkspace);
          }
 
          std::vector<unsigned int> insertStarts;
          insertStarts.push_back(0);
          std::vector<unsigned int> insertStrides;
          insertStrides.push_back(1);
          std::vector<unsigned int> insertDimensions;
          insertDimensions.push_back(newSize);
          std::vector<unsigned int> insertDataSpaceDimensions;
          insertDataSpaceDimensions.push_back(newSize);

          newDataController = 
            shared_dynamic_cast<XdmfHDF5Controller>
            (this->createController(curFileName,
                                    dataSetPath.str(),
                                    curArray->getArrayType(),
                                    insertStarts,
                                    insertStrides,
                                    insertDimensions,
                                    insertDataSpaceDimensions));
        }

        if(!newDataController) {
          // If the controller wasn't generated by append
          newDataController =
            shared_dynamic_cast<XdmfHDF5Controller>
            (this->createController(curFileName,
                                    dataSetPath.str(),
                                    curArray->getArrayType(),
                                    curStart,
                                    curStride,
                                    curDimensions,
                                    curDataSize));
        }

        newDataController->setArrayOffset(curArrayOffset);

        array.insert(newDataController);

        fileNameWalker++;
        arrayWalker++;
        startWalker++;
        strideWalker++;
        dimensionWalker++;
        dataSizeWalker++;
        arrayOffsetWalker++;


      }

    }

    if(closeDatatype) {
      status = H5Tclose(datatype);
    }

    if(mReleaseData) {
      array.release();
    }
  }
}
