extern "C" {
#ifdef HAS_OLD_HEADER
# include <ffmpeg/avformat.h>
#else
# include <libavformat/avformat.h>
#endif
}

int main()
{
  avformat_alloc_context();
  return 0;
}

