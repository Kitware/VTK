/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfHeavyDataWriter.cpp                                             */
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

#include "XdmfHeavyDataWriter.hpp"
#include "XdmfHeavyDataController.hpp"
#include "XdmfArray.hpp"
#include "XdmfArrayType.hpp"
#include "XdmfError.hpp"
#include "XdmfSystemUtils.hpp"
#include <cstdio>
#include <list>
#include <vector>

XdmfHeavyDataWriter::XdmfHeavyDataWriter(const double compression,
                                         const unsigned int overhead) :
  mAllowSplitDataSets(false),
  mDataSetId(0),
  mFileIndex(0),
  mFilePath(""),
  mFileSizeLimit(0),
  mMode(Default),
  mCompressionRatio(compression),
  mFileOverhead(overhead)
{
}

XdmfHeavyDataWriter::XdmfHeavyDataWriter(const std::string & filePath,
                                         const double compression,
                                         const unsigned int overhead) :
  mAllowSplitDataSets(false),
  mDataSetId(0),
  mFileIndex(0),
  mFilePath(XdmfSystemUtils::getRealPath(filePath)),
  mFileSizeLimit(0),
  mMode(Default),
  mReleaseData(false),
  mCompressionRatio(compression),
  mFileOverhead(overhead)
{
}

XdmfHeavyDataWriter::~XdmfHeavyDataWriter()
{
}

void
XdmfHeavyDataWriter::controllerSplitting(XdmfArray & array,
                                         const int fapl,
                                         int & controllerIndexOffset,
                                         shared_ptr<XdmfHeavyDataController> heavyDataController,
                                         std::string checkFileName,
                                         std::string checkFileExt,
                                         std::string dataSetPath,
                                         std::vector<unsigned int> dimensions,
                                         std::vector<unsigned int> dataspaceDimensions,
                                         std::vector<unsigned int> start,
                                         std::vector<unsigned int> stride,
                                         std::list<std::string> & filesWritten,
                                         std::list<shared_ptr<XdmfArray> > & arraysWritten,
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
          int checkfilesize = getDataSetSize(testFile.str(), dataSetPath, fapl);
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
      // Check size to see if it's within range
      if (array.getArrayType() == XdmfArrayType::String()) {
        // Size needed is equal to the dataspaceDimensions if in hyperslab mode
        // Otherwise is equal to the size of the written array
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
        unsigned int remainingSize = 0;
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
        if (remainingSize + previousDataSize + fileSize
            < (unsigned int)getFileSizeLimit()*(1024*1024)) {
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
                              stride[j] * previousDimensions[j]) -
                             previousDataSizes[j];
              while (newStart < 0) {
                newStart += stride[j];
              }
              partialStarts.push_back(newStart);
              // Stride should not change in this algorithm
              partialStrides.push_back(stride[j]);
              // Total up number of blocks for the higher dimesions
              // and subtract the amount already written
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
              // Total up number of blocks for the higher dimesions
              // and subtract the amount already written
              // Since it isn't hyperslab dimensions
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
          // Otherwise, take remaining size and start removing dimensions
          // until the dimension block is less, then take a fraction of the dimension
          // Calculate the number of values of the data type you're using will fit
          unsigned int usableSpace = (getFileSizeLimit()*(1024*1024) - fileSize);
          if (previousDataSize + fileSize > (unsigned int)getFileSizeLimit()*(1024*1024)) {
            usableSpace = 0;
          }
          usableSpace += hyperslabSize-previousDataSize;
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
                unsigned int blockSizeSubtotal = 0;
                unsigned int dimensionSizeTotal = 1;
                unsigned int dimensionIndex = 0;
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
                // Determine how many of those blocks will fit
                unsigned int numBlocks = usableSpace / blockSizeSubtotal;
                // This should be less than the current value for the dimension
                // Add dimensions as required
                unsigned int j = 0;
                for (; j < dimensionIndex; ++j) {
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
                  //this is for non-hyperslab and specific cases of hyperslab
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
              unsigned int blockSizeSubtotal = 0;
              unsigned int tempTotal = 0;
              unsigned int dimensionSizeTotal = 1;
              unsigned int dimensionIndex = 0;
              // Find the dimension that was split
              while (dimensionIndex < dataspaceDimensions.size()
                     && blockSizeSubtotal <= usableSpace) {
                // This is totally different for strings
                dimensionSizeTotal *= dimensions[dimensionIndex];
                tempTotal = blockSizeSubtotal;
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
              // Move back one dimension so we're working
              // on the dimension that was split, not the one after it
              dimensionIndex--;
              blockSizeSubtotal = tempTotal;
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
                  // Determine how many values from the array
                  // will fit into the blocks being used
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
                // Place dimensions into previous dimensions for later iterations
              }
              else {
                // If this is larger than usable space, try the next file
                // If moving to next file
                // just do nothing and pass out of the if statement
                // but also check if specified file size is too small
                if ((unsigned int)getFileSizeLimit()*(1024*1024)
                    < blockSizeSubtotal) {
                  // This shouldn't ever trigger
                  // but it's good to cover ourselves
                  // and throw an error if the block size won't work
                  XdmfError::message(XdmfError::FATAL,
                                     "Error: Dimension Block size"
                                     " / Maximum File size mismatch.");
                }
              }
            }
          }
          // Move to next file
          setFileIndex(getFileIndex()+1);
        }
      }
      else {
        // If needed split the written array
        // into smaller arrays based on dimension blocks
        // Working with strings has a more
        // resource intensive version of this algorithm
        // Size needed is equal to the dataspaceDimensions if in hyperslab mode
        // otherwise is equal to the size of the written array
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
        remainingValues -= amountAlreadyWritten;
        // Reduce by number of values already written
        if (remainingValues == 0) {//end if no remaining values
          break;
        }
        unsigned int dataItemSize =
          (unsigned int)((double) (array.getArrayType()->getElementSize()) *
                         mCompressionRatio);
        // If remaining size is less than available space, just write all of what's left
        if ((remainingValues * dataItemSize) + previousDataSize + fileSize
            < (unsigned int)getFileSizeLimit()*(1024*1024)) {
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
              int newStart = (start[j] + stride[j] * previousDimensions[j])
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
                                      fileSize) / dataItemSize;
          if ((unsigned int)getFileSizeLimit()*(1024*1024) < fileSize) {
            usableSpace = 0;
          }
          usableSpace += hyperslabSize-previousDataSize;
          // If the array hasn't been split
          if (amountAlreadyWritten == 0) {
            // See if it will fit in the next file
            // If it will just go to the next file
            // Otherwise split it.
            if ((remainingValues * dataItemSize) + getFileOverhead() >
                (unsigned int)getFileSizeLimit()*(1024*1024)
                && usableSpace > 0) {
              if (getAllowSetSplitting()) {
                // Figure out the size of the largest block that will fit.
                unsigned int blockSizeSubtotal = 1;
                unsigned int dimensionIndex = 0;
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
                  if (((int)displacement * (int)stride[j]) + (start[j] % stride[j])
                      < numBlocks) {
                    displacement++;
                  }
                  displacement -= start[j]/stride[j];
                  if (start[j] > numBlocks) {
                    displacement = 0;
                  }
                  if (dimensions[j] <= displacement) {
                    // If there are less values than there are space for, just write all of them.
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
            // This case should not come up often as it requires truly gigantic data sets
            // See if it will fit in the next file
            // If it will just go to the next file
            // Otherwise split it.
            if ((remainingValues * dataItemSize) + getFileOverhead() >
                (unsigned int)getFileSizeLimit()*(1024*1024)
                && usableSpace > 0) {
              unsigned int blockSizeSubtotal = 1;
              unsigned int dimensionIndex = 0;
              // Find the dimension that was split
              while (dimensionIndex < dataspaceDimensions.size()
                    && blockSizeSubtotal <= amountAlreadyWritten) {
                blockSizeSubtotal *= dataspaceDimensions[dimensionIndex];
                dimensionIndex++;
              }
              // It should end on the "blockSizeSubtotal <= arrayStartIndex" statement
              // the other half is for backup
              // Move back one dimension so we're working on the dimension that was split
              // not the one after it
              dimensionIndex--;
              blockSizeSubtotal /= dataspaceDimensions[dimensionIndex];
              unsigned int j = 0;
              for (j = 0; j < dimensionIndex; ++j) {
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
                  int newStart = (start[j] + stride[j] * previousDimensions[j])
                                 - previousDataSizes[j];
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
                  unsigned int displacement = numBlocks / stride[j];
                  if (((int)displacement * (int)stride[j])
                        + (newStart % stride[j])
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
                if ((unsigned int)getFileSizeLimit()*(1024*1024) < blockSizeSubtotal) {
                  // This shouldn't ever trigger, but it's good to cover ourselves
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
          shared_ptr<XdmfArray> partialArray = XdmfArray::New();
          if (array.getArrayType() == XdmfArrayType::Int8()) {
            partialArray->initialize(XdmfArrayType::Int8(), 0);
            char * movedData = new char[containedInDimensions];
            array.getValues(containedInPriorDimensions,
                            movedData,
                            containedInDimensions);
            partialArray->insert(0, movedData, containedInDimensions);
            delete movedData;
          }
          else if (array.getArrayType() == XdmfArrayType::Int16()) {
            partialArray->initialize(XdmfArrayType::Int16(), 0);
            short * movedData = new short[containedInDimensions];
            array.getValues(containedInPriorDimensions,
                            movedData,
                            containedInDimensions);
            partialArray->insert(0, movedData, containedInDimensions);
            delete movedData;
          }
          else if (array.getArrayType() == XdmfArrayType::Int32()) {
            partialArray->initialize(XdmfArrayType::Int32(), 0);
            int * movedData = new int[containedInDimensions];
            array.getValues(containedInPriorDimensions,
                            movedData,
                            containedInDimensions);
            partialArray->insert(0, movedData, containedInDimensions);
            delete movedData;
          }
          else if (array.getArrayType() == XdmfArrayType::Int64()) {
            partialArray->initialize(XdmfArrayType::Int64(), 0);
            long * movedData = new long[containedInDimensions];
            array.getValues(containedInPriorDimensions,
                            movedData,
                            containedInDimensions);
            partialArray->insert(0, movedData, containedInDimensions);
            delete movedData;
          }
          else if (array.getArrayType() == XdmfArrayType::Float32()) {
            partialArray->initialize(XdmfArrayType::Float32(), 0);
            float * movedData = new float[containedInDimensions];
            array.getValues(containedInPriorDimensions,
                            movedData,
                            containedInDimensions);
            partialArray->insert(0, movedData, containedInDimensions);
            delete movedData;
          }
          else if (array.getArrayType() == XdmfArrayType::Float64()) {
            partialArray->initialize(XdmfArrayType::Float64(), 0);
            double * movedData = new double[containedInDimensions];
            array.getValues(containedInPriorDimensions,
                            movedData,
                            containedInDimensions);
            partialArray->insert(0, movedData, containedInDimensions);
            delete movedData;
          }
          else if (array.getArrayType() == XdmfArrayType::UInt8()) {
            partialArray->initialize(XdmfArrayType::UInt8(), 0);
            unsigned char * movedData = new unsigned char[containedInDimensions];
            array.getValues(containedInPriorDimensions,
                            movedData,
                            containedInDimensions);
            partialArray->insert(0, movedData, containedInDimensions);
            delete movedData;
          }
          else if (array.getArrayType() == XdmfArrayType::UInt16()) {
            partialArray->initialize(XdmfArrayType::UInt16(), 0);
            unsigned short * movedData = new unsigned short[containedInDimensions];
            array.getValues(containedInPriorDimensions,
                            movedData,
                            containedInDimensions);
            partialArray->insert(0, movedData, containedInDimensions);
            delete movedData;
          }
          else if (array.getArrayType() == XdmfArrayType::UInt32()) {
            partialArray->initialize(XdmfArrayType::UInt32(), 0);
            unsigned int * movedData = new unsigned int[containedInDimensions];
            array.getValues(containedInPriorDimensions,
                            movedData,
                            containedInDimensions);
            partialArray->insert(0, movedData, containedInDimensions);
            delete movedData;
          }
          else if (array.getArrayType() == XdmfArrayType::String()) {
            partialArray->initialize(XdmfArrayType::String(), 0);
            for (int j = containedInPriorDimensions;
                 j < containedInPriorDimensions + containedInDimensions;
                 ++j) {
               partialArray->pushBack(array.getValue<std::string>(j));
            }
          }
          arraysWritten.push_back(partialArray);
          filesWritten.push_back(testFile.str());
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
    shared_ptr<XdmfArray> partialArray = XdmfArray::New();
    // Need to copy by duplicating the contents of the array
    unsigned int j = controllerIndexOffset;

    if (mMode == Default) {
      std::stringstream testFile;
      if (getFileIndex() == 0) {
        // If sequentially named files need to be created or referenced
        testFile << checkFileName << "." << checkFileExt;
      }
      else {
        testFile << checkFileName << getFileIndex() << "." << checkFileExt;
      }
      heavyDataController =
        this->createController(testFile.str(),
                               heavyDataController->getDataSetPath(),
                               array.getArrayType(),
                               start,
                               stride,
                               dimensions,
                               dataspaceDimensions);
    }
    else {
      heavyDataController =
        this->createController(heavyDataController->getFilePath(),
                               heavyDataController->getDataSetPath(),
                               array.getArrayType(),
                               start,
                               stride,
                               dimensions,
                               dataspaceDimensions);
    }

    int movedSize = 0;
    if (array.getArrayType() == XdmfArrayType::Int8()){
      partialArray->initialize(XdmfArrayType::Int8(), 0);
      if ((array.getSize() - controllerIndexOffset) <=
           heavyDataController->getSize()) {
        movedSize = array.getSize() - controllerIndexOffset;
      }
      else if (heavyDataController->getSize() <
               (array.getSize() - controllerIndexOffset)) {
        movedSize = heavyDataController->getSize();
      }
      char * movedData = new char[movedSize];
      array.getValues(controllerIndexOffset, movedData, movedSize);
      partialArray->insert(0, movedData, movedSize);
      j+=movedSize;
      delete movedData;
    }
    else if (array.getArrayType() == XdmfArrayType::Int16()){
      partialArray->initialize(XdmfArrayType::Int16(), 0);
      if ((array.getSize() - controllerIndexOffset) <=
          heavyDataController->getSize()) {
        movedSize = array.getSize() - controllerIndexOffset;
      }
      else if (heavyDataController->getSize() <
               (array.getSize() - controllerIndexOffset)) {
        movedSize = heavyDataController->getSize();
      }
      short * movedData = new short[movedSize];
      array.getValues(controllerIndexOffset, movedData, movedSize);
      partialArray->insert(0, movedData, movedSize);
      j+=movedSize;
      delete movedData;
    }
    else if (array.getArrayType() == XdmfArrayType::Int32()){
      partialArray->initialize(XdmfArrayType::Int32(), 0);
      if ((array.getSize() - controllerIndexOffset) <=
           heavyDataController->getSize()) {
        movedSize = array.getSize() - controllerIndexOffset;
      }
      else if (heavyDataController->getSize() <
               (array.getSize() - controllerIndexOffset)) {
        movedSize = heavyDataController->getSize();
      }
      int * movedData = new int[movedSize];
      array.getValues(controllerIndexOffset, movedData, movedSize);
      partialArray->insert(0, movedData, movedSize);
      j+=movedSize;
      delete movedData;
    }
    else if (array.getArrayType() == XdmfArrayType::Int64()){
      partialArray->initialize(XdmfArrayType::Int64(), 0);
      if ((array.getSize() - controllerIndexOffset) <=
           heavyDataController->getSize()) {
        movedSize = array.getSize() - controllerIndexOffset;
      }
      else if (heavyDataController->getSize() <
               (array.getSize() - controllerIndexOffset)) {
        movedSize = heavyDataController->getSize();
      }
      long * movedData = new long[movedSize];
      array.getValues(controllerIndexOffset, movedData, movedSize);
      partialArray->insert(0, movedData, movedSize);
      j+=movedSize;
      delete movedData;
    }
    else if (array.getArrayType() == XdmfArrayType::Float32()){
      partialArray->initialize(XdmfArrayType::Float32(), 0);
      if ((array.getSize() - controllerIndexOffset) <=
          heavyDataController->getSize()) {
        movedSize = array.getSize() - controllerIndexOffset;
      }
      else if (heavyDataController->getSize() <
               (array.getSize() - controllerIndexOffset)) {
        movedSize = heavyDataController->getSize();
      }
      float * movedData = new float[movedSize];
      array.getValues(controllerIndexOffset, movedData, movedSize);
      partialArray->insert(0, movedData, movedSize);
      j+=movedSize;
      delete movedData;
    }
    else if (array.getArrayType() == XdmfArrayType::Float64()){
      partialArray->initialize(XdmfArrayType::Float64(), 0);
      if ((array.getSize() - controllerIndexOffset) <=
          heavyDataController->getSize()) {
        movedSize = array.getSize() - controllerIndexOffset;
      }
      else if (heavyDataController->getSize() <
               (array.getSize() - controllerIndexOffset)) {
        movedSize = heavyDataController->getSize();
      }
      double * movedData = new double[movedSize];
      array.getValues(controllerIndexOffset, movedData, movedSize);
      partialArray->insert(0, movedData, movedSize);
      j+=movedSize;
      delete movedData;
    }
    else if (array.getArrayType() == XdmfArrayType::UInt8()){
      partialArray->initialize(XdmfArrayType::UInt8(), 0);
      if ((array.getSize() - controllerIndexOffset) <=
          heavyDataController->getSize()) {
        movedSize = array.getSize() - controllerIndexOffset;
      }
      else if (heavyDataController->getSize() <
               (array.getSize() - controllerIndexOffset)) {
        movedSize = heavyDataController->getSize();
      }
      unsigned char * movedData = new unsigned char[movedSize];
      array.getValues(controllerIndexOffset, movedData, movedSize);
      partialArray->insert(0, movedData, movedSize);
      j+=movedSize;
      delete movedData;
    }
    else if (array.getArrayType() == XdmfArrayType::UInt16()){
      partialArray->initialize(XdmfArrayType::UInt16(), 0);
      if ((array.getSize() - controllerIndexOffset) <=
          heavyDataController->getSize()) {
        movedSize = array.getSize() - controllerIndexOffset;
      }
      else if (heavyDataController->getSize() <
               (array.getSize() - controllerIndexOffset)) {
        movedSize = heavyDataController->getSize();
      }
      unsigned short * movedData = new unsigned short[movedSize];
      array.getValues(controllerIndexOffset, movedData, movedSize);
      partialArray->insert(0, movedData, movedSize);
      j+=movedSize;
      delete movedData;
    }
    else if (array.getArrayType() == XdmfArrayType::UInt32()) {
      partialArray->initialize(XdmfArrayType::UInt32(), 0);
      if ((array.getSize() - controllerIndexOffset) <=
          heavyDataController->getSize()) {
        movedSize = array.getSize() - controllerIndexOffset;
      }
      else if (heavyDataController->getSize() <
               (array.getSize() - controllerIndexOffset)) {
        movedSize = heavyDataController->getSize();
      }
      unsigned int * movedData = new unsigned int[movedSize];
      array.getValues(controllerIndexOffset, movedData, movedSize);
      partialArray->insert(0, movedData, movedSize);
      j+=movedSize;
      delete movedData;
    }
    else if (array.getArrayType() == XdmfArrayType::String()) {
      // closeDatatype is only true if strings are being used
      partialArray->initialize(XdmfArrayType::String(), 0);
      // Transfering via loop because the getValues function
      // is not fully tested with strings
      for (j = controllerIndexOffset;
           j < controllerIndexOffset + heavyDataController->getSize()
           && j < array.getSize();
           ++j){
        partialArray->pushBack(array.getValue<std::string>(j));
      }
    }
    arrayOffsetsWritten.push_back(controllerIndexOffset);
    // Set the offset to the point after the end of the current subset
    controllerIndexOffset = j;

    arraysWritten.push_back(partialArray);
    filesWritten.push_back(heavyDataController->getFilePath());
    // Also need to push the starts and strides loaded from the HeavyDataController
    startsWritten.push_back(start);
    stridesWritten.push_back(stride);
    dimensionsWritten.push_back(dimensions);
    dataSizesWritten.push_back(dataspaceDimensions);
  }
}

int
XdmfHeavyDataWriter::getAllowSetSplitting()
{
  return mAllowSplitDataSets;
}

int
XdmfHeavyDataWriter::getFileIndex()
{
  return mFileIndex;
}

unsigned int
XdmfHeavyDataWriter::getFileOverhead()
{
  return mFileOverhead;
}

std::string
XdmfHeavyDataWriter::getFilePath() const
{
  return mFilePath;
}

int
XdmfHeavyDataWriter::getFileSizeLimit()
{
  return mFileSizeLimit;
}

XdmfHeavyDataWriter::Mode
XdmfHeavyDataWriter::getMode() const
{
  return mMode;
}

bool 
XdmfHeavyDataWriter::getReleaseData() const
{
  return mReleaseData;
}

void
XdmfHeavyDataWriter::setAllowSetSplitting(bool newAllow)
{
  mAllowSplitDataSets = newAllow;
}

void
XdmfHeavyDataWriter::setFileIndex(int newSize)
{
  mFileIndex = newSize;
}

void
XdmfHeavyDataWriter::setFileSizeLimit(int newSize)
{
  mFileSizeLimit = newSize;
}

void
XdmfHeavyDataWriter::setMode(const Mode mode)
{
  mMode = mode;
}

void
XdmfHeavyDataWriter::setReleaseData(const bool releaseData)
{
  mReleaseData = releaseData;
}
