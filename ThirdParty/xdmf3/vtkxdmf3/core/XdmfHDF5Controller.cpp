/*****************************************************************************/
/*                                    Xdmf                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfHDF5Controller.cpp                                              */
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

#include <H5public.h>
#include <hdf5.h>
#include <numeric>
#include <sstream>
#include "XdmfArray.hpp"
#include "XdmfArrayType.hpp"
#include "XdmfError.hpp"
#include "XdmfHDF5Controller.hpp"
#include "XdmfSystemUtils.hpp"

unsigned int XdmfHDF5Controller::mMaxOpenedFiles = 0;
static std::map<std::string, hid_t> mOpenFiles;
std::map<std::string, unsigned int> XdmfHDF5Controller::mOpenFileUsage;

shared_ptr<XdmfHDF5Controller>
XdmfHDF5Controller::New(const std::string & hdf5FilePath,
                        const std::string & dataSetPath,
                        const shared_ptr<const XdmfArrayType> & type,
                        const std::vector<unsigned int> & start,
                        const std::vector<unsigned int> & stride,
                        const std::vector<unsigned int> & dimensions,
                        const std::vector<unsigned int> & dataspaceDimensions)
{
  shared_ptr<XdmfHDF5Controller> 
    p(new XdmfHDF5Controller(hdf5FilePath,
                             dataSetPath,
                             type,
                             start,
                             stride,
                             dimensions,
                             dataspaceDimensions));
  return p;
}

XdmfHDF5Controller::XdmfHDF5Controller(const std::string & hdf5FilePath,
                                       const std::string & dataSetPath,
                                       const shared_ptr<const XdmfArrayType> & type,
                                       const std::vector<unsigned int> & start,
                                       const std::vector<unsigned int> & stride,
                                       const std::vector<unsigned int> & dimensions,
                                       const std::vector<unsigned int> & dataspaceDimensions) :
  XdmfHeavyDataController(hdf5FilePath,
                          type,
                          dimensions),
  mDataSetPath(dataSetPath),
  mDataspaceDimensions(dataspaceDimensions),
  mStart(start),
  mStride(stride)
{
}

XdmfHDF5Controller::~XdmfHDF5Controller()
{
}

void
XdmfHDF5Controller::closeFiles()
{
  for (std::map<std::string, hid_t>::iterator closeIter = mOpenFiles.begin();
       closeIter != mOpenFiles.end();
       ++closeIter) {
    H5Fclose(closeIter->second);
  }
  mOpenFiles.clear();
  mOpenFileUsage.clear();
}

std::string
XdmfHDF5Controller::getDataSetPath() const
{
  return mDataSetPath;
}

std::string
XdmfHDF5Controller::getDescriptor() const
{
  return ":" + mDataSetPath;
}

std::vector<unsigned int> 
XdmfHDF5Controller::getDataspaceDimensions() const
{
  return mDataspaceDimensions;
}

std::string
XdmfHDF5Controller::getName() const
{
  return "HDF";
}

unsigned int
XdmfHDF5Controller::getMaxOpenedFiles()
{
  return XdmfHDF5Controller::mMaxOpenedFiles;
}

void 
XdmfHDF5Controller::getProperties(std::map<std::string, std::string> & collectedProperties) const
{
  collectedProperties["Format"] = this->getName();
}

std::vector<unsigned int> 
XdmfHDF5Controller::getStart() const
{
  return mStart;
}

std::vector<unsigned int> 
XdmfHDF5Controller::getStride() const
{
  return mStride;
}

void
XdmfHDF5Controller::read(XdmfArray * const array)
{
  this->read(array, H5P_DEFAULT);
}

void
XdmfHDF5Controller::read(XdmfArray * const array, const int fapl)
{
  herr_t status;
  hid_t hdf5Handle;
  if (XdmfHDF5Controller::mMaxOpenedFiles == 0) {
    hdf5Handle = H5Fopen(mFilePath.c_str(), H5F_ACC_RDONLY, fapl);
  }
  else {
    std::map<std::string, hid_t>::iterator checkOpen = mOpenFiles.find(mFilePath);
    if (checkOpen == mOpenFiles.end()) {
      // If the number of open files would become larger than allowed
      if (mOpenFiles.size() + 1 > mMaxOpenedFiles) {
        // Close least used one
        std::map<std::string, unsigned int>::iterator walker = mOpenFileUsage.begin();
        std::string oldestFile = walker->first;
        while (walker != mOpenFileUsage.end()) {
          // We want the file with the fewest accesses
          // If two are tied, we use the older one
          if (mOpenFileUsage[oldestFile] > walker->second) {
            oldestFile = walker->first;
          }
          ++walker;
        }
        status = H5Fclose(mOpenFiles[oldestFile]);
        mOpenFiles.erase(oldestFile);
        mOpenFileUsage.erase(oldestFile);
      }
      hdf5Handle = H5Fopen(mFilePath.c_str(), H5F_ACC_RDONLY, fapl);
      mOpenFiles[mFilePath] = hdf5Handle;
      mOpenFileUsage[mFilePath] = 1;
    }
    else {
      hdf5Handle = checkOpen->second;
      mOpenFileUsage[mFilePath]++;
    }
  }

  const hid_t dataset = H5Dopen(hdf5Handle, mDataSetPath.c_str(), H5P_DEFAULT);
  const hid_t dataspace = H5Dget_space(dataset);

  const unsigned int dataspaceDims = H5Sget_simple_extent_ndims(dataspace);
  const std::vector<hsize_t> count(mDimensions.begin(), mDimensions.end());

  if(dataspaceDims != mDimensions.size()) {
    // special case where the number of dimensions of the hdf5 dataset
    // does not equal the number of dimensions in the light data
    // description - in this case we cannot properly take a hyperslab
    // selection, so we assume we are reading the entire dataset and
    // check whether that is ok to do
    const int numberValuesHDF5 = H5Sget_select_npoints(dataspace);
    const int numberValuesXdmf = 
      std::accumulate(mDimensions.begin(),
                      mDimensions.end(),
                      1,
                      std::multiplies<unsigned int>());
    if(numberValuesHDF5 != numberValuesXdmf) {
      XdmfError::message(XdmfError::FATAL,
                         "Number of dimensions in light data description in "
                         "Xdmf does not match number of dimensions in hdf5 "
                         "file.");
    }
  }
  else {
    const std::vector<hsize_t> start(mStart.begin(), mStart.end());
    const std::vector<hsize_t> stride(mStride.begin(), mStride.end());

    status = H5Sselect_hyperslab(dataspace,
                                 H5S_SELECT_SET,
                                 &start[0],
                                 &stride[0],
                                 &count[0],
                                 NULL);
  }

  const hssize_t numVals = H5Sget_select_npoints(dataspace);
  hid_t memspace = H5Screate_simple(mDimensions.size(),
                                    &count[0],
                                    NULL);

  /* status = H5Sselect_hyperslab(memspace,
     H5S_SELECT_SET,
     &memStart[0],
     &memStride[0],
     &memCount[0],
     NULL);*/

  hid_t datatype = H5T_NO_CLASS;
  bool closeDatatype = false;
  if(mType == XdmfArrayType::Int8()) {
    datatype = H5T_NATIVE_CHAR;
  }
  else if(mType == XdmfArrayType::Int16()) {
    datatype = H5T_NATIVE_SHORT;
  }
  else if(mType == XdmfArrayType::Int32()) {
    datatype = H5T_NATIVE_INT;
  }
  else if(mType == XdmfArrayType::Int64()) {
    datatype = H5T_NATIVE_LONG;
  }
  else if(mType == XdmfArrayType::Float32()) {
    datatype = H5T_NATIVE_FLOAT;
  }
  else if(mType == XdmfArrayType::Float64()) {
    datatype = H5T_NATIVE_DOUBLE;
  }
  else if(mType == XdmfArrayType::UInt8()) {
    datatype = H5T_NATIVE_UCHAR;
  }
  else if(mType == XdmfArrayType::UInt16()) {
    datatype = H5T_NATIVE_USHORT;
  }
  else if(mType == XdmfArrayType::UInt32()) {
    datatype = H5T_NATIVE_UINT;
  }
  else if(mType == XdmfArrayType::String()) {
    datatype = H5Tcopy(H5T_C_S1);
    H5Tset_size(datatype, H5T_VARIABLE);
    closeDatatype = true;
  }
  else {
    XdmfError::message(XdmfError::FATAL,
                       "Unknown XdmfArrayType encountered in hdf5 "
                       "controller.");
  }

  array->initialize(mType, mDimensions);

  if(numVals != array->getSize()) {
    std::stringstream errOut;
    errOut << "Number of values in hdf5 dataset (" << numVals;
    errOut << ")\ndoes not match allocated size in XdmfArray (" << array->getSize() << ").";
    XdmfError::message(XdmfError::FATAL,
                       errOut.str());
  }
  if(closeDatatype) {
    char ** data = new char*[numVals];
    status = H5Dread(dataset,
                     datatype,
                     memspace,
                     dataspace,
                     H5P_DEFAULT,
                     data);
    for(hssize_t i=0; i<numVals; ++i) {
      array->insert<std::string>(i, data[i]);
    }
    status = H5Dvlen_reclaim(datatype,
                             dataspace,
                             H5P_DEFAULT,
                             data);
    delete [] data;
  }
  else {
    status = H5Dread(dataset,
                     datatype,
                     memspace,
                     dataspace,
                     H5P_DEFAULT,
                     array->getValuesInternal());
  }

  status = H5Sclose(dataspace);
  status = H5Sclose(memspace);
  status = H5Dclose(dataset);
  if(closeDatatype) {
    status = H5Tclose(datatype);
  }
  if (XdmfHDF5Controller::mMaxOpenedFiles == 0) {
    status = H5Fclose(hdf5Handle);
  }
}

void
XdmfHDF5Controller::setMaxOpenedFiles(unsigned int newMax)
{
  XdmfHDF5Controller::mMaxOpenedFiles = newMax;
}
