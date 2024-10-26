#!/bin/bash

# Assign the parameters to descriptive variable names
source_image1=$1
source_image2=$2
output_directory=$3
projwin_coords=$4
output_file=$5
color=$6

# Create the output directory if it does not exist
mkdir -p "$output_directory"

# Extract the base names of the source images to use in the output file names
base_name1=$(basename "$source_image1" .tif)
base_name2=$(basename "$source_image2" .tif)

# Temporary file paths
temp_file1="$output_directory/${base_name1}.tif"
temp_file2="$output_directory/${base_name2}.tif"

case $color in
    blue)
        image1=$temp_file2
        image2=$temp_file2
        image3=$temp_file1
        ;;
    green)
        image1=$temp_file2
        image2=$temp_file1
        image3=$temp_file2
        ;;
    red)
        image1=$temp_file1
        image2=$temp_file2
        image3=$temp_file2
        ;;
    *)
        echo "Invalid color. Please specify blue, green, or red."
        exit 1
        ;;
esac

# Run gdal_translate on the first image
gdal_translate -projwin $projwin_coords -b 2 "$source_image1" "$temp_file1"

# Run gdal_translate on the second image
gdal_translate -projwin $projwin_coords -b 2 "$source_image2" "$temp_file2"

# Run gdal_merge.py to merge the images
gdal_merge.py -separate -o "$output_directory/${output_file}_${color}.tif" -co PHOTOMETRIC=RGB -v "$image1" "$image2" "$image3"

rm "$temp_file1" "$temp_file2"
