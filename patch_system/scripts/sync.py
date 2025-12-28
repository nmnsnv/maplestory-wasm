#!/usr/bin/env python3
import json
import os
import subprocess
import sys
from pathlib import Path

def run_git(cmd, cwd, check=True):
    """Run a git command in the specified directory."""
    print(f"Running: {' '.join(cmd)} in {cwd}")
    try:
        subprocess.run(
            cmd,
            cwd=cwd,
            check=check,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
    except subprocess.CalledProcessError as e:
        print(f"Error running git command: {e.cmd}")
        print(f"Stdout: {e.stdout}")
        print(f"Stderr: {e.stderr}")
        raise

def sync_repo(name, config, root_dir):
    """Sync a single repository."""
    url = config['url']
    rev = config['rev']
    rel_path = config['path']
    
    target_dir = root_dir / rel_path
    
    print(f"[{name}] Syncing to {target_dir}...")
    
    # 1. Ensure directory exists or clone
    if not target_dir.exists():
        target_dir.parent.mkdir(parents=True, exist_ok=True)
        print(f"[{name}] Cloning {url}...")
        run_git(['git', 'clone', url, str(target_dir)], cwd=root_dir)
    elif not (target_dir / '.git').exists():
        print(f"[{name}] Directory exists but not a git repo. Cleaning up...")
        # Safety check: simplistic protection
        import shutil
        shutil.rmtree(target_dir)
        target_dir.parent.mkdir(parents=True, exist_ok=True)
        run_git(['git', 'clone', url, str(target_dir)], cwd=root_dir)
    else:
        # Check if remote matches? Skipping for simplicity, assume correct repo.
        pass

    # 2. Fetch
    print(f"[{name}] Fetching origin...")
    run_git(['git', 'fetch', 'origin'], cwd=target_dir)
    
    # 3. Checkout detached HEAD
    print(f"[{name}] Checking out {rev}...")
    run_git(['git', 'checkout', rev], cwd=target_dir)
    
    # 4. Clean
    print(f"[{name}] Cleaning working tree...")
    run_git(['git', 'reset', '--hard', 'HEAD'], cwd=target_dir)
    run_git(['git', 'clean', '-fdx'], cwd=target_dir)

def main():
    script_dir = Path(__file__).parent.resolve()
    system_root = script_dir.parent
    lock_file = system_root / 'deps.lock.json'
    
    if not lock_file.exists():
        print(f"Error: {lock_file} not found.")
        sys.exit(1)
        
    with open(lock_file, 'r') as f:
        data = json.load(f)
        
    repos = data.get('repos', {})
    if not repos:
        print("No repos found in deps.lock.json")
        return

    # Root for checking out source code. 
    # Current design: checkout relative to 'patch_system/'? 
    # Or should patch_system be standalone in the product root?
    # User said: my-product/ ... patches/ ... 
    # So we probably want to checkout relative to my-product root.
    # system_root is 'patch_system/'. Parent is 'my-product/'.
    # Let's assume we want to checkout in the parent of 'patch_system'.
    
    checkout_root = system_root.parent
    
    for name, config in repos.items():
        try:
            sync_repo(name, config, checkout_root)
        except Exception as e:
            print(f"Failed to sync {name}: {e}")
            sys.exit(1)

    print("Sync completed successfully.")

if __name__ == "__main__":
    main()
