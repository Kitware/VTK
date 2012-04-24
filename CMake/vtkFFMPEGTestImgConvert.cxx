extern "C" {
#ifdef HAS_OLD_HEADER
# include <ffmpeg/avcodec.h>
#else
# include <libavcodec/avcodec.h>
#endif
}

int main()
{
  img_convert(0, PIX_FMT_RGB24,
              0, PIX_FMT_RGB24,
              0, 0);
  return 0;
}
