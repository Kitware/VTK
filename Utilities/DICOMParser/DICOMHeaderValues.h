
#include "DICOMTypes.h"

struct DICOMModalityHeaderValues 
{
  virtual void Print(std::ostream& os) = 0;
};

struct DICOMHeaderValues 
{
  void PrintHeader(std::ostream& s, DICOMHeaderValues const& header)
  {
    s << "  Manufacturer : " << (header.manufacturer ? header.manufacturer : "NULL") << std::endl;
    s << "  Study UID : " << (header.studyUID ? header.studyUID : "NULL") << std::endl;
    s << "  Series UID : " << (header.seriesUID ? header.seriesUID : "NULL") << std::endl;
    s << "  Image UID : " << (header.imageUID ? header.imageUID : "NULL") << std::endl;
    s << "  Transfer syntax UID : " << (header.transferSyntaxUID ? header.transferSyntaxUID : "NULL") << std::endl;

    s << "  Image Number : "    << header.image_num  << std::endl;
    s << "  Width : "        << header.width << std::endl;
    s << "  Height : "        << header.height << std::endl;
    s << "  Bit Depth : "        << header.depth << std::endl;
    s << "  Slice Thickness (mm) : "    << header.thickness  << std::endl;
    s << "  Pixel Spacing (mm) : "   << header.pix_spacing  << std::endl;
    s << "  Slice Spacing (mm) : " << header.slice_spacing << std::endl;
    //  s << "  Background Pixel : "      << header.bgshade << std::endl;
    //  s << "  Pixel Offset : " << header.pixel_offset << std::endl;

    //  s << "  kV : "           << header.kV << std::endl;
    //  s << "  mA : "           << header.mA << std::endl;
    //  s << "  Recon Kernel : "        << (header.reconType ? header.reconType : "NULL") << std::endl;

    s << "  Header Size (bytes) : "  << header.header_size << std::endl;
  
    s << "  Patient Position Upper Left (RAS) : " << (header.patient_position_ul ? header.patient_position_ul : "NULL")  << std::endl;
    s << "  Patient Position Cosines : " << (header.patient_position_cosines ? header.patient_position_cosines : "NULL") << std::endl;
    s << "  Upper Left Corner RAS : " << header.upper_left_R << ", " << header.upper_left_A << ", " << header.upper_left_S << std::endl;
    s << "  Lower Left Corner RAS : " << header.lower_left_R << ", " << header.lower_left_A << ", " << header.lower_left_S << std::endl;
    s << "  Upper Right Corner RAS : " << header.upper_right_R << ", " << header.upper_right_A << ", " << header.upper_right_S << std::endl;
    s << "  Lower Right Corner RAS : " << header.lower_right_R << ", " << header.lower_right_A << ", " << header.lower_right_S << std::endl;
    s << "  Isocenter Pixel Coordinates : " << header.RA_origin_x << ", " << header.RA_origin_y << std::endl;
  }


  DICOMHeaderValues()
  {
    magic_num[0] = 'D';
    magic_num[1] = 'I';
    magic_num[2] = 'C';
    magic_num[3] = 'M';
    header_size = 0;
    width = 0;
    height = 0;
    depth = 0;
    thickness = 0;
    pix_spacing = 0;
    slice_spacing = 0;
    image_num = 0;
    upper_left_R = 0;
    upper_left_A = 0;
    upper_left_S = 0;
    upper_right_R = 0;
    upper_right_A = 0;
    upper_right_S = 0;
    lower_right_R = 0;
    lower_right_A = 0;
    lower_right_S = 0;
    lower_left_R = 0;
    lower_left_A = 0;
    lower_left_S = 0;
    dir_cos_row_L = 0;
    dir_cos_row_P = 0;
    dir_cos_row_S = 0;

    dir_cos_col_L = 0;
    dir_cos_col_P = 0;
    dir_cos_col_S = 0;

    RA_origin_x = 0;
    RA_origin_y = 0;
    
    
    studyUID = NULL;
    seriesUID = NULL;
    imageUID = NULL;
    transferSyntaxUID = NULL;
    patient_position_ul = NULL;
    patient_position_cosines = NULL;

    manufacturer = NULL;
  }

  char       magic_num[4];
  quadbyte   header_size;  // unsigned int (4 bytes)
  quadbyte   width;
  quadbyte   height;
  quadbyte   depth; // number of bits/pixel
  float      thickness;
  float      pix_spacing;
  float      slice_spacing;

  int        image_num;

  float      upper_left_R;
  float      upper_left_A;
  float      upper_left_S;
  float      upper_right_R;
  float      upper_right_A;
  float      upper_right_S;
 
  float      lower_right_R;
  float      lower_right_A;
  float      lower_right_S;

  float      lower_left_R;
  float      lower_left_A;
  float      lower_left_S;

  float      dir_cos_row_L;
  float      dir_cos_row_P;
  float      dir_cos_row_S;

  float      dir_cos_col_L;
  float      dir_cos_col_P;
  float      dir_cos_col_S;

  // Pixel coordinates of the RA origin on the slice.
  int        RA_origin_x;
  int        RA_origin_y;

  // protocol information

  char*      studyUID;
  char*      seriesUID;
  char*      imageUID;
  char*      transferSyntaxUID;
  char*      patient_position_ul;
  char*      patient_position_cosines;

  char*      manufacturer;

  DICOMModalityHeaderValues* modality_header;
};


struct DICOMMRHeaderValues : public DICOMModalityHeaderValues 
{
    
};

struct DICOMCTHeaderValues : public DICOMModalityHeaderValues 
{
  DICOMCTHeaderValues()
  {
    kV = 0;
    mA = 0;

    reconType = NULL;
    pixel_offset = 0;
    bgshade = 0;
  }

  float      kV;
  float      mA;
  char*      reconType;

  float      pixel_offset;  // pixel_val + pixel_offset = HU
  doublebyte bgshade;       // pixel intensity of unused region

};

