#!/usr/bin/env python3
import json
import os
import subprocess
import sys
import hashlib
import time
from pathlib import Path

def calculate_file_hash(filepath):
    sha = hashlib.sha256()
    with open(filepath, 'rb') as f:
        while True:
            data = f.read(65536)
            if not data:
                break
            sha.update(data)
    return sha.hexdigest()

def reset_repo(target_dir, repo_key):
    """Reset a git repo to a clean state, discarding all local changes."""
    print(f"[{repo_key}] Resetting {target_dir} to clean state...")
    
    # Hard reset to HEAD - discards staged and unstaged changes to tracked files
    result = subprocess.run(
        ['git', 'reset', '--hard', 'HEAD'],
        cwd=target_dir,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    if result.returncode != 0:
        print(f"[{repo_key}] Warning: git reset failed: {result.stderr.decode()}")
    
    # Clean untracked files and directories
    result = subprocess.run(
        ['git', 'clean', '-fd'],
        cwd=target_dir,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    if result.returncode != 0:
        print(f"[{repo_key}] Warning: git clean failed: {result.stderr.decode()}")
    
    return True

def apply_patches_generic(system_root):
    # Load configuration to map patch-groups to repo paths
    lock_file = system_root / 'deps.lock.json'
    if not lock_file.exists():
        print("deps.lock.json not found.")
        return False
        
    with open(lock_file, 'r') as f:
        data = json.load(f)
    
    # Map 'patch_group_name' -> 'target_path'
    # The user says "put files in the client/ dir by the filename".
    # Structure: patch_system/patches/<repo_key>/<file>.patch
    # <repo_key> in deps.lock.json corresponds to a path.
    
    # So we simply iterate over keys in deps.lock.json.
    # If patches/<key> exists, we apply them to the mapped path.
    # This IS generic as long as deps.lock.json defines the mapping.
    
    repos = data.get('repos', {})
    checkout_root = system_root.parent

    for repo_key, config in repos.items():
        patches_dir = system_root / 'patches' / repo_key
        # patchinfo_dir = system_root / 'patchinfo' / repo_key
        target_dir = checkout_root / config['path']
        
        if not patches_dir.exists():
            continue
            
        if not target_dir.exists():
            print(f"[{repo_key}] Target directory {target_dir} missing. Run sync.py?")
            continue
            
        patches = sorted([p for p in patches_dir.glob('*.patch')])
        if not patches:
            continue

        # Reset repo to clean state before applying patches
        reset_repo(target_dir, repo_key)
        
        print(f"[{repo_key}] Applying patches to {target_dir}...")
        # patchinfo_dir.mkdir(parents=True, exist_ok=True)

        for patch_file in patches:
            patch_name = patch_file.name
            
            # Check if this patch file contains a clear target file path?
            # Standard git apply uses the patch header.
            # a/Path/To/File b/Path/To/File
            # We are running git apply inside 'target_dir'.
            # So if patches/client/Constants.h.patch contains "a/Constants.h", it works.
            
            try:
                # Try simple check first
                subprocess.run(
                   ['git', 'apply', '--3way', '--whitespace=nowarn', str(patch_file)],
                   cwd=target_dir,
                   check=True,
                   stdout=subprocess.PIPE,
                   stderr=subprocess.PIPE
                )
                
                # # Success
                # json.dump({
                #     "hash": calculate_file_hash(patch_file),
                #     "time": time.time()
                # }, open(patchinfo_dir / (patch_name + ".info.json"), 'w'), indent=2)
                
            except subprocess.CalledProcessError as e:
                 # Check if already applied (reverse check)
                rev = subprocess.run(
                    ['git', 'apply', '--check', '--reverse', str(patch_file)],
                    cwd=target_dir,
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE
                )
                if rev.returncode == 0:
                     print(f"  [SKIP] {patch_name} (Already applied)")
                else:
                     print(f"  [ERROR] Failed {patch_name}")
                     # Decode bytes if possible, or print as is
                     err_msg = e.stderr.decode('utf-8') if e.stderr else "No stderr"
                     print(f"Git Error: {err_msg}")
                     return False
        
        # Explicitly reset index to ensure no files are staged/tracked by the patch application
        # This satisfies the user requirement: "don't want you to add (track) files"
        subprocess.run(['git', 'reset', 'HEAD'], cwd=target_dir, check=False)
                     
    return True

if __name__ == "__main__":
    script_dir = Path(__file__).parent.resolve()
    system_root = script_dir.parent
    if not apply_patches_generic(system_root):
        sys.exit(1)
    print("Done.")
