# Publishing Checklist

## Repository hygiene

- [ ] `.gitignore` excludes build and IDE artifacts
- [ ] `README.md` explains scope, layout, build, outputs, and protocol variants
- [ ] `docs/` contains quickstart, API references, wire format, and release metadata
- [ ] `wiki/` pages are ready to copy into the GitHub wiki repository
- [ ] root directory contains only top-level repository assets, not stray specs or source files

## GitHub setup

- [ ] repository description set from `docs/GITHUB_METADATA.md`
- [ ] repository topics set from `docs/GITHUB_METADATA.md`
- [ ] wiki enabled and seeded from `wiki/`
- [ ] homepage URL set if project website or docs page exists
- [ ] issue tracker and discussions enabled only if you plan to maintain them

## Legal and release readiness

- [ ] add a `LICENSE` file before public release
- [ ] decide whether public documentation should stay English-only or bilingual
- [ ] decide semantic version for the first public tag
- [ ] verify exported shared-library API is acceptable for public consumers

## Build and test gate

- [ ] `cmake -S . -B build_clean`
- [ ] `cmake --build build_clean --config Release`
- [ ] `ctest --test-dir build_clean -C Release --output-on-failure`

## Release bundle

- [ ] include sender DLL and import library
- [ ] include receiver DLL and import library
- [ ] include legacy DLL and import library if compatibility distribution is intended
- [ ] include public headers from `include/`
- [ ] include changelog or release notes for the first public version
- [ ] prepare Linux source or binary bundle if Linux consumers are expected
