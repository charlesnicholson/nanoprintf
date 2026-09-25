-- nanoprintf's toolchain manifest, and envy's root marker. The cache lives under
-- build/ with everything else this repo generates, so no build step writes to $HOME.
-- `make clean` spares it; `rm -rf build` does not. `envy cache --shared` or
-- ENVY_CACHE_ROOT opts into a shared cache.

-- @envy schema "1"
-- @envy version "0.4.10"
-- @envy sha256sums "025057d81b0ec79da6bb4a463f356afeb9ca3f0e07aff2db07e8c92b7de30378"
-- @envy bin "bin"
-- @envy cache-local "build/envy-cache"
-- @envy deploy "true"
-- @envy root "true"

BUNDLES = {
  ["envy"] = {
    identity = "envy.package-specs@r9",
    source = "https://github.com/envy-package-manager/package-specs.git",
    ref = "21ed35ea163297b4cb6041698483b51d8f4d75e0",
  },
}

PACKAGES = {
  { spec = "envy.doctest-cpp@r0", bundle = "envy", options = { version = "2.5.3" } },

  { spec = "envy.ruff@r2", bundle = "envy", options = { version = "0.16.8" } },

  { spec = "envy.python@r3", bundle = "envy",
    options = { version = "3.14.7", release = "20260901",
                provide_python = true, provide_python3 = true } },
}
