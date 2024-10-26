# Assign the parameters to descriptive variable names
new_map=$1
old_map=$2
outdir=$3
outfile_name=$4
raster_extent=$5
raster=$6

IFS=' ' read -r x1 y1 x2 y2 <<EOF
$raster_extent
EOF


# Swapping x and y coordinates for vector extent (cause it should be "xmin ymin xmax ymax")
# but not the top-left and bottom-right coordinates
if [ 1 -eq "$(echo "$x1 > $x2" | bc)" ]; then
    temp=$x1
    x1=$x2
    x2=$temp
fi

if [ 1 -eq "$(echo "$y1 > $y2" | bc)" ]; then
    temp=$y1
    y1=$y2
    y2=$temp
fi

vector_extent="${x1} ${y1} ${x2} ${y2}"

temp_vector="${outdir}vector_data.tif"
temp_vector_alpha="${outdir}vector_data_alpha.tif"
temp_raster="${outdir}raster_base.tif"
temp_raster_alpha="${outdir}raster_base_alpha.tif"
out_path="${outdir}${outfile_name}.tif"

# Create the output directory if it does not exist
mkdir -p "$outdir"

# Render an older map with new raster creation to blue channel
echo "\nRendering older map..."
gdal_rasterize -init 0 -burn 0 0 255 -tr 0.4 0.4 -te $vector_extent "$old_map" "$temp_vector"

# Render newer map to same raster to red channel
echo "\nRendering newer map..."
gdal_rasterize -b 1 -burn 255 "$new_map" "$temp_vector"

if [ -z "$raster" ]; then

# Making the black background transparent
echo "\nAdding transparency to vector..."
gdalwarp -srcnodata "0 0 0" -dstalpha "$temp_vector" "$out_path"

# Cleanup
rm "$temp_vector"

else

# Making the black background transparent
echo "\nAdding transparency to vector..."
gdalwarp -srcnodata "0 0 0" -dstalpha "$temp_vector" "$temp_vector_alpha"

# Extracting raster background
echo "\nExtracting raster background..."
gdal_translate -projwin $raster_extent "$raster" "$temp_raster"

# Adding an alpha channel into it
echo "\nAdding transparency to raster..."
gdalwarp -srcnodata "0 0 0" -dstalpha "$temp_raster" "$temp_raster_alpha"

# Merging rendered vector composit with raster background
echo "\nMerging..."
gdalwarp -srcalpha "$temp_raster_alpha" "$temp_vector_alpha" "$out_path"

# Cleanup
rm "$temp_vector" "$temp_vector_alpha" "$temp_raster" "$temp_raster_alpha"

fi


