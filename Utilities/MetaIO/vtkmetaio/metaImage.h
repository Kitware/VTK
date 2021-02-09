/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#include "metaTypes.h"

#ifndef ITKMetaIO_METAIMAGE_H
#  define ITKMetaIO_METAIMAGE_H

#  include "metaUtils.h"
#  include "metaObject.h"

#  include "metaImageTypes.h"
#  include "metaImageUtils.h"

/*!    MetaImage (.h and .cpp)
 *
 * Description:
 *    Reads and Writes MetaImageFiles.
 *    MetaImageFiles can be in one of two possible formats:
 *       a combined header/data format, typically designated .mha files
 *       or as separate header and data files, typically designated
 *       .mhd and .mda files
 *
 * Features:
 *    Header information is in ascii format - for easy creation, editing,
 *    and review. If separate files are used, a specified number of
 *    header-bytes in the datafile can be skipped
 *       - in this manner, different file formats (e.g., bmp, img,
 *          and /usr/Image) can be converted to metaImageFile format by
 *          merely specifying a separate text header (.mhd) file and in that
 *          file specifying how many data-header-bytes should be skipped.
 *          Thereby the data files can serve a dual role (as bitmap files
 *          and as metaImage data files)
 *    Supports all pixel formats
 *    Handles byte ordering (MSB/LSB)
 *    Provides the ability to handle associated medical image
 *          information (element size, element spacing, etc).
 *    Has required and optional header data (provides rapid formation
 *          or extensive documentation).
 *    REQUIRED: NDims, DimSize, ByteOrderMSB, ElementDataType, DataFileName
 *
 * \author Stephen R. Aylward
 *
 * \date August 29, 1999
 *
 * Depends on:
 *    MetaUtils.h
 *    MetaFileLib.h
 */

#  if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE
{
#  endif

class METAIO_EXPORT MetaImage : public MetaObject
{
public:
  // Constructors & Destructor
  MetaImage();

  explicit MetaImage(const char * _headerName);

  explicit MetaImage(MetaImage * _im); // share memory

  MetaImage(int               _nDims,
            const int *       _dimSize,
            const float *     _elementSpacing,
            MET_ValueEnumType _elementType,
            int               _elementNumberOfChannels = 1,
            void *            _elementData = nullptr);

  MetaImage(int               _nDims,
            const int *       _dimSize,
            const double *    _elementSpacing,
            MET_ValueEnumType _elementType,
            int               _elementNumberOfChannels = 1,
            void *            _elementData = nullptr);

  MetaImage(int               _x,
            int               _y,
            double            _elementSpacingX,
            double            _elementSpacingY,
            MET_ValueEnumType _elementType,
            int               _elementNumberOfChannels = 1,
            void *            _elementData = nullptr);

  MetaImage(int               _x,
            int               _y,
            int               _z,
            double            _elementSpacingX,
            double            _elementSpacingY,
            double            _elementSpacingZ,
            MET_ValueEnumType _elementType,
            int               _elementNumberOfChannels = 1,
            void *            _elementData = nullptr);

  ~MetaImage() override;

  void
  PrintInfo() const override;

  void
  CopyInfo(const MetaObject * _object) override;

  void
  Clear() override;

  // Legacy for floating point elementSpacing
  bool
  InitializeEssential(int               _nDims,
                      const int *       _dimSize,
                      const float *     _elementSpacing,
                      MET_ValueEnumType _elementType,
                      int               _elementNumberOfChannels = 1,
                      void *            _elementData = nullptr,
                      bool              _allocElementMemory = true);

  bool
  InitializeEssential(int               _nDims,
                      const int *       _dimSize,
                      const double *    _elementSpacing,
                      MET_ValueEnumType _elementType,
                      int               _elementNumberOfChannels = 1,
                      void *            _elementData = nullptr,
                      bool              _allocElementMemory = true);

  int
  HeaderSize() const;
  void
  HeaderSize(int _headerSize);

  MET_ImageModalityEnumType
  Modality() const;
  void
  Modality(MET_ImageModalityEnumType _modality);

  //    DimSize(...)
  //       REQUIRED Field
  //       Number of elements along each dimension
  const int *
  DimSize() const;
  int
  DimSize(int _i) const;
  // void  DimSize(const int * _dimSize);
  // void  DimSize(int _i, int _value);

  //    Quantity()
  //       Not a field in file
  //       Total number of elements in image (Prod(dimSize[i]))
  std::streamoff
  Quantity() const;

  //    SubQuantity(...)
  //       Not a field in file
  //       Number of elements in image spanning sub-dimensions
  //       E.g., elements per line, 2D sub-image, 3D sub-volume,
  const std::streamoff *
  SubQuantity() const;
  std::streamoff
  SubQuantity(int _i) const;

  //    SequenceID(...)
  //       Optional Field
  //       DICOM designation of this image relative to other images
  //         acquired at the same time
  const float *
  SequenceID() const;
  float
  SequenceID(int _i) const;
  void
  SequenceID(const float * _sequenceID);
  void
  SequenceID(int _i, float _value);

  //    ElemSize(...)
  //       Optional Field
  //       Physical size (in MM) of each element in the image
  //       (0 = xSize, 1 = ySize, 2 = zSize)
  bool
  ElementSizeValid() const;
  void
  ElementSizeValid(bool _elementSizeValid);
  const double *
  ElementSize() const;
  double
  ElementSize(int _i) const;
  void
  ElementSize(const double * _elementSize);
  void
  ElementSize(const float * _elementSize); // legacy
  void
  ElementSize(int _i, double _value);

  MET_ValueEnumType
  ElementType() const;
  void
  ElementType(MET_ValueEnumType _elementType);

  int
  ElementNumberOfChannels() const;
  void
  ElementNumberOfChannels(int _elementNumberOfChannels);

  //    ElemMakeByteOrderMSB(), ElemMakeByteOrderLSB(),
  //    ElemByteOrderSwap(), ElemByteOrderFix()
  //       The following functions are available only after
  //       ReadImageData() or if _read_and_close=TRUE when read
  // if streaming is used, then the size of buffer in total number
  // of elements, should be passed as an argument, otherwise the
  // internal value Quantity() will be used
  void
  ElementByteOrderSwap(std::streamoff _quantity = 0);
  bool
  ElementByteOrderFix(std::streamoff _quantity = 0);

  //    Min(...) Max(...)
  //       The default max returned is the largest allowed by
  //         ElemNBytes (12 bit uint16_t will give 4096 max).
  //       This may not represent the true max.   Use _reCalc=true
  //         to force a calcuation of the actual max element value.
  bool
  ElementMinMaxValid() const;
  void
  ElementMinMaxValid(bool _elementMinMaxValid);
  void
  ElementMinMaxRecalc();
  double
  ElementMin() const;
  void
  ElementMin(double _elementMin);
  double
  ElementMax() const;
  void
  ElementMax(double _elementMax);

  double
  ElementToIntensityFunctionSlope() const;
  void
  ElementToIntensityFunctionSlope(double _elementToIntensityFunctionSlope);
  double
  ElementToIntensityFunctionOffset() const;
  void
  ElementToIntensityFunctionOffset(double _elementOffset);

  bool
  AutoFreeElementData() const;
  void
  AutoFreeElementData(bool _autoFreeElementData);


  const char *
  ElementDataFileName() const;
  void
  ElementDataFileName(const char * _elementDataFileName);

  void *
  ElementData();
  double
  ElementData(std::streamoff _i) const;
  bool
  ElementData(std::streamoff _i, double _v);
  void
  ElementData(void * _elementData, bool _autoFreeElementData = false);

  //    ConverTo(...)
  //       Converts to a new data type
  //       Rescales using Min and Max (see above)
  bool
  ConvertElementDataTo(MET_ValueEnumType _elementType = MET_USHORT, double _toMin = 0, double _toMax = 0);
  bool
  ConvertElementDataToIntensityData(MET_ValueEnumType _elementType = MET_SHORT);
  bool
  ConvertIntensityDataToElementData(MET_ValueEnumType _elementType = MET_USHORT);

  static bool
  CanRead(const char * _headerName = nullptr) ;

  bool
  Read(const char * _headerName = nullptr ) override
  {
    return MetaImage::Read(_headerName, true, nullptr);
  }

  bool
  Read(const char * _headerName, bool _readElements, void * _buffer = nullptr);

  bool
  ReadROI(int *        _indexMin,
          int *        _indexMax,
          const char * _headerName = nullptr,
          bool         _readElements = true,
          void *       _buffer = nullptr,
          unsigned int subSamplingFactor = 1);


  static bool
  CanReadStream(std::ifstream * _stream) ;

  bool
  ReadStream(int _nDims, std::ifstream * _stream, bool _readElements = true, void * _buffer = nullptr);

  bool
  ReadROIStream(int *           _indexMin,
                int *           _indexMax,
                int             _nDims,
                std::ifstream * _stream,
                bool            _readElements = true,
                void *          _buffer = nullptr,
                unsigned int    subSamplingFactor = 1);

  bool
  Write(const char * _headName = nullptr) override
  {
    return MetaImage::Write(_headName, nullptr, true, nullptr, false);
  }

  bool
  Write(const char * _headName,
        const char * _dataName,
        bool         _writeElements = true,
        const void * _constElementData = nullptr,
        bool         _append = false);

  bool
  WriteROI(int *        _indexMin,
           int *        _indexMax,
           const char * _headName = nullptr,
           const char * _dataName = nullptr,
           bool         _writeElements = true,
           const void * _constElementData = nullptr,
           bool         _append = false);

  bool
  WriteStream(std::ofstream * _stream, bool _writeElements = true, const void * _constElementData = nullptr);


  bool
  Append(const char * _headName = nullptr) override;


  typedef std::pair<long, long> CompressionOffsetType;

  // PROTECTED
protected:
  MET_ImageModalityEnumType m_Modality;


  MET_CompressionTableType * m_CompressionTable{};

  int            m_DimSize[10]{};
  std::streamoff m_SubQuantity[10]{};
  std::streamoff m_Quantity{};

  int m_HeaderSize{};

  float m_SequenceID[4]{};

  bool   m_ElementSizeValid{};
  double m_ElementSize[10]{};

  MET_ValueEnumType m_ElementType;

  int m_ElementNumberOfChannels{};

  bool   m_ElementMinMaxValid{};
  double m_ElementMin{};
  double m_ElementMax{};

  double m_ElementToIntensityFunctionSlope{};
  double m_ElementToIntensityFunctionOffset{};

  bool m_AutoFreeElementData{};

  void * m_ElementData{};

  std::string m_ElementDataFileName;


  void
  M_ResetValues();

  void
  M_SetupReadFields() override;

  void
  M_SetupWriteFields() override;

  bool
  M_Read() override;

  // _dataQuantity is expressed in number of pixels. Internally it will be
  // scaled by the number of components and number of bytes per component.
  bool
  M_ReadElements(std::ifstream * _fstream, void * _data, std::streamoff _dataQuantity);

  // _totalDataQuantity and _dataQuantity are expressed in number of pixels.
  // Internally they will be scaled by the number of components and number of
  // bytes per component.
  bool
  M_ReadElementsROI(std::ifstream * _fstream,
                    void *          _data,
                    std::streamoff  _dataQuantity,
                    int *           _indexMin,
                    int *           _indexMax,
                    unsigned int    subSamplingFactor = 1,
                    std::streamoff  _totalDataQuantity = 0);

  bool
  M_ReadElementData(std::ifstream * _fstream, void * _data, std::streamoff _dataQuantity);

  bool
  M_WriteElements(std::ofstream * _fstream, const void * _data, std::streamoff _dataQuantity);

  bool
  M_WriteElementsROI(std::ofstream * _fstream,
                     const void *    _data,
                     std::streampos  _dataPos,
                     const int *     _indexMin,
                     const int *     _indexMax);

  bool
  M_WriteElementData(std::ofstream * _fstream, const void * _data, std::streamoff _dataQuantity);

  static bool
  M_FileExists(const char * filename) ;

  static bool
  FileIsFullPath(const char * in_name) ;

  static std::string
  M_GetTagValue(const std::string & buffer, const char * tag) ;

  // PRIVATE
private:
  void
  InitHelper(int               _nDims,
             const int *       _dimSize,
             const double *    _elementSpacing,
             MET_ValueEnumType _elementType,
             int               _elementNumberOfChannels,
             void *            _elementData);
};

#  if (METAIO_USE_NAMESPACE)
};
#  endif


#endif
