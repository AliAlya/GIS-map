# Navigation Mapping System

## Overview
This project is a comprehensive mapping system designed for routing, pathfinding, and data visualization. It provides functionality for loading map data, calculating routes, and drawing maps with dynamic rendering and pathfinding features. The system leverages `ezgl` for graphical rendering and `StreetsDatabaseAPI` for map data management.

---

## Features
### Pathfinding
- **Shortest Path Calculation**:
  - Implements pathfinding algorithms with heuristics to find the shortest path between intersections.
  - Includes turn penalties for more realistic travel time estimation.

- **Direction Generation**:
  - Dynamically generates turn-by-turn directions for paths between intersections.

### Map Visualization
- **Dynamic Rendering**:
  - Supports various zoom levels and layers for features, streets, points of interest (POIs), and street names.
  - Light and dark mode for improved visibility.

- **Interactive Features**:
  - Supports mouse-based and input-based intersection selection for pathfinding.
  - Displays user-selected POIs and intersections with visual highlights.

### Map Data Integration
- **Street and Intersection Data**:
  - Loads street segment lengths, intersection data, and feature areas.
  - Supports data for multiple cities with on-demand loading.

- **OSM Data**:
  - Parses OpenStreetMap (OSM) data for features like highways, parks, and buildings.

### API Integration
- **Weather Data**:
  - Includes functionality for fetching and displaying weather data for the current map.

---

## Requirements
### Hardware
- A Linux-based machine (tested on Ubuntu).
- Graphics support for rendering.

### Software
- **Dependencies**:
  - `ezgl` for rendering.
  - `StreetsDatabaseAPI` for map data.
  - GTK+ for UI.
  - cURL for API calls.
- **Compiler**:
  - A modern C++ compiler (e.g., GCC 11 or later).
- **Libraries**:
  - `cmath`, `thread`, `vector`, `curl`, `chrono`.

---

## How It Works
1. **Map Loading**:
   - Loads map data (`.streets.bin`) and OSM data (`.osm.bin`) through `StreetsDatabaseAPI` and `OSMDatabaseAPI`.

2. **Pathfinding**:
   - Uses heuristic-based shortest path algorithms for navigation.
   - Accounts for turn penalties when switching streets.

3. **Rendering**:
   - Renders the map dynamically based on user interaction and zoom levels.
   - Uses `ezgl` for graphics and allows toggling layers like streets, features, and POIs.

4. **Interaction**:
   - Users can input intersections or click on the map to select start and end points.
   - Provides real-time directions and highlights paths on the map.

5. **Dynamic Features**:
   - Automatically adjusts map detail based on the level of zoom.
   - Supports both light and dark color schemes.

---

## Project Structure
- **`m1.cpp`**:
  - Handles loading of map data, including intersections, street segments, and feature points.
  - Provides utility functions for geographic calculations (e.g., distance between points).

- **`m2.cpp`**:
  - Implements rendering logic using `ezgl`.
  - Provides interactive features like pathfinding and weather integration.

- **`m3.cpp`**:
  - Contains pathfinding algorithms and travel time calculations.
  - Integrates routing logic with visualization.

- **DataStructures.h**:
  - Defines custom data structures for intersections, streets, and features.

- **Graphics**:
  - Leverages `ezgl` for dynamic map rendering.

---

## How to Run
1. **Setup Environment**:
   - Install dependencies:
     ```bash
     sudo apt-get install libgtk-3-dev libcurl4-openssl-dev
     ```
   - Clone and navigate to the project directory.

2. **Compile**:
   - Use the provided `Makefile` to build the project:
     ```bash
     make
     ```

3. **Run**:
   - Execute the binary:
     ```bash
     ./mapper <map_name>.streets.bin
     ```

4. **Usage**:
   - Navigate the map using mouse clicks or intersection inputs.
   - Toggle settings (e.g., POIs, features, dark mode) using the UI.

---

## Example Usage
1. Select two intersections to find the shortest path.
2. Enable POIs to display nearby points of interest.
3. Switch to dark mode for nighttime viewing.

---

## Future Enhancements
- Improve heuristics for pathfinding.
- Add support for public transit routes.
- Enhance API integration for live traffic data.
