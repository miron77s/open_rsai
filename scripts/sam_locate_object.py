import sam_utility as su
import sys
from tqdm import tqdm
import numpy as np
import cv2
from segment_anything import sam_model_registry, SamPredictor



if len(sys.argv) < 3:
    print("Please provide SAM model path and markup directory name")
    exit()

sam_checkpoint = sys.argv [1]
model_type = "vit_h"

device = "cuda"

sam = sam_model_registry[model_type](checkpoint=sam_checkpoint)
sam.to(device=device)

predictor = SamPredictor(sam)

directory_name = sys.argv [2]

jpg_files = su.get_jpg_filenames_without_extension(directory_name)
print(f"Number of jpg files: {len(jpg_files)}")

mean_iou = 0

for file in tqdm(jpg_files):
    image = cv2.imread(f"{directory_name}/{file}.jpg")
    image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

    bbox = su.read_coordinates(f"{directory_name}/{file}.bbox")

    positives = su.wkt_to_np_array(f"{directory_name}/{file}.points")
    #negatives = wkt_to_np_array(f"{directory_name}/{file}.negative")

    positive_labels = np.full(positives.shape[0], 1)
    #negative_labels = np.full(negatives.shape[0], 0)

    marks = positives
    labels = positive_labels
    #marks = np.concatenate((positives, negatives), axis=0)
    #labels = np.concatenate((positive_labels, negative_labels), axis=0)

    predictor.set_image(image)

    masks, scores, logits = predictor.predict(
        point_coords=None,
        point_labels=None,
        box=bbox[None, :],
        multimask_output=True,
     )

    max_iou = 0
    best_mask = None

    for i, (mask, score) in enumerate(zip(masks, scores)):
        array_image = (mask.astype(np.uint8)) * 255
        opencv_image = cv2.cvtColor(array_image, cv2.COLOR_GRAY2BGR)
        opencv_image = cv2.cvtColor(opencv_image, cv2.COLOR_BGR2GRAY)
        #cv2.imwrite(f"{directory_name}/{file}_{i}_pred.jpg", opencv_image)

        major_contour = su.opencv2ogr(su.get_largest_contour(opencv_image))
        ref_contour = su.wkt_to_ogr(f"{directory_name}/{file}.object")

        iou = su.compute_iou(major_contour, ref_contour)

        if iou > max_iou:
            max_iou = iou
            best_mask = opencv_image  # If so, update the max IoU and save the mask

        #mask_image = Image.fromarray(mask)
        #mask_image.save(f"{directory_name}/{file}_{i}_pred.jpg")

    mean_iou += max_iou

    merged = su.merge_image_with_mask(image, best_mask, 0.5, (255, 0, 0))
    su.cv_show_box(bbox, merged)

    cv2.imwrite(f"{directory_name}/{file}_pred_{(int(max_iou * 100))}.jpg", merged)

    with open(f"{directory_name}/{file}_pred.iou", 'w') as iou_file:
        iou_file.write(str(max_iou))
        iou_file.close()

print("mean iou = ", mean_iou / len(jpg_files))
