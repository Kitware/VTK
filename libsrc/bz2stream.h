/* Created (x) 2003, by Sandor Fazekas. For public domain. */
/* Last modified: $Date: 2009-08-31 14:46:54 $ $Revision: 1.1 $ */

#ifndef BZ2STREAM_H
#define BZ2STREAM_H

/* C++ stream classes for the bzip2 compression library. This is a */
/* lightweight C++ class library built on top of the excelent bzip2 */
/* compression library. Please note that the files handled by this */
/* library are opened in binary mode, which means that what you write */
/* is what you get, and there are no character substitutions (see e.g. */
/* Win32's two-character new-line). */

/* This code was inspired by Aaron Isotton's implementation of */
/* C++ Stream Classes for the bzip2 compression library. */

/* Modifications: 2009, Dominik Szczerba <dominik@itis.ethz.ch> */
/*                added multiple instances of std:: prefixes */

/* INCLUDE */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <exception>
#include <string>

/* INCLUDE */

#include <bzlib.h>

/* DEFINE */

/* These define some default values. */

#define BZ2S_BLOCK_SIZE_100_K       9       /* 1..9 */
#define BZ2S_WORK_FACTOR            0       /* 0..255 */
#define BZ2S_SMALL                  0       /* TRUE or FALSE */
#define BZ2S_IN_BUF_LEN             1024    /* > 0 */
#define BZ2S_OUT_BUF_LEN            1024    /* > 0 */
#define BZ2S_OUT_BACK_LEN           256     /* >= 0 */

/* USE NAMESPACE */

//using namespace std;

/* CLASS DECLARATION */

class obz2buf : public std::streambuf
{
private:
  int bzBlockSize100K;
  int bzWorkFactor; 

private:
  int bzInBufLen;
  int bzOutBufLen;

private:
  FILE* file;

private:
  bz_stream bzStrm;

private:
  char* bzInBuf;
  char* bzOutBuf;

public:
  inline obz2buf( 
      int blockSize100K = BZ2S_BLOCK_SIZE_100_K, 
      int workFactor = BZ2S_WORK_FACTOR,
      int inBufLen = BZ2S_IN_BUF_LEN, 
      int outBufLen = BZ2S_OUT_BUF_LEN 
    ) :
      bzBlockSize100K ( blockSize100K ),
      bzWorkFactor ( workFactor ),
      bzInBufLen ( inBufLen ), 
      bzOutBufLen ( outBufLen )
  { 
    reset(); /* initialize class */
  }

public:
  virtual ~obz2buf() 
  {
    close(); /* ignore error */
  }

public:
  inline bool is_open() const { return file != NULL; }

public:
  obz2buf* open( const char* fileName )
  {
    if( ! isParmOk() || is_open() ) return NULL;
    if( ! fileOpen( fileName ) ) return NULL; /* open file */
    if( ! bzInit() ) return NULL; /* initialize the BZ stream */
    return this;
  }

  obz2buf* open( const std::string& fileName )
  {
    if( ! isParmOk() || is_open() ) return NULL;
    if( ! fileOpen( fileName.c_str() ) ) return NULL; /* open file */
    if( ! bzInit() ) return NULL; /* initialize the BZ stream */
    return this;
  }

  obz2buf* open( int fileDesc )
  {
    if( ! isParmOk() || is_open() ) return NULL; 
    if( ! fileOpen( fileDesc ) ) return NULL; /* open file */
    if( ! bzInit() ) return NULL; /* initialize the BZ stream */
    return this;
  }

public:
  obz2buf* close()
  {
    if( ! is_open() ) return NULL;
    bool done = bzProcess() && bzFinish();
    bzEnd(); /* close BZ stream */
    bool closed = fileClose(); /* close file */
    reset(); /* reinitialize class */
    return ( done && closed ) ? this : NULL;
  }

protected:
  virtual int overflow( int c ) 
  {
    if( c != EOF ) 
    {
      /* we allocated place for one more element! */
      *pptr() = c; pbump( 1 ); /* add one more element */
    }
    /* process buffered elements */
    return bzProcess() ? ( c != EOF ? c : !EOF ) : EOF;
  }

protected:
  virtual int sync() 
  {
    /* process buffered elements and flush the BZ stream */
    return ( bzProcess() && bzFlush() ) ? 0 : -1;
  }

private:
  inline bool bzInit()
  {
    bzInBuf = ( char* ) malloc( bzInBufLen );
    if( bzInBuf == NULL ) return false;

    bzOutBuf = ( char* ) malloc( bzOutBufLen );
    if( bzOutBuf == NULL ) return false;

    memset( bzInBuf, 0, bzInBufLen );
    memset( bzOutBuf, 0, bzOutBufLen );

    {
      int retVal = BZ2_bzCompressInit( &bzStrm, 
        bzBlockSize100K, 0, bzWorkFactor );
      if( retVal != BZ_OK ) return false;
    }

    /* position streambuf's put positions */
    setp( bzInBuf, bzInBuf + bzInBufLen - 1 );

    return true;
  }

private:
  inline void bzEnd()
  {
    BZ2_bzCompressEnd( &bzStrm );
    if( bzInBuf != NULL ) free( bzInBuf );
    if( bzOutBuf != NULL ) free( bzOutBuf );
  }

private:
  inline bool bzProcess()
  {
    int inLen = pptr() - pbase();

    bzStrm.next_in = pbase();
    bzStrm.avail_in = inLen;
        
    while( bzStrm.avail_in > 0 ) 
    {
      bzStrm.next_out = bzOutBuf;
      bzStrm.avail_out = bzOutBufLen;
      
      BZ2_bzCompress( &bzStrm, BZ_RUN );

      if( ! fileWrite() ) return false;
    }

    pbump( - inLen );

    return true;
  }

private:
  inline bool bzFlush() 
  {
    bool flushed = false;

    bzStrm.next_in = NULL;
    bzStrm.avail_in = 0;
    
    while( ! flushed )
    {
      bzStrm.next_out = bzOutBuf;
      bzStrm.avail_out = bzOutBufLen;

      {
        int retVal = BZ2_bzCompress( &bzStrm, BZ_FLUSH );
        flushed = ( retVal == BZ_RUN_OK );
      }

      if( ! fileWrite() || ! fileFlush() ) return false;
    } 

    return true;
  }

private:
  inline bool bzFinish() 
  {
    bool finished = false;

    bzStrm.next_in = NULL;
    bzStrm.avail_in = 0;
    
    while( ! finished )
    {
      bzStrm.next_out = bzOutBuf;
      bzStrm.avail_out = bzOutBufLen;

      {
        int retVal = BZ2_bzCompress( &bzStrm, BZ_FINISH );
        finished = ( retVal == BZ_STREAM_END );
      }

      if( ! fileWrite() ) return false;
    }

    return true;
  }

private:
  inline bool fileOpen( const char* fileName )
  {
    file = fopen( fileName, "wb" );
    return file != NULL;
  }

  inline bool fileOpen( int fileDesc )
  {
    file = fdopen( fileDesc, "wb" );
    return file != NULL;
  }

private:
  inline bool fileClose()
  {
    return fclose( file ) == 0;
  }

private:
  inline bool fileWrite()
  {
    int outLen = bzOutBufLen - bzStrm.avail_out;
    int retVal = fwrite( bzOutBuf, 1, outLen, file );
    return retVal == outLen;
  }

private:
  inline bool fileFlush()
  {
    return fflush( file ) == 0;
  }

private:
  inline bool isParmOk() const
  {
    if( bzBlockSize100K < 1 || bzBlockSize100K > 9 ) return false;
    if( bzWorkFactor < 0 || bzWorkFactor > 250 ) return false;
    if( bzInBufLen <= 0 ) return false;
    if( bzOutBufLen <= 0 ) return false;
    return true;
  }

private:
  inline void reset()
  {
    file = NULL; 
    memset( &bzStrm, 0, sizeof( bz_stream ) );
    bzInBuf = bzOutBuf = NULL;
    setp( NULL, NULL );
  }
};

/* CLASS DECLARATION */

class ibz2buf : public std::streambuf
{
private:
  int bzSmall;

private:
  int bzInBufLen;
  int bzOutBufLen;

private:
  int bzOutBackLen;

private:
  FILE* file;
  bool isBadType;

private:
  bz_stream bzStrm;

private:
  char* bzInBuf;
  char* bzOutBuf;

private:
  char* bzInBegin;
  char* bzInEnd;

private:
  char* bzOutBasePos;
  int bzOutBaseLen;

public:
  inline ibz2buf( 
      int small = BZ2S_SMALL,
      int inBufLen = BZ2S_IN_BUF_LEN, 
      int outBufLen = BZ2S_OUT_BUF_LEN + BZ2S_OUT_BACK_LEN, 
      int outBufBackLen = BZ2S_OUT_BACK_LEN 
    ) :
      bzSmall ( small ),
      bzInBufLen ( inBufLen ), 
      bzOutBufLen ( outBufLen ),
      bzOutBackLen ( outBufBackLen )
  { 
    reset(); /* initialize class */
  }

public:
  virtual ~ibz2buf() 
  {
    close(); /* ignore error */
  }

public:
  inline bool is_open() const { return file != NULL; }
  inline bool is_bad_type() const { return isBadType; }

public:
  ibz2buf* open( const char* fileName )
  {
    if( ! isParmOk() || is_open() ) return NULL;
    if( ! fileOpen( fileName ) ) return NULL; /* open file */
    if( ! bzInit() ) return NULL; /* initialize the BZ stream */
    return this;
  }

  ibz2buf* open( const std::string& fileName )
  {
    if( ! isParmOk() || is_open() ) return NULL;
    if( ! fileOpen( fileName.c_str() ) ) return NULL; /* open file */
    if( ! bzInit() ) return NULL; /* initialize the BZ stream */
    return this;
  }

  ibz2buf* open( int fileDesc )
  {
    if( ! isParmOk() || is_open() ) return NULL; 
    if( ! fileOpen( fileDesc ) ) return NULL; /* open file */
    if( ! bzInit() ) return NULL; /* initialize the BZ stream */
    return this;
  }

public:
  ibz2buf* close()
  {
    if( ! is_open() ) return NULL;
    bzEnd(); /* close BZ stream */
    bool closed = fileClose(); /* close file */
    reset(); /* reinitialize class */
    return closed ? this : NULL;
  }

protected:
  virtual int underflow() 
  {
    bool empty = true;

    int outLen = gptr() - eback(); /* calc size of putback area */
    int backLen = ( outLen < bzOutBackLen ) ? outLen : bzOutBackLen;

    if( backLen > 0 ) /* prepare putback area */
      memcpy( bzOutBasePos - backLen, gptr() - backLen, backLen );

    while( empty ) 
    {
      if( ( bzInBegin == bzInEnd ) && ! fileRead() ) return EOF;
      
      bzStrm.next_in = bzInBegin;
      bzStrm.avail_in = bzInEnd - bzInBegin;

      bzStrm.next_out = bzOutBasePos;
      bzStrm.avail_out = bzOutBaseLen;

      {
        int retVal = BZ2_bzDecompress( &bzStrm );

        empty = ( bzStrm.avail_out == ( unsigned ) bzOutBaseLen );

        isBadType = 
          ( retVal == BZ_DATA_ERROR_MAGIC ) ||
          ( retVal == BZ_DATA_ERROR );

        if( retVal == BZ_STREAM_END )
        {
          if( empty ) return EOF;
        }
        else if( retVal != BZ_OK ) 
        {
          return EOF;
        }
      }
      
      bzInBegin = bzInEnd - bzStrm.avail_in;
    } 

    setg( bzOutBasePos - backLen, bzOutBasePos,
         bzOutBasePos + bzOutBaseLen - bzStrm.avail_out );

    return *( ( unsigned char* ) gptr() );
  }

private:
  inline bool bzInit()
  {
    bzInBuf = ( char* ) malloc( bzInBufLen );
    if( bzInBuf == NULL ) return false;

    bzOutBuf = ( char* ) malloc( bzOutBufLen );
    if( bzOutBuf == NULL ) return false;

    memset( bzInBuf, 0, bzInBufLen );
    memset( bzOutBuf, 0, bzOutBufLen );

    {
      int retVal = BZ2_bzDecompressInit( &bzStrm, 0, bzSmall );
      if( retVal != BZ_OK ) return false;
    }

    bzInBegin = bzInEnd = bzInBuf;

    bzOutBasePos = bzOutBuf + bzOutBackLen;
    bzOutBaseLen = bzOutBufLen - bzOutBackLen;

    setg( bzOutBasePos, bzOutBasePos, bzOutBasePos );

    return true;
  }

private:
  inline void bzEnd()
  {
    BZ2_bzDecompressEnd( &bzStrm );
    if( bzInBuf != NULL ) free( bzInBuf );
    if( bzOutBuf != NULL ) free( bzOutBuf );
  }

private:
  inline bool fileOpen( const char* fileName )
  {
    file = fopen( fileName, "rb" );
    return file != NULL;
  }

  inline bool fileOpen( int fileDesc )
  {
    file = fdopen( fileDesc, "rb" );
    return file != NULL;
  }

private:
  inline bool fileClose()
  {
    return fclose( file ) == 0;
  }

private:
  inline bool fileRead()
  {
    int inLen = fread( bzInBuf, 1, bzInBufLen, file );
    bzInBegin = bzInBuf; bzInEnd = bzInBegin + inLen;
    return inLen > 0;
  }

private:
  inline bool isParmOk() const
  {
    if( bzInBufLen <= 0 ) return false;
    if( bzOutBufLen <= 0 ) return false;
    if( bzOutBackLen < 0 ) return false;
    if( bzOutBackLen >= bzOutBufLen ) return false;
    return true;
  }

private:
  inline void reset()
  {
    file = NULL; 
    isBadType = false;
    memset( &bzStrm, 0, sizeof( bz_stream ) );
    bzInBuf = bzOutBuf = NULL;
    bzInBegin = bzInEnd = NULL;
    bzOutBasePos = NULL;
    bzOutBaseLen = 0;
    setg( NULL, NULL, NULL );
  }
};

/* CLASS DECLARATION */

class obz2stream : public std::ostream
{
private:
  obz2buf buf;

public:
  inline explicit obz2stream(
      int blockSize100K = BZ2S_BLOCK_SIZE_100_K, 
      int workFactor = BZ2S_WORK_FACTOR,
      int inBufLen = BZ2S_IN_BUF_LEN, 
      int outBufLen = BZ2S_OUT_BUF_LEN 
    ) : std::ostream( &buf ), 
        buf( blockSize100K, workFactor, inBufLen, outBufLen )
  {}

  inline explicit obz2stream(
      const char* fileName,
      int blockSize100K = BZ2S_BLOCK_SIZE_100_K, 
      int workFactor = BZ2S_WORK_FACTOR,
      int inBufLen = BZ2S_IN_BUF_LEN, 
      int outBufLen = BZ2S_OUT_BUF_LEN 
    ) : std::ostream( &buf ), 
        buf( blockSize100K, workFactor, inBufLen, outBufLen )
  { 
    open( fileName ); 
  }

  inline explicit obz2stream(
      const std::string& fileName,
      int blockSize100K = BZ2S_BLOCK_SIZE_100_K, 
      int workFactor = BZ2S_WORK_FACTOR,
      int inBufLen = BZ2S_IN_BUF_LEN, 
      int outBufLen = BZ2S_OUT_BUF_LEN 
    ) : std::ostream( &buf ), 
        buf( blockSize100K, workFactor, inBufLen, outBufLen )
  { 
    open( fileName ); 
  }

public:
  virtual ~obz2stream() {}

public:
  inline obz2buf* rdbuf() const { return ( obz2buf* ) &buf; }

public:
  inline bool is_open() const { return buf.is_open(); }

public:
  inline void open( const char* fileName )
  {
    if( buf.open( fileName ) == NULL ) setstate( failbit );
  }

  inline void open( const std::string& fileName )
  {
    if( buf.open( fileName ) == NULL ) setstate( failbit );
  }

  inline void open( int fileDesc )
  {
    if( buf.open( fileDesc ) == NULL ) setstate( failbit );
  }

public:
  inline void close() 
  { 
    if( buf.close() == NULL ) setstate( failbit ); 
  }
};

/* CLASS DECLARATION */

class ibz2stream : public std::istream
{
private:
  ibz2buf buf;

public:
  inline explicit ibz2stream(
      int small = BZ2S_SMALL,
      int inBufLen = BZ2S_IN_BUF_LEN, 
      int outBufLen = BZ2S_OUT_BUF_LEN + BZ2S_OUT_BACK_LEN, 
      int outBufBackLen = BZ2S_OUT_BACK_LEN 
    ) : std::istream( &buf ), 
        buf( small, inBufLen, outBufLen, outBufBackLen )
  {}

  inline explicit ibz2stream(
      const char* fileName,
      int small = BZ2S_SMALL,
      int inBufLen = BZ2S_IN_BUF_LEN, 
      int outBufLen = BZ2S_OUT_BUF_LEN + BZ2S_OUT_BACK_LEN, 
      int outBufBackLen = BZ2S_OUT_BACK_LEN 
    ) : std::istream( &buf ), 
        buf( small, inBufLen, outBufLen, outBufBackLen )
  { 
    open( fileName ); 
  }

  inline explicit ibz2stream(
      const std::string& fileName,
      int small = BZ2S_SMALL,
      int inBufLen = BZ2S_IN_BUF_LEN, 
      int outBufLen = BZ2S_OUT_BUF_LEN + BZ2S_OUT_BACK_LEN, 
      int outBufBackLen = BZ2S_OUT_BACK_LEN 
    ) : std::istream( &buf ), 
        buf( small, inBufLen, outBufLen, outBufBackLen )
  { 
    open( fileName ); 
  }

public:
  virtual ~ibz2stream() {}

public:
  inline ibz2buf* rdbuf() const { return ( ibz2buf* ) &buf; }

public:
  inline bool is_open() const { return buf.is_open(); }
  inline bool is_bad_type() const { return buf.is_bad_type(); }

public:
  inline void open( const char* fileName )
  {
    if( buf.open( fileName ) == NULL ) setstate( failbit );
  }

  inline void open( const std::string& fileName )
  {
    if( buf.open( fileName ) == NULL ) setstate( failbit );
  }

  inline void open( int fileDesc )
  {
    if( buf.open( fileDesc ) == NULL ) setstate( failbit );
  }

public:
  inline void close() 
  { 
    if( buf.close() == NULL ) setstate( failbit ); 
  }
};

#endif /* BZ2STREAM_H */

/* End of File */
