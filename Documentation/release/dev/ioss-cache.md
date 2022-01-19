# IOSS Reader: fix incorrect cache

IOSS reader had a potential to read side sets incorrectly due to a
false hit in the internal cache used by the reader to minimize disk reads.
Fixed that.
