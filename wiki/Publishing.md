# Publishing

Use the repository files below as the publish bundle:

- `README.md` for the landing page
- `docs/GITHUB_METADATA.md` for description and topics
- `docs/PUBLISHING_CHECKLIST.md` for pre-release review
- `docs/DEPLOYMENT_CHECKLIST.md` for runtime deployment validation
- `docs/ASYNC_SPLIT_API.md`, `docs/LEGACY_DLL_API.md`, and `docs/SPECIFICATION.md` for the long-form technical reference

## Before making the repository public

1. Add a license file.
2. Run a clean Release build and tests.
3. Confirm the exported shared-library API is the one you want to support publicly.
4. Copy the contents of the `wiki/` directory into the GitHub wiki repository if wiki is enabled.
