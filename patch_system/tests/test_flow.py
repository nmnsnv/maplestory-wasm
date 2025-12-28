#!/usr/bin/env python3
import os
import sys
import shutil
import subprocess
import json
import unittest
from pathlib import Path

# Constants
TEST_DIR = Path("/tmp/patch_system_test")
UPSTREAM_DIR = TEST_DIR / "upstream"
PRODUCT_DIR = TEST_DIR / "product"
PATCH_SYSTEM_DIR = PRODUCT_DIR / "patch_system"
SCRIPTS_DIR = PATCH_SYSTEM_DIR / "scripts"

# Point to actual scripts we implemented
REAL_SCRIPTS_DIR = Path(__file__).parent.parent / "scripts"

def run_cmd(cmd, cwd, check=True):
    subprocess.run(cmd, cwd=cwd, check=check, shell=False)

class TestPatchSystem(unittest.TestCase):
    def setUp(self):
        if TEST_DIR.exists():
            shutil.rmtree(TEST_DIR)
        TEST_DIR.mkdir()
        UPSTREAM_DIR.mkdir()
        PRODUCT_DIR.mkdir()
        
        # Setup Scripts (copy them to test env to simulate real usage)
        shutil.copytree(REAL_SCRIPTS_DIR.parent, PATCH_SYSTEM_DIR)
        
        # Make scripts executable? They are python files, we run with python3
        
        # 1. Create Upstream Repo
        run_cmd(['git', 'init', str(UPSTREAM_DIR)], cwd=TEST_DIR)
        run_cmd(['git', 'config', 'user.email', 'test@example.com'], cwd=UPSTREAM_DIR)
        run_cmd(['git', 'config', 'user.name', 'Tester'], cwd=UPSTREAM_DIR)
        
        # Commit 1
        (UPSTREAM_DIR / "dummy.txt").write_text("v1\n")
        run_cmd(['git', 'add', 'dummy.txt'], cwd=UPSTREAM_DIR)
        run_cmd(['git', 'commit', '-m', 'Initial commit'], cwd=UPSTREAM_DIR)
        self.rev1 = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=UPSTREAM_DIR).decode().strip()
        
        # Commit 2
        (UPSTREAM_DIR / "dummy.txt").write_text("v1\nv2\n")
        run_cmd(['git', 'add', 'dummy.txt'], cwd=UPSTREAM_DIR)
        run_cmd(['git', 'commit', '-m', 'Second commit'], cwd=UPSTREAM_DIR)
        self.rev2 = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=UPSTREAM_DIR).decode().strip()

    def test_workflow(self):
        # 2. Configure deps.lock.json for Rev1
        deps = {
            "repos": {
                "test_repo": {
                    "url": str(UPSTREAM_DIR),
                    "rev": self.rev1,
                    "path": "src/test_repo"
                }
            }
        }
        with open(PATCH_SYSTEM_DIR / "deps.lock.json", 'w') as f:
            json.dump(deps, f)
            
        # 3. sync.py
        print("\n--- Testing sync.py ---")
        subprocess.check_call([sys.executable, str(SCRIPTS_DIR / "sync.py")], cwd=PATCH_SYSTEM_DIR)
        
        target_repo = PRODUCT_DIR / "src" / "test_repo"
        self.assertTrue(target_repo.exists())
        self.assertTrue((target_repo / "dummy.txt").exists())
        self.assertEqual((target_repo / "dummy.txt").read_text(), "v1\n")
        
        # 4. Create a patch
        print("\n--- Creating sample patch ---")
        patches_dir = PATCH_SYSTEM_DIR / "patches" / "test_repo"
        patches_dir.mkdir(parents=True)
        
        patch_content = """diff --git a/new_file.txt b/new_file.txt
new file mode 100644
index 0000000..e69de29
--- /dev/null
+++ b/new_file.txt
@@ -0,0 +1 @@
+patched content
"""
        (patches_dir / "001-test.patch").write_text(patch_content)
        
        # 5. apply_patches.py
        print("\n--- Testing apply_patches.py ---")
        subprocess.check_call([sys.executable, str(SCRIPTS_DIR / "apply_patches.py")], cwd=PATCH_SYSTEM_DIR)
        
        # Verify content
        self.assertTrue((target_repo / "new_file.txt").exists())
        self.assertEqual((target_repo / "new_file.txt").read_text(), "patched content\n")
        
        # Verify patchinfo
        info_file = PATCH_SYSTEM_DIR / "patchinfo" / "test_repo" / "001-test.patchinfo.json"
        self.assertTrue(info_file.exists())
        
        # 6. Check Idempotency
        print("\n--- Testing Idempotency ---")
        # Ensure it doesn't fail or apply twice
        subprocess.check_call([sys.executable, str(SCRIPTS_DIR / "apply_patches.py")], cwd=PATCH_SYSTEM_DIR)
        # Content should be same
        self.assertEqual((target_repo / "new_file.txt").read_text(), "patched content\n")
        
        # 7. update_patches.py
        print("\n--- Testing update_patches.py (clean rebase) ---")
        # We want to move to rev2 (which has "v1\nv2\n")
        # Our patch adds "patched" after "v1".
        # Context: "v1".
        # Upstream: "v1\nv2".
        # If we apply "v1\n+patched", it might conflict if context is strict.
        # git apply -3 handles 3-way.
        # Patch base: v1
        # Target: v1, v2.
        # Result should be v1, patched, v2? Or v1, v2, patched?
        # Let's see.
        
        subprocess.check_call([
            sys.executable, 
            str(SCRIPTS_DIR / "update_patches.py"), 
            "test_repo", 
            self.rev2
        ], cwd=PATCH_SYSTEM_DIR)
        
        # Verify target is at rev2 + patch
        # Current HEAD should be rev2 + 1 commit
        # Content?
        self.assertTrue((target_repo / "new_file.txt").exists())
        print("Patched file exists after update.")
        
        # Check patch file was updated
        # Ideally it is now a diff against rev2.
        
        # 8. Verify the loop
        # deps.lock sync should fail if we didn't update deps.lock?
        # But let's check if apply_patches sees it as 'done'.
        # update_patches wrote patchinfo.
        subprocess.check_call([sys.executable, str(SCRIPTS_DIR / "apply_patches.py")], cwd=PATCH_SYSTEM_DIR)

if __name__ == "__main__":
    unittest.main()
