# Smart Leaf Codes

Some notes about the current drawbacks of the map implementation:

- **No support for removal/editing**: This is primarily because the map generates a static output, and Folium does not support live updates. A potential workaround could be refreshing the entire map, but this isn't a significant issue since the devices aren't expected to move frequently. Additionally, if we incorporate a charge value, it might be easier to display that information on a separate widget rather than on the map itself.

- **Limited interactivity**: Currently, you can click or hover over the markers to view information, but for now, it only displays the device ID.

- **Data storage**: I haven't implemented it yet, but im guessing the plan is to store all received data in a CSV file or a similar format for easier access for different parts of the message 