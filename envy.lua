-- nanoprintf's toolchain manifest, and envy's root marker. The cache lives under
-- build/ with everything else this repo generates, so no build step writes to $HOME.
-- `make clean` spares it; `rm -rf build` does not. `envy cache --shared` or
-- ENVY_CACHE_ROOT opts into a shared cache.

-- @envy schema "1"
-- @envy version "0.2.3"
-- @envy sha256sums "e1941f91fa9ff5412537ae1a6ac94c87710256978253e37a2e327a810e2bc5c5"
-- @envy bin "bin"
-- @envy cache-local "build/envy-cache"
-- @envy deploy "true"
-- @envy root "true"

BUNDLES = {
  ["envy"] = {
    identity = "envy.package-specs@r2",
    source = "https://github.com/envy-package-manager/package-specs.git",
    ref = "4abc43074b424400f7d518ef925f8ab8d4624060",
  },
}

PACKAGES = {
  { spec = "envy.doctest-cpp@r0", bundle = "envy", options = { version = "2.5.3" } },

  { spec = "envy.ruff@r0", bundle = "envy", options = { version = "0.16.0" } },

  { spec = "envy.python@r1", bundle = "envy",
    options = { version = "3.13.14", release = "20260623",
                provide_python = true, provide_python3 = true } },
}
