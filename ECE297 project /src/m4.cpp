#include "m3.h"
#include "m1.h"
#include "DataStructures.h"
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <algorithm>
#include <queue>
#include "StreetsDatabaseAPI.h"
#include <math.h>
#include "FindSimplePath.h"
#include <boost/functional/hash.hpp>
#include <thread>
#include <set>
#include <type_traits>
#include "m4.h"

void createTimeMatrix([[maybe_unused]]double turn_penalty);
class position;
position getClosestPD(position currentPos);
position getClosestDepot(position currentPos);
position naivePD(position currentPos);
position naiveDepot(position currentPos);
std::vector<CourierSubPath> posToSubPath (std::vector<position> &positions);
enum posType {pickUp=0, dropOff, depot};
void updateRemainingDeliveries (IntersectionIdx id, posType updateType);
void swapRand(std::vector<position> &sub, const double turn_penalty);

std::unordered_map<std::pair<int, int>, double, boost::hash<std::pair<int, int>>> timeMatrix;
std::vector<int> allDests;
class position;
bool check_legal(std::vector<position> &all_positions);
std::vector<CourierSubPath> perturb(std::vector<CourierSubPath> sub);
std::vector<position> swapForPerturb(std::vector<position> &sub, const double turn_penalty);
double subPathTravelTime(std::vector<position> &sub, const double turn_penalty);


std::vector<std::string> posTypeText = {"pickUp", "dropOff", "depot"};
class position {
    public:
    IntersectionIdx intersection;
    posType type;      // type of intersection
    int deliveryId;    // the delivery it belongs to, -1 if depot
    bool legal;        // if it can be visited, always true for pickUp or depot
    IntersectionIdx dropItx;

    position (int deliveryIndex, int numDeliveries, bool drop, IntersectionIdx id, IntersectionIdx dropId=-1) {
        legal = true;
        intersection = id;
        dropItx = dropId;
        if (deliveryIndex > numDeliveries) {
            type = depot;
            deliveryId = -1;
        }
        else {
            deliveryId = deliveryIndex;
            if (drop) {
                type = dropOff;
                legal = false;
            }
            else {
                type = pickUp;
            }
        }
    }

    position() {
        intersection = -1;
        type = depot;
        deliveryId = -1;
        legal = false;
        dropItx = -1;
    }

    void display() {
        std::cout << "#" << deliveryId << ": " << intersection << " (" << posTypeText[type] << ") " << (type==pickUp ? ((std::string)"drop at: " + std::to_string(dropItx)) : (std::string)"pick from " + std::to_string(intersection)) << std::endl;
    }
};

class deliveryStatus {
    public:
    posType status = pickUp;
    IntersectionIdx pick, drop;
    deliveryStatus(IntersectionIdx p, IntersectionIdx d) {
        pick = p;
        drop = d;
    }
    void markPicked() {
        status = dropOff;
    }
};


std::unordered_map<int, deliveryStatus> remainingDeliveries; // delivery index: (posType, dropItx)
std::unordered_map<IntersectionIdx, position> posInfo;
std::unordered_map<int, std::map<double, position>> sortedTimeMatrix;
void createTimeMatrix([[maybe_unused]]double turn_penalty) {
    sortedTimeMatrix.clear();
    timeMatrix.clear();
    for (int start=0; start<allDests.size(); start++) {
        std::map<double, position> curRowMap;
        auto startPoint = allDests[start];
        for (int end=0; end<allDests.size(); end++) {
            auto endPoint = allDests[end];
            auto index = std::make_pair(startPoint, endPoint);
            if (startPoint == endPoint) {timeMatrix[index] = 0;}
            else{
                double distance = distanceBetweenP2D(intersections[startPoint].xy_loc, intersections[endPoint].xy_loc);
                timeMatrix[index] = distance;
                curRowMap.insert(std::make_pair(distance, posInfo.at(endPoint)));
            }
        }
        sortedTimeMatrix[startPoint] = curRowMap;
    }
}

struct checkDepot {
    bool operator()( const std::pair<double, position> rowElement) const { 
        return rowElement.second.type == depot; 
    }
};

position getClosestDepot(position currentPos) {
    std::map<double, position> currRow = sortedTimeMatrix[currentPos.intersection];
    auto it = std::find_if( currRow.begin(), currRow.end(), checkDepot());
    return it->second;
}

position naiveDepot (position currentPos) {
    std::map<double, position> currRow = sortedTimeMatrix[currentPos.intersection];
    double smallestD = 9999999;
    position closest;
    for (auto it = (currRow.begin()); it != currRow.end(); it++) {
        if ((it->second.type == depot)) {
            position candidate = it->second;
            double dist = distanceBetweenP2D(intersections[currentPos.intersection].xy_loc, intersections[candidate.intersection].xy_loc);
            if (dist<smallestD) {
                smallestD = dist;
                closest = candidate;
            }
        }
    }
    return closest;
}

struct checkLegalPD {
    bool operator()( const std::pair<double, position> rowElement) const { 
        position element = rowElement.second;
        if (element.type == depot) {return false;}              // never return depots
        int deliveryID = element.deliveryId;
        if (!remainingDeliveries.count(deliveryID)) {return false;} // if a completed delivery
        return element.type == remainingDeliveries.at(deliveryID).status;   // if delivery is ready for this point
    }
};

position getClosestPD (position currentPos) {
    std::map<double, position> currRow = sortedTimeMatrix[currentPos.intersection];
    auto it = std::find_if( currRow.begin(), currRow.end(), checkLegalPD());    // get closest pick/drop that is legal
    if (it == currRow.end()) {
        if (!remainingDeliveries.empty()) {
            return posInfo.at(remainingDeliveries.begin()->second.drop);
        }
    }
    return it->second;
}

position naivePD (position currentPos) {
    std::map<double, position> currRow = sortedTimeMatrix[currentPos.intersection];
    double smallestD = 9999999;
    position closest;
    for (auto it = (currRow.begin()); it != currRow.end(); it++) {
        if ((it->second.type == pickUp) || ((it->second.type == dropOff) && it->second.legal)){
            position candidate = it->second;
            double dist = distanceBetweenP2D(intersections[currentPos.intersection].xy_loc, intersections[candidate.intersection].xy_loc);
            if (dist<smallestD) {
                smallestD = dist;
                closest = candidate;
            }
        }
    }
    return closest;
}

std::vector<CourierSubPath> posToSubPath (std::vector<position> &positions) {
    std::vector<CourierSubPath> subPaths;
    for (int ind=0; ind<(positions.size()-1); ind++) {
        CourierSubPath sub;
        sub.start_intersection = positions[ind].intersection;
        sub.end_intersection = positions[ind+1].intersection;

        auto input = std::make_pair(sub.start_intersection, sub.end_intersection);
        sub.subpath = findPathBetweenIntersections(input, 0);

        subPaths.push_back(sub);
    }

    return subPaths;
}

void updateRemainingDeliveries (IntersectionIdx id, posType updateType) { // update all deliveries that get pickedup/droppedoff at intersection id
    std::vector<int> IDsToDelete;
    for (const auto& element: remainingDeliveries) {
        int deliveryID = element.first;
        deliveryStatus delivery = element.second;       // info of this delivery
        if (updateType == pickUp) {                     // if doing pickup
            if (delivery.pick == id) {                  // if this delivery is picked at id
                remainingDeliveries.at(deliveryID).markPicked();  // mark as picked
                // std::cout << deliveryID << ": picked: " << id << std::endl;
            }
        }
        if (updateType == dropOff) {   // if doing dropoff
            if (delivery.drop == id && delivery.status == dropOff) {  // if this delivery is dropped at id and it is ready
                IDsToDelete.push_back(deliveryID);  // mark as delivered
                // std::cout << deliveryID << ": dropped: " << id << std::endl;
            }
        }
    }

    for (auto deliveryID: IDsToDelete) {
        remainingDeliveries.erase(deliveryID); 
        // std::cout << "erasing " << deliveryID << std::endl;
    }
}

std::vector<CourierSubPath> travelingCourier(const std::vector<DeliveryInf>& deliveries, const std::vector<IntersectionIdx>& depots, const float turn_penalty){
    std::cout << "reached beginning of travelingCourier" <<std::endl;

    auto turnPenalty = (double) turn_penalty;
    remainingDeliveries.clear();
    posInfo.clear();
    // generating data
    int numDeliveries = deliveries.size();
    allDests.clear();
    for (int deliveryIndex=0; deliveryIndex<numDeliveries; deliveryIndex++) {
        IntersectionIdx pick = deliveries[deliveryIndex].pickUp;
        IntersectionIdx drop = deliveries[deliveryIndex].dropOff;

        // if (!checkVectorDupe(allDests, pick)) {
        //     std::cout << pick << " " << deliveryIndex << " DUPE PICK ";
        //     for (auto a: allDests) {if (a == pick) std::cout  << "|"<< pick;}
        //     std::cout << std::endl;
        // }
        // if (!checkVectorDupe(allDests, drop)) {
        //     std::cout << drop << " " << deliveryIndex << " DUPE DROP ";
        //     for (auto a: allDests) {if (a == drop) std::cout << "|" << drop;}
        //     std::cout << std::endl;
        // }

        allDests.push_back(pick); 
        posInfo.insert(std::make_pair(pick, position(deliveryIndex, numDeliveries, false, pick, drop)));

        allDests.push_back(drop);
        posInfo.insert(std::make_pair(drop, position(deliveryIndex, numDeliveries, true, drop)));
        remainingDeliveries.insert(std::make_pair(deliveryIndex, deliveryStatus(pick, drop)));  // delivery index: (posType, dropItx)
    }
    for (auto depot: depots) {
        allDests.push_back(depot);          //
        posInfo.insert(std::make_pair(depot, position(1, 0, true, depot)));
    }
    createTimeMatrix(turnPenalty);
    std::vector<position> solution;
    position currPos = posInfo.at(depots[0]);
    solution.push_back(currPos);
    while (!remainingDeliveries.empty()) {
        // std::cout << remainingDeliveries.size() << " left: " ;
        position nextPos = getClosestPD(currPos);
        if (nextPos.intersection == -1) {break;}
        updateRemainingDeliveries(nextPos.intersection, nextPos.type);

        if (nextPos.type == pickUp) {
            posInfo.at(nextPos.dropItx).legal = true;
        }

        currPos = nextPos;
        solution.push_back(currPos);
    }

    // for (auto pos: solution) {pos.display();}
    // std::cout << (check_legal(solution) ? "LEGAL" : "NOT LEGAL") << std::endl;
    std::vector<CourierSubPath> subPaths = posToSubPath(solution);
    // swapForPerturb(solution, turn_penalty);
    std::cout << "reached end of travelingCourier" <<std::endl;
    swapRand(solution, turn_penalty);
    

    return subPaths;
}

std::pair<int, int> genPair(int size) {
    int Ind1 = std::rand() % size;
    int Ind2 = std::rand() % size;
    while (Ind1 == Ind2) {
        Ind2 = std::rand() % size;
    }
    return std::make_pair(Ind1, Ind2);
}

void swapRand(std::vector<position> &sub, const double turn_penalty){
    std::vector<position> bestSolution(sub);
    std::unordered_set<std::pair<int, int>, boost::hash<std::pair<int, int>>> knownSwaps;

    double bestTime = subPathTravelTime(bestSolution, turn_penalty);
    bestSolution.erase(bestSolution.begin());
    int size = bestSolution.size();

    int permLimit = 0;
    for (int n=0; n<size; n++) {permLimit+=n;}
    std::srand(std::time(NULL));
    int Ind1, Ind2;

    int time=0;
    int perms=0;
    while (perms<permLimit) {
        auto swapP = genPair(size);
        time++;

        if (knownSwaps.count(swapP)) {
            // std::cout<< "known swap!" << std::endl;
            continue;
        }
        if (knownSwaps.count(std::make_pair(swapP.second, swapP.first))) {continue;}
        time++;
        perms++;

        knownSwaps.insert(std::make_pair(swapP.first, swapP.second));
        knownSwaps.insert(std::make_pair(swapP.second, swapP.first));
        std::swap(bestSolution[swapP.first], bestSolution[swapP.second]);
        auto candidateSol = bestSolution;

        if (!check_legal(bestSolution)) {
            std::swap(bestSolution[swapP.first], bestSolution[swapP.second]);
            time++;
            continue;
        }
        else {
            std::cout << "found legal perm!" << std::endl;
            std::cout <<swapP.first<< "<-->"<< swapP.second<< std::endl;
            double candidateTime = subPathTravelTime(bestSolution, turn_penalty);
            if (candidateTime < bestTime) {
                bestTime = candidateTime;
                std::cout << "found better sol!" << std::endl;
            }
            else {
                std::swap(bestSolution[swapP.first], bestSolution[swapP.second]);
            }
        }
        time++;
    }
    std::cout << perms << "/" << permLimit << " perms" << std::endl;
    // for (auto p: bestSolution) {std::cout << p.deliveryId << " ";}
    // std::cout << std::endl;
    // for (auto p: bestSolution) {std::cout << p.deliveryId << " ";}
    // std::cout << std::endl;
}

std::vector<position> swapForPerturb(std::vector<position> &sub, const double turn_penalty){
    std::cout << "reached beginning of swapForPerturb" <<std::endl;

    std::vector<position> subPlaceholder(sub); //store copy of original solution in subPlaceHolder
    double originalLength = subPathTravelTime(sub, turn_penalty);

    auto startTime = std::chrono::high_resolution_clock::now();
    double currTime = 0.0;
    std::vector<position> newPath(sub);
    bool samePath = true;
    std::unordered_set<std::pair<int, int>, boost::hash<std::pair<int, int>>> knownSwaps;



    while(currTime < 20) {
       // std::cout << "curr time is: " << currTime << std::endl;
        
        int numSubPaths = subPlaceholder.size();
        std::srand(std::time(NULL));

        // int randIndex1 = std::rand() % numSubPaths;
        // int randIndex2 = randIndex1 + 1;

        int randIndex1 = std::rand() % (numSubPaths-3) + 1;
        int randIndex2 = std::rand() %  (numSubPaths-2) + 1 ;

        if(randIndex2 >= numSubPaths - 3){
            randIndex2 = numSubPaths-2 ;
        }

        std::pair<int,int> swapValues, swapValuesRev;
        swapValues.first = randIndex1;
        swapValues.second = randIndex2;

        swapValuesRev.second = randIndex1;
        swapValuesRev.first = randIndex2;

        
        if(knownSwaps.count(swapValues)!=0 || knownSwaps.count(swapValuesRev)!=0 ){
            continue;
        }
        else{
            if(randIndex1 != randIndex2){
            knownSwaps.insert(swapValues);
                std::swap(subPlaceholder[randIndex1], subPlaceholder[randIndex2]);
                std::cout << "trying " << randIndex1 << " swapped with " << randIndex2 << std::endl;

                    subPlaceholder.erase(subPlaceholder.begin());
                    subPlaceholder.pop_back();

                if(check_legal(subPlaceholder)){

                    std::cout<<"entered if"<<"\n";

                    // position newEndDepot = getClosestDepot(subPlaceholder.end()[-2]);
                    // subPlaceholder.back()= newEndDepot;

                    double subLength = subPathTravelTime(subPlaceholder, turn_penalty);
                    if(subLength < originalLength){
                        std::cout << "found better path " <<std::endl;
                        newPath.assign(subPlaceholder.begin(), subPlaceholder.end()); 
                        originalLength = subLength;
                        knownSwaps.clear();
                    }
                    else if(subLength > originalLength){
                        std::cout << "did not find better path " <<std::endl;
                    }
                }
            }
        }
        auto endTime = std::chrono::high_resolution_clock::now();
                currTime = std::chrono::duration_cast<std::chrono::duration<double>> (endTime-startTime).count();

    }
    std::cout << "reached end of swapForPerturb" << std::endl;
    return newPath;
}

double subPathTravelTime(std::vector<position> &subA, const double turn_penalty){

    std::cout << "reached beginning of subPathTravelTime" <<std::endl;
    double length = 0.0;
    std::pair<IntersectionIdx, IntersectionIdx> intersectionsOfPath;
    int subpathSize = subA.size();

    for(int travelTime = 0; travelTime < subpathSize - 1; travelTime++){
        // std::cout << "entered loop" <<std::endl;
        intersectionsOfPath.first = subA[travelTime].intersection;
        intersectionsOfPath.second = subA[travelTime + 1].intersection;
        length += computePathTravelTime(findSimplePath(intersectionsOfPath), turn_penalty);
    }
    std::cout << "reached end of subPathTravelTime" <<std::endl;

    return length;
}

bool check_legal(std::vector<position> &all_positions){
  //  std::cout << "reached beginning of check_legal" <<std::endl;


    std::unordered_set<int> delivery_Ids;
    for(position pos: all_positions){
        if(pos.type == pickUp){
            delivery_Ids.insert(pos.deliveryId); 
        }
        else if(pos.type == dropOff){
           if(!delivery_Ids.count(pos.deliveryId)){return false;}
        }
    }
    return delivery_Ids.empty();
}

