/* Test to determine whether Theora supports 4:4:4 Chroma-subsampling */
#include <theora/theora.h>
#include <theora/codec.h>
#include <theora/theoraenc.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  th_info thInfo;
  th_info_init(&thInfo);
  thInfo.frame_width = 256;
  thInfo.frame_height = 256;
  thInfo.pic_width = 256;
  thInfo.pic_height = 256;
  thInfo.pic_x = 0;
  thInfo.pic_y = 0;
  thInfo.colorspace = TH_CS_ITU_REC_470BG;
  thInfo.target_bitrate = 0;
  thInfo.quality = 63;
  thInfo.keyframe_granule_shift = 6;
  thInfo.fps_numerator = 1;
  thInfo.fps_denominator = 1;
  thInfo.aspect_numerator = 1;
  thInfo.aspect_denominator = 1;
  thInfo.pixel_fmt = TH_PF_444;
  th_enc_ctx *ctx = th_encode_alloc(&thInfo);
  if(ctx) {
    return EXIT_SUCCESS;
  } else {
    return EXIT_FAILURE;
  }
}
