static uint _t2(rev_decode_block, Int, DIMS)(bitstream* stream, int minbits, int maxbits, Int* iblock);

/* public functions -------------------------------------------------------- */

/* decode contiguous integer block */
uint
_t2(zfp_decode_block, Int, DIMS)(zfp_stream* zfp, Int* iblock)
{
  return REVERSIBLE(zfp) ? _t2(rev_decode_block, Int, DIMS)(zfp->stream, zfp->minbits, zfp->maxbits, iblock) : _t2(decode_block, Int, DIMS)(zfp->stream, zfp->minbits, zfp->maxbits, zfp->maxprec, iblock);
}
