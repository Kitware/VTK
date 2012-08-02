#include <XdmfArray.h>
#include <XdmfHDF.h>

char const * const kDatasetName = "FILE:TestFile.h5:/XdmfHDFSerial";

int main( int argc, char* argv[] ) {
  
  XdmfHDF* H5 = new XdmfHDF();
  XdmfArray* MyData = new XdmfArray();

  MyData->SetNumberType( XDMF_FLOAT32_TYPE );
  MyData->SetNumberOfElements( 100 );
  MyData->Generate( 0, 99 );

  H5->CopyType( MyData );
  H5->CopyShape( MyData );
  H5->Open( kDatasetName, "w" );
  H5->Write( MyData );
  H5->Close();

  XdmfHDF* H5In = new XdmfHDF();
  H5In->Open( kDatasetName, "r" );
  XdmfArray* result = H5In->Read();
  H5In->Close();

  bool failure = false;
  for ( size_t i = 0; i < 100; ++i ) {
    float value = result->GetValueAsFloat32( i );
    std::cout << i << " " << value << std::endl;
    failure = ( value != i );
  }

  delete result;
  delete MyData;
  delete H5;
  delete H5In;

  if ( failure ) {
    return -1;
  } else {
    return 0;
  }
};

