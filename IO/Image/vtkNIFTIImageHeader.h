/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNIFTIImageHeader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkNIFTIImageHeader
 * @brief   Store NIfTI header information.
 *
 * This class stores the header of a NIfTI file in a VTK-friendly format.
 * By using this class, it is possible to specify the header information
 * that will be stored in a file written by the vtkNIFTIImageWriter.  Note
 * that the SForm and QForm orientation information in this class will be
 * ignored by the writer if an SForm and QForm have been explicitly set
 * via the writer's SetSForm and SetQForm methods.  Also note that all
 * info like Dim, PixDim, DataType, etc. will be ignored by the writer
 * because this information must instead be taken from the vtkImageData
 * information.  Finally, note that the vtkNIFTIImageWriter will ignore the
 * Descrip field, since it has its own SetDescription method.
 * @par Thanks:
 * This class was contributed to VTK by the Calgary Image Processing and
 * Analysis Centre (CIPAC).
 * @sa
 * vtkNIFTIImageReader, vtkNIFTIImageWriter
*/

#ifndef vtkNIFTIImageHeader_h
#define vtkNIFTIImageHeader_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkObject.h"

struct nifti_1_header;
struct nifti_2_header;

//----------------------------------------------------------------------------
class VTKIOIMAGE_EXPORT vtkNIFTIImageHeader : public vtkObject
{
public:

  /**
   * NIFTI intent codes.
   */
  enum IntentCodeEnum {
    IntentNone = 0,
    IntentCorrel = 2,
    IntentTTest = 3,
    IntentFTest = 4,
    IntentZScore = 5,
    IntentChiSQ = 6,
    IntentBeta = 7,
    IntentBinom = 8,
    IntentGamma = 9,
    IntentPoisson = 10,
    IntentNormal = 11,
    IntentFTestNonc = 12,
    IntentChiSQNonc = 13,
    IntentLogistic = 14,
    IntentLaplace = 15,
    IntentUniform = 16,
    IntentTTestNonc = 17,
    IntentWeibull = 18,
    IntentChi = 19,
    IntentInvGauss = 20,
    IntentExtVal = 21,
    IntentPVal = 22,
    IntentLogPVal = 23,
    IntentLog10PVal = 24,
    IntentEstimate = 1001,
    IntentLabel = 1002,
    IntentNeuroName = 1003,
    IntentGenMatrix = 1004,
    IntentSymMatrix = 1005,
    IntentDispVect = 1006,
    IntentVector = 1007,
    IntentPointSet = 1008,
    IntentTriangle = 1009,
    IntentQuaternion = 1010,
    IntentDimless = 1011,
    IntentTimeSeries = 2001,
    IntentNodeIndex = 2002,
    IntentRGBVector = 2003,
    IntentRGBAVector = 2004,
    IntentShape = 2005
  };

  /**
   * NIFTI transform codes.
   */
  enum XFormCodeEnum {
    XFormUnkown = 0,
    XFormScannerAnat = 1,
    XFormAlignedAnat = 2,
    XFormTalairach = 3,
    XFormMNI152 = 4
  };

  /**
   * NIFTI slice codes.
   */
  enum SliceCodeEnum {
    SliceUnknown = 0,
    SliceSeqInc = 1,
    SliceSeqDec = 2,
    SliceAltInc = 3,
    SliceAltDec = 4,
    SliceAltInc2 = 5,
    SliceAltDec2 = 6
  };

  /**
   * NIFTI unit codes.
   */
  enum UnitsXYZTEnum {
    UnitsUnknown = 0,
    UnitsMeter = 1,
    UnitsMM = 2,
    UnitsMicron = 3,
    UnitsSpace = 7,
    UnitsSec = 8,
    UnitsMSec = 16,
    UnitsUSec = 24,
    UnitsHz = 32,
    UnitsPPM = 40,
    UnitsRads = 48,
    UnitsTime = 56
  };

  /**
   * NIFTI data types.
   * Types RGB24 and RGB32 are represented in VTK as a multi-component
   * unsigned char image.  Complex values are represented as two-component
   * images.  The NIFTI types Float128 and Complex256 are not supported.
   */
  enum DataTypeEnum {
    TypeUInt8 = 2,
    TypeInt16 = 4,
    TypeInt32 = 8,
    TypeFloat32 = 16,
    TypeComplex64 = 32,
    TypeFloat64 = 64,
    TypeRGB24 = 128,
    TypeInt8 = 256,
    TypeUInt16 = 512,
    TypeUInt32 = 768,
    TypeInt64 = 1024,
    TypeUInt64 = 1280,
    TypeFloat128 = 1536,
    TypeComplex128 = 1792,
    TypeComplex256 = 2048,
    TypeRGBA32 = 2304
  };

  /**
   * NIFTI header sizes.
   */
  enum HeaderSizeEnum {
    NIFTI1HeaderSize = 348,
    NIFTI2HeaderSize = 540
  };

  //@{
  /**
   * Static method for construction.
   */
  static vtkNIFTIImageHeader *New();
  vtkTypeMacro(vtkNIFTIImageHeader, vtkObject);
  //@}

  /**
   * Print information about this object.
   */
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Get the magic number for the NIFTI file as a null-terminated string.
   */
  const char *GetMagic() { return this->Magic; }

  /**
   * Get the offset to the pixel data within the file.
   */
  vtkTypeInt64 GetVoxOffset() { return this->VoxOffset; }

  /**
   * Get the data type.
   */
  int GetDataType() { return this->DataType; }

  /**
   * Get the number of bits per pixel.
   */
  int GetBitPix() { return this->BitPix; }

  /**
   * Get the nth dimension of the data, where GetDim(0) returns the
   * number of dimensions that are defined for the file.
   */
  vtkTypeInt64 GetDim(int i) {
    return (i < 0 || i > 7 ? 0 : this->Dim[i]); }

  /**
   * Get the sample spacing in the nth dimension. If GetPixDim(0) is
   * negative, then the quaternion for the qform describes the correct
   * orientation only after the slice ordering has been reversed.
   */
  double GetPixDim(int i) {
    return (i < 0 || i > 7 ? 0.0 : this->PixDim[i]); }

  //@{
  /**
   * Get the NIFTI intent code.  This is an enumerated value in the NIFTI
   * header that states what the data is intended to be used for.
   */
  vtkSetMacro(IntentCode, int);
  int GetIntentCode() { return this->IntentCode; }
  //@}

  /**
   * Get the intent name.  This should match the intent code.
   */
  void SetIntentName(const char *name);
  const char *GetIntentName() { return this->IntentName; }

  //@{
  /**
   * Get one of the NIFTI intent parameters.  The definition of these
   * parameters varies according to the IntentCode.
   */
  vtkSetMacro(IntentP1, double);
  double GetIntentP1() { return this->IntentP1; }
  vtkSetMacro(IntentP2, double);
  double GetIntentP2() { return this->IntentP2; }
  vtkSetMacro(IntentP3, double);
  double GetIntentP3() { return this->IntentP3; }
  //@}

  //@{
  /**
   * Get the scale and slope to apply to the data in order to get
   * the real-valued data values.
   */
  vtkSetMacro(SclSlope, double);
  double GetSclSlope() { return this->SclSlope; }
  vtkSetMacro(SclInter, double);
  double GetSclInter() { return this->SclInter; }
  //@}

  //@{
  /**
   * Get the calibrated range of the data, i.e. the values stored in the
   * cal_min and cal_max fields in the header.
   */
  vtkSetMacro(CalMin, double);
  double GetCalMin() { return this->CalMin; }
  vtkSetMacro(CalMax, double);
  double GetCalMax() { return this->CalMax; }
  //@}

  //@{
  /**
   * Get the slice_duration and toffset from the header.
   */
  vtkSetMacro(SliceDuration, double);
  double GetSliceDuration() { return this->SliceDuration; }
  vtkSetMacro(TOffset, double);
  double GetTOffset() { return this->TOffset; }
  //@}

  //@{
  /**
   * Get the slice range for the data.
   */
  vtkSetMacro(SliceStart, vtkTypeInt64);
  vtkTypeInt64 GetSliceStart() { return this->SliceStart; }
  vtkSetMacro(SliceEnd, vtkTypeInt64);
  vtkTypeInt64 GetSliceEnd() { return this->SliceEnd; }
  //@}

  //@{
  /**
   * Get the slice code for the data.
   */
  vtkSetMacro(SliceCode, int);
  int GetSliceCode() { return this->SliceCode; }
  //@}

  //@{
  /**
   * Get a bitfield that describes the units for the first 4 dims.
   */
  vtkSetMacro(XYZTUnits, int);
  int GetXYZTUnits() { return this->XYZTUnits; }
  //@}

  //@{
  /**
   * Get a bitfield with extra information about the dimensions, it
   * states which dimensions are the phase encode, frequency encode,
   * and slice encode dimensions for MRI acquisitions.
   */
  vtkSetMacro(DimInfo, int);
  int GetDimInfo() { return this->DimInfo; }
  //@}

  /**
   * Get a null-terminated file descriptor, this usually gives the
   * name of the software that wrote the file. It will have a maximum
   * length of 80 characters.  Use ASCII to ensure compatibility with
   * all NIFTI software, the NIFTI standard itself does not specify what
   * encodings are permitted.
   */
  void SetDescrip(const char *descrip);
  const char *GetDescrip() { return this->Descrip; }

  /**
   * Get an auxilliary file, e.g. a color table, that is associated
   * with this data.  The length of the filename must be a maximum of
   * 24 characters, and it will be assumed to be in the same directory
   * as the NIFTI file.
   */
  void SetAuxFile(const char *auxfile);
  const char *GetAuxFile() { return this->AuxFile; }

  //@{
  /**
   * Get the QForm or SForm code.
   */
  vtkSetMacro(QFormCode, int);
  int GetQFormCode() { return this->QFormCode; }
  vtkSetMacro(SFormCode, int);
  int GetSFormCode() { return this->SFormCode; }
  //@}

  //@{
  /**
   * Get information about the quaternion transformation.  Note that
   * the vtkNIFTIImageWriter ignores this part of the header if a quaternion
   * has been set via vtkNIFTIImageWriter::SetQFormMatrix().
   */
  vtkSetMacro(QuaternB, double);
  double GetQuaternB() { return this->QuaternB; }
  vtkSetMacro(QuaternC, double);
  double GetQuaternC() { return this->QuaternC; }
  vtkSetMacro(QuaternD, double);
  double GetQuaternD() { return this->QuaternD; }
  vtkSetMacro(QOffsetX, double);
  double GetQOffsetX() { return this->QOffsetX; }
  vtkSetMacro(QOffsetY, double);
  double GetQOffsetY() { return this->QOffsetY; }
  vtkSetMacro(QOffsetZ, double);
  double GetQOffsetZ() { return this->QOffsetZ; }
  //@}

  //@{
  /**
   * Get information about the matrix transformation.  Note that
   * the vtkNIFTIImageWriter ignores this part of the header if a matrix
   * has been set via vtkNIFTIImageWriter::SetSFormMatrix().
   */
  vtkSetVector4Macro(SRowX, double);
  vtkGetVector4Macro(SRowX, double);
  vtkSetVector4Macro(SRowY, double);
  vtkGetVector4Macro(SRowY, double);
  vtkSetVector4Macro(SRowZ, double);
  vtkGetVector4Macro(SRowZ, double);
  //@}

  /**
   * Initialize the header to default values.
   */
  void Initialize();

  /**
   * Make a copy of the header.
   */
  void DeepCopy(vtkNIFTIImageHeader *o);

  //@{
  /**
   * Set the values from an existing nifti struct, or store
   * the values in an existing nifti struct.
   */
  void SetHeader(const nifti_1_header *hdr);
  void GetHeader(nifti_1_header *hdr);
  void SetHeader(const nifti_2_header *hdr);
  void GetHeader(nifti_2_header *hdr);
  //@}

protected:
  vtkNIFTIImageHeader();
  ~vtkNIFTIImageHeader();

  char Magic[12];
  vtkTypeInt64 VoxOffset;
  int DataType;
  int BitPix;
  vtkTypeInt64 Dim[8];
  double PixDim[8];
  int IntentCode;
  char IntentName[18];
  double IntentP1;
  double IntentP2;
  double IntentP3;
  double SclSlope;
  double SclInter;
  double CalMin;
  double CalMax;
  double SliceDuration;
  double TOffset;
  vtkTypeInt64 SliceStart;
  vtkTypeInt64 SliceEnd;
  int SliceCode;
  int XYZTUnits;
  int DimInfo;
  char Descrip[82];
  char AuxFile[26];
  int QFormCode;
  int SFormCode;
  double QuaternB;
  double QuaternC;
  double QuaternD;
  double QOffsetX;
  double QOffsetY;
  double QOffsetZ;
  double SRowX[4];
  double SRowY[4];
  double SRowZ[4];

  void SetStringValue(char *x, const char *y, size_t n);

private:
  vtkNIFTIImageHeader(const vtkNIFTIImageHeader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkNIFTIImageHeader&) VTK_DELETE_FUNCTION;
};

#endif // vtkNIFTIImageHeader_h
