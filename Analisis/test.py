import matplotlib.pyplot as plt

# Generate some data
x = [1, 2, 3, 4, 5]
y1 = [1, 2, 3, 4, 5]
y2 = [5, 4, 3, 2, 1]

# Create figure and plot
fig, ax = plt.subplots()
line1, = ax.plot(x, y1, label='Line 1')
line2, = ax.plot(x, y2, label='Line 2')

# Function to toggle visibility
def toggle_visibility(event):
    legend = event.artist
    index = legend.get_gid()
    line = lines[index]
    if line.get_visible():
        line.set_visible(False)
    else:
        line.set_visible(True)
    plt.draw()

# Connect pick event to the figure
fig.canvas.mpl_connect('pick_event', toggle_visibility)

# Store lines in a list
lines = [line1, line2]

# Add legend
leg = ax.legend(loc='upper left', fancybox=True)
for legline, origline in zip(leg.get_lines(), lines):
    legline.set_picker(5)  # 5 points tolerance
    legline.set_gid(str(lines.index(origline)))

plt.show()
