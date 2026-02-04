import os
from PIL import Image

# Set target directory and size
target_size = (100, 100)
directory = "./images" # Change this to your folder path

for filename in os.listdir(directory):
    if filename.endswith(".png"):
        path = os.path.join(directory, filename)
        with Image.open(path) as img:
            # High-quality LANCZOS filter is recommended for downscaling
            resized = img.resize(target_size, Image.Resampling.LANCZOS)
            resized.save(f"resized_{filename}")
            print(f"Processed: {filename}")
