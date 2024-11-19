#pragma once

#define DEFAULT_VECTOR_MAP_OUTPUT_DRIVER    "ESRI Shapefile"
#define DEFAULT_SPATIAL_REFERENCE_EPSG      3395
#define DEFAULT_IDENTIFICATION_FIELD_NAME   "osm_id"

#define DEFAULT_ENCODING                    "UTF-8"
#define DEFAULT_DELIMITERS                  ","

#define DEFAULT_FIELD_SIZE                  -1
#define DEFAULT_FIELD_CAN_BE_NULL           true

/// Default output layers names
#define DEFAULT_INLIERS_LAYER_NAME          "inliers"
#define DEFAULT_BOUNDS_LAYER_NAME           "bounds"
#define DEFAULT_ROOF_LAYER_NAME             "roofs"
#define DEFAULT_PROJECTION_LAYER_NAME       "projes"
#define DEFAULT_SHADE_LAYER_NAME            "shades"
#define DEFAULT_MARKUP_LAYER_NAME           "markup"
#define DEFAULT_SEGMENTS_DIRECTORY          "segments"
#define DEFAULT_VARIANTS_DIRECTORY          "variants"
#define DEFAULT_ROOFS_SUBDIRECTORY          "roofs"
#define DEFAULT_STRUCTURE_SUBDIRECTORY      "structures"
#define DEFAULT_SEGMENT_FILE_EXT            ".jpg"
#define DEFAULT_SEGMENT_WKT_FILE_EXT        ".wkt"
#define DEFAULT_OBJECT_WKT_FILE_PREFIX      "obj_"

/// Default output features names and values
#define DEFAULT_PROJ_STEP_X_FIELD_NAME      "proj_x"
#define DEFAULT_PROJ_STEP_Y_FIELD_NAME      "proj_y"
#define DEFAULT_SHADE_STEP_X_FIELD_NAME     "shade_x"
#define DEFAULT_SHADE_STEP_Y_FIELD_NAME     "shade_y"
#define DEFAULT_VECTOR_MAX_LENGTH_NAME      "max_len"

/// Default utility param values
#define DEFAULT_MIN_RASTER_OBJ_SIZE_VALUE   "30"
#define DEFAULT_MAX_PROJECTION_LEN_VALUE    "70"
#define DEFAULT_SHADE_MASK_SIZE_VALUE       "50"
#define DEFAULT_SEGMENTIZE_STEP_VALUE       "1.0"
#define DEFAULT_PROJECTION_STEP_VALUE       "1.0"
#define DEFAULT_ROOF_POSITION_WALK_VALUE    "15.0"
#define DEFAULT_HEIGHT_FACTOR_VALUE         "0.0"
#define DEFAULT_ROOF_VARIANTS_VALUE         "20"
#define DEFAULT_SHADE_VARIANTS_VALUE        "5"
#define DEFAULT_RECONSTRUCTION_MODE         "auto"
#define DEFAULT_INTERACTION_MODE            "internal"
#define DEFAULT_SEG_ANY_TILE_BUFFER_VALUE   "200"
#define DEFAULT_SEG_ANY_EDGES_WIDTH_VALUE   "5"
#define DEFAULT_MIN_FIRST_POS_WEIGHT        "150"
#define DEFAULT_MAX_FIRST_POS_DEVIATION     "1.5"
#define DEFAULT_MARKUP_BALANCE              "0.5"
#define DEFAULT_MARKUP_VALIDATION           "0.15"
#define DEFAULT_MARKUP_OVERLAP              "0.2"
#define DEFAULT_MARKUP_TILE_SIZES           "500,500"
#define DEFAULT_MARKUP_CLASSES              ( std::string ( DEFAULT_ROOFS_SUBDIRECTORY ) + "," + DEFAULT_PROJECTION_LAYER_NAME + "," + DEFAULT_SHADE_LAYER_NAME )
#define DEFAULT_MARKUP_FORMAT               "yolo"
#define DEFAULT_MARKUP_APPEND_MODE          "append"
#define DEFAULT_MARKUP_REPLACE_MODE         "replace"

#define DEFAULT_OBJECT_ID_FIELD_NAME        "FID"
#define DEFAULT_OBJECT_HEIGHT_FIELD_NAME    "height"

#define DEFAULT_META_TYPE_FIELD_NAME        "type"
#define DEFAULT_PROJECTION_FIELD_VALUE      "projection"
#define DEFAULT_SHADE_FIELD_VALUE           "shade"

/// Maximin values
#define MAXIMUM_MARKUP_VALIDATION           0.5
#define DEFAULT_IOU_MATCH_THRESH_VALUE      "0.9"
#define DEFAULT_IOU_MIN_THRESH_VALUE        "0.02"
#define DEFAULT_OUTDATED_LAYER_NAME         "outdated"
#define DEFAULT_UPCOMMING_LAYER_NAME        "upcomming"
#define DEFAULT_UPDATED_LAYER_NAME          "updated"
