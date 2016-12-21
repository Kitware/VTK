#ifndef vtklz4_mangle_h
#define vtklz4_mangle_h

/*

This header file mangles all symbols exported from the lz4 library.
It is included in all files while building the lz4 library.  Due to
namespace pollution, no lz4 headers should be included in .h files in
VTK.

The following command was used to obtain the symbol list:

nm liblz4.a  | grep " [TR] "
*/

#define LZ4_compress                              vtk_LZ4_compress
#define LZ4_compressBound                         vtk_LZ4_compressBound
#define LZ4_compress_continue                     vtk_LZ4_compress_continue
#define LZ4_compress_default                      vtk_LZ4_compress_default
#define LZ4_compress_destSize                     vtk_LZ4_compress_destSize
#define LZ4_compress_fast                         vtk_LZ4_compress_fast
#define LZ4_compress_fast_continue                vtk_LZ4_compress_fast_continue
#define LZ4_compress_fast_extState                vtk_LZ4_compress_fast_extState
#define LZ4_compress_fast_force                   vtk_LZ4_compress_fast_force
#define LZ4_compress_forceExtDict                 vtk_LZ4_compress_forceExtDict
#define LZ4_compress_limitedOutput                vtk_LZ4_compress_limitedOutput
#define LZ4_compress_limitedOutput_continue       vtk_LZ4_compress_limitedOutput_continue
#define LZ4_compress_limitedOutput_withState      vtk_LZ4_compress_limitedOutput_withState
#define LZ4_compress_withState                    vtk_LZ4_compress_withState
#define LZ4_create                                vtk_LZ4_create
#define LZ4_createStream                          vtk_LZ4_createStream
#define LZ4_createStreamDecode                    vtk_LZ4_createStreamDecode
#define LZ4_decompress_fast                       vtk_LZ4_decompress_fast
#define LZ4_decompress_fast_continue              vtk_LZ4_decompress_fast_continue
#define LZ4_decompress_fast_usingDict             vtk_LZ4_decompress_fast_usingDict
#define LZ4_decompress_fast_withPrefix64k         vtk_LZ4_decompress_fast_withPrefix64k
#define LZ4_decompress_safe                       vtk_LZ4_decompress_safe
#define LZ4_decompress_safe_continue              vtk_LZ4_decompress_safe_continue
#define LZ4_decompress_safe_forceExtDict          vtk_LZ4_decompress_safe_forceExtDict
#define LZ4_decompress_safe_partial               vtk_LZ4_decompress_safe_partial
#define LZ4_decompress_safe_usingDict             vtk_LZ4_decompress_safe_usingDict
#define LZ4_decompress_safe_withPrefix64k         vtk_LZ4_decompress_safe_withPrefix64k
#define LZ4_freeStream                            vtk_LZ4_freeStream
#define LZ4_freeStreamDecode                      vtk_LZ4_freeStreamDecode
#define LZ4_loadDict                              vtk_LZ4_loadDict
#define LZ4_resetStream                           vtk_LZ4_resetStream
#define LZ4_resetStreamState                      vtk_LZ4_resetStreamState
#define LZ4_saveDict                              vtk_LZ4_saveDict
#define LZ4_setStreamDecode                       vtk_LZ4_setStreamDecode
#define LZ4_sizeofState                           vtk_LZ4_sizeofState
#define LZ4_sizeofStreamState                     vtk_LZ4_sizeofStreamState
#define LZ4_slideInputBuffer                      vtk_LZ4_slideInputBuffer
#define LZ4_uncompress                            vtk_LZ4_uncompress
#define LZ4_uncompress_unknownOutputSize          vtk_LZ4_uncompress_unknownOutputSize
#define LZ4_versionNumber                         vtk_LZ4_versionNumber
#define LZ4_compressHC                            vtk_LZ4_compressHC
#define LZ4_compress_HC                           vtk_LZ4_compress_HC
#define LZ4_compressHC2                           vtk_LZ4_compressHC2
#define LZ4_compressHC2_continue                  vtk_LZ4_compressHC2_continue
#define LZ4_compressHC2_limitedOutput             vtk_LZ4_compressHC2_limitedOutput
#define LZ4_compressHC2_limitedOutput_continue    vtk_LZ4_compressHC2_limitedOutput_continue
#define LZ4_compressHC2_limitedOutput_withStateHC vtk_LZ4_compressHC2_limitedOutput_withStateHC
#define LZ4_compressHC2_withStateHC               vtk_LZ4_compressHC2_withStateHC
#define LZ4_compress_HC_continue                  vtk_LZ4_compress_HC_continue
#define LZ4_compressHC_continue                   vtk_LZ4_compressHC_continue
#define LZ4_compress_HC_extStateHC                vtk_LZ4_compress_HC_extStateHC
#define LZ4_compressHC_limitedOutput              vtk_LZ4_compressHC_limitedOutput
#define LZ4_compressHC_limitedOutput_continue     vtk_LZ4_compressHC_limitedOutput_continue
#define LZ4_compressHC_limitedOutput_withStateHC  vtk_LZ4_compressHC_limitedOutput_withStateHC
#define LZ4_compressHC_withStateHC                vtk_LZ4_compressHC_withStateHC
#define LZ4_createHC                              vtk_LZ4_createHC
#define LZ4_createStreamHC                        vtk_LZ4_createStreamHC
#define LZ4_freeHC                                vtk_LZ4_freeHC
#define LZ4_freeStreamHC                          vtk_LZ4_freeStreamHC
#define LZ4_loadDictHC                            vtk_LZ4_loadDictHC
#define LZ4_resetStreamHC                         vtk_LZ4_resetStreamHC
#define LZ4_resetStreamStateHC                    vtk_LZ4_resetStreamStateHC
#define LZ4_saveDictHC                            vtk_LZ4_saveDictHC
#define LZ4_sizeofStateHC                         vtk_LZ4_sizeofStateHC
#define LZ4_sizeofStreamStateHC                   vtk_LZ4_sizeofStreamStateHC
#define LZ4_slideInputBufferHC                    vtk_LZ4_slideInputBufferHC
#define LZ4F_compressBegin                        vtk_LZ4F_compressBegin
#define LZ4F_compressBound                        vtk_LZ4F_compressBound
#define LZ4F_compressEnd                          vtk_LZ4F_compressEnd
#define LZ4F_compressFrame                        vtk_LZ4F_compressFrame
#define LZ4F_compressFrameBound                   vtk_LZ4F_compressFrameBound
#define LZ4F_compressUpdate                       vtk_LZ4F_compressUpdate
#define LZ4F_createCompressionContext             vtk_LZ4F_createCompressionContext
#define LZ4F_createDecompressionContext           vtk_LZ4F_createDecompressionContext
#define LZ4F_decompress                           vtk_LZ4F_decompress
#define LZ4F_flush                                vtk_LZ4F_flush
#define LZ4F_freeCompressionContext               vtk_LZ4F_freeCompressionContext
#define LZ4F_freeDecompressionContext             vtk_LZ4F_freeDecompressionContext
#define LZ4F_getErrorName                         vtk_LZ4F_getErrorName
#define LZ4F_getFrameInfo                         vtk_LZ4F_getFrameInfo
#define LZ4F_isError                              vtk_LZ4F_isError
#define XXH32                                     vtk_XXH32
#define XXH32_createState                         vtk_XXH32_createState
#define XXH32_digest                              vtk_XXH32_digest
#define XXH32_freeState                           vtk_XXH32_freeState
#define XXH32_reset                               vtk_XXH32_reset
#define XXH32_update                              vtk_XXH32_update
#define XXH64                                     vtk_XXH64
#define XXH64_createState                         vtk_XXH64_createState
#define XXH64_digest                              vtk_XXH64_digest
#define XXH64_freeState                           vtk_XXH64_freeState
#define XXH64_reset                               vtk_XXH64_reset
#define XXH64_update                              vtk_XXH64_update

#endif
