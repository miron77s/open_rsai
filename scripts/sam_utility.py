import numpy as np
import cv2
import os
from osgeo import ogr
import fnmatch


def cv_show_box(box, image):
    x0, y0, x1, y1 = int( box[0][0] ), int ( box[0][1] ), int ( box[1][0] ), int ( box[1][1] )
    cv2.rectangle(image, (x0, y0), (x1, y1), (0, 255, 255), 2)


def get_jpg_filenames_without_extension(directory, exceptions=None):
    if exceptions is None:
        exceptions = []

    filenames = []
    for f in os.listdir(directory):
        if f.endswith('.jpg') or f.endswith('.jpeg'):
            filename_without_extension = os.path.splitext(f)[0]
            if not any(fnmatch.fnmatch(filename_without_extension, pattern) for pattern in exceptions):
                filenames.append(filename_without_extension)
    return filenames


def wkt_to_np_array(filename):
    with open(filename, 'r') as file:
        data = file.read().replace('\n', '')

        # Create a geometry from the WKT string
        geom = ogr.CreateGeometryFromWkt(data)

        # Get the points from the MultiPoint geometry
        points = [(point.GetX(), point.GetY()) for point in geom]

        # Convert the points into a numpy array
        coordinates = np.array(points)

        return coordinates


def wkt_to_ogr(filename):
    with open(filename, 'r') as file:
        data = file.read().replace('\n', '')

        # Create a geometry from the WKT string
        geom = ogr.CreateGeometryFromWkt(data)

        return geom


def read_coordinates(file_name):
    # Initialize an empty list to store the coordinates
    coordinates_list = []

    # Open the file
    with open(file_name, 'r') as file:
        # Read each line in the file
        for line in file:
            # Remove the brackets and split the line into parts by comma
            parts = line.replace('[', '').replace(']', '').split(',')

            for part in parts:
                # Split the part into x and y coordinates
                x, y = part.split()

                # Convert the parts to float and add them to the list
                coordinates_list.append((float(x), float(y)))

    # Convert the list to a numpy array and return it
    return np.array(coordinates_list)


def get_largest_contour(image):
    # Find contours - cv2.RETR_EXTERNAL is for outer contours
    contours, _ = cv2.findContours(image, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    # Find the largest contour
    largest_contour = max(contours, key=cv2.contourArea)

    return largest_contour


# Function to convert OGR geometry to OpenCV contour
def ogr2contour(geom):
    # Get exterior ring
    ring = geom.GetGeometryRef(0)

    # Get points of the ring as a list of tuples
    points = ring.GetPoints()

    # Convert to contour
    contour = np.array(points, dtype=np.int32).reshape((-1, 1, 2))

    return contour


def opencv2ogr(contour):
    # create an empty OGR Geometry
    ring = ogr.Geometry(ogr.wkbLinearRing)

    # iterate over the contour points
    for i in range(len(contour)):
        # get the i-th contour point
        contour_point = contour[i]

        # add the point to the OGR Geometry
        ring.AddPoint(float(contour_point[0][0]), float(contour_point[0][1]))

    # Close the ring
    ring.CloseRings()

    # Create polygon
    poly = ogr.Geometry(ogr.wkbPolygon)
    poly.AddGeometry(ring)
    return poly


# Function to compute IoU
def compute_iou(poly1, poly2):
    # Compute intersection and union of the two polygons
    intersection = poly1.Intersection(poly2)
    union = poly1.Union(poly2)

    # Compute areas of the intersection and union
    intersection_area = intersection.GetArea()
    union_area = union.GetArea()

    # Compute IoU
    iou = intersection_area / union_area
    return iou


def merge_image_with_mask(img, mask, alpha, color):
    # Create a 3 channel version of the mask
    mask_rgb = cv2.cvtColor(mask, cv2.COLOR_GRAY2BGR)

    # Create a colored image of the same size as the mask
    color_img = np.zeros_like(mask_rgb)
    color_img[:, :, 0] = color[0] * (mask_rgb[:, :, 0] > 0)
    color_img[:, :, 1] = color[1] * (mask_rgb[:, :, 1] > 0)
    color_img[:, :, 2] = color[2] * (mask_rgb[:, :, 2] > 0)

    # Alpha blending only on masked areas
    masked = np.where(mask_rgb > 0, img * (1 - alpha) + color_img * alpha, img)
    masked_uint8 = np.clip(masked, 0, 255).astype('uint8')
    masked_uint8 = cv2.cvtColor(masked_uint8, cv2.COLOR_BGR2RGB)

    return masked_uint8


def draw_contours(anns, edges_width):
    if len(anns) == 0:
        return
    sorted_anns = sorted(anns, key=(lambda x: x['area']), reverse=True)

    # Create an empty image to draw the contours on
    img = np.zeros((sorted_anns[0]['segmentation'].shape[0], sorted_anns[0]['segmentation'].shape[1]), dtype=np.uint8)
    for ann in sorted_anns:
        m = ann['segmentation'].astype(np.uint8)

        # Convert m to binary image
        _, m = cv2.threshold(m, 0.5, 255, cv2.THRESH_BINARY)

        # Check if segment is not empty
        if np.any(m):
            # Find contours of the mask
            contours, _ = cv2.findContours(m, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

            # Draw contours on the image with increased thickness
            cv2.drawContours(img, contours, -1, (255), edges_width)

    return img


def draw_segments(anns):
    if len(anns) == 0:
        return
    sorted_anns = sorted(anns, key=(lambda x: x['area']), reverse=True)

    # Create an empty image to draw the contours on
    img = np.ones((sorted_anns[0]['segmentation'].shape[0], sorted_anns[0]['segmentation'].shape[1], 4))
    img[:,:,3] = 0
    for ann in sorted_anns:
        m = ann['segmentation']
        color_mask = np.concatenate([np.random.random(3), [0.35]])
        img[m] = color_mask * 255
    return img
