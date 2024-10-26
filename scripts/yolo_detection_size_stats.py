import sys
import re
import matplotlib.pyplot as plt

def parse_file_and_plot_histogram(file_path):
    # Regular expression to match lines with detections including 'roofs' or other identifiers
    pattern = re.compile(r'\s*([a-zA-Z0-9_]+):\s*\d+%.*width:\s*(\d+).*height:\s*(\d+)')

    # List to store the maximum of width and height values
    max_dimensions = []

    # Open the file and read line by line
    with open(file_path, 'r') as file:
        for line in file:
            match = pattern.search(line)
            if match:
                # Calculate the max of width and height for each detection and add to the list
                width = int(match.group(2))
                height = int(match.group(3))
                max_dimensions.append(max(width, height))

    if not max_dimensions:
        print("No width and height values found in the file.")
        return

    # Find the minimum value among the maximum dimensions
    min_value = min(max_dimensions)

    # Plot the histogram
    plt.figure(figsize=(10, 6))
    counts, bins, patches = plt.hist(max_dimensions, bins=500, color='blue', edgecolor='black', alpha=0.7)
    #plt.title('Histogram of Maximum Dimensions (Width or Height)')
    plt.xlabel('Value')
    plt.ylabel('Frequency')

    # Mark the minimal value on the plot
    min_bin = min(bins[:-1], key=lambda x: abs(x - min_value))
    min_height = counts[list(bins[:-1]).index(min_bin)]
    plt.annotate(f'Min: {min_value}', xy=(min_value, min_height), xytext=(min_value, min_height + 10),
                 arrowprops=dict(facecolor='red', shrink=0.05), horizontalalignment='center')

    # Save the histogram to a file
    plt.savefig('histogram.png')
    print('Histogram saved to histogram.png')

    # Display the histogram
    plt.show()

# Entry point of the script
if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Usage: python script.py <file_path>")
        sys.exit(1)

    file_path = sys.argv[1]
    parse_file_and_plot_histogram(file_path)
