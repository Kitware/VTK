# Renew internal ensight point cache to avoid shadowing

Previously, the EnSight 6 readers' point caches were always reallocated in
place. This would interfere with things that might store pointers to certain
data downstream (like the temporal cache).

This fix adds a renewal mechanism to the internal point cache for the
EnSight 6 readers.
