#include <mpi.h>
#include <XdmfArray.h>
#include <XdmfHDF.h>


/// Simple memory buffer implementation that keeps track of it's stream pointer.
class Buffer {
private:
  std::size_t m_size;
  char* m_data;
  char* m_put;
  char* m_tell;

public:
  Buffer( std::size_t lsize ) :
    m_size( lsize ),
    m_data( new char[lsize] ),
    m_put( m_data ),
    m_tell( m_data )
  {}
  
  ~Buffer() {
    delete [] m_data;
  }

  /// put a single value into the buffer
  template< typename T > 
  void put( const T& t ) {
    std::size_t lsize = sizeof( T );
    memcpy( m_put, &t, lsize );
    m_put += lsize;
  }

  /// copy a contiguous block into the buffer
  void put( void* data, std::size_t lsize ) {
    memcpy( m_put, data, lsize );
    m_put += lsize;
  }

  /// Copy a single value into the buffer.
  template< typename T >
  T tell() {
    std::size_t tsize = sizeof( T );
    T tmp;
    memcpy( &tmp, m_tell, tsize );
    m_tell += tsize;
    return tmp;
  }

  /// copy a contiguous block of data from the buffer to an already allocated
  /// location
  void tell( void* out, std::size_t lsize ) {
    memcpy( out, m_tell, lsize );
    m_tell += lsize;
  }

  std::size_t size() const {
    return m_size;
  }

  char* pointer() {
    return m_data;
  }

  void reset() {
    m_put = m_data;
    m_tell = m_data;
  }
};

/// Callback implements parallel IO by communicating to rank 0 in MPI_COMM_WORLD.
class CommunicationCallback :
  public XdmfOpenCallback,
  public XdmfWriteCallback,
  public XdmfCloseCallback,
  public XdmfReadCallback
{
private:
  int mCommRank;
  int mCommSize;
public:

  CommunicationCallback() {
    MPI_Comm_size( MPI_COMM_WORLD, &mCommSize );
    MPI_Comm_rank( MPI_COMM_WORLD, &mCommRank );
  }

  XdmfInt32 DoOpen( 
    XdmfHeavyData* ds, 
    XdmfConstString name, 
    XdmfConstString access ) 
  {
  // If HDF5 is compiled with Parallel IO, we must use collective open
#ifndef H5_HAVE_PARALLEL
    if ( mCommRank == 0 ) {
      return ds->DoOpen( name, access );
    } else {
      return XDMF_SUCCESS;
    }
#else
    return ds->DoOpen( name, access );
#endif
  }

  XdmfInt32 DoClose( XdmfHeavyData* ds )
  {
#ifndef H5_HAVE_PARALLEL
    if ( mCommRank == 0 ) {
      return ds->DoClose();
    } else {
      return XDMF_SUCCESS;
    }
#else
    return ds->DoClose();
#endif
  }

  XdmfInt32 DoWrite( XdmfHeavyData* ds, XdmfArray* array )
  {
    MPI_Status stat;
    // this is a really bad implementation that assumes rank 0 has the same data
    // size as everyone else, but we're really just going for a simple
    // example here.  The real coalescing implementation will require a few more
    // classes to handle buffering the data cleanly and robustly.

    XdmfInt64 start[1], stride[1], count[1];
    XdmfInt32 slab_rank = ds->GetHyperSlab( start, stride, count );
    std::size_t slab_info_size = 
      sizeof( XdmfInt32 ) // slab rank
      + slab_rank * sizeof( XdmfInt64 ) * 3; // start, stride, and count
    Buffer buf( slab_info_size + array->GetCoreLength() );

    if ( mCommRank != 0 ) {
      // copy local data to the buffer for sending
      buf.put( slab_rank );
      for ( int i = 0; i < slab_rank; ++i ) {
        buf.put( start[i] );
        buf.put( stride[i] );
        buf.put( count[i] );
      }
      buf.put( array->GetDataPointer(), array->GetCoreLength() );
      MPI_Send( 
        buf.pointer(), 
        buf.size(), 
        MPI_BYTE, 
        0, 
        0,
        MPI_COMM_WORLD );
    } else {
      // first, it's easy to write my own data
      ds->DoWrite( array );

      int processes_received = 1; // I've written local data
      while ( processes_received < mCommSize ) {
        MPI_Recv(
          buf.pointer(),
          buf.size(),
          MPI_BYTE,
          MPI_ANY_SOURCE,
          0,
          MPI_COMM_WORLD,
          &stat );
        processes_received++;

        // pull the information from the buffer
        buf.reset();
        slab_rank = buf.tell< XdmfInt32 >();
        for( int i = 0; i < slab_rank; ++i ) {
          start[i] = buf.tell< XdmfInt64 >();
          stride[i] = buf.tell< XdmfInt64 >();
          count[i] = buf.tell< XdmfInt64 >();
        }
        ds->SelectHyperSlab( start, stride, count );
        XdmfArray* recv = new XdmfArray;
        recv->CopyShape( array );
        buf.tell( recv->GetDataPointer(), recv->GetCoreLength() );
        ds->DoWrite( recv );
        delete recv;
      }
    }
    
    return XDMF_SUCCESS;
  }

  XdmfArray* DoRead( XdmfHeavyData* ds, XdmfArray* array )
  {
    if ( mCommRank == 0 ) {
      return ds->DoRead( array );
    } else {
      return NULL;
    }
  }
};

char const * const kDatasetName = "FILE:TestFile.h5:/XdmfHDFMPI";

int main( int argc, char* argv[] ) {
  MPI_Init( &argc, &argv );
 
  int rank;
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );

  XdmfHDF* H5 = new XdmfHDF();
  CommunicationCallback* cb = new CommunicationCallback;
  H5->setOpenCallback( cb );
  H5->setWriteCallback( cb );
  H5->setCloseCallback(cb );
  XdmfArray* MyData = new XdmfArray();

  MyData->SetNumberType( XDMF_FLOAT32_TYPE );
  MyData->SetNumberOfElements( 25 );
  MyData->Generate( rank * 25, rank*25 + 24 );

  H5->CopyType( MyData );
  XdmfInt64 dims[1], start[1], stride[1], count[1];
  dims[0] = 100;
  H5->SetShape( 1, dims );
  start[0] = rank * 25;
  stride[0] = 1;
  count[0] = 25;
  H5->SelectHyperSlab( start, stride, count );
  H5->Open( kDatasetName, "w" );
  H5->Write( MyData );
  H5->Close();
  
  bool failure = false;

  XdmfHDF* H5In = new XdmfHDF();
  H5In->setReadCallback( cb );
  H5In->setOpenCallback( cb );
  H5In->Open( kDatasetName, "r" );
  XdmfArray* result = H5In->Read();

  if ( result ) {
    for ( size_t i = 0; i < 100; ++i ) {
      float value = result->GetValueAsFloat32( i );
      std::cout << i << " " << value << std::endl;
      failure = ( value != i );
    }
  }

  delete H5;
  delete cb;
  delete MyData;
  delete H5In;
  delete result;

  MPI_Finalize();

  if ( failure ) {
    return -1;
  } else {
    return 0;
  }
};

