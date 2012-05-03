#ifndef vtk_zlib_mangle_h
#define vtk_zlib_mangle_h

/*

This header file mangles all symbols exported from the zlib library.
It is included in all files while building the zlib library.  Due to
namespace pollution, no zlib headers should be included in .h files in
VTK.

The following command was used to obtain the symbol list:

nm libvtkzlib.so |grep " [TRD] "

This is the way to recreate the whole list:

nm libvtkzlib.so |grep " [TRD] " | awk '{ print "#define "$3" vtk_zlib_"$3 }'

REMOVE the "_init" and "_fini" entries.

*/

#define adler32 vtk_zlib_adler32
#define adler32_combine vtk_zlib_adler32_combine
#define compress vtk_zlib_compress
#define compress2 vtk_zlib_compress2
#define compressBound vtk_zlib_compressBound
#define crc32 vtk_zlib_crc32
#define crc32_combine vtk_zlib_crc32_combine
#define get_crc_table vtk_zlib_get_crc_table
#define deflate vtk_zlib_deflate
#define deflateBound vtk_zlib_deflateBound
#define deflateCopy vtk_zlib_deflateCopy
#define deflateEnd vtk_zlib_deflateEnd
#define deflateInit2_ vtk_zlib_deflateInit2_
#define deflateInit_ vtk_zlib_deflateInit_
#define deflateParams vtk_zlib_deflateParams
#define deflatePrime vtk_zlib_deflatePrime
#define deflateReset vtk_zlib_deflateReset
#define deflateSetDictionary vtk_zlib_deflateSetDictionary
#define deflateSetHeader vtk_zlib_deflateSetHeader
#define deflateTune vtk_zlib_deflateTune
#define deflate_copyright vtk_zlib_deflate_copyright
#define gzclearerr vtk_zlib_gzclearerr
#define gzclose vtk_zlib_gzclose
#define gzdirect vtk_zlib_gzdirect
#define gzdopen vtk_zlib_gzdopen
#define gzeof vtk_zlib_gzeof
#define gzerror vtk_zlib_gzerror
#define gzflush vtk_zlib_gzflush
#define gzgetc vtk_zlib_gzgetc
#define gzgets vtk_zlib_gzgets
#define gzopen vtk_zlib_gzopen
#define gzprintf vtk_zlib_gzprintf
#define gzputc vtk_zlib_gzputc
#define gzputs vtk_zlib_gzputs
#define gzread vtk_zlib_gzread
#define gzrewind vtk_zlib_gzrewind
#define gzseek vtk_zlib_gzseek
#define gzsetparams vtk_zlib_gzsetparams
#define gztell vtk_zlib_gztell
#define gzungetc vtk_zlib_gzungetc
#define gzwrite vtk_zlib_gzwrite
#define inflate_fast vtk_zlib_inflate_fast
#define inflate vtk_zlib_inflate
#define inflateCopy vtk_zlib_inflateCopy
#define inflateEnd vtk_zlib_inflateEnd
#define inflateGetHeader vtk_zlib_inflateGetHeader
#define inflateInit2_ vtk_zlib_inflateInit2_
#define inflateInit_ vtk_zlib_inflateInit_
#define inflatePrime vtk_zlib_inflatePrime
#define inflateReset vtk_zlib_inflateReset
#define inflateSetDictionary vtk_zlib_inflateSetDictionary
#define inflateSync vtk_zlib_inflateSync
#define inflateSyncPoint vtk_zlib_inflateSyncPoint
#define inflate_copyright vtk_zlib_inflate_copyright
#define inflate_table vtk_zlib_inflate_table
#define _dist_code vtk_zlib__dist_code
#define _length_code vtk_zlib__length_code
#define _tr_align vtk_zlib__tr_align
#define _tr_flush_block vtk_zlib__tr_flush_block
#define _tr_init vtk_zlib__tr_init
#define _tr_stored_block vtk_zlib__tr_stored_block
#define _tr_tally vtk_zlib__tr_tally
#define uncompress vtk_zlib_uncompress
#define zError vtk_zlib_zError
#define z_errmsg vtk_zlib_z_errmsg
#define zcalloc vtk_zlib_zcalloc
#define zcfree vtk_zlib_zcfree
#define zlibCompileFlags vtk_zlib_zlibCompileFlags
#define zlibVersion vtk_zlib_zlibVersion

#endif
