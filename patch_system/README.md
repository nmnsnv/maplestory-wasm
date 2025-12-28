# patch_system

A standalone, deterministic patch queue system inspired by Brave and Linux distro workflows. This system allows you to maintain patches on top of upstream Git repositories without forking them.

## Overview

The system clones specific revisions of upstream repositories and applies a set of patches on top of them. It ensures reproducibility by locking upstream revisions and tracking applied patches via hashes.

### Directory Structure

```
patch_system/
├── deps.lock.json    # Configuration of upstream repos (URL, revision, path)
├── patches/          # Patches organized by repo key
│   └── example_repo/
│       ├── 001-fix-bug.patch
│       └── 002-add-feature.patch
├── patchinfo/        # Metadata for applied patches (auto-generated)
└── scripts/
    ├── sync.py           # Clones/Fetches and checks out pinned revisions
    ├── apply_patches.py  # Applies patches idempotently
    └── update_patches.py # Helper to rebase patches onto new upstream versions
```

## Workflows

### 1. Sync Dependencies
To prepare the workspace, run:
```bash
python3 patch_system/scripts/sync.py
```
This reads `deps.lock.json` and ensures all repositories are cloned and checked out to the specified `rev`. **Warning**: This will clean the working trees of the target repositories.

### 2. Apply Patches (Auto)
To apply patches to all repos:
```bash
python3 patch_system/scripts/apply_patches.py
```
This applies patches directly to the working tree. **It does NOT create commits.**

### 3. Creating/Updating Patches (Auto)
To save your work:
1. Make changes in `src/<repo>`.
2. **Stage** any new files if you want them included (`git add`).
   - Untracked files are ignored (useful if you have random junk).
3. Run:
   ```bash
   python3 patch_system/scripts/update_patches.py
   ```
   This will capture the **entire state** of your changes (diff vs pinned revision) into a single `changes.patch` file per repo, replacing old patches.

### 4. Updating Upstream
1. Update `deps.lock.json` manually with new revision.
2. Run `sync.py`.
3. Run `apply_patches.py`.
   - If conflicts occur, `git apply` will fail and tell you.
4. Resolve issues by editing files.
5. Run `update_patches.py` to save the matched state.

## Comparison to Alternatives

- **Git Submodules**: Bind to a specific commit but don't support patching easily (you need a fork).
- **Forks**: Require maintaining a full copy of the repo. Hard to sync with upstream tags/branches.
- **Quilt**: Similar concept, but this system is tailored for Git-native workflows and integrates dependency management.

## Common Failure Modes

- **Patch Conflict**: If `apply_patches.py` fails, it usually means the patch does not apply cleanly to the pinned revision. Check if the upstream code changed (if you changed the rev) or if the patch is malformed.
- **Idempotency**: If you modify a patch file, `apply_patches.py` will re-apply it (and fail if the code is already changed). You should revert the repo (`sync.py`) before applying modified patches.
