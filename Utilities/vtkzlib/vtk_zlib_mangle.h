#ifndef vtk_zlib_mangle_h
#define vtk_zlib_mangle_h

/*

This header file mangles all symbols exported from the zlib library.
It is included in all files while building the zlib library.  Due to
namespace pollution, no zlib headers should be included in .h files in
VTK.

The following command was used to obtain the symbol list:

nm libvtkzlib.a |grep " [TR] "

*/

#define deflate_copyright vtk_zlib_deflate_copyright
#define _length_code vtk_zlib__length_code
#define _dist_code vtk_zlib__dist_code
#define _tr_align vtk_zlib__tr_align
#define _tr_flush_block vtk_zlib__tr_flush_block
#define _tr_init vtk_zlib__tr_init
#define _tr_stored_block vtk_zlib__tr_stored_block
#define _tr_tally vtk_zlib__tr_tally
#define adler32 vtk_zlib_adler32
#define compress vtk_zlib_compress
#define compress2 vtk_zlib_compress2
#define crc32 vtk_zlib_crc32
#define deflate vtk_zlib_deflate
#define deflateCopy vtk_zlib_deflateCopy
#define deflateEnd vtk_zlib_deflateEnd
#define deflateInit2_ vtk_zlib_deflateInit2_
#define deflateInit_ vtk_zlib_deflateInit_
#define deflateParams vtk_zlib_deflateParams
#define deflateReset vtk_zlib_deflateReset
#define deflateSetDictionary vtk_zlib_deflateSetDictionary
#define get_crc_table vtk_zlib_get_crc_table
#define gzclose vtk_zlib_gzclose
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
#define gzwrite vtk_zlib_gzwrite
#define inflate vtk_zlib_inflate
#define inflateEnd vtk_zlib_inflateEnd
#define inflateInit2_ vtk_zlib_inflateInit2_
#define inflateInit_ vtk_zlib_inflateInit_
#define inflateReset vtk_zlib_inflateReset
#define inflateSetDictionary vtk_zlib_inflateSetDictionary
#define inflateSync vtk_zlib_inflateSync
#define inflateSyncPoint vtk_zlib_inflateSyncPoint
#define inflate_blocks vtk_zlib_inflate_blocks
#define inflate_blocks_free vtk_zlib_inflate_blocks_free
#define inflate_blocks_new vtk_zlib_inflate_blocks_new
#define inflate_blocks_reset vtk_zlib_inflate_blocks_reset
#define inflate_blocks_sync_point vtk_zlib_inflate_blocks_sync_point
#define inflate_codes vtk_zlib_inflate_codes
#define inflate_codes_free vtk_zlib_inflate_codes_free
#define inflate_codes_new vtk_zlib_inflate_codes_new
#define inflate_copyright vtk_zlib_inflate_copyright
#define inflate_fast vtk_zlib_inflate_fast
#define inflate_flush vtk_zlib_inflate_flush
#define inflate_mask vtk_zlib_inflate_mask
#define inflate_set_dictionary vtk_zlib_inflate_set_dictionary
#define inflate_trees_bits vtk_zlib_inflate_trees_bits
#define inflate_trees_dynamic vtk_zlib_inflate_trees_dynamic
#define inflate_trees_fixed vtk_zlib_inflate_trees_fixed
#define uncompress vtk_zlib_uncompress
#define zError vtk_zlib_zError
#define zcalloc vtk_zlib_zcalloc
#define zcfree vtk_zlib_zcfree
#define zlibVersion vtk_zlib_zlibVersion
#define z_errmsg vtk_zlib_z_errmsg

#endif
