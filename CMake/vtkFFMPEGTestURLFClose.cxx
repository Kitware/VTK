extern "C" {
#ifdef HAS_OLD_HEADER
# include <ffmpeg/avformat.h>
#else
# include <libavformat/avformat.h>
#endif
}

int main()
{
  AVFormatContext *ctx;
  url_fclose(&ctx->pb);
  return 0;
}
