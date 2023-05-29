#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include "LatLon.h"
#include "StreetsDatabaseAPI.h"
#include "ezgl/point.hpp"
#include "ezgl/rectangle.hpp"
#include <cmath>
#include <set>
#include "m1.h"


//globals to store weather (c), conditions, windspeeds (km/h), feelsLike (c)
std::string weather;
std::string windSpeeds;
std::string feelsLike;
std::string snowOrRain;

ezgl::point2d LatLonToPoint2D(LatLon point, double latAvg);
LatLon point2dToLatLon(ezgl::point2d point);
void setLatLonWorldBounds();
void setupCoords();
bool checkRectangleOverlap(ezgl::rectangle r1, ezgl::rectangle r2);
double distanceBetweenP2D(ezgl::point2d p1, ezgl::point2d p2);
StreetSegmentIdx getSegmentFromIntersections(IntersectionIdx from, IntersectionIdx to);



std::unordered_map<OSMID, const OSMEntity*>osm_node_map;
 
std::vector<StreetSegmentInfo> allStreetSegments;
std::vector<double> allStreetLengths;
std::vector<std::vector<LatLon>> allFeaturePoints;
std::vector<std::vector<ezgl::point2d>> allFeaturePoints2d;
std::vector<std::pair<double, int>> areaPairs;
std::vector<int> sortedAreas;
std::vector<LatLon> allIntersectionPositions;
std::vector<std::unordered_set<IntersectionIdx>> AllIntersectionsOfStreets;

//initializing our struct 
struct segment_info {
        std::string highway;
        std::string name;
        bool oneWay;
        ezgl::point2d from, to;
        std::vector<ezgl::point2d> curvePoints;
};

struct POIinfo {
    std::string name;
    std::string type;
    ezgl::point2d position; 
};

std::vector<segment_info> SSInfo;
std::vector<POIinfo> allPOIs;
std::vector<int> level1StreetSegs;
std::vector<int> level2StreetSegs;
std::vector<int> level3StreetSegs;

std::vector<std::vector<StreetSegmentIdx>> intersectionSegments;
std::vector<double> SegmentLengthStorage;
std::multimap<std::string, int> streetIdsFromNames;

double worldLatAvg;
double worldMinLon;
double worldMinLat;
double worldMaxLon;
double worldMaxLat;


ezgl::rectangle initial_world({0, 0}, 
                             {1, 1});
ezgl::point2d LatLonToPoint2D(LatLon point, double latAvg){
    // (x, y) = (R ·lon ·cos(latavg), R ·lat)
    double x = kEarthRadiusInMeters * (point.longitude() * kDegreeToRadian) * cos(latAvg);
    double y = kEarthRadiusInMeters * (point.latitude() * kDegreeToRadian);
    return {x, y};
}

double pi = 2*acos(0.0);

double distanceBetweenP2D(ezgl::point2d p1, ezgl::point2d p2) {
    return sqrt(pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2));
}

//function to go from point2d to LatLon
//Passing 2d point to this function 
LatLon point2dToLatLon(ezgl::point2d point){

    //worldLatAvg is a global variable 
    double lat = (point.y / kEarthRadiusInMeters) * (180/pi);
    double lon = (point.x / (kEarthRadiusInMeters * cos(worldLatAvg))) * (180/pi);

    LatLon lat_lon(lat, lon); //construct an object of LatLon
    return lat_lon; 
}

void setLatLonWorldBounds() {
    LatLon initialPos = allIntersectionPositions[0];
    worldMinLon = initialPos.longitude();
    worldMinLat = initialPos.latitude();
    worldMaxLon = initialPos.longitude();
    worldMaxLat = initialPos.latitude();
    for (auto pos: allIntersectionPositions) {
        worldMinLon = std::min(worldMinLon, pos.longitude());
        worldMinLat = std::min(worldMinLat, pos.latitude());
        worldMaxLon = std::max(worldMaxLon, pos.longitude());
        worldMaxLat = std::max(worldMaxLat, pos.latitude());
    }
}

void setupCoords() {
    worldLatAvg = ((worldMinLat * kDegreeToRadian) + worldMaxLat * kDegreeToRadian) / 2.0;
    LatLon worldMin(worldMinLat, worldMinLon);
    LatLon worldMax(worldMaxLat, worldMaxLon);
    ezgl::point2d worldMinXY = LatLonToPoint2D(worldMin, worldLatAvg);
    ezgl::point2d worldMaxXY = LatLonToPoint2D(worldMax, worldLatAvg);
    ezgl::rectangle world_bounds({worldMinXY.x, worldMinXY.y}, 
                                        {worldMaxXY.x, worldMaxXY.y});
    initial_world = world_bounds;
}

struct intersection_info{

    ezgl::point2d xy_loc;
    std::string inter_name;
    bool highlight = false;
};

std::vector<intersection_info> intersections;

struct feature_info {
    std::string name;
    double area;
    ezgl::rectangle bounds;
    ezgl::point2d centroid;
    bool closed;
    FeatureType type;
};
std::vector<feature_info> featureInfo;

// checks to see if two rectangles overlap
bool checkRectangleOverlap(ezgl::rectangle r1, ezgl::rectangle r2) {
    if (r1.top_left().x > r2.bottom_right().x || r2.top_left().x > r1.bottom_right().x) return false;

    if (r1.bottom_right().y > r2.top_left().y || r2.bottom_right().y > r1.top_left().y) return false;

    return true;
}

// returns the street segment between 2 intersections
StreetSegmentIdx getSegmentFromIntersections(IntersectionIdx from, IntersectionIdx to) {
    for (StreetSegmentIdx segment: findStreetSegmentsOfIntersection(from)) { // go over all segments of the first intersection
        StreetSegmentInfo seg = allStreetSegments[segment];
        if (seg.to == to || seg.from == to) {return segment;}       // return the one that leads to the correct intersection
    }                                                                        
    return -1;                                                           // return -1 if a match was not found
}

struct sortByTravelTime {
  bool operator ()(const int& lhs, const int& rhs) const {
    return findStreetSegmentTravelTime(lhs) < findStreetSegmentTravelTime(rhs);
  }
};
std::unordered_map<int, std::set<int, sortByTravelTime>> neighbourTravelTimes;

std::vector<int> searchPathIntersections;
std::vector<std::string> UserPromptedDirections;
