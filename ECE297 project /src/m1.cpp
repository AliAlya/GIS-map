/* 
 * Copyright 2023 University of Toronto
 *
 * Permission is hereby granted, to use this software and associated 
 * documentation files (the "Software") in course work at the University 
 * of Toronto, or for personal use. Other uses are prohibited, in 
 * particular the distribution of the Software either publicly or to third 
 * parties.
 *
 * The above copyright notice and this permission notice shall be included in   
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "m1.h"
#include "DataStructures.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include "OSMDatabaseAPI.h"
#include "OSMEntity.h"
#include "LatLon.h"
#include <cstdlib>
#include <limits>
#include <chrono>
#include <thread>


// loadMap will be called with the name of the file that stores the "layer-2"
// map data accessed through StreetsDatabaseAPI: the street and intersection 
// data that is higher-level than the raw OSM data). 
// This file name will always end in ".streets.bin" and you 
// can call loadStreetsDatabaseBIN with this filename to initialize the
// layer 2 (StreetsDatabase) API.
// If you need data from the lower level, layer 1, API that provides raw OSM
// data (nodes, ways, etc.) you will also need to initialize the layer 1 
// OSMDatabaseAPI by calling loadOSMDatabaseBIN. That function needs the 
// name of the ".osm.bin" file that matches your map -- just change 
// ".streets" to ".osm" in the map_streets_database_filename to get the proper
// name.

// loads all OSMNodes
bool loadNodes();
// loads all street lengths
bool loadStreetLengths();
// loads all intersection positions
bool loadIntersectionPositions();
// loads all features with points
bool loadAllFeatures();
// returns the value for the term for the area algorithm
double areaTerm(FeatureIdx, int, double);
//loads all streets with their intersections
bool loadAllIntersectionsOfStreets();
//compare strings upto certain length
bool stringCompare(std::string &preF, std::string const &key);
//preload findStreetSegmentOfIntersection
bool loadMapfSSOI();
//preload findStreetSegLengthgit
void loadmapfSSL();
//preload findStreetIdFromPartialStreetName
void loadMapfStIFPS();
//load allStreetSegment's highway value, name, and oneWay value
void loadStreetSegStruct();
void loadAllPOIStruct();
bool loadDatabases(std::string map_streets_database_filename);
void loadAllIntersectionData();
void loadAllStreetLengthData();
void loadAllSegmentData();
void threadGroup1();
void threadGroup2();


std::string filename;

bool loadMapfSSOI() 
{
    intersectionSegments.resize(getNumIntersections());
    //loop through all intersections
    for (int inter = 0; inter < getNumIntersections(); inter++)
    {
        std::set<int, sortByTravelTime> neighs;
        std::vector<IntersectionIdx> vecI;
        //get the corresponding street segment and srote it in vector
        for (int segs = 0; segs < getNumIntersectionStreetSegment(inter); segs++)
        {
            intersectionSegments[inter].push_back(getIntersectionStreetSegment(inter, segs));
        }
        for (int itx: findAdjacentIntersections(inter)) {
            neighs.insert(itx);
        }
        neighbourTravelTimes[inter] = neighs;
    }
    return true;
}

void loadmapfSSL()
{

    SegmentLengthStorage.resize(getNumStreetSegments());
    int numOfStreetSegs = getNumStreetSegments();

    for (int SSID = 0; SSID < numOfStreetSegs; SSID++)
    {
        //vector<int> VecL;

        //reset distance for each intersection, Set initial distance to 0
        double dist = 0.0;

        for (int len = 0; len < 1; len++)
        {

            //make street segment info readable - struct
            StreetSegmentInfo sLengthInfo;

            sLengthInfo = getStreetSegmentInfo(SSID);

            //determine start and end intersections - class LatLon
            LatLon sStart = getIntersectionPosition(sLengthInfo.from);
            LatLon sEnd = getIntersectionPosition(sLengthInfo.to);
            LatLon cpIterator;

            // std::cout<< "its working upto here"<< std::endl;

            //curvepoint LatLon iterator (curve number - 1)
            int cpCount = sLengthInfo.numCurvePoints - 1;

            //If straight path, find straight path distance
            if (sLengthInfo.numCurvePoints == 0)
            {
                //  std::cout<< "its working upto here 0"<< std::endl;
                SegmentLengthStorage[SSID] = (findDistanceBetweenTwoPoints(sStart, sEnd));

                // calculate next segment length
                continue;
            }

            //Base case if 1 curvepoint
            if (cpCount == 0)
            {

                dist = dist + findDistanceBetweenTwoPoints(sStart, getStreetSegmentCurvePoint(SSID, 0));
                dist = dist + findDistanceBetweenTwoPoints(sEnd, getStreetSegmentCurvePoint(SSID, 0));

                // std::cout<< "its working upto here 4"<< std::endl;
                SegmentLengthStorage[SSID] = dist;

                // calculate next segment length
                continue;
            }
            else
            {
                //Iterate through all segment curvepoints starting from intersection 'to'
                while (cpCount >= 0)
                {
                    cpIterator = getStreetSegmentCurvePoint(SSID, cpCount);
                    dist += findDistanceBetweenTwoPoints(cpIterator, sEnd);
                    sEnd = cpIterator;
                    cpCount--;
                }
                //1 curvepoint (left), calculate dist from start intersection to curvepoint
                dist += findDistanceBetweenTwoPoints(sStart, sEnd);

                SegmentLengthStorage[SSID] = dist;
            }
        }
    }
    return;
}

//preloading street names and IDS for findStreetIdsFromPartialStreetName
void loadMapfStIFPS()
{

    //Add each street to map streetIdsFromNames
    for (int street = 0; street < getNumStreets(); street++)
    {
        //remove spaces and lowercase names
        std::string temp = getStreetName(street);
        auto filterStreetName = std::remove(temp.begin(), temp.end(), ' ');

        temp.erase(filterStreetName, temp.end());
        transform(temp.begin(), temp.end(), temp.begin(), ::tolower);
        // getStreetName(street).erase(remove_if(getStreetName(street).begin(), getStreetName(street).end(), ' ');
        // transform(getStreetName(street).begin(), getStreetName(street).end(), getStreetName(street).begin(), ::tolower);

        //Add street name and segment to streetIdsFromNames map
        streetIdsFromNames.insert(std::make_pair(temp, street));
    }
    return;
}

bool loadNodes(){
    
    for(int i = 0; i < getNumberOfNodes(); i++){
        const OSMNode* current = getNodeByIndex(i);
        osm_node_map.insert(std::make_pair(current->id(), current));
    }

    for (int wayID = 0; wayID < getNumberOfWays(); wayID++) {
        const OSMWay* current = getWayByIndex(wayID);
        osm_node_map.insert(std::make_pair(current->id(), current));
    }
    return true;
} 

bool loadStreetLengths() {
    // Iterate through all streets and store their lengths
    double sum = 0.0;
    allStreetLengths.resize(getNumStreets());
    for (int i=0; i < getNumStreetSegments(); i++){
        sum = SegmentLengthStorage[i];
        allStreetLengths[allStreetSegments[i].streetID] += sum;
    }
    return true;
}

bool loadIntersectionPositions(){
    // Iterate through all Intersections positions and put them in a vector sorted by IntersectionIdx
    allIntersectionPositions.resize(getNumIntersections());
    for (int i=0; i<getNumIntersections();i++) {
        allIntersectionPositions[i] = getIntersectionPosition(i);
        intersection_info curr;

        curr.xy_loc = LatLonToPoint2D(allIntersectionPositions[i], worldLatAvg); //LatLon to point2d
        curr.inter_name = getIntersectionName(i);
        curr.highlight = false; 

        intersections.push_back(curr); //push this struct back into intersections vector
    }
    return true;
}

bool loadAllFeatures() {
    // Iterate through all features and put them in a 2d vector sorted by FeatureID
    int size = getNumFeatures();
    allFeaturePoints.resize(size);
    allFeaturePoints2d.resize(size);
    featureInfo.resize(size);
    areaPairs.resize(size);
    sortedAreas.resize(size);

    for (int featureID = 0; featureID < size; featureID++) {
        
        std::vector<LatLon> v1;
        std::vector<ezgl::point2d> pts;
        ezgl::point2d initialPt = LatLonToPoint2D(getFeaturePoint(featureID, 0), worldLatAvg);
        double minX = initialPt.x; 
        double minY = initialPt.y; 
        double maxX = initialPt.x; 
        double maxY = initialPt.y;
        double ySum = 0.0;
        double xSum = 0.0;
        int numPoints = getNumFeaturePoints(featureID);
        ezgl::point2d centroid;
        for (int j=0; j<numPoints; j++) {
            LatLon llpt = getFeaturePoint(featureID, j);
            v1.push_back(llpt);
            ezgl::point2d point = LatLonToPoint2D(llpt, worldLatAvg);
            pts.push_back(point);
            minX = std::min(point.x, minX);
            minY = std::min(point.y, minY);
            maxX = std::max(point.x, maxX);
            maxY = std::max(point.y, maxY);
            ySum += point.y;
            xSum += point.x;
        }

        ezgl::rectangle bound{{minX, minY}, {maxX, maxY}};
        centroid.x = xSum/numPoints;
        centroid.y = ySum/numPoints;

        allFeaturePoints2d[featureID] = pts;
        allFeaturePoints[featureID] = v1;
        double areaF = bound.area();
        feature_info currInfo = {getFeatureName(featureID), areaF, bound, centroid, pts.front() == pts.back(), getFeatureType(featureID)};
        featureInfo[featureID] = currInfo;
        areaPairs[featureID] = std::make_pair(areaF, featureID);
    }

    //Sort the Feature Area from Greatest to Smallest
    std::sort(areaPairs.rbegin(), areaPairs.rend());
    int c = 0;
    for (std::pair<double, int> pair: areaPairs) {
        sortedAreas[c] = pair.second;
        c++;
    }

    return true;
}

bool loadAllIntersectionsOfStreets(){
    // Iterates through allStreetSegments, getting associated intersections and returning them
    AllIntersectionsOfStreets.resize(getNumStreets());
    IntersectionIdx currFrom, currTo;
    for (auto currSeg:allStreetSegments) { // Range-based loop
        currFrom = currSeg.from;
        currTo = currSeg.to;
        AllIntersectionsOfStreets[currSeg.streetID].insert(currFrom);
        AllIntersectionsOfStreets[currSeg.streetID].insert(currTo);
    }

    return true;
}

void loadStreetSegStruct(){   
    int streetNum = 0; //indexing through allStreetSegments
    SSInfo.resize(getNumStreetSegments());
    std::vector<std::string> L1{"motorway", "motorway_link", 
                                "trunk", "trunk_link",
                                "primary", "primary_link",
                                "secondary", "secondary_link"};

    std::vector<std::string> L2{"tertiary", "tertiary_link",
                                "unclassified",
                                "residential"};

    std::vector<std::string> L3{"service", "living_street", ""};

    for(auto streetseg: allStreetSegments){
        //instantiation of struct's object 
        segment_info curr;
        curr.highway = getOSMNodeTagValue(streetseg.wayOSMID, "highway"); //returns the value assigned to key 'highway'
        curr.name = getStreetName(streetseg.streetID);
        curr.oneWay = (streetseg.oneWay);
        curr.from = LatLonToPoint2D(allIntersectionPositions[streetseg.from], worldLatAvg);
        curr.to = LatLonToPoint2D(allIntersectionPositions[streetseg.to], worldLatAvg);
        
        std::vector<ezgl::point2d> currCurvePoints;
        currCurvePoints.resize(streetseg.numCurvePoints);
        if (streetseg.numCurvePoints != 0) {
            for (int pt = 0; pt < streetseg.numCurvePoints; pt++)
                currCurvePoints[pt] = LatLonToPoint2D(getStreetSegmentCurvePoint(streetNum, pt), worldLatAvg);
        }
        curr.curvePoints = currCurvePoints;
        
        SSInfo[streetNum] = curr; //push back that struct into our SSInfo vector
        std::string type = curr.highway;
        if (!(checkVectorDupe(L1, type))) {level1StreetSegs.push_back(streetNum);}
        else if (!(checkVectorDupe(L2, type))) {level2StreetSegs.push_back(streetNum);}
        else {level3StreetSegs.push_back(streetNum);}
        streetNum++;
    }
}

//Helper function to compare prefix and street name by reference
bool stringCompare(std::string &preF, std::string const &key)
{

    int sz = preF.length();

    //truncate streetname to prefix size
    std::string s = key.substr(0, sz);

    //compare names
    if (preF == s)
    {
        return true;
    }

    return false;
}

std::vector<double> LatLonToXY(LatLon point, double latAvg){
    // (x, y) = (R ·lon ·cos(latavg), R ·lat)
    double x = kEarthRadiusInMeters * (point.longitude() * kDegreeToRadian) * cos(latAvg);
    double y = kEarthRadiusInMeters * (point.latitude() * kDegreeToRadian);
    // Store in vector and return
    std::vector<double> xy;
    xy.resize(2);
    xy[0] = x;
    xy[1] = y;
    return xy;
}

double areaTerm(FeatureIdx feature_id, int idx, double latAvg){
    // Takes a feature's point number and generates xn*y(n+1)-yn*x(n+1)
    std::vector<LatLon> currFeature = allFeaturePoints[feature_id];
    LatLon p1 = currFeature[idx];
    LatLon p2;
    if (currFeature.size() == (idx + 1)){
        p2 = currFeature[0]; // Special case where current point is last point
    } else {p2 = currFeature[idx+1];} // Base case

    std::vector<double> xy1 = LatLonToXY(p1, latAvg);
    std::vector<double> xy2 = LatLonToXY(p2, latAvg);

    double term = xy1[0]*xy2[1] - xy1[1]*xy2[0]; //x1y2 − y1x2

    return term;
}

double findFeatureArea(FeatureIdx feature_id){
    // Returns the area of a feature given an id
    // Generates 
    std::vector<LatLon> featurePts = allFeaturePoints[feature_id];
    int numPoints = featurePts.size();
    double latMax = -361.0;
    double latMin = 361.0;
    double currLat;
    if (numPoints == 0) {return 0;} // Check if theres 0 points
    if (!(featurePts[0] == featurePts.back())) {return 0;} // Check if last point = first point

    // Calculate average latitude of shape
    for (int i = 0; i < numPoints; i++) {
        currLat = featurePts[i].latitude();
        if (currLat > latMax) {latMax = featurePts[i].latitude();}
        if (currLat < latMin) {latMin = featurePts[i].latitude();}
    }
    double latAvg = ((latMin * kDegreeToRadian) + latMax * kDegreeToRadian)/2.0;
    
    // Sum up all terms in numerator of equation
    double sumPoints = 0;
    for (int i = 0; i < numPoints; i++) {
        sumPoints = sumPoints + areaTerm(feature_id, i, latAvg);
    }

    // Divide by 2 to complete equation and return
    sumPoints = sumPoints/2.0;
    if (sumPoints < 0) {return sumPoints*-1;}
    return sumPoints;
}

IntersectionIdx findClosestIntersection(LatLon my_position){
    // Iterates through all intersections and returns closest by using findDistanceBetweenTwoPoints
    IntersectionIdx closestIdx = 0;
    double currDist = findDistanceBetweenTwoPoints(my_position, getIntersectionPosition(0));
    for (int i=0; i<getNumIntersections(); i++) {
        if (findDistanceBetweenTwoPoints(my_position, getIntersectionPosition(i)) < currDist) {
            currDist = findDistanceBetweenTwoPoints(my_position, getIntersectionPosition(i));
            closestIdx = i;
        }
    }
    return closestIdx;
}

double findStreetLength(StreetIdx street_id){
    // Queries the allStreetLengths vector 
    return allStreetLengths[street_id];
}

std::vector<IntersectionIdx> findIntersectionsOfStreet(StreetIdx street_id){
    // Queries the AllIntersectionsOfStreets vector
    std::vector<int> inters(AllIntersectionsOfStreets[street_id].begin(), AllIntersectionsOfStreets[street_id].end()); 
    return inters;
}

std::vector<StreetSegmentIdx> findStreetSegmentsOfIntersection(IntersectionIdx intersection_id)
{
    return intersectionSegments[intersection_id];
}

std::vector<IntersectionIdx> findAdjacentIntersections(IntersectionIdx intersection_id){

    //Returning vector
    std::vector<IntersectionIdx> listOfAdjacentIntersections; 

    //Vector of all connectingStreetSegments to the given 'intersection_id' 
    std::vector<StreetSegmentIdx> connectingStreetSegments = findStreetSegmentsOfIntersection(intersection_id);

    StreetSegmentInfo segInfo;

    //Get street segment info for each connectingStreetSegment 
    for(int i = 0; i < connectingStreetSegments.size(); i++){
        segInfo = getStreetSegmentInfo(connectingStreetSegments[i]);

        //Checking for cul de sacs
        if((segInfo.oneWay) && segInfo.from == intersection_id){
            //Duplicate Checking
            if(find(listOfAdjacentIntersections.begin(), listOfAdjacentIntersections.end(), segInfo.to) == listOfAdjacentIntersections.end()){
                //Storing the intersection 
                listOfAdjacentIntersections.push_back(segInfo.to); 
            }
        }
        //Check for one ways
        else if(segInfo.to == intersection_id && !segInfo.oneWay){
            //Duplicate Checking
            if(find(listOfAdjacentIntersections.begin(), listOfAdjacentIntersections.end(), segInfo.from) == listOfAdjacentIntersections.end()){
                //Storing the intersection 
                listOfAdjacentIntersections.push_back(segInfo.from); 
            } 
        }
        else if(segInfo.from == intersection_id && !segInfo.oneWay){
            //Duplicate Checking
            if(find(listOfAdjacentIntersections.begin(), listOfAdjacentIntersections.end(), segInfo.to) == listOfAdjacentIntersections.end()){
                //Storing the intersection 
                listOfAdjacentIntersections.push_back(segInfo.to); 
            }        
        }
    }
    return listOfAdjacentIntersections;
}

std::vector<IntersectionIdx> findIntersectionsOfTwoStreets(StreetIdx street_id1, StreetIdx street_id2){

    //Returning vector
    std::vector<IntersectionIdx> allInterOfTwoStreets; 

    //Retrieving all the intersections of street_id1 and street_id2
    std::vector<IntersectionIdx> interOfStreet1 = findIntersectionsOfStreet(street_id1);

    std::vector<IntersectionIdx> interOfStreet2 = findIntersectionsOfStreet(street_id2);

    //Find matching intersection of both streets
    for(int i = 0; i < interOfStreet1.size(); i++){

        for(int j = 0; j < interOfStreet2.size(); j++){

            //If match found; store intersection
            if(interOfStreet1[i] == interOfStreet2[j]){
                allInterOfTwoStreets.push_back(interOfStreet1[i]);
            }
        }
    }
    return allInterOfTwoStreets;
}

double findStreetSegmentTravelTime(StreetSegmentIdx street_segment_id){

    //Returning variable
    double travelTime;

    double streetSegLength = findStreetSegmentLength(street_segment_id);

    //Using StreetsDatabaseAPI.h to get speedLimit of street segment
    double speedLim = allStreetSegments[street_segment_id].speedLimit;

    //Total segment travel time
    travelTime = (streetSegLength / speedLim);

    return travelTime;
}

double findStreetSegmentLength(StreetSegmentIdx street_segment_id)
{
    return SegmentLengthStorage[street_segment_id];
}

double findDistanceBetweenTwoPoints(LatLon point_1, LatLon point_2){

    //Returning variable
    double distanceBwPoints;
    
    double lon1, lat1, x1, y1; 
    double lon2, lat2, x2, y2; 

    //Accessing lat and lon of both points
    lat1 = point_1.latitude();
    lon1 = point_1.longitude();
    
    lat2 = point_2.latitude();
    lon2 = point_2.longitude();
    
    //Converting degrees to radians
    lat1 = lat1*kDegreeToRadian;
    lon1 = lon1*kDegreeToRadian;
    lat2 = lat2*kDegreeToRadian;
    lon2 = lon2*kDegreeToRadian;


    double lat_avg = ((lat1+lat2)/2);

    x1 = (kEarthRadiusInMeters * lon1 * cos(lat_avg));
    y1 = (kEarthRadiusInMeters * lat1);

    x2 = (kEarthRadiusInMeters * lon2 * cos(lat_avg));
    y2 = (kEarthRadiusInMeters * lat2);
    
    //Pythagoras thm to compute the distance
    double ySquared = (y2 - y1) * (y2 - y1); 
    double xSquared = (x2 - x1) * (x2 - x1);

    distanceBwPoints = sqrt(ySquared + xSquared);
    
    return distanceBwPoints;
}

void loadAllPOIStruct() {

    allPOIs.resize(getNumPointsOfInterest());

    for(int poi = 0; poi < getNumPointsOfInterest(); poi++){

        POIinfo curr;
        curr.name = getPOIName(poi);
        curr.type = getPOIType(poi);
        curr.position = LatLonToPoint2D(getPOIPosition(poi), worldLatAvg);
        allPOIs[poi] = curr; //push back curr into our allPOI vector
    }
}

POIIdx findClosestPOI(LatLon my_position, std::string POItype){

    //Returning variable
    POIIdx POIIndex = 0;

    double currDist = std::numeric_limits<double>::infinity(); 

    //loop through ALL the point of interests for matching POItype(s)
    for(int i = 0; i < getNumPointsOfInterest(); i++){
        //If found a matching type, get POI position and check if closest distance
        if(getPOIType(i) == POItype){ 
            if(findDistanceBetweenTwoPoints(getPOIPosition(i), my_position) < currDist){
                //Update currDist
                currDist = findDistanceBetweenTwoPoints(getPOIPosition(i), my_position);
                POIIndex = i;
            }
        }
    }
    return POIIndex;
}

std::string getOSMNodeTagValue (OSMID OSMid, std::string key){

    //Returning string
    std::string nodeTagValue; 
    
    //Node unordered map
    const OSMEntity* node = osm_node_map.at(OSMid);

    //Loop through every tag to find a match
    for (int i = 0; i < getTagCount(node); i++){

        if (getTagPair(node, i).first == key) {
            //If found; assign 'value' to return variable
            nodeTagValue = getTagPair(node, i).second;
        }
    }
    return nodeTagValue;
}

std::vector<StreetIdx> findStreetIdsFromPartialStreetName(std::string street_prefix)
{

    std::vector<StreetIdx> partialNameStreets;

    //temp name storer to remove white spaces
    auto filterPrefix = std::remove((street_prefix).begin(), street_prefix.end(), ' ');

    //fix street ID whitespaces + make all letter lowercase
    street_prefix.erase(filterPrefix, street_prefix.end());
    transform(street_prefix.begin(), street_prefix.end(), street_prefix.begin(), ::tolower);

    //std::cout<<street_prefix<< std::endl;

    //int lengthOfPrefix = street_prefix.length();

    //starting point for iterating through map
    auto iterateMap = streetIdsFromNames.lower_bound(street_prefix);

    //if prefixes not found, return empty
    if (street_prefix.empty() == true)
    {

        return partialNameStreets;
    }

    //check if street name exists from prefix//
    if (!stringCompare(street_prefix, iterateMap->first))
    {
        return partialNameStreets;
    }

    //street of given prefix exists
    else
    {

        //iterate from the first alphabetical streetbame of given prefix
        for (auto iterator = iterateMap; iterator != streetIdsFromNames.end(); iterator++)
        {

            //if streetname becomes diff prefix, exit loop, else add street id to vector
            if (!stringCompare(street_prefix, iterator->first))
            {
                break;
            }

            partialNameStreets.push_back(iterator->second);
        }

        //return vector of streetnames
        return partialNameStreets;
    }
}


void loadAllIntersectionData() { 
    loadIntersectionPositions(); // load all intersection positions into a vector // good
    setLatLonWorldBounds();
    setupCoords();
}

void loadAllSegmentData() {
    allStreetSegments.resize(getNumStreetSegments());
    for (int i = 0; i < getNumStreetSegments(); i++){ // load all street segments
        StreetSegmentInfo info = getStreetSegmentInfo(i);
        allStreetSegments[i] = info;
    }
    loadAllIntersectionsOfStreets(); // load all intersections with streets //good
}

void threadGroup1(){
    std::vector<std::thread> allThreads;
    std::thread seg1(loadmapfSSL);
    std::thread seg2(loadAllSegmentData);

    seg1.join();
    seg2.join();

    allThreads.emplace_back(std::thread(loadNodes)); // load all OSMNodes into a map
    allThreads.emplace_back(std::thread(loadMapfSSOI));
    allThreads.emplace_back(std::thread(loadMapfStIFPS));
    allThreads.emplace_back(std::thread(loadStreetLengths)); //needs ssl and seg
    for (auto &th: allThreads) th.join();
}

void threadGroup2(){
    std::vector<std::thread> allThreads;
    loadAllIntersectionData();
    allThreads.emplace_back(std::thread(loadAllFeatures)); //III
    allThreads.emplace_back(std::thread(loadAllPOIStruct)); //III
    for (auto &th: allThreads) th.join();
}

bool loadMap(std::string map_streets_database_filename) {
    auto startTime = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> allThreads;
    filename = map_streets_database_filename;
    bool load_successful = false; //Indicates whether the map has loaded 
                                  //successfully

    std::cout << "loadMap: " << map_streets_database_filename << std::endl;
    //
    int len = map_streets_database_filename.length();

    if (!(map_streets_database_filename.substr(len-11,len-1) == "streets.bin")){return false;} // check for valid filename
    loadStreetsDatabaseBIN (map_streets_database_filename);
    loadOSMDatabaseBIN (map_streets_database_filename.replace(len-11,len-1, "osm.bin"));
    
    std::thread group1(threadGroup1);
    std::thread group2(threadGroup2);

    group1.join();
    group2.join();
    loadStreetSegStruct();
    
    load_successful = true; //Make sure this is updated to reflect whether
                            //loading the map succeeded or failed

    // std::cout << getOSMNodeTagValue(OSMID(26237443), "highway") << std::endl;
    auto endTime = std::chrono::high_resolution_clock::now();
    auto loadTime = std::chrono::duration_cast<std::chrono::duration<double>> (endTime-startTime);
    std::cout<< map_streets_database_filename << " load time: " << loadTime.count() << std::endl;
    return load_successful;
}

void closeMap() {
    //Clean-up your map related data structures here
    closeStreetDatabase();
    closeOSMDatabase();
    allStreetSegments.clear();
    allStreetLengths.clear();
    allFeaturePoints.clear();
    allFeaturePoints2d.clear();
    SSInfo.clear();
    level1StreetSegs.clear();
    level2StreetSegs.clear();
    level3StreetSegs.clear();
    intersections.clear();
    allIntersectionPositions.clear();
    intersectionSegments.clear();
    SegmentLengthStorage.clear();
    streetIdsFromNames.clear();
    osm_node_map.clear();
    featureInfo.clear();
    neighbourTravelTimes.clear();
}



