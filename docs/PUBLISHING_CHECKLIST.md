# Publishing Checklist

## Repository hygiene

- [ ] `.gitignore` excludes build and IDE artifacts
- [ ] `README.md` explains scope, build, outputs, and protocol variants
- [ ] `docs/` contains quickstart, API references, wire format, and release metadata
- [ ] `wiki/` pages are ready to copy into the GitHub wiki repository
- [ ] remove accidental local artifacts before first commit if they were already tracked

## GitHub setup

- [ ] repository description set from `docs/GITHUB_METADATA.md`
- [ ] repository topics set from `docs/GITHUB_METADATA.md`
- [ ] wiki enabled and seeded from `wiki/`
- [ ] homepage URL set if project website or docs page exists
- [ ] issue tracker/discussions enabled only if you plan to maintain them

## Legal and release readiness

- [ ] add a `LICENSE` file before public release
- [ ] confirm whether bundled documentation should stay in Slovak, English, or both
- [ ] decide semantic version for first public tag
- [ ] verify export surface of shared libraries is acceptable for public consumers

## Build and test gate

- [ ] `cmake -S . -B build_clean`
- [ ] `cmake --build build_clean --config Release`
- [ ] `ctest --test-dir build_clean -C Release --output-on-failure`

## Release bundle

- [ ] include sender DLL and import library
- [ ] include receiver DLL and import library
- [ ] include legacy DLL and import library if you want compatibility distribution
- [ ] include public headers
- [ ] include changelog or release notes for the first public version
