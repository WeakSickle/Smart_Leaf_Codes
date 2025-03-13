import networkx as nx
import osmnx as ox
from geopy.geocoders import Nominatim
import folium
from PyQt5 import QtCore, QtGui, QtWidgets
import asyncio
from pyppeteer import launch
import pyppeteer
from PIL import Image
import io
import os
import sys
import requests

response = requests.get('https://ipinfo.io')
data = response.json()
location = data['loc'].split(',')  # Returns [latitude, longitude]
latitude, longitude = float(location[0]), float(location[1])

print(f"Latitude: {latitude}, Longitude: {longitude}")

g = Nominatim(user_agent="Leafgps");
reverse = g.reverse("{},{}".format(latitude,longitude))

print("\nYour Address:")
print(reverse)

reverse = str(reverse)

# Create a folium map centered at the location
m = folium.Map(location=[latitude, longitude], zoom_start=14)

folium.Marker(
    location=[latitude, longitude],
    tooltip="Me",
    popup=reverse,
    icon=folium.Icon(icon="cloud"),
).add_to(m)

# Save the map to an HTML file
m.save("map.html")

async def html_to_image(html_file, image_file):
    # Launch a headless browser using pyppeteer (automatically downloads Chromium)
    browser = await launch(headless=True)
    page = await browser.newPage()
    
    # Get the absolute path of the HTML file
    html_file = os.path.abspath("map.html")  # Replace with your file name andd location
    print(html_file)
    file_path = f"file:///{html_file}"

    await page.goto(file_path)

    await page.waitFor(2000)
    
    # Capture a screenshot of the page
    screenshot = await page.screenshot()
    
    # Close the browser
    await browser.close()

    # Convert the screenshot to an image using Pillow
    img = Image.open(io.BytesIO(screenshot))
    img.save(image_file)
    print(f"Image saved as {image_file}")

# Example usage
html_file = 'map.html'  # Path to your HTML file
image_file = 'map_image.png'  # Output image file (PNG)

# Run the function asynchronously
asyncio.get_event_loop().run_until_complete(html_to_image(html_file, image_file))
