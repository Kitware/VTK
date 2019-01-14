/*****************************************************************************/
/*                                    Xdmf                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfTIFFController.cpp                                              */
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

#include <fstream>
#include <sstream>
#include "XdmfArray.hpp"
#include "XdmfArrayType.hpp"
#include "XdmfTIFFController.hpp"
#include "XdmfError.hpp"

#include "tiff.h"
#include "tiffio.h"

shared_ptr<XdmfTIFFController>
XdmfTIFFController::New(const std::string & filePath,
                        const shared_ptr<const XdmfArrayType> & type,
                        const std::vector<unsigned int> & dimensions)
{
  shared_ptr<XdmfTIFFController> p(new XdmfTIFFController(filePath,
                                                          type,
                                                          std::vector<unsigned int>(dimensions.size(), 0),
                                                          std::vector<unsigned int>(dimensions.size(), 1),
                                                          dimensions,
                                                          dimensions));
  return p;
}

shared_ptr<XdmfTIFFController>
XdmfTIFFController::New(const std::string & filePath,
                        const shared_ptr<const XdmfArrayType> & type,
                        const std::vector<unsigned int> & starts,
                        const std::vector<unsigned int> & strides,
                        const std::vector<unsigned int> & dimensions,
                        const std::vector<unsigned int> & dataspaces)
{
  shared_ptr<XdmfTIFFController> p(new XdmfTIFFController(filePath,
                                                          type,
                                                          starts,
                                                          strides,
                                                          dimensions,
                                                          dataspaces));
  return p;
}

XdmfTIFFController::XdmfTIFFController(const std::string & filePath,
                                       const shared_ptr<const XdmfArrayType> & type,
                                       const std::vector<unsigned int> & starts,
                                       const std::vector<unsigned int> & strides,
                                       const std::vector<unsigned int> & dimensions,
                                       const std::vector<unsigned int> & dataspaces) :
  XdmfHeavyDataController(filePath,
                          type,
                          starts,
                          strides,
                          dimensions,
                          dataspaces)
{
}

XdmfTIFFController::XdmfTIFFController(const XdmfTIFFController& refController):
  XdmfHeavyDataController(refController)
{
}

XdmfTIFFController::~XdmfTIFFController()
{
}

shared_ptr<XdmfHeavyDataController>
XdmfTIFFController::createSubController(const std::vector<unsigned int> & starts,
                                        const std::vector<unsigned int> & strides,
                                        const std::vector<unsigned int> & dimensions)
{
  return XdmfTIFFController::New(mFilePath,
                                 mType,
                                 starts,
                                 strides,
                                 dimensions,
                                 mDataspaceDimensions);
}

void
XdmfTIFFController::readToArray(XdmfArray * const array,
                                void * pointer,
                                unsigned int offset,
                                unsigned int start,
                                unsigned int stride,
                                unsigned int amount,
                                shared_ptr<const XdmfArrayType> type)
{
  if (type == XdmfArrayType::UInt32())
  {
    unsigned int * offsetpointer = &(((unsigned int *)pointer)[start]);
    array->insert(offset,
                  offsetpointer,
                  amount,
                  1,
                  stride);
  }
  else if (type == XdmfArrayType::UInt16())
  {
    unsigned short * offsetpointer = &(((unsigned short *)pointer)[start]);
    array->insert(offset,
                  offsetpointer,
                  amount,
                  1,
                  stride);
  }
  else if (type == XdmfArrayType::UInt8())
  {
    unsigned char * offsetpointer = &(((unsigned char *)pointer)[start]);
    array->insert(offset,
                  offsetpointer,
                  amount,
                  1,
                  stride);
  }
}

std::string
XdmfTIFFController::getName() const
{
  return "TIFF";
}

unsigned int
XdmfTIFFController::getNumberDirectories() const
{
  TIFF* tif = TIFFOpen(mFilePath.c_str(), "r");
  unsigned int count = 0;
  if (tif) {
    do {
      count++;
    } while (TIFFReadDirectory(tif));
    TIFFClose(tif);
  }
  return count;
}

void
XdmfTIFFController::getProperties(std::map<std::string, std::string> & collectedProperties) const
{
  collectedProperties["Format"] = this->getName();
  std::stringstream seekStream;
}

void
XdmfTIFFController::read(XdmfArray * const array)
{
  TIFF* tif = TIFFOpen(mFilePath.c_str(), "r");

  unsigned int compression = 0;

  TIFFGetField(tif, TIFFTAG_COMPRESSION, &compression);

  unsigned int currentDirectory = 0;

  if (tif && mStart.size() >= 3) {
    // setting the start for the first directory
    TIFFSetDirectory(tif, mStart[2]);
    currentDirectory = mStart[2];
  }

  unsigned int amountWritten = 0;
  // Only used for 1d controllers
  unsigned int sizeLeft = this->getSize();
  if (!array->isInitialized()) {
    array->initialize(this->getType());
  }
  if (array->getSize() != this->getSize()) {
    array->resize(mDimensions, 0);
  }

  // For single dimension version only
  unsigned int currentRowStart = mStart[0];
  unsigned int scanlineIndex = 0;

  if (mDimensions.size() > 1)
  {
    scanlineIndex = mStart[1];
  }

  if (tif) {
    bool validDir = true;
    while (validDir) {
      // Directories are handled by the third dimension
      // If no directories are specified, progress as far
      // as needed to fill the dimensions provided.
      unsigned int imagelength, bitsPerSample;
      tdata_t buf;
      unsigned int row;
      unsigned int scanlinesize = TIFFScanlineSize(tif);

      shared_ptr<const XdmfArrayType> tiffDataType = array->getArrayType();

      TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);

      TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imagelength);

      if (compression == 1) {
        // Specific to non-compressed read

        if (bitsPerSample / 8 == 1) {
          tiffDataType = XdmfArrayType::UInt8();
        }
        else if (bitsPerSample / 8 == 2) {
          tiffDataType = XdmfArrayType::UInt16();
        }
        else if (bitsPerSample / 8 == 4) {
          tiffDataType = XdmfArrayType::UInt32();
        }

        // the buffer is a number of bytes equal to the scan line size
        buf = _TIFFmalloc(scanlinesize );

        scanlinesize /= array->getArrayType()->getElementSize();

        if (mDimensions.size() == 1)
        {
          if (sizeLeft == 0) {
            break;
          }
          // If there is one dimensions then we treat the entire entry as a single dataset.
          // We need to adjust the starting point accordingly.
          for (row = 0; row < imagelength; ++row)
          {
            TIFFReadScanline(tif, buf, row);
            unsigned int amountRead = sizeLeft;
            if ((scanlinesize - currentRowStart) / mStride[0] <= sizeLeft) {
              amountRead = (scanlinesize - currentRowStart) / mStride[0];
              if (scanlinesize % mStride[0] != 0 &&
                  currentRowStart % mStride[0] <= scanlinesize - (amountRead * mStride[0] + currentRowStart))
              {
                 amountRead++;
              }
            }
            readToArray(array,
                        buf,
                        amountWritten,
                        currentRowStart,
                        mStride[0],
                        amountRead,
                        tiffDataType);
            // check to see how the start matches up with the scanline size
            amountWritten += amountRead;
            if (sizeLeft == 0) {
              break;
            }

            if (amountRead < sizeLeft) {
              sizeLeft = sizeLeft - amountRead;
            }
            else {
              sizeLeft = 0;
            }
            if (((int)((amountRead * mStride[0]) + currentRowStart)) - scanlinesize >= 0)
            {
              currentRowStart = ((amountRead * mStride[0]) + currentRowStart) - scanlinesize;
            }
            else
            {
              currentRowStart = ((amountRead * (mStride[0] + 1)) + currentRowStart) - scanlinesize;
            }
          }
        }
        else {
        // Dimensions correspond to scanline size and number of scanlines
          unsigned int rowstride = mStride[1];
          unsigned int currentRowStart = mStart[0];
          for (row = mStart[1]; row < imagelength && row < mDataspaceDimensions[1]; row+=rowstride)
          {
            TIFFReadScanline(tif, buf, row);
            readToArray(array,
                        buf,
                        amountWritten,
                        currentRowStart,
                        mStride[0],
                        mDimensions[0],
                        tiffDataType);
            amountWritten += mDimensions[0];
          }
        }
        _TIFFfree(buf);

      }
      else if (compression == 5)
      {
        // In this case we need to use strips instead of scanlines
        // scanline size is in bytes when dealing with compression
        if (bitsPerSample == 1) {
          tiffDataType = XdmfArrayType::UInt8();
        }
        else if (bitsPerSample == 2) {
          tiffDataType = XdmfArrayType::UInt16();
        }
        else if (bitsPerSample == 4) {
          tiffDataType = XdmfArrayType::UInt32();
        }

        // the buffer is a number of bytes equal to the scan line size
        buf = _TIFFmalloc(TIFFStripSize(tif));

        scanlinesize /= array->getArrayType()->getElementSize();

        // For each strip in the directory
        for (unsigned int strip = 0; strip < TIFFNumberOfStrips(tif); strip++)
        {
          if (sizeLeft == 0) {
            break;
          }

          unsigned int currentStripSize = TIFFReadEncodedStrip(tif, strip, buf, -1);
          currentStripSize = currentStripSize / array->getArrayType()->getElementSize();
          // Size is in bits, and is not necessarily the same per strip
          unsigned int numberScanlines = currentStripSize / scanlinesize;
          // For the case of a partial scanline
          if (currentStripSize % scanlinesize != 0) {
            ++numberScanlines;
          }
          // If singledimensional
          // then write out the strip as if it was a scanline

          if (mDimensions.size() == 1)
          {
            unsigned int amountRead = sizeLeft;
            if ((currentStripSize - currentRowStart) / mStride[0] <= sizeLeft) {
              amountRead = (currentStripSize - currentRowStart) / mStride[0];
              if (currentStripSize % mStride[0] != 0 &&
                  currentRowStart % mStride[0] <= currentStripSize - (amountRead * mStride[0] + currentRowStart))
              {
                 amountRead++;
              }
            }
            readToArray(array,
                        buf,
                        amountWritten,
                        currentRowStart,
                        mStride[0],
                        amountRead,
                        tiffDataType);
            amountWritten += amountRead;
            if (sizeLeft == 0) {
              break;
            }

            if (amountRead < sizeLeft) {
              sizeLeft = sizeLeft - amountRead;
            }
            else {
              sizeLeft = 0;
            }
            if (((int)((amountRead * mStride[0]) + currentRowStart)) - currentStripSize >= 0)
            {
              currentRowStart = ((amountRead * mStride[0]) + currentRowStart) - currentStripSize;
            }
            else
            {
              currentRowStart = ((amountRead * (mStride[0] + 1)) + currentRowStart) - currentStripSize;
            }
          }
          else
          {
            currentRowStart = scanlineIndex;
            // If multidimensional
            // Loop through the scanlines in the strip
            for (; scanlineIndex < numberScanlines; scanlineIndex += mStride[1])
            {
              readToArray(array,
                          buf,
                          amountWritten,
                          currentRowStart,
                          mStride[0],
                          mDimensions[0],
                          tiffDataType);
              amountWritten += mDimensions[0];
              currentRowStart = currentRowStart + scanlinesize * mStride[1];
            }
            scanlineIndex = scanlineIndex % mStride[1];
          }
        }
      }

      if (mStride.size() >= 3)
      {
        currentDirectory += mStride[2];
      }
      else
      {
        ++currentDirectory;
      }

      validDir = TIFFSetDirectory(tif, currentDirectory);
    }
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Error: Invalid TIFF file");
  }

  TIFFClose(tif);
}

// C Wrappers

XDMFTIFFCONTROLLER * XdmfTIFFControllerNew(char * filePath,
                                           int type,
                                           unsigned int * dimensions,
                                           unsigned int numDims,
                                           int * status)
{
  XDMF_ERROR_WRAP_START(status)
  try
  {
    std::vector<unsigned int> dimVector(dimensions, dimensions + numDims);
    shared_ptr<const XdmfArrayType> buildType = shared_ptr<XdmfArrayType>();
    switch (type) {
      case XDMF_ARRAY_TYPE_UINT8:
        buildType = XdmfArrayType::UInt8();
        break;
      case XDMF_ARRAY_TYPE_UINT16:
        buildType = XdmfArrayType::UInt16();
        break;
      case XDMF_ARRAY_TYPE_UINT32:
        buildType = XdmfArrayType::UInt32();
        break;
      case XDMF_ARRAY_TYPE_INT8:
        buildType = XdmfArrayType::Int8();
        break;
      case XDMF_ARRAY_TYPE_INT16:
        buildType = XdmfArrayType::Int16();
        break;
      case XDMF_ARRAY_TYPE_INT32:
        buildType = XdmfArrayType::Int32();
        break;
      case XDMF_ARRAY_TYPE_INT64:
        buildType = XdmfArrayType::Int64();
        break;
      case XDMF_ARRAY_TYPE_FLOAT32:
        buildType = XdmfArrayType::Float32();
        break;
      case XDMF_ARRAY_TYPE_FLOAT64:
        buildType = XdmfArrayType::Float64();
        break;
      default:
        XdmfError::message(XdmfError::FATAL,
                           "Error: Invalid ArrayType.");
        break;
    }
    shared_ptr<XdmfTIFFController> generatedController = XdmfTIFFController::New(std::string(filePath), buildType, dimVector);
    return (XDMFTIFFCONTROLLER *)((void *)(new XdmfTIFFController(*generatedController.get())));
  }
  catch (...)
  {
    std::vector<unsigned int> dimVector(dimensions, dimensions + numDims);
    shared_ptr<const XdmfArrayType> buildType = shared_ptr<XdmfArrayType>();
    switch (type) {
      case XDMF_ARRAY_TYPE_UINT8:
        buildType = XdmfArrayType::UInt8();
        break;
      case XDMF_ARRAY_TYPE_UINT16:
        buildType = XdmfArrayType::UInt16();
        break;
      case XDMF_ARRAY_TYPE_UINT32:
        buildType = XdmfArrayType::UInt32();
        break;
      case XDMF_ARRAY_TYPE_INT8:
        buildType = XdmfArrayType::Int8();
        break;
      case XDMF_ARRAY_TYPE_INT16:
        buildType = XdmfArrayType::Int16();
        break;
      case XDMF_ARRAY_TYPE_INT32:
        buildType = XdmfArrayType::Int32();
        break;
      case XDMF_ARRAY_TYPE_INT64:
        buildType = XdmfArrayType::Int64();
        break;
      case XDMF_ARRAY_TYPE_FLOAT32:
        buildType = XdmfArrayType::Float32();
        break;
      case XDMF_ARRAY_TYPE_FLOAT64:
        buildType = XdmfArrayType::Float64();
        break;
      default:
        XdmfError::message(XdmfError::FATAL,
                           "Error: Invalid ArrayType.");
        break;
    }
    shared_ptr<XdmfTIFFController> generatedController = XdmfTIFFController::New(std::string(filePath), buildType, dimVector);
    return (XDMFTIFFCONTROLLER *)((void *)(new XdmfTIFFController(*generatedController.get())));
  }
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

XDMFTIFFCONTROLLER * XdmfTIFFControllerNewHyperslab(char * filePath,
                                                    int type,
                                                    unsigned int * start,
                                                    unsigned int * stride,
                                                    unsigned int * dimensions,
                                                    unsigned int * dataspaceDimensions,
                                                    unsigned int numDims,
                                                    int * status)
{
  XDMF_ERROR_WRAP_START(status)
  try
  {
    std::vector<unsigned int> startVector(start, start + numDims);
    std::vector<unsigned int> strideVector(stride, stride + numDims);
    std::vector<unsigned int> dimVector(dimensions, dimensions + numDims);
    std::vector<unsigned int> dataspaceVector(dataspaceDimensions, dataspaceDimensions + numDims);
    shared_ptr<const XdmfArrayType> buildType = shared_ptr<XdmfArrayType>();
    switch (type) {
      case XDMF_ARRAY_TYPE_UINT8:
        buildType = XdmfArrayType::UInt8();
        break;
      case XDMF_ARRAY_TYPE_UINT16:
        buildType = XdmfArrayType::UInt16();
        break;
      case XDMF_ARRAY_TYPE_UINT32:
        buildType = XdmfArrayType::UInt32();
        break;
     case XDMF_ARRAY_TYPE_INT8:
        buildType = XdmfArrayType::Int8();
        break;
      case XDMF_ARRAY_TYPE_INT16:
        buildType = XdmfArrayType::Int16();
        break;
      case XDMF_ARRAY_TYPE_INT32:
        buildType = XdmfArrayType::Int32();
        break;
      case XDMF_ARRAY_TYPE_INT64:
        buildType = XdmfArrayType::Int64();
        break;
      case XDMF_ARRAY_TYPE_FLOAT32:
        buildType = XdmfArrayType::Float32();
        break;
      case XDMF_ARRAY_TYPE_FLOAT64:
        buildType = XdmfArrayType::Float64();
        break;
      default:
        XdmfError::message(XdmfError::FATAL,
                           "Error: Invalid ArrayType.");
        break;
    }
    shared_ptr<XdmfTIFFController> generatedController = XdmfTIFFController::New(std::string(filePath), buildType, startVector, strideVector, dimVector, dataspaceVector);
    return (XDMFTIFFCONTROLLER *)((void *)(new XdmfTIFFController(*generatedController.get())));
  }
  catch (...)
  {
    std::vector<unsigned int> startVector(start, start + numDims);
    std::vector<unsigned int> strideVector(stride, stride + numDims);
    std::vector<unsigned int> dimVector(dimensions, dimensions + numDims);
    std::vector<unsigned int> dataspaceVector(dataspaceDimensions, dataspaceDimensions + numDims);
    shared_ptr<const XdmfArrayType> buildType = shared_ptr<XdmfArrayType>();
    switch (type) {
      case XDMF_ARRAY_TYPE_UINT8:
        buildType = XdmfArrayType::UInt8();
        break;
      case XDMF_ARRAY_TYPE_UINT16:
        buildType = XdmfArrayType::UInt16();
        break;
      case XDMF_ARRAY_TYPE_UINT32:
        buildType = XdmfArrayType::UInt32();
        break;
     case XDMF_ARRAY_TYPE_INT8:
        buildType = XdmfArrayType::Int8();
        break;
      case XDMF_ARRAY_TYPE_INT16:
        buildType = XdmfArrayType::Int16();
        break;
      case XDMF_ARRAY_TYPE_INT32:
        buildType = XdmfArrayType::Int32();
        break;
      case XDMF_ARRAY_TYPE_INT64:
        buildType = XdmfArrayType::Int64();
        break;
      case XDMF_ARRAY_TYPE_FLOAT32:
        buildType = XdmfArrayType::Float32();
        break;
      case XDMF_ARRAY_TYPE_FLOAT64:
        buildType = XdmfArrayType::Float64();
        break;
      default:
        XdmfError::message(XdmfError::FATAL,
                           "Error: Invalid ArrayType.");
        break;
    }
    shared_ptr<XdmfTIFFController> generatedController = XdmfTIFFController::New(std::string(filePath), buildType, startVector, strideVector, dimVector, dataspaceVector);
    return (XDMFTIFFCONTROLLER *)((void *)(new XdmfTIFFController(*generatedController.get())));
  }
  XDMF_ERROR_WRAP_END(status)
  return NULL;
}

// C Wrappers for parent classes are generated by macros

XDMF_HEAVYCONTROLLER_C_CHILD_WRAPPER(XdmfTIFFController, XDMFTIFFCONTROLLER)
