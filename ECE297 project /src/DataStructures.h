#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <unordered_set>
#include "LatLon.h"
#include "StreetsDatabaseAPI.h"
#include <set>
#include "ezgl/rectangle.hpp"
#include "ezgl/point.hpp"




//globals to store weather (c), conditions, windspeeds (km/h), feelsLike (c)
extern std::string weather;
extern std::string windSpeeds;
extern std::string feelsLike;
extern std::string snowOrRain;


extern std::unordered_map<OSMID, const OSMEntity *> osm_node_map;

extern std::vector<StreetSegmentInfo> allStreetSegments;
extern std::vector<double> allStreetLengths;
extern std::vector<std::vector<LatLon>> allFeaturePoints;
extern std::vector<std::vector<ezgl::point2d>> allFeaturePoints2d;
extern std::vector<std::pair<double, int>> areaPairs;
extern std::vector<int> sortedAreas;
extern std::vector<int> level1StreetSegs;
extern std::vector<int> level2StreetSegs;
extern std::vector<int> level3StreetSegs;


extern std::vector<LatLon> allIntersectionPositions;
extern std::vector<std::unordered_set<IntersectionIdx>> AllIntersectionsOfStreets;

//initializing our struct 
struct segment_info {
        std::string highway;
        std::string name;
        bool oneWay;
        ezgl::point2d from, to;
        std::vector<ezgl::point2d> curvePoints;
};
extern std::vector<segment_info> SSInfo; 

struct POIinfo {
    std::string name;
    std::string type;
    ezgl::point2d position; 
};
extern std::vector<POIinfo> allPOIs;

struct intersection_info{

    ezgl::point2d xy_loc;
    std::string inter_name;
    bool highlight = false;
};
extern std::vector<intersection_info> intersections;

struct feature_info {
    std::string name;
    double area;
    ezgl::rectangle bounds;
    ezgl::point2d centroid;
    bool closed;
    FeatureType type;
};
extern std::vector<feature_info> featureInfo;

struct sortByTravelTime {
  bool operator ()(const int& lhs, const int& rhs) const {
    return findStreetSegmentTravelTime(lhs) < findStreetSegmentTravelTime(rhs);
  }
};

extern std::vector<std::vector<StreetSegmentIdx>> intersectionSegments;
extern std::vector<double> SegmentLengthStorage;
extern std::multimap<std::string, int> streetIdsFromNames;
extern std::unordered_map<int, std::set<int, sortByTravelTime>> neighbourTravelTimes;

// converts LatLon to XY given latAvg
std::vector<double> LatLonToXY(LatLon, double);
extern double worldLatAvg;
extern double worldMinLon;
extern double worldMinLat;
extern double worldMaxLon;
extern double worldMaxLat;
extern double pi;

double distanceBetweenP2D(ezgl::point2d p1, ezgl::point2d p2);
extern ezgl::rectangle initial_world;
ezgl::point2d LatLonToPoint2D(LatLon point, double latAvg);
LatLon point2dToLatLon(ezgl::point2d point);

void setLatLonWorldBounds();
void setupCoords();

template<typename Type>
bool checkVectorDupe(std::vector<Type> &vec, Type &item) {
    // returns TRUE if vector does NOT contain an item
    if (!(std::find(vec.begin(), vec.end(), item) != vec.end())) {
        return true;
    } else {return false;}
}

bool checkRectangleOverlap(ezgl::rectangle r1, ezgl::rectangle r2);
StreetSegmentIdx getSegmentFromIntersections(IntersectionIdx from, IntersectionIdx to);

extern std::vector<int> searchPathIntersections;
extern std::vector<std::string> UserPromptedDirections;
