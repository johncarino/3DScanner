import os
import time
import subprocess

WATCH_DIR = "/home/john/ensc351/public/myApps"  # Update this to your local path
COLMAP_BIN = "colmap"

def run_full_pipeline():
    print("=" * 60)
    print("Scan detected! Starting full COLMAP pipeline...")
    print("=" * 60)
    
    # Setup paths
    db_path = os.path.join(WATCH_DIR, "database.db")
    sparse_dir = os.path.join(WATCH_DIR, "sparse")
    dense_path = os.path.join(WATCH_DIR, "dense")
    
    # Clean up old reconstruction data
    import shutil
    if os.path.exists(db_path):
        os.remove(db_path)
        print(f"Removed old database: {db_path}")
    if os.path.exists(sparse_dir):
        shutil.rmtree(sparse_dir)
        print(f"Removed old sparse folder: {sparse_dir}")
    if os.path.exists(dense_path):
        shutil.rmtree(dense_path)
        print(f"Removed old dense folder: {dense_path}")
    
    os.makedirs(sparse_dir, exist_ok=True)
    
    #PHASE 1: SPARSE RECONSTRUCTION
    print("\n### PHASE 1: SPARSE RECONSTRUCTION ###\n")
    
    # 1. Feature Extraction (GPU enabled)
    print("--> 1. Extracting Features...")
    subprocess.run([
        COLMAP_BIN, "feature_extractor",
        "--database_path", db_path,
        "--image_path", WATCH_DIR,
        "--SiftExtraction.use_gpu", "1" 
    ])

    # 2. Feature Matching
    print("--> 2. Matching Features...")
    subprocess.run([
        COLMAP_BIN, "exhaustive_matcher",
        "--database_path", db_path,
        "--SiftMatching.use_gpu", "1"
    ])

    # 3. Sparse Reconstruction (The Point Cloud)
    print("--> 3. Running Sparse Reconstruction...")
    subprocess.run([
        COLMAP_BIN, "mapper",
        "--database_path", db_path,
        "--image_path", WATCH_DIR,
        "--output_path", sparse_dir
    ])
    
    print(f"\n✓ Sparse reconstruction complete! Model saved in {sparse_dir}")
    
    #PHASE 2: DENSE RECONSTRUCTION & MESHING 
    print("\n### PHASE 2: DENSE RECONSTRUCTION & MESHING ###\n")
    
    # Auto-detect the correct sparse path
    path_check = os.path.join(sparse_dir, "0")
    if os.path.exists(path_check) and os.path.exists(os.path.join(path_check, "cameras.bin")):
        sparse_path = path_check
    else:
        sparse_path = sparse_dir
    
    print(f"Using Sparse Path: {sparse_path}")
    os.makedirs(dense_path, exist_ok=True)

    # 4. Image Undistortion
    print("--> 4. Undistorting Images...")
    subprocess.run([
        COLMAP_BIN, "image_undistorter",
        "--image_path", WATCH_DIR,
        "--input_path", sparse_path,
        "--output_path", dense_path,
        "--output_type", "COLMAP",
        "--max_image_size", "2000"
    ])

    # 5. Dense Stereo Reconstruction
    print("--> 5. Dense Stereo Reconstruction...")
    print("    (This may take a while depending on image count...)")
    
    result = subprocess.run([
        COLMAP_BIN, "patch_match_stereo",
        "--workspace_path", dense_path,
        "--workspace_format", "COLMAP",
        "--PatchMatchStereo.gpu_index=0"  # Use first GPU
    ])
    
    if result.returncode != 0:
        print("ERROR: Dense reconstruction failed. Check the logs above.")
        return

    # 6. Stereo Fusion
    print("--> 6. Stereo Fusion...")
    subprocess.run([
        COLMAP_BIN, "stereo_fusion",
        "--workspace_path", dense_path,
        "--workspace_format", "COLMAP",
        "--input_type", "geometric",
        "--output_path", os.path.join(dense_path, "fused.ply")
    ])

    # 7. Poisson Meshing
    print("--> 7. Poisson Meshing...")
    fused_file = os.path.join(dense_path, "fused.ply")
    meshed_file = os.path.join(dense_path, "meshed.ply")
    
    if os.path.exists(fused_file) and os.path.getsize(fused_file) > 1000:
        subprocess.run([
            COLMAP_BIN, "poisson_mesher",
            "--input_path", fused_file,
            "--output_path", meshed_file,
            "--PoissonMeshing.trim", "0"
        ])
        
        print("\n" + "=" * 60)
        print("✓ COMPLETE! Your 3D model:")
        print(f"  Mesh: {meshed_file}")
        print("\nTo add texture, use external tools like:")
        print("  - Blender: Import mesh, UV unwrap, bake textures from images")
        print("  - MeshLab: Texture projection from images")
        print("  - CloudCompare: Color from images")
        print("=" * 60 + "\n")
    else:
        print("\n" + "=" * 60)
        print("⚠ WARNING: fused.ply is empty or missing.")
        print("Dense reconstruction may have failed to produce points.")
        print("=" * 60 + "\n")

# Watch remote drive for the "done.txt" file that the C program spits out
print("=" * 60)
print(f"Watching {WATCH_DIR} for new scans...")
print("Waiting for 'done.txt' to trigger processing...")
print("=" * 60 + "\n")

while True:
    done_file = os.path.join(WATCH_DIR, "done.txt")
    
    if os.path.exists(done_file):
        time.sleep(1)
        run_full_pipeline()
        # Remove the flag so we don't run again immediately
        os.remove(done_file)
        print("\nReady for next scan.\n")
        print("=" * 60)
        print(f"Watching {WATCH_DIR} for new scans...")
        print("=" * 60 + "\n")
        
    time.sleep(2)
