#include "m3.h"
#include "m1.h"
#include "DataStructures.h"
#include <unordered_set>
#include <iostream>
#include <algorithm>
#include <queue>
#include "StreetsDatabaseAPI.h"
#include <math.h>
#include "FindSimplePath.h"
#include "m4.h"

std::unordered_map<int, double> Fs;
double heuristic(IntersectionIdx start, IntersectionIdx end);
double travelTime(IntersectionIdx from, IntersectionIdx to);
//void path_Directions();

// Returns the time required to travel along the path specified, in seconds.
double computePathTravelTime(const std::vector<StreetSegmentIdx>& path, const double turn_penalty) {
    double time = 0;
    if (path.empty()) {return time;}                // if the path is empty, return 0

    StreetSegmentInfo currSegInfo = allStreetSegments[path.front()];
    StreetIdx currStreet = currSegInfo.streetID;
    for (StreetSegmentIdx segment: path) {              // iterate over every segment in the path
        currSegInfo = allStreetSegments[segment];
        time += findStreetSegmentTravelTime(segment);   // add the travel time of the current segment

        if (currStreet != currSegInfo.streetID) {
            time += turn_penalty;                       // if there is a change in streets (a turn), apply the turn penalty
            currStreet = currSegInfo.streetID;          // update the current street
        }
    }
    return time;
}

// returns the heuristic given 2 nodes
double heuristic(IntersectionIdx start, IntersectionIdx end) {
    intersection_info p1 = intersections[start];
    intersection_info p2 = intersections[end];
    double h = distanceBetweenP2D(p1.xy_loc, p2.xy_loc); // h = distance between nodes
    return h*0.8;                                        // return underestimate
    // return 0;
}

class sortByF {
    public:
    bool operator()(int i1, int i2) {
        return Fs[i1] > Fs[i2];
    }
};

std::vector<StreetSegmentIdx> findPathBetweenIntersections(const std::pair<IntersectionIdx, IntersectionIdx> intersect_ids, const double turn_penalty) {
    return findSimplePath(intersect_ids);
}




