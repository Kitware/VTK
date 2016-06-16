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

#include <H5public.h>
#include <hdf5.h>
#include <sstream>
#include <cstdio>
#include <cmath>
#include <set>
#include <list>
#include <string.h>
#include "XdmfItem.hpp"
#include "XdmfArray.hpp"
#include "XdmfArrayType.hpp"
#include "XdmfError.hpp"
#include "XdmfHDF5Controller.hpp"
#include "XdmfHDF5Writer.hpp"
#include "XdmfSystemUtils.hpp"

namespace {

  const static unsigned int DEFAULT_CHUNK_SIZE = 1000;

}

XdmfHDF5Writer::XdmfHDF5WriterImpl::XdmfHDF5WriterImpl():
  mHDF5Handle(-1),
  mFapl(H5P_DEFAULT),
  mChunkSize(DEFAULT_CHUNK_SIZE),
  mOpenFile(""),
  mDepth(0)
{
};

XdmfHDF5Writer::XdmfHDF5WriterImpl::~XdmfHDF5WriterImpl()
{
  closeFile();
};

void
XdmfHDF5Writer::XdmfHDF5WriterImpl::closeFile()
{
  if(mHDF5Handle >= 0) {
    H5Fclose(mHDF5Handle);
    mHDF5Handle = -1;
  }
  mOpenFile = "";
};

int
XdmfHDF5Writer::XdmfHDF5WriterImpl::openFile(const std::string & filePath,
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
                          mFapl);
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
    mHDF5Handle = H5Fcreate(filePath.c_str(),
                            H5F_ACC_TRUNC,
                            H5P_DEFAULT,
                            mFapl);
  }

  // Restore previous error handler
  H5Eset_auto2(0, old_func, old_client_data);

  return toReturn;
}

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
  mImpl(new XdmfHDF5WriterImpl()),
  mUseDeflate(false),
  mDeflateFactor(0)
{
}

XdmfHDF5Writer::XdmfHDF5Writer(const XdmfHDF5Writer & writerRef) :
  XdmfHeavyDataWriter(writerRef.getFilePath(), 1, 800),
  mImpl(new XdmfHDF5WriterImpl()),
  mUseDeflate(false),
  mDeflateFactor(0)
{
}

XdmfHDF5Writer::~XdmfHDF5Writer()
{
  delete mImpl;
}

void
XdmfHDF5Writer::controllerSplitting(XdmfArray & array,
                                    int & controllerIndexOffset,
                                    shared_ptr<XdmfHeavyDataController> heavyDataController,
                                    const std::string & checkFileName,
                                    const std::string & checkFileExt,
                                    const std::string & dataSetPath,
                                    int dataSetId,
                                    const std::vector<unsigned int> & dimensions,
                                    const std::vector<unsigned int> & dataspaceDimensions,
                                    const std::vector<unsigned int> & start,
                                    const std::vector<unsigned int> & stride,
                                    std::list<std::string> & filesWritten,
                                    std::list<std::string> & datasetsWritten,
                                    std::list<int> & datasetIdsWritten,
                                    std::list<void *> & arraysWritten,
                                    std::list<std::vector<unsigned int> > & startsWritten,
                                    std::list<std::vector<unsigned int> > & stridesWritten,
                                    std::list<std::vector<unsigned int> > & dimensionsWritten,
                                    std::list<std::vector<unsigned int> > & dataSizesWritten,
                                    std::list<unsigned int> & arrayOffsetsWritten)
{
  // This is the file splitting algorithm
  if (getFileSizeLimit() > 0) {
    // Only if the file limit is positive, disabled if 0 or negative
    unsigned int previousDataSize = 0;

    std::vector<unsigned int> previousDimensions;
    std::vector<unsigned int> previousDataSizes;
    unsigned int amountAlreadyWritten = 0;
    // Even though theoretically this could be an infinite loop
    // if all possible files with the specified name are produced
    // the chances of that happening are small.
    // It can handle up to 65535 different files.
    // This value may vary depending on the compiler and platform.
    // The variable UINT_MAX holds the value in question.
    // If all files are take up it will loop until a file opens up
    // since adding past the max causes overflow.

    unsigned int containedInController = 1;
    for (unsigned int j = 0; j < dataspaceDimensions.size(); ++j) {
      containedInController *= dataspaceDimensions[j];
    }
    int hyperslabSize = 0;
    while (amountAlreadyWritten < containedInController) {

      std::vector<unsigned int> partialStarts;
      std::vector<unsigned int> partialStrides;
      std::vector<unsigned int> partialDimensions;
      std::vector<unsigned int> partialDataSizes;

      std::stringstream testFile;
      if (getFileIndex() == 0) {
      // If sequentially named files need to be created or referenced
        testFile << checkFileName << "." << checkFileExt;
      }
      else {
        testFile << checkFileName << getFileIndex() << "." << checkFileExt;
      }
      FILE *checkFile = NULL;
      unsigned int fileSize = 0;
      // If the file doesn't exist the size is 0 because there's no data
      // Get the file stream
      checkFile = fopen(testFile.str().c_str(), "a");
      if (checkFile != NULL) {
        // Set the file pointer to end of file
        fseek(checkFile, 0, SEEK_END);
        // Get the file size, in bytes
        fileSize = ftell(checkFile);

        // If overwrite subtract previous data size.
        if (mMode == Overwrite || mMode == Hyperslab) {
          // Find previous data size
          std::stringstream currentDataSetPath;
          currentDataSetPath << dataSetPath;
          if (dataSetId >= 0)
          {
            currentDataSetPath << dataSetId;
          }
          int checkfilesize = getDataSetSize(testFile.str(), currentDataSetPath.str());
          if (checkfilesize < 0) {
            checkfilesize = 0;
          }
          unsigned int checksize = (unsigned int)checkfilesize;
          if (mMode == Overwrite) {
            if (checksize > fileSize) {
              fileSize = 0;
            }
            else {
              fileSize = fileSize - checksize;
              // Remove previous set's size, since it's overwritten
            }
            if (fileSize == 0) {
              fileSize += getFileOverhead();
            }
          }
          else if (mMode == Hyperslab) {
            hyperslabSize = checksize;
          }
        }
        if (fileSize == 0) {
          fileSize += getFileOverhead();
        }
        fclose(checkFile);
      }
      else if (previousDataSize == 0) {
        fileSize += getFileOverhead();
      }
      if (fileSize > (unsigned int)getFileSizeLimit()*(1024*1024)) {
        fileSize = (unsigned int)getFileSizeLimit()*(1024*1024);
      }
      //Start of splitting section

      // If needed split the written array
      // into smaller arrays based on dimension blocks
      // Working with strings has a more
      // resource intensive version of this algorithm
      // Size needed is equal to the dataspaceDimensions if in hyperslab mode
      // otherwise is equal to the size of the written array
      unsigned int remainingSize = 0;
      unsigned int dataItemSize = 1;
      if (array.getArrayType() == XdmfArrayType::String()) {
        unsigned int remainingValues = 0;
        unsigned int sizeArrayIndex = 0;
        if (mMode == Hyperslab) {
          remainingValues += 1;
          sizeArrayIndex += 1;
          for (unsigned int j = 0; j < dataspaceDimensions[j]; ++j) {
            remainingValues *= dataspaceDimensions[j];
            sizeArrayIndex *= dimensions[j];
          }
        }
        else {
          remainingValues += array.getSize();
          sizeArrayIndex = amountAlreadyWritten;
        }
        remainingValues -= amountAlreadyWritten;
        // Reduce by number of values already written
        if (remainingValues == 0) {
          // End if no remaining values
          break;
        }
        // If remaining size is less than available space, just write all of what's left
        // Calculate remaining size
        for (unsigned int j = sizeArrayIndex; j < array.getSize(); ++j) {
          remainingSize +=
            (unsigned int)((double)(array.getValue<std::string>(j).size()) *
                           8.0 * mCompressionRatio);
        }
        if (mMode == Hyperslab) {
          // Size is estimated based on averages
          remainingSize = (remainingSize /
                           (array.getSize() - sizeArrayIndex)) *
                           remainingValues;
        }
      }
      else {
        unsigned int remainingValues = 0;
        if (mMode == Hyperslab) {
          remainingValues += 1;
          for (unsigned int j = 0; j < dataspaceDimensions.size(); ++j) {
            remainingValues *= dataspaceDimensions[j];
          }
        }
        else {
          remainingValues += 1;
          for (unsigned int j = 0; j < dimensions.size(); ++j) {
            remainingValues *= dimensions[j];
          }
        }
        if ((int)remainingValues - (int) amountAlreadyWritten < 0) {
          remainingValues = 0;
        }
        else {
          remainingValues -= amountAlreadyWritten;
        }
        // Reduce by number of values already written
        if (remainingValues == 0) {//end if no remaining values
          break;
        }
        dataItemSize =
          (unsigned int)((double) (array.getArrayType()->getElementSize()) *
                         mCompressionRatio);
        // If remaining size is less than available space, just write all of what's left
        remainingSize = remainingValues * dataItemSize;
      }
      if (remainingSize + previousDataSize + fileSize - (hyperslabSize * dataItemSize)
          <= (unsigned int)getFileSizeLimit()*(1024*1024)) {
        // If the array hasn't been split
        if (amountAlreadyWritten == 0) {
          // Just pass all data to the partial vectors
          for (unsigned int j = 0; j < dimensions.size(); ++j) {
            // Done using a loop so that data is copied, not referenced
            partialStarts.push_back(start[j]);
            partialStrides.push_back(stride[j]);
            partialDimensions.push_back(dimensions[j]);
            partialDataSizes.push_back(dataspaceDimensions[j]);
          }
        }
        else {
          // If the array has been split
          int dimensionIndex = previousDimensions.size() - 1;
          // Loop previous dimensions in
          int j = 0;
          for (j = 0; j < dimensionIndex; ++j) {
            partialStarts.push_back(start[j]);
            partialStrides.push_back(stride[j]);
            partialDimensions.push_back(dimensions[j]);
            partialDataSizes.push_back(dataspaceDimensions[j]);
          }
          if (mMode == Hyperslab) {
            int newStart = (start[j] +
                            stride[j] * previousDimensions[j])
                           - previousDataSizes[j];
            while (newStart < 0) {
              newStart += stride[j];
            }
            partialStarts.push_back(newStart);
            // Stride should not change in this algorithm
            partialStrides.push_back(stride[j]);
            // Total up number of blocks for
            // the higher dimesions and subtract the amount already written
            unsigned int dimensiontotal = dimensions[j];
            unsigned int dataspacetotal = dataspaceDimensions[j];
            for (unsigned int k = j + 1; k < dimensions.size(); ++k) {
              dimensiontotal *= dimensions[k];
              dataspacetotal *= dataspaceDimensions[k];
            }
            if (previousDimensions.size() > 0) {
              partialDimensions.push_back(dimensiontotal-previousDimensions[j]);
            }
            else {
              partialDimensions.push_back(dimensiontotal);
            }
            if (previousDataSizes.size() > 0) {
              partialDataSizes.push_back(dataspacetotal-previousDataSizes[j]);
            }
            else {
              partialDataSizes.push_back(dataspacetotal);
            }
          }
          else {
            // Start and stride are not used outside of hyperslab
            partialStarts.push_back(start[j]);
            partialStrides.push_back(stride[j]);
            // Total up number of blocks for
            // the higher dimesions and subtract the amount already written
            // since it isn't hyperslab dimensions
            // and dataspacedimensions should be the same
            unsigned int dimensiontotal = dimensions[j];
            for (unsigned int k = j + 1; k < dimensions.size(); ++k) {
              dimensiontotal *= dimensions[k];
            }
            if (previousDimensions.size() > 0) {
              partialDimensions.push_back(dimensiontotal-previousDimensions[j]);
            }
            else {
              partialDimensions.push_back(dimensiontotal);
            }
            if (previousDataSizes.size() > 0) {
              partialDataSizes.push_back(dimensiontotal-previousDataSizes[j]);
            }
            else {
              partialDataSizes.push_back(dimensiontotal);
            }
          }
        }
      }
      else {
        // Otherwise, take remaining size
        // and start removing dimensions until the dimension block is less
        // then take a fraction of the dimension
        // Calculate the number of values of the data type you're using will fit
        unsigned int usableSpace = (getFileSizeLimit()*(1024*1024) -
                                    (fileSize + previousDataSize)) / dataItemSize;
        if ((unsigned int)getFileSizeLimit()*(1024*1024) < previousDataSize + fileSize) {
          usableSpace = 0;
        }
        usableSpace += hyperslabSize;
        // If the array hasn't been split
        if (amountAlreadyWritten == 0) {
          // See if it will fit in the next file
          // If it will just go to the next file
          // Otherwise split it.
          if (remainingSize + getFileOverhead() >
              (unsigned int)getFileSizeLimit()*(1024*1024)
              && usableSpace > 0) {
            if (getAllowSetSplitting()) {
              // Figure out the size of the largest block that will fit.
              unsigned int blockSizeSubtotal = 1;
              unsigned int dimensionIndex = 0;
              if (array.getArrayType() == XdmfArrayType::String()) {
                unsigned int dimensionSizeTotal = 1;
                unsigned int previousBlockSize = 0;
                // Find the dimension that was split
                while (dimensionIndex < dataspaceDimensions.size()
                       && blockSizeSubtotal <= usableSpace) {
                  // This is totally different for strings
                  dimensionSizeTotal *= dimensions[dimensionIndex];
                  previousBlockSize = blockSizeSubtotal;
                  blockSizeSubtotal = 0;
                  for (unsigned int k = 0; k < dimensionSizeTotal; ++k) {
                    if (amountAlreadyWritten + k > array.getSize()) {
                      XdmfError::message(XdmfError::FATAL,
                                         "Error: Invalid Dimension in HDF5 Write.\n");
                    }
                    blockSizeSubtotal +=
                      array.getValue<std::string>(amountAlreadyWritten + k).size();
                  }
                  dimensionIndex++;
                }
                // It should end on the "blockSizeSubtotal <= usableSpace" statement
                // the other half is for backup
                // move back one dimension so we're working
                // on the dimension that was split, not the one after it
                dimensionIndex--;
                blockSizeSubtotal = previousBlockSize;
              }
              else {
                // Find the dimension that was split
                while (dimensionIndex < dataspaceDimensions.size()
                       && blockSizeSubtotal <= usableSpace) {
                  blockSizeSubtotal *= dataspaceDimensions[dimensionIndex];
                  dimensionIndex++;
                }
                // It should end on the "blockSizeSubtotal <= arrayStartIndex" statement
                // the other half is for backup
                // Move back one dimension so we're working on the dimension that was split
                // not the one after it
                dimensionIndex--;
                blockSizeSubtotal /= dataspaceDimensions[dimensionIndex];
              }
              // Determine how many of those blocks will fit
              unsigned int numBlocks = usableSpace / blockSizeSubtotal;
              // This should be less than the current value for the dimension
              // Add dimensions as required.
              unsigned int j = 0;
              for (j = 0; j < dimensionIndex; ++j) {
                partialStarts.push_back(start[j]);
                partialStrides.push_back(stride[j]);
                partialDimensions.push_back(dimensions[j]);
                partialDataSizes.push_back(dataspaceDimensions[j]);
              }
              if (start[j] > numBlocks) {
                partialStarts.push_back(numBlocks-1);
              }
              else {
                partialStarts.push_back(start[j]);
              }
              partialStrides.push_back(stride[j]);
              partialDataSizes.push_back(numBlocks);
              if (dimensions[j] == dataspaceDimensions[j]) {
                // This is for non-hyperslab and specific cases of hyperslab
                partialDimensions.push_back(numBlocks);
              }
              else {
                // For hyperslab in general
                // Determine how many values from the array will fit
                // into the blocks being used with the dimensions specified
                unsigned int displacement = numBlocks / stride[j];
                if (((int)displacement * (int)stride[j])
                      + (start[j] % stride[j])
                    < numBlocks) {
                  displacement++;
                }
                displacement -= start[j]/stride[j];
                if (start[j] > numBlocks) {
                  displacement = 0;
                }
                if (dimensions[j] <= displacement) {
                  // If there are less values than there are space for
                  // just write all of them.
                  partialDimensions.push_back(dimensions[j]);
                }
                else {
                  // Otherwise write what space allows for
                  partialDimensions.push_back(displacement);
                }
              }
            }
            else {
              // Just pass all data to the partial vectors
              for (unsigned int j = 0; j < dimensions.size(); ++j) {
                // Done using a loop so that data is copied, not referenced
                partialStarts.push_back(start[j]);
                partialStrides.push_back(stride[j]);
                partialDimensions.push_back(dimensions[j]);
                partialDataSizes.push_back(dataspaceDimensions[j]);
              }
            }
          }
        }
        else {
          // If the array has been split
          // This case should not come up often
          // as it requires truly gigantic data sets
          // See if the remaining data will fit in the next file
          // If yes, skip to it
          // If no, split
          if (remainingSize + getFileOverhead() >
              (unsigned int)getFileSizeLimit()*(1024*1024)
              && usableSpace > 0) {
            // Figure out the size of the largest block that will fit.
            unsigned int blockSizeSubtotal = 1;
            unsigned int dimensionIndex = 0;
            if (array.getArrayType() == XdmfArrayType::String()) {
              unsigned int dimensionSizeTotal = 1;
              unsigned int previousBlockSize = 0;
              // Find the dimension that was split
              while (dimensionIndex < dataspaceDimensions.size()
                     && blockSizeSubtotal <= usableSpace) {
                // This is totally different for strings
                dimensionSizeTotal *= dimensions[dimensionIndex];
                previousBlockSize = blockSizeSubtotal;
                blockSizeSubtotal = 0;
                for (unsigned int k = 0; k < dimensionSizeTotal; ++k) {
                  if (amountAlreadyWritten + k > array.getSize()) {
                    XdmfError::message(XdmfError::FATAL,
                                       "Error: Invalid Dimension in HDF5 Write.\n");
                  }
                  blockSizeSubtotal +=
                    array.getValue<std::string>(amountAlreadyWritten + k).size();
                }
                dimensionIndex++;
              }
              // It should end on the "blockSizeSubtotal <= usableSpace" statement
              // the other half is for backup
              // move back one dimension so we're working
              // on the dimension that was split, not the one after it
              dimensionIndex--;
              blockSizeSubtotal = previousBlockSize;
            }
            else {
              // Find the dimension that was split
              while (dimensionIndex < dataspaceDimensions.size()
                     && blockSizeSubtotal <= usableSpace) {
                blockSizeSubtotal *= dataspaceDimensions[dimensionIndex];
                dimensionIndex++;
              }
              // It should end on the "blockSizeSubtotal <= arrayStartIndex" statement
              // the other half is for backup
              // Move back one dimension so we're working on the dimension that was split
              // not the one after it
              dimensionIndex--;
              blockSizeSubtotal /= dataspaceDimensions[dimensionIndex];
            }
            unsigned int j = 0;
            for (; j < dimensionIndex; ++j) {
              partialStarts.push_back(start[j]);
              partialStrides.push_back(stride[j]);
              partialDimensions.push_back(dimensions[j]);
              partialDataSizes.push_back(dataspaceDimensions[j]);
            }
            // Continue if the block is smaller than the available size
            if (blockSizeSubtotal <=usableSpace) {
             // Find number of blocks that will fit
              // This should be less than the current value for the dimension
              unsigned int numBlocks = usableSpace / blockSizeSubtotal;
              // Add dimensions to the partial vectors
              if (mMode == Hyperslab) {
                int newStart = (start[j] +
                                stride[j] * previousDimensions[j]) -
                               previousDataSizes[j];
                while (newStart < 0) {
                  newStart += stride[j];
                }
                partialStarts.push_back(newStart);
                // Stride should not change in this algorithm
                partialStrides.push_back(stride[j]);
                partialDataSizes.push_back(numBlocks);
                // Determine how many values from the array will fit
                // into the blocks being used
                // with the dimensions specified
                unsigned int displacement = (numBlocks - newStart)
                                            / stride[j];
                if (((int)displacement * (int)stride[j]) + (newStart % stride[j])
                    < numBlocks) {
                  displacement++;
                }
                displacement -= newStart/stride[j];
                if (newStart > (int)numBlocks) {
                  displacement = 0;
                }
                if ((dimensions[j] - previousDimensions[j]) <= displacement) {
                  // If there are less values than there are space for
                  // just write all of them.
                  partialDimensions.push_back(dimensions[j] - previousDimensions[j]);
                }
                else {
                  // Otherwise write what space allows for
                  partialDimensions.push_back(displacement);
                }
              }
              else {
                // Start and stride are only specified in hyperslab
                partialStarts.push_back(start[j]);
                partialStrides.push_back(stride[j]);
                partialDataSizes.push_back(numBlocks);
                partialDimensions.push_back(numBlocks);
              }
              // Place dimensions into previous dimensions
              // for later iterations
            }
            else {
              // If this is larger than usable space, try the next file
              // If moving to next file
              // just do nothing and pass out of the if statement
              // but also check if specified file size is too small
              if ((unsigned int)getFileSizeLimit()*(1024*1024)
                  < blockSizeSubtotal) {
                // This shouldn't ever trigger,
                // but it's good to cover ourselves
                // Throw an error if the block size won't work
                XdmfError::message(XdmfError::FATAL,
                                   "Error: Dimension Block size"
                                   " / Maximum File size mismatch.\n");
              }
            }
          }
        }
        // Move to next file
        setFileIndex(getFileIndex()+1);
      }

      if (partialDimensions.size() > 0) {
        // Building the array to be written
        int containedInDimensions = 1;
        // Count moved
        for (unsigned int j = 0 ; j < partialDimensions.size(); ++j) {
          containedInDimensions *= partialDimensions[j];
        }
        // Starting index
        int containedInPriorDimensions = controllerIndexOffset;
        int startOffset = 1;
        for (unsigned int j = 0; j < previousDimensions.size(); ++j) {
          startOffset *= previousDimensions[j];
        }
        if (previousDimensions.size() == 0) {
          startOffset = 0;
        }
        containedInPriorDimensions += startOffset;
        int dimensionTotal = 1;
        for (unsigned int j = 0; j < dimensions.size(); ++j) {
          dimensionTotal *= dimensions[j];
        }
        if (containedInDimensions > 0) {
          void * partialArray = NULL;
          if (array.getArrayType() == XdmfArrayType::Int8()) {
            partialArray =
              &(((char *)array.getValuesInternal())[containedInPriorDimensions]);
          }
          else if (array.getArrayType() == XdmfArrayType::Int16()) {
            partialArray =
              &(((short *)array.getValuesInternal())[containedInPriorDimensions]);
          }
          else if (array.getArrayType() == XdmfArrayType::Int32()) {
            partialArray =
              &(((int *)array.getValuesInternal())[containedInPriorDimensions]);
          }
          else if (array.getArrayType() == XdmfArrayType::Int64()) {
            partialArray =
              &(((long *)array.getValuesInternal())[containedInPriorDimensions]);
          }
          else if (array.getArrayType() == XdmfArrayType::Float32()) {
            partialArray =
              &(((float *)array.getValuesInternal())[containedInPriorDimensions]);
          }
          else if (array.getArrayType() == XdmfArrayType::Float64()) {
            partialArray =
              &(((double *)array.getValuesInternal())[containedInPriorDimensions]);
          }
          else if (array.getArrayType() == XdmfArrayType::UInt8()) {
            partialArray =
              &(((unsigned char *)array.getValuesInternal())[containedInPriorDimensions]);
          }
          else if (array.getArrayType() == XdmfArrayType::UInt16()) {
            partialArray =
              &(((unsigned short *)array.getValuesInternal())[containedInPriorDimensions]);
          }
          else if (array.getArrayType() == XdmfArrayType::UInt32()) {
            partialArray =
              &(((unsigned int *)array.getValuesInternal())[containedInPriorDimensions]);
          }
          else if (array.getArrayType() == XdmfArrayType::String()) {
            partialArray =
              &(((std::string *)array.getValuesInternal())[containedInPriorDimensions]);
          }
          arraysWritten.push_back(partialArray);
          filesWritten.push_back(testFile.str());
          datasetsWritten.push_back(dataSetPath);
          datasetIdsWritten.push_back(dataSetId);
          startsWritten.push_back(partialStarts);
          stridesWritten.push_back(partialStrides);
          dimensionsWritten.push_back(partialDimensions);
          dataSizesWritten.push_back(partialDataSizes);
          arrayOffsetsWritten.push_back(containedInPriorDimensions);
        }
        if (mMode == Hyperslab) {
          containedInPriorDimensions -= controllerIndexOffset;
        }
        if (containedInDimensions + containedInPriorDimensions == dimensionTotal) {
          controllerIndexOffset += dimensionTotal;
        }
        // For hyperslab the space is controlled by the dataspace dimensions
        // So use that since the dimensions should be equal
        // to the dataspace dimensions in all other variations
        // Total up written data space
        unsigned int writtenDataSpace = 1;
        for (unsigned int j = 0; j < partialDataSizes.size(); ++j) {
          writtenDataSpace *= partialDataSizes[j];
        }
        amountAlreadyWritten += writtenDataSpace;
        // Generate previous dimensions
        if (previousDataSizes.size() == 0) {
          previousDataSizes = partialDataSizes;
          previousDimensions = partialDimensions;
        }
        else {
          // Determine if the sizes match
          // If they do, add the top values together
          // Otherwise, compress the higher dimensions and then add them
          if (previousDimensions.size() == partialDimensions.size()) {
            previousDimensions[previousDimensions.size()-1] +=
              partialDimensions[previousDimensions.size()-1];
          }
          else if (previousDimensions.size() < partialDimensions.size()) {
            unsigned int overflowDimensions = 1;
            for (unsigned int j = previousDimensions.size() - 1;
                 j < partialDimensions.size();
                 ++j) {
              overflowDimensions *= partialDimensions[j];
            }
            previousDimensions[previousDimensions.size()-1] += overflowDimensions;
          }
          else if (previousDimensions.size() > partialDimensions.size()) {
            unsigned int overflowDimensions = 1;
            for (unsigned int j = partialDimensions.size() - 1;
                 j < previousDimensions.size();
                 ++j) {
              overflowDimensions *= previousDimensions[j];
            }
            previousDimensions.resize(partialDimensions.size());
            previousDimensions[partialDimensions.size()-1] = overflowDimensions;
            previousDimensions[previousDimensions.size()-1] +=
              partialDimensions[previousDimensions.size()-1];
          }
          if (previousDataSizes.size() == partialDataSizes.size()) {
            previousDataSizes[previousDataSizes.size()-1] +=
              partialDataSizes[previousDataSizes.size()-1];
          }
          else if (previousDataSizes.size() < partialDataSizes.size()) {
            unsigned int overflowDataSizes = 1;
            for (unsigned int j = previousDataSizes.size() - 1;
                 j < partialDataSizes.size();
                 ++j) {
              overflowDataSizes *= partialDataSizes[j];
            }
            previousDataSizes[previousDataSizes.size()-1] += overflowDataSizes;
          }
          else if (previousDataSizes.size() > partialDataSizes.size()) {
            unsigned int overflowDataSizes = 1;
            for (unsigned int j = partialDataSizes.size() - 1;
                 j < previousDataSizes.size();
                 ++j) {
              overflowDataSizes *= previousDataSizes[j];
            }
            previousDataSizes.resize(partialDataSizes.size());
            previousDataSizes[partialDataSizes.size()-1] = overflowDataSizes;
            previousDataSizes[previousDataSizes.size()-1] +=
              partialDataSizes[previousDataSizes.size()-1];
          }
        }
      }
      ++dataSetId;
    }

    if (mMode == Append) {
      // If the written filename is different write add the previous controller
      if (*(filesWritten.rbegin()) != heavyDataController->getFilePath()) {
        // Should also be different from previous controller
        if (filesWritten.size() > 1) {
          if (*(filesWritten.rbegin()) != *((filesWritten.rbegin())++)) {
            array.insert(heavyDataController);
          }
        }
        else {
          array.insert(heavyDataController);
        }
      }
    }
  }
  else {
    // Otherwise work with the full array
    void * partialArray = NULL;
    // Need to copy by duplicating the contents of the array
    unsigned int j = controllerIndexOffset;
    std::string writtenFileName = "";
    if (mMode == Default) {
      std::stringstream testFile;
      if (getFileIndex() == 0) {
        // If sequentially named files need to be created or referenced
        testFile << checkFileName << "." << checkFileExt;
      }
      else {
        testFile << checkFileName << getFileIndex() << "." << checkFileExt;
      }
      writtenFileName = testFile.str();
    }
    else {
      writtenFileName = heavyDataController->getFilePath();
    }

    if (array.getArrayType() == XdmfArrayType::Int8()){
      partialArray =
        &(((char *)array.getValuesInternal())[controllerIndexOffset]);
    }
    else if (array.getArrayType() == XdmfArrayType::Int16()){
      partialArray =
        &(((short *)array.getValuesInternal())[controllerIndexOffset]);
    }
    else if (array.getArrayType() == XdmfArrayType::Int32()){
      partialArray =
        &(((int *)array.getValuesInternal())[controllerIndexOffset]);
    }
    else if (array.getArrayType() == XdmfArrayType::Int64()){
      partialArray =
        &(((long *)array.getValuesInternal())[controllerIndexOffset]);
    }
    else if (array.getArrayType() == XdmfArrayType::Float32()){
      partialArray =
        &(((float *)array.getValuesInternal())[controllerIndexOffset]);
    }
    else if (array.getArrayType() == XdmfArrayType::Float64()){
      partialArray =
        &(((double *)array.getValuesInternal())[controllerIndexOffset]);
    }
    else if (array.getArrayType() == XdmfArrayType::UInt8()){
      partialArray =
        &(((unsigned char *)array.getValuesInternal())[controllerIndexOffset]);
    }
    else if (array.getArrayType() == XdmfArrayType::UInt16()){
      partialArray =
        &(((unsigned short *)array.getValuesInternal())[controllerIndexOffset]);
    }
    else if (array.getArrayType() == XdmfArrayType::UInt32()) {
      partialArray =
        &(((unsigned int *)array.getValuesInternal())[controllerIndexOffset]);
    }
    else if (array.getArrayType() == XdmfArrayType::String()) {
      partialArray =
        &(((std::string *)array.getValuesInternal())[controllerIndexOffset]);
    }
    arrayOffsetsWritten.push_back(controllerIndexOffset);
    // Set the offset to the point after the end of the current subset
    controllerIndexOffset = j;

    arraysWritten.push_back(partialArray);
    filesWritten.push_back(writtenFileName);
    datasetsWritten.push_back(dataSetPath);
    datasetIdsWritten.push_back(dataSetId);
    // Also need to push the starts and strides loaded from the HeavyDataController
    startsWritten.push_back(start);
    stridesWritten.push_back(stride);
    dimensionsWritten.push_back(dimensions);
    dataSizesWritten.push_back(dataspaceDimensions);
  }
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
XdmfHDF5Writer::getDataSetSize(shared_ptr<XdmfHeavyDataController> descriptionController)
{
  return getDataSetSize(descriptionController->getFilePath(),
                        shared_dynamic_cast<XdmfHDF5Controller>(descriptionController)->getDataSetPath());
}

int
XdmfHDF5Writer::getDataSetSize(const std::string & fileName, const std::string & dataSetName)
{
  hid_t handle = -1;
  H5E_auto_t old_func;
  void * old_client_data;
  H5Eget_auto(0, &old_func, &old_client_data);
  H5Eset_auto2(0, NULL, NULL);
  if (XdmfSystemUtils::getRealPath(fileName) !=  mImpl->mOpenFile) {
    // Save old error handler and turn off error handling for now

    if(H5Fis_hdf5(fileName.c_str()) > 0) {
      handle = H5Fopen(fileName.c_str(),
                       H5F_ACC_RDWR,
                       mImpl->mFapl);
    }
    else {
      // This is where it currently fails
      handle = H5Fcreate(fileName.c_str(),
                         H5F_ACC_TRUNC,
                         H5P_DEFAULT,
                         mImpl->mFapl);
    }
  }
  else {
    handle = mImpl->mHDF5Handle;
  }

  // Restore previous error handler
  H5Eset_auto2(0, old_func, old_client_data);

  if (!H5Lexists(mImpl->mHDF5Handle,
                 dataSetName.c_str(),
                 H5P_DEFAULT))
  {
     return 0;
  }

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

int
XdmfHDF5Writer::getDeflateFactor() const
{
  return mDeflateFactor;
}

bool
XdmfHDF5Writer::getUseDeflate() const
{
  return mUseDeflate;
}

void 
XdmfHDF5Writer::closeFile()
{
  mImpl->closeFile();
}

void
XdmfHDF5Writer::openFile()
{
  mDataSetId = mImpl->openFile(mFilePath,
                               mDataSetId);
}

void
XdmfHDF5Writer::setChunkSize(const unsigned int chunkSize)
{
  mImpl->mChunkSize = chunkSize;
}

void
XdmfHDF5Writer::setDeflateFactor(int factor)
{
  mDeflateFactor = factor;
}

void
XdmfHDF5Writer::setUseDeflate(bool status)
{
  mUseDeflate = status;
}

void
XdmfHDF5Writer::visit(XdmfArray & array,
                      const shared_ptr<XdmfBaseVisitor> visitor)
{
  mImpl->mDepth++;
  std::set<const XdmfItem *>::iterator checkWritten = mImpl->mWrittenItems.find(&array);
  if (checkWritten == mImpl->mWrittenItems.end()) {
    // If it has children send the writer to them too.
    array.traverse(visitor);
    if (array.isInitialized() && array.getSize() > 0) {
      // Only do this if the object has not already been written
      this->write(array);
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
XdmfHDF5Writer::write(XdmfArray & array)
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
     // discard controllers of the wrong type
      if (shared_ptr<XdmfHDF5Controller> controller =
            shared_dynamic_cast<XdmfHDF5Controller>(array.getHeavyDataController(i)) )
      {
        previousControllers.push_back(array.getHeavyDataController(i));
      }
    }

    // Remove controllers from the array
    // they will be replaced by the controllers created by this function.
    while(array.getNumberHeavyDataControllers() != 0) {
      array.removeHeavyDataController(array.getNumberHeavyDataControllers() -1);
    }

    bool hasControllers = true;

    if (previousControllers.size() == 0) {
      // Create a temporary controller if the array doesn't have one
      hasControllers = false;
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
      std::list<std::string> datasetsWritten;
      std::list<int> datasetIdsWritten;
      std::list<void *> arraysWritten;
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

      // If this is in hyperslab mode, this loop will need to execute multiple times
      // Otherwise the boolean is used simply to start it and one pass is made
      bool startedloop = false;
      unsigned int origFileIndex = getFileIndex();
      while ((mMode == Hyperslab
              && i < previousControllers.size())
             || !startedloop) {
        // Hyperslab mode wants to assign all data using the current location
        // without writing until all data sets are determined

        startedloop = true;

        shared_ptr<XdmfHDF5Controller> heavyDataController =
          shared_dynamic_cast<XdmfHDF5Controller>(previousControllers[i]);
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

          // Write to the previous dataset
          dataSetPath.str(std::string());
          dataSetPath << heavyDataController->getDataSetPath();
          hdf5FilePath = heavyDataController->getFilePath();
          if(mMode == Hyperslab) {
            // Start, stride, and dataspace dimensions only matter for hyperslab mode
            dataspaceDimensions = heavyDataController->getDataspaceDimensions();
            start = heavyDataController->getStart();
            stride = heavyDataController->getStride();
          }
        }
        else {
          dataSetPath.str(std::string());
          dataSetPath << "Data" << mDataSetId;
        }

        // Check here for if the file would become
        // larger than the limit after the addition.
        // Then check subsequent files for the same limitation
        std::string passPath = dataSetPath.str();
        controllerSplitting(array,
                            controllerIndexOffset,
                            heavyDataController,
                            checkFileName,
                            checkFileExt,
                            heavyDataController->getDataSetPrefix(),
                            heavyDataController->getDataSetId(),
                            dimensions,
                            dataspaceDimensions,
                            start,
                            stride,
                            filesWritten,
                            datasetsWritten,
                            datasetIdsWritten,
                            arraysWritten,
                            startsWritten,
                            stridesWritten,
                            dimensionsWritten,
                            dataSizesWritten,
                            arrayOffsetsWritten);

        if (mMode == Hyperslab)
        {
          // In hyperslab mode, reset the file index and move to next iteration
          i++;
          setFileIndex(origFileIndex);
        }

      }

      std::list<std::string>::iterator fileNameWalker = filesWritten.begin();
      std::list<std::string>::iterator datasetWalker = datasetsWritten.begin();
      std::list<int>::iterator datasetIdWalker = datasetIdsWritten.begin();
      std::list<void *>::iterator arrayWalker = arraysWritten.begin();
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
        std::string currDataset = *datasetWalker;
        int currDatasetId = *datasetIdWalker;
        void * curArray = *arrayWalker;
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
                          mDataSetId);
        }

        if (currDatasetId >= 0)
        {
          mDataSetId = currDatasetId;
          dataSetPath.str(std::string());
          dataSetPath << currDataset << mDataSetId;
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
        if(dataset >=0 &&
           (mMode == Default ||
            (mMode == Hyperslab && !hasControllers))) {
          while(true) {
            dataSetPath.str(std::string());
            dataSetPath << currDataset << ++mDataSetId;
            if(!H5Lexists(mImpl->mHDF5Handle,
                          dataSetPath.str().c_str(),
                          H5P_DEFAULT)) {
              //Close previous dataset
              status = H5Dclose(dataset);
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

          // Set ZLIB / DEFLATE Compression
          if (mUseDeflate)
          {
            status = H5Pset_deflate(property, mDeflateFactor);
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
          if (shared_ptr<XdmfHDF5Controller> setPathController =
                shared_dynamic_cast<XdmfHDF5Controller>(previousControllers[i])) {
            if (dataSetPath.str() != setPathController->getDataSetPath()) {
              datasize = 0;
            }
          }
          else {
            datasize = 0;
          }

          unsigned int sizeTotal = 1;

          for (unsigned int dataSizeIter = 0; dataSizeIter < curDataSize.size(); ++dataSizeIter) {
            sizeTotal = sizeTotal * curDataSize[dataSizeIter];
          }

          // Resize to fit size of old and new data.
          hsize_t newSize = sizeTotal + datasize;
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
            XdmfError::message(XdmfError::FATAL,                            \
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
            XdmfError::message(XdmfError::FATAL,                            \
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
                          curArray);

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

        H5Fflush(mImpl->mHDF5Handle, H5F_SCOPE_GLOBAL);

	// This is causing a lot of overhead
        if(closeFile) {
          mImpl->closeFile();
        }

        // Attach a new controller to the array
        shared_ptr<XdmfHDF5Controller> newDataController =
          shared_ptr<XdmfHDF5Controller>();
        //This generates an empty pointer

        unsigned int newSize;
        if(mMode == Append) {
          // Find data size
          mImpl->openFile(curFileName,
                          mDataSetId);
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
            shared_dynamic_cast<XdmfHDF5Controller>(this->createController(curFileName,
                                                    dataSetPath.str(),
                                                    array.getArrayType(),
                                                    insertStarts,
                                                    insertStrides,
                                                    insertDimensions,
                                                    insertDataSpaceDimensions));
        }

        if(!newDataController) {
          // If the controller wasn't generated by append
          newDataController =
            shared_dynamic_cast<XdmfHDF5Controller>(this->createController(curFileName,
                                                    dataSetPath.str(),
                                                    array.getArrayType(),
                                                    curStart,
                                                    curStride,
                                                    curDimensions,
                                                    curDataSize));
        }

        newDataController->setArrayOffset(curArrayOffset);

        array.insert(newDataController);

        fileNameWalker++;
        datasetWalker++;
        datasetIdWalker++;
        arrayWalker++;
        startWalker++;
        strideWalker++;
        dimensionWalker++;
        dataSizeWalker++;
        arrayOffsetWalker++;

        if (mMode == Default) {
          dataSetPath.str(std::string());
          dataSetPath << "Data" << ++mDataSetId;
        }

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

// C Wrappers

XDMFHDF5WRITER * XdmfHDF5WriterNew(char * fileName, int clobberFile)
{
  try
  {
    shared_ptr<XdmfHDF5Writer> generatedWriter = XdmfHDF5Writer::New(std::string(fileName), clobberFile);
    return (XDMFHDF5WRITER *)((void *)(new XdmfHDF5Writer(*generatedWriter.get())));
  }
  catch (...)
  {
    shared_ptr<XdmfHDF5Writer> generatedWriter = XdmfHDF5Writer::New(std::string(fileName), clobberFile);
    return (XDMFHDF5WRITER *)((void *)(new XdmfHDF5Writer(*generatedWriter.get())));
  }
}

void XdmfHDF5WriterCloseFile(XDMFHDF5WRITER * writer, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfHDF5Writer *)writer)->closeFile();
  XDMF_ERROR_WRAP_END(status)
}

unsigned int XdmfHDF5WriterGetChunkSize(XDMFHDF5WRITER * writer, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  return ((XdmfHDF5Writer *)writer)->getChunkSize();
  XDMF_ERROR_WRAP_END(status)
  return 0;
}

void XdmfHDF5WriterOpenFile(XDMFHDF5WRITER * writer, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfHDF5Writer *)writer)->openFile();
  XDMF_ERROR_WRAP_END(status)
}

void XdmfHDF5WriterSetChunkSize(XDMFHDF5WRITER * writer, unsigned int chunkSize, int * status)
{
  XDMF_ERROR_WRAP_START(status)
  ((XdmfHDF5Writer *)writer)->setChunkSize(chunkSize);
  XDMF_ERROR_WRAP_END(status)
}

// C Wrappers for parent classes are generated by macros

XDMF_HEAVYWRITER_C_CHILD_WRAPPER(XdmfHDF5Writer, XDMFHDF5WRITER)
