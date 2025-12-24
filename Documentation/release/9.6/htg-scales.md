## HyperTreeGridScales Getters deprecation

Removed const qualifiers on getters, that actually mutate internal state by caching data. Deprecate `GetScales[X/Y/Z]`, replaced by `ComputeScale[X/Y/Z]`.
