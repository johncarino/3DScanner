import os
import subprocess

# --- CONFIGURATION ---
# Update this path to where your current scan images and 'sparse' folder are
PROJECT_PATH = "/home/john/ensc351/public/myApps" 
COLMAP_BIN = "colmap"

def finish_processing():
    print(f"Resuming processing in: {PROJECT_PATH}")
    
    # --- FIX 1: Auto-detect the correct sparse path ---
    # Check if '0' folder exists, otherwise use the main sparse folder
    path_check = os.path.join(PROJECT_PATH, "sparse", "0")
    if os.path.exists(path_check) and os.path.exists(os.path.join(path_check, "cameras.bin")):
        sparse_path = path_check
    else:
        sparse_path = os.path.join(PROJECT_PATH, "sparse")
    
    print(f"Using Sparse Path: {sparse_path}")

    dense_path = os.path.join(PROJECT_PATH, "dense")
    os.makedirs(dense_path, exist_ok=True)

    # 1. Image Undistortion
    print("--> 1. Undistorting Images...")
    subprocess.run([
        COLMAP_BIN, "image_undistorter",
        "--image_path", PROJECT_PATH,
        "--input_path", sparse_path,
        "--output_path", dense_path,
        "--output_type", "COLMAP",
        "--max_image_size", "2000"
    ])

    # 2. Dense Reconstruction
    # --- FIX 2: Explicitly print this step to catch errors ---
    print("--> 2. Dense Stereo Reconstruction...")
    print("    (Note: If this step finishes instantly, it FAILED.)")
    
    result = subprocess.run([
        COLMAP_BIN, "patch_match_stereo",
        "--workspace_path", dense_path,
        "--workspace_format", "COLMAP",
        "--PatchMatchStereo.gpu_index=-1"  # Try adding '=' sign for compatibility
    ])
    
    if result.returncode != 0:
        print("ERROR: Dense reconstruction failed. Check the logs above.")
        return

    # 3. Stereo Fusion
    print("--> 3. Stereo Fusion...")
    subprocess.run([
        COLMAP_BIN, "stereo_fusion",
        "--workspace_path", dense_path,
        "--workspace_format", "COLMAP",
        "--input_type", "geometric",
        "--output_path", os.path.join(dense_path, "fused.ply")
    ])

    # 4. Meshing
    print("--> 4. Poisson Meshing...")
    # Check if fused.ply actually has data before trying to mesh
    fused_file = os.path.join(dense_path, "fused.ply")
    if os.path.exists(fused_file) and os.path.getsize(fused_file) > 1000:
        subprocess.run([
            COLMAP_BIN, "poisson_mesher",
            "--input_path", fused_file,
            "--output_path", os.path.join(dense_path, "meshed.ply")
        ])
        print("DONE! Your model is at:", os.path.join(dense_path, "meshed.ply"))
    else:
        print("FAILURE: fused.ply is empty. Dense reconstruction returned 0 points.")

if __name__ == "__main__":
    finish_processing()