#!/usr/bin/env python3
import os
import shutil
import subprocess
import time
import sys
import unittest

"""
A comprehensive testing script for file_sync.c
Tests various scenarios including:
- Basic synchronization
- Creating non-existent destination directories
- Handling non-existent source directories
- File modification time comparisons
- Invalid command-line arguments
- Empty directories
"""

class FileSyncTest(unittest.TestCase):
    
    @classmethod
    def setUpClass(cls):
        # Compile the program first
        try:
            subprocess.run(["gcc", "-o", "file_sync", "file_sync.c"], check=True)
            print("Successfully compiled file_sync.c")
        except subprocess.CalledProcessError:
            print("Error compiling file_sync.c")
            sys.exit(1)
    
    def setUp(self):
        # Create test directories
        self.test_dir = "file_sync_test"
        self.source_dir = os.path.join(self.test_dir, "source")
        self.dest_dir = os.path.join(self.test_dir, "dest")
        
        # Remove any existing test directories
        if os.path.exists(self.test_dir):
            shutil.rmtree(self.test_dir)
        
        # Create fresh source directory
        os.makedirs(self.source_dir)
    
    def tearDown(self):
        # Clean up test directories
        if os.path.exists(self.test_dir):
            shutil.rmtree(self.test_dir)
    
    def create_test_file(self, path, content, sleep_time=1):
        with open(path, 'w') as f:
            f.write(content)
        time.sleep(sleep_time)  # Ensure different modification times
    
    def run_file_sync(self, src_dir, dest_dir, expected_return_code=0):
        result = subprocess.run(["./file_sync", src_dir, dest_dir], 
                               capture_output=True, text=True)
        if expected_return_code is not None:
            self.assertEqual(result.returncode, expected_return_code, 
                           f"Expected return code {expected_return_code}, got {result.returncode}. Stderr: {result.stderr}")
        return result.stdout, result.stderr, result.returncode
    
    def test_basic_sync(self):
        """Test basic file synchronization."""
        # Create test files
        self.create_test_file(os.path.join(self.source_dir, "file1.txt"), "Test file 1")
        self.create_test_file(os.path.join(self.source_dir, "file2.txt"), "Test file 2")
        
        # Run file sync
        stdout, _, _ = self.run_file_sync(self.source_dir, self.dest_dir)
        
        # Verify files were copied
        self.assertTrue(os.path.exists(os.path.join(self.dest_dir, "file1.txt")))
        self.assertTrue(os.path.exists(os.path.join(self.dest_dir, "file2.txt")))
        
        # Verify output message
        self.assertIn("New file found: file1.txt", stdout)
        self.assertIn("New file found: file2.txt", stdout)
        self.assertIn("Synchronization complete", stdout)
        
        print("Basic synchronization test passed.")
    
    def test_nonexistent_source(self):
        """Test with non-existent source directory."""
        stdout, _, return_code = self.run_file_sync("nonexistent_dir", self.dest_dir, expected_return_code=1)
        
        # Verify error message
        self.assertIn("does not exist", stdout)
        
        print("Non-existent source directory test passed.")
    
    def test_create_destination(self):
        """Test creating a non-existent destination directory."""
        # Create test files
        self.create_test_file(os.path.join(self.source_dir, "file1.txt"), "Test file 1")
        
        # Use a non-existent destination
        new_dest = os.path.join(self.test_dir, "new_dest")
        
        # Run file sync
        stdout, _, _ = self.run_file_sync(self.source_dir, new_dest)
        
        # Verify destination was created
        self.assertTrue(os.path.exists(new_dest))
        self.assertIn(f"Created destination directory '{new_dest}'", stdout)
        
        print("Create destination directory test passed.")
    
    def test_newer_files_update(self):
        """Test that newer files in source replace older files in destination."""
        # Create destination directory
        os.makedirs(self.dest_dir)
        
        # Create older file in destination
        self.create_test_file(os.path.join(self.dest_dir, "file1.txt"), "Old content")
        time.sleep(2)  # Ensure time difference
        
        # Create newer file in source
        self.create_test_file(os.path.join(self.source_dir, "file1.txt"), "New content")
        
        # Run file sync
        stdout, _, _ = self.run_file_sync(self.source_dir, self.dest_dir)
        
        # Verify file was updated
        self.assertIn("is newer in source", stdout)
        with open(os.path.join(self.dest_dir, "file1.txt"), 'r') as f:
            self.assertEqual(f.read(), "New content")
        
        print("Newer file update test passed.")
    
    def test_older_files_skip(self):
        """Test that older files in source don't replace newer files in destination."""
        # Create destination directory
        os.makedirs(self.dest_dir)
        
        # Create newer file in destination
        self.create_test_file(os.path.join(self.source_dir, "file1.txt"), "Old content")
        time.sleep(2)  # Ensure time difference
        
        # Create newer file in destination
        self.create_test_file(os.path.join(self.dest_dir, "file1.txt"), "Newer content")
        
        # Run file sync
        stdout, _, _ = self.run_file_sync(self.source_dir, self.dest_dir)
        
        # Verify file was not updated
        self.assertIn("is newer in destination", stdout)
        with open(os.path.join(self.dest_dir, "file1.txt"), 'r') as f:
            self.assertEqual(f.read(), "Newer content")
        
        print("Older file skip test passed.")
    
    def test_identical_files(self):
        """Test that identical files are properly detected and skipped."""
        # Create destination directory
        os.makedirs(self.dest_dir)
        
        # Create identical files
        self.create_test_file(os.path.join(self.source_dir, "file1.txt"), "Same content")
        self.create_test_file(os.path.join(self.dest_dir, "file1.txt"), "Same content")
        
        # Run file sync
        stdout, _, _ = self.run_file_sync(self.source_dir, self.dest_dir)
        
        # Verify file was identified as identical
        self.assertIn("is identical", stdout)
        
        print("Identical files test passed.")
    
    def test_empty_source(self):
        """Test synchronization with an empty source directory."""
        # Source directory is already empty from setUp
        
        # Run file sync
        stdout, _, _ = self.run_file_sync(self.source_dir, self.dest_dir)
        
        # Verify synchronization completes without errors
        self.assertIn("Synchronization complete", stdout)
        
        print("Empty source directory test passed.")
    
    def test_invalid_args(self):
        """Test with invalid number of arguments."""
        # Test with only one argument
        result = subprocess.run(["./file_sync", self.source_dir], 
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 1)
        self.assertIn("Usage:", result.stdout)
        
        # Test with three arguments
        result = subprocess.run(["./file_sync", self.source_dir, self.dest_dir, "extra_arg"], 
                               capture_output=True, text=True)
        self.assertEqual(result.returncode, 1)
        self.assertIn("Usage:", result.stdout)
        
        print("Invalid arguments test passed.")
    
    def test_alphabetical_order(self):
        """Test that files are processed in alphabetical order."""
        # Create files in non-alphabetical order
        self.create_test_file(os.path.join(self.source_dir, "c_file.txt"), "C content")
        self.create_test_file(os.path.join(self.source_dir, "a_file.txt"), "A content")
        self.create_test_file(os.path.join(self.source_dir, "b_file.txt"), "B content")
        
        # Run file sync
        stdout, _, _ = self.run_file_sync(self.source_dir, self.dest_dir)
        
        # Check order of processing in output
        a_pos = stdout.find("New file found: a_file.txt")
        b_pos = stdout.find("New file found: b_file.txt")
        c_pos = stdout.find("New file found: c_file.txt")
        
        self.assertTrue(a_pos < b_pos < c_pos, "Files were not processed in alphabetical order")
        
        print("Alphabetical order test passed.")

class CustomTestResult(unittest.TextTestResult):
    def __init__(self, stream, descriptions, verbosity):
        super().__init__(stream, descriptions, verbosity)
        self.tests_run = 0
        self.tests_failed = 0
    
    def startTest(self, test):
        super().startTest(test)
        self.tests_run += 1
    
    def addFailure(self, test, err):
        super().addFailure(test, err)
        self.tests_failed += 1
    
    def addError(self, test, err):
        super().addError(test, err)
        self.tests_failed += 1

if __name__ == "__main__":
    print("Starting file_sync tests...")
    runner = unittest.TextTestRunner(verbosity=2, resultclass=CustomTestResult)
    result = runner.run(unittest.makeSuite(FileSyncTest))
    
    if result.tests_failed == 0 and result.tests_run > 0:
        print("\n✓ SUCCESS! All tests passed!")
    else:
        print(f"\nTests failed: {result.tests_failed}/{result.tests_run}")
