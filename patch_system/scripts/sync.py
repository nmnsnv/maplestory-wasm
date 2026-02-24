#!/usr/bin/env python3
import json
import shutil
import subprocess
import sys
from pathlib import Path


def run_git(cmd, cwd, check=True):
    print(f"Running: {' '.join(cmd)} in {cwd}")
    result = subprocess.run(
        cmd,
        cwd=cwd,
        check=check,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    return result


def ensure_repo(name, url, target_dir, root_dir):
    if not target_dir.exists():
        target_dir.parent.mkdir(parents=True, exist_ok=True)
        print(f"[{name}] Cloning {url} into {target_dir}...")
        run_git(["git", "clone", url, str(target_dir)], cwd=root_dir)
        return

    if not (target_dir / ".git").exists():
        print(f"[{name}] Directory exists but is not a git repository. Recreating...")
        shutil.rmtree(target_dir)
        target_dir.parent.mkdir(parents=True, exist_ok=True)
        run_git(["git", "clone", url, str(target_dir)], cwd=root_dir)
        return

    remote_url = run_git(["git", "remote", "get-url", "origin"], cwd=target_dir).stdout.strip()
    if remote_url != url:
        print(f"[{name}] Updating origin URL from {remote_url} to {url}...")
        run_git(["git", "remote", "set-url", "origin", url], cwd=target_dir)


def resolve_target_commit(name, target_dir, config):
    branch = config.get("branch")
    rev = config.get("rev")

    if branch:
        remote_ref = f"origin/{branch}"
        print(f"[{name}] Resolving latest commit from {remote_ref}...")
        result = run_git(["git", "rev-parse", remote_ref], cwd=target_dir)
        return result.stdout.strip(), f"branch {branch}"

    if rev:
        print(f"[{name}] Using legacy pinned revision {rev} (deprecated; prefer 'branch').")
        return rev, f"rev {rev}"

    raise ValueError(f"[{name}] Missing required key: 'branch' (or legacy 'rev').")


def sync_repo(name, config, checkout_root):
    url = config.get("url")
    rel_path = config.get("path")
    if not url or not rel_path:
        raise ValueError(f"[{name}] Each repo entry requires 'url' and 'path'.")

    target_dir = checkout_root / rel_path
    print(f"[{name}] Syncing {target_dir}...")

    ensure_repo(name, url, target_dir, checkout_root)

    print(f"[{name}] Fetching from origin...")
    run_git(["git", "fetch", "--prune", "origin"], cwd=target_dir)

    commit, source_desc = resolve_target_commit(name, target_dir, config)

    print(f"[{name}] Checking out {commit} ({source_desc})...")
    run_git(["git", "checkout", "--detach", commit], cwd=target_dir)

    print(f"[{name}] Cleaning working tree...")
    run_git(["git", "reset", "--hard", "HEAD"], cwd=target_dir)
    run_git(["git", "clean", "-fdx"], cwd=target_dir)

    short_commit = run_git(["git", "rev-parse", "--short", "HEAD"], cwd=target_dir).stdout.strip()
    print(f"[{name}] Synced to {short_commit}.")


def sync_submodules(checkout_root):
    gitmodules = checkout_root / ".gitmodules"
    if not gitmodules.exists():
        print("No .gitmodules found. Skipping submodule sync.")
        return

    print("[submodule] Syncing all configured submodules...")
    run_git(["git", "submodule", "sync", "--recursive"], cwd=checkout_root)
    run_git(
        [
            "git",
            "submodule",
            "update",
            "--init",
            "--recursive",
            "--force",
        ],
        cwd=checkout_root,
    )


def main():
    script_dir = Path(__file__).parent.resolve()
    system_root = script_dir.parent
    lock_file = system_root / "deps.lock.json"

    if not lock_file.exists():
        print(f"Error: {lock_file} not found.")
        sys.exit(1)

    with lock_file.open("r", encoding="utf-8") as f:
        data = json.load(f)

    repos = data.get("repos", {})

    checkout_root = system_root.parent
    for name, config in repos.items():
        try:
            sync_repo(name, config, checkout_root)
        except Exception as exc:
            print(f"Failed to sync {name}: {exc}")
            sys.exit(1)

    try:
        sync_submodules(checkout_root)
    except Exception as exc:
        print(f"Failed to sync submodules: {exc}")
        sys.exit(1)

    print("Sync completed successfully.")


if __name__ == "__main__":
    main()
