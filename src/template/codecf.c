/* maximum number of bit planes to encode */
static uint
precision(int maxexp, uint maxprec, int minexp, int dims)
{
  return MIN(maxprec, (uint)MAX(0, maxexp - minexp + 2 * (dims + 1)));
}
