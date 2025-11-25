# this is a python script that monitors the shared NFS folder for new scans
# the code on the Beagley-AI will create a 'done.txt' file in the shared folder when a new scan is ready
# the photos will be imported into COLMAP for processing when the 'done.txt' file is detected 

import os
import time
import subprocess

# Path to the shared NFS folder
WATCH_DIR = "/home/john/ensc351/public/myApps" # Update this to your local path
COLMAP_BIN = "colmap"

def run_colmap():
    print("Scan detected! Starting COLMAP processing...")
    
    # Create a database file
    db_path = os.path.join(WATCH_DIR, "database.db")
    
    # 1. Feature Extraction (CPU Only flags added for VM)
    print("--> Extracting Features...")
    subprocess.run([
        COLMAP_BIN, "feature_extractor",
        "--database_path", db_path,
        "--image_path", WATCH_DIR,
        "--SiftExtraction.use_gpu", "0" 
    ])

    # 2. Feature Matching
    print("--> Matching Features...")
    subprocess.run([
        COLMAP_BIN, "exhaustive_matcher",
        "--database_path", db_path,
        "--SiftMatching.use_gpu", "0"
    ])

    # 3. Sparse Reconstruction (The Point Cloud)
    print("--> Running Sparse Reconstruction...")
    sparse_dir = os.path.join(WATCH_DIR, "sparse")
    os.makedirs(sparse_dir, exist_ok=True)
    
    subprocess.run([
        COLMAP_BIN, "mapper",
        "--database_path", db_path,
        "--image_path", WATCH_DIR,
        "--output_path", sparse_dir
    ])
    
    print(f"Processing complete! Model saved in {sparse_dir}")

print(f"Watching {WATCH_DIR} for 'done.txt'...")

while True:
    done_file = os.path.join(WATCH_DIR, "done.txt")
    
    if os.path.exists(done_file):
        # Wait a moment to ensure file buffers are flushed
        time.sleep(1)
        
        # Run the processing
        run_colmap()
        
        # Remove the flag so we don't run again immediately
        os.remove(done_file)
        print("Ready for next scan.")
        
    time.sleep(2)