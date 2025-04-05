import sam_utility as su
import sys
import numpy as np
import cv2
import os
from tqdm import tqdm
from segment_anything import sam_model_registry, SamAutomaticMaskGenerator, SamPredictor


if len(sys.argv) < 4:
    print("Please provide SAM model path, images directory name and edges width value")
    exit()

sam_checkpoint = sys.argv[1]
model_type = "vit_h"

device = "cuda"

sam = sam_model_registry[model_type](checkpoint=sam_checkpoint)
sam.to(device=device)

mask_generator = SamAutomaticMaskGenerator(
    model=sam,
    #points_per_batch = 384,
    stability_score_thresh=0.9
)

directory_name = sys.argv[2]
edges_width = int(sys.argv[3])

filename_exceptions = ['*_edges', '*_heat', '*_segms', '*_tile']

jpg_files = su.get_jpg_filenames_without_extension(directory_name, filename_exceptions)
print(f"Number of jpg files: {len(jpg_files)}")

for file in tqdm(jpg_files):
    edges_file_name = f"{directory_name}/{file}_edges.jpg"
    segms_file_name = f"{directory_name}/{file}_segms.jpg"
    wkt_file_name = f"{directory_name}/{file}.wkt"
    
    if not os.path.isfile(edges_file_name) or not os.path.isfile(segms_file_name) or not os.path.isfile(wkt_file_name):
        # Read the image using OpenCV
        image = cv2.imread(f"{directory_name}/{file}.jpg")
        
        # Convert from BGR to RGB
        image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
        
        # No need to convert to tensor or normalize, we pass the image directly
        # Generate the masks
        masks = mask_generator.generate(image)
        
        edges = su.draw_contours(masks, edges_width)
        cv2.imwrite(edges_file_name, edges)
        segmented = su.draw_segments(masks)
        cv2.imwrite(segms_file_name, segmented)

        with open(f"{directory_name}/{file}.wkt", 'w') as wkt_file:
            for mask in masks:
                m = mask['segmentation'].astype(np.uint8)
                _, m = cv2.threshold(m, 0.5, 255, cv2.THRESH_BINARY)

                if np.any(m):
                    major_contour = su.get_largest_contour(m)
                    ogr_geom = su.opencv2ogr(major_contour)
                    wkt_geom = ogr_geom.ExportToWkt()
                    wkt_file.write(wkt_geom + '\n')

