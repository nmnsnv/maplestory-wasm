#!/usr/bin/env python3
import json
import os
import subprocess
import sys
import shutil
import hashlib
import time
from pathlib import Path

def update_patches_generic(system_root):
    lock_file = system_root / 'deps.lock.json'
    if not lock_file.exists():
        print("deps.lock.json missing.")
        return
        
    with open(lock_file, 'r') as f:
        data = json.load(f)
        
    repos = data.get('repos', {})
    checkout_root = system_root.parent
    
    for repo_key, config in repos.items():
        patches_dir = system_root / 'patches' / repo_key
        target_dir = checkout_root / config['path']
        
        if not target_dir.exists():
            continue
            
        # Capture changed files
        try:
            subprocess.run(['git', 'add', '-A'], cwd=target_dir, check=True)
            status = subprocess.check_output(
                ['git', 'diff', 'HEAD', '--name-only', '--staged'],
                cwd=target_dir, text=True
            ).strip()
            
            if not status:
                subprocess.run(['git', 'reset', 'HEAD'], cwd=target_dir)
                continue
                
            changed_files = status.splitlines()
            
            # Prepare dir
            if patches_dir.exists():
                 # We prefer to keep existing patches if they are not touched?
                 # User said "update patch files". implies refresh.
                 # Safest to clean and re-gen to avoid stale "Constants.h.patch" if Constants.h was reverted.
                 shutil.rmtree(patches_dir)
            patches_dir.mkdir(parents=True)
            
            print(f"[{repo_key}] Updating {len(changed_files)} patches...")
            
            for fpath in changed_files:
                diff = subprocess.check_output(
                    ['git', 'diff', 'HEAD', '--binary', '--staged', '--', fpath],
                    cwd=target_dir
                )
                
                # Name: just the filename? or path? 
                # "know to put files in the client/ dir by the filename"
                # If we have src/client/subdir/File.cpp
                # Patch name: subdir_File.cpp.patch? Or just File.cpp.patch?
                # Using full relative path (safe name) allows uniqueness.
                safe_name = fpath.replace('/', '_').replace('\\', '_') + ".patch"
                
                with open(patches_dir / safe_name, 'wb') as f:
                    f.write(diff)
                    
            subprocess.run(['git', 'reset', 'HEAD'], cwd=target_dir)
            
        except subprocess.CalledProcessError as e:
            print(f"Error {e}")
            subprocess.run(['git', 'reset', 'HEAD'], cwd=target_dir)

if __name__ == "__main__":
    script_dir = Path(__file__).parent.resolve()
    system_root = script_dir.parent
    update_patches_generic(system_root)
