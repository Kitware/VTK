## ImageData temporal reading cache fix

ImageData temporal cache had a cache invalidation issue when switching between time steps. The cache is now properly cleared when time step change.
