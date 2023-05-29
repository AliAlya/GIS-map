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

#include "m2.h"
#include "m1.h"
#include "m3.h"
#include "LatLon.h"
#include "DataStructures.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <string.h>
#include <sstream> 
#include <curl/curl.h>
#include <cstring>
#include "StreetsDatabaseAPI.h"
#include "ezgl/application.hpp"
#include "ezgl/graphics.hpp"
#include "ezgl/point.hpp"
#include "ezgl/color.hpp"
#include <unistd.h>               
#include <gtk/gtk.h>
#include <typeinfo>

std::vector <int> foundPath;
bool draw_path1 = false;
int prev_clicked;
int prev_click1;
int prev_click2;
double screenArea;
LatLon clickPos;
ezgl::point2d prev_poi;
void draw_main_canvas(ezgl::renderer * g);
void initial_setup(ezgl::application * application, bool new_window);
void act_on_mouse_press(ezgl::application * application, GdkEventButton * event, double x, double y);
void fill_features(ezgl::renderer * g);
void checkLOD(ezgl::renderer * g);
void display_POI(GtkButton *button, ezgl::application * app);
void toggleScheme(GtkSwitch*, gboolean state, ezgl::application* app);
void togglePathMode(GtkSwitch*, gboolean state, ezgl::application* app);
void weather_cbk(GtkWidget* widget, ezgl::application* app);
bool Relevant_Information_sorting(char* ApiCall);
void preLoad_API_Data();
void draw_city_names(GtkComboBoxText* self, ezgl::application* app);
int findInter(std::string street1, std::string street2);
void changeMap(GtkButton *button, gpointer box);
void draw_toggle(GtkToggleButton *togglebutton, ezgl::application* app);
void help_cbk(GtkWidget* /*widget*/, ezgl::application* app);
void path_Directions(std::vector<StreetSegmentIdx> directions_path);
void findIntersectionsPath(std::vector<StreetSegmentIdx> ssPath, int interStart);

GObject * citySelectBox;
GObject * streetsToggle;
GObject * featuresToggle;
GObject * featureNameToggle;
GObject * streetNameToggle;
GObject * citySelectButton;
GObject * helpButton;
GObject * darkModeSwitch;
GObject * poiSelectButton;
GObject * poiSelectBox;
GObject * weatherUpdateButton;
GObject * poiNameToggle;
GObject * poiToggle;
GObject * intersectionSearch1;
GObject * intersectionSearch2;
GObject * entry1;
GObject * entry2;
GObject * inter_entry1;
GObject * inter_entry2;
GObject * poiEntry;
GObject * intersectionButton;
GObject * clearButton;
GObject * pathModeToggle;
GObject * searchInter1;
GObject * searchInter2;
GObject * intersectionButton1;

void handleEntry (GtkEditable *widget, GtkComboBoxText *combo);
void doSearch(GtkButton *button, ezgl::application* app);
void clearButtonH(GtkButton *button, ezgl::application* app);
ezgl::application * appPtr;

//Global name of current city
std::string cityName;

struct drawSettings {
    bool streets;
    bool features;
    bool feature_names;
    bool street_names;
    bool poi;
    bool poi_names;
};
drawSettings draw_settings = {true, true, true, true, true, true};

double drawTime;
auto startDrawTime = std::chrono::high_resolution_clock::now();
bool getDrawTime = false;
bool poiGO = false;
int drawnFeatures;
int drawnSegs;
bool initial_setup_complete = false;
bool max_zoom_set = false;
ezgl::rectangle max_zoom;
double LODRatio;
int LODLevel;
double pixelToM;
ezgl::rectangle currView;
std::vector<int> interHighlights;
bool findPath = false; 
bool darkMode = false;
ezgl::point2d carLoc;
ezgl::point2d pinLoc;
int clickCount = 0;
void draw_intersections(ezgl::renderer * g);
void draw_street_line (ezgl::renderer * g, int streetID);
void draw_street_segments(ezgl::renderer * g);
void draw_street_names(ezgl::renderer * g, int segID, segment_info &OURinfo, double x_bound, ezgl::point2d text_loc, double x,
double y, ezgl::point2d point1, ezgl::point2d point2, bool curve);
void draw_feature(ezgl::renderer * g, ezgl::color col, int feature_id);
void draw_all_feature_names(ezgl::renderer * g);
void draw_feature_names(ezgl::renderer * g, int feature_id);
void draw_feature_text(ezgl::renderer * g, int feature_id);
void draw_POI_pins(ezgl::renderer *g);
void load_weather_API(double LatCity, double LonCity);
void draw_feature_line(ezgl::renderer * g, ezgl::color col, std::vector<ezgl::point2d> &featPoints);
void clearInterHighlights();
int getLineWidth(std::string highway);
void draw_car(ezgl::renderer *g);
void draw_pin(ezgl::renderer *g);
double getO (ezgl::point2d point1, ezgl::point2d point2, bool curve);

struct colour_scheme {
    ezgl::color UNKNOWN;
    ezgl::color PARK;
    ezgl::color BEACH;
    ezgl::color LAKE;
    ezgl::color RIVER;
    ezgl::color ISLAND;
    ezgl::color BUILDING;
    ezgl::color GREENSPACE;
    ezgl::color GOLFCOURSE;
    ezgl::color STREAM;
    ezgl::color GLACIER;   
    ezgl::color BACKGROUND;
    ezgl::color HIGHWAYS;
    ezgl::color STREET_FILL;
    ezgl::color STREET_TEXT;
    ezgl::color FEATURE_TEXT;
};

//Setting colour values for light and dark mode
colour_scheme light_mode = 
                        {   ezgl::color(0,0,0), // UNKNOWN
                            ezgl::color(179, 224, 166), // PARK
                            ezgl::color(246, 231, 180), // BEACH
                            ezgl::color(154, 204, 255), // LAKE
                            ezgl::color(154, 204, 255), // RIVER
                            ezgl::color(179, 224, 166), // ISLAND
                            ezgl::color(186, 182, 158), // BUILDING
                            ezgl::color(179, 224, 166), // GREENSPACE
                            ezgl::color(179, 224, 166), // GOLFCOURSE
                            ezgl::color(154, 204, 255), // STREAM
                            ezgl::color(176, 235, 255), // GLACIER
                            ezgl::color(255, 255, 255),  // BACKGROUND
                            ezgl::RED, // HIGHWAYS
                            ezgl::color(195, 204, 183),   // STREET_FILL
                            ezgl::color(61,61,61),   // STREET_TEXT
                            ezgl::color(15, 56, 146) };  // FEATURE_TEXT

colour_scheme dark_mode = 
                        {   ezgl::color(0,0,0),
                            ezgl::color(37, 84, 27),
                            ezgl::color(88, 66, 34),
                            ezgl::color(12, 38, 70),
                            ezgl::color(47, 71, 133),
                            ezgl::color(37, 84, 27),
                            ezgl::color(121, 106, 83),
                            ezgl::color(37, 84, 27),
                            ezgl::color(37, 84, 27),
                            ezgl::color(47, 71, 133),
                            ezgl::color(176, 235, 255),
                            ezgl::color(77, 80, 97), //dark grey
                            ezgl::color(102, 19, 0), 
                            ezgl::color(128, 131, 153), //street fill
                            ezgl::color(200,200,200), //street text 
                            ezgl::color(15, 56, 146) }; 


colour_scheme current_scheme = light_mode;


void drawMap() {
    // Set up the ezgl graphics window and hand control to it, as shown in the 
    // ezgl example program. 
    // This function will be called by both the unit tests (ece297exercise) 
    // and your main() function in main/src/main.cpp.
    // The unit tests always call loadMap() before calling this function
    // and call closeMap() after this function returns.
    // setupCoords();

    ezgl::application::settings settings;
    settings.main_ui_resource = "/homes/a/alyaser1/ece297/work/mapper/libstreetmap/resources/main.ui";
    // Note: the "main.ui" file has a GtkWindow called "MainWindow".
    settings.window_identifier = "MainWindow";

    // Note: the "main.ui" file has a GtkDrawingArea called "MainCanvas".
    settings.canvas_identifier = "MainCanvas";

    // Create our EZGL applications
    ezgl::application application(settings);
    startDrawTime = std::chrono::high_resolution_clock::now();
    application.add_canvas("MainCanvas", draw_main_canvas, initial_world, ezgl::color(77, 80, 97));  
    auto endDrawTime = std::chrono::high_resolution_clock::now(); 
    auto totalDrawTime = std::chrono::duration_cast<std::chrono::duration<double>> (endDrawTime-startDrawTime);
    drawTime = totalDrawTime.count();
    // g_signal_connect(G_OBJECT("MainCanvas"), "scroll_event", G_CALLBACK(LOD), &application);

    application.run(initial_setup, act_on_mouse_press, nullptr, nullptr);
}
// 1.0
// 0.0477
// 0.0062
// 0.0008
void checkLOD(ezgl::renderer * g) {
    currView = g->get_visible_world();
    LODRatio = currView.area() / (initial_world.area());
    screenArea = currView.area();
    pixelToM =  currView.width()/g->get_visible_screen().width();
    if (screenArea > 3.42e+9) LODLevel = 0;
    else if (screenArea <= 3.44e+9 && screenArea > 5.76e+7) LODLevel = 1;
    else if (screenArea <= 5.76e+7 && screenArea > 2.69e+6) LODLevel = 2;
    else if (screenArea <= 2.69e+6 && screenArea > 1.26e+5) LODLevel = 3;
    else if (screenArea <= 1.26e+5 && screenArea > 0.85e+5) LODLevel = 4;
    else if(screenArea <= 0.85e+5 && screenArea > 5.85e+3) LODLevel = 5;
    else LODLevel = 6;
}

void initial_setup(ezgl::application * application, bool /*new_window*/ ) {
    appPtr = application;
    const GdkRGBA grey = {0.83, 0.83, 0.83, 0.85};
    GObject * searchGrid = application->get_object("searches");
    gtk_widget_override_background_color ((GtkWidget *)searchGrid, gtk_widget_get_state_flags((GtkWidget *)searchGrid), &grey);

    GObject * poiSelectGrid = application->get_object("poiSelectGrid");
    gtk_widget_override_background_color ((GtkWidget *)poiSelectGrid, gtk_widget_get_state_flags((GtkWidget *)poiSelectGrid), &grey);

    GObject * findPathGrid = application ->get_object("findPathGrid");
    gtk_widget_override_background_color ((GtkWidget *)findPathGrid, gtk_widget_get_state_flags((GtkWidget *)findPathGrid), &grey);

    GObject * citySelectGrid = application->get_object("citySelectGrid");
    gtk_widget_override_background_color ((GtkWidget *)citySelectGrid, gtk_widget_get_state_flags((GtkWidget *)citySelectGrid), &grey);

    GObject * settingsExpander = application->get_object("settingsExpander");
    gtk_widget_override_background_color ((GtkWidget *)settingsExpander, gtk_widget_get_state_flags((GtkWidget *)settingsExpander), &grey);

    GObject * cityExpander = application->get_object("cityExpander");
    gtk_widget_override_background_color ((GtkWidget *)cityExpander, gtk_widget_get_state_flags((GtkWidget *)cityExpander), &grey);

    darkModeSwitch = application->get_object("DarkModeSwitch");
    g_signal_connect(darkModeSwitch, "state-set", G_CALLBACK(toggleScheme), application);
    
    weatherUpdateButton = application->get_object("weatherUpdateButton");
    g_signal_connect(weatherUpdateButton, "clicked", G_CALLBACK(weather_cbk), application);

    citySelectBox = application->get_object("citySelectBox");
    citySelectButton = application->get_object("citySelectButton");
    g_signal_connect(citySelectButton, "clicked", G_CALLBACK(changeMap), application);

    helpButton = application->get_object("helpButton");
    g_signal_connect(helpButton, "clicked", G_CALLBACK(help_cbk), application);
    streetsToggle = application->get_object("streetsToggle");
    g_signal_connect(streetsToggle, "toggled", G_CALLBACK(draw_toggle), application);

    featuresToggle = application->get_object("featuresToggle");
    g_signal_connect(featuresToggle, "toggled", G_CALLBACK(draw_toggle), application);

    featureNameToggle = application->get_object("featureNameToggle");
    g_signal_connect(featureNameToggle, "toggled", G_CALLBACK(draw_toggle), application);

    streetNameToggle = application->get_object("streetNameToggle");
    g_signal_connect(streetNameToggle, "toggled", G_CALLBACK(draw_toggle), application);

    poiNameToggle = application->get_object("poiNameToggle");
    g_signal_connect(poiNameToggle, "toggled", G_CALLBACK(draw_toggle), application);

    pathModeToggle = application->get_object("pathModeToggle");
    g_signal_connect(pathModeToggle, "state-set", G_CALLBACK(togglePathMode), application);

    poiToggle = application->get_object("poiToggle");
    g_signal_connect(poiToggle, "toggled", G_CALLBACK(draw_toggle), application);

    poiEntry = application->get_object("poiEntry");
    poiSelectBox = application->get_object("poiSelectBox");
    poiSelectButton = application->get_object("poiSelectButton");
    g_signal_connect(poiSelectButton, "clicked", G_CALLBACK(display_POI), application);

    clearButton = application->get_object("clearButton");
    g_signal_connect(clearButton, "clicked", G_CALLBACK(clearButtonH), application);

    intersectionButton = application->get_object("intersectionButton");
    g_signal_connect(intersectionButton, "clicked", G_CALLBACK(doSearch), application);

    entry1 = application->get_object("entry1");
    entry2 = application->get_object("entry2");
    intersectionSearch1 = application->get_object("intersectionSearch1");
    intersectionSearch2 = application->get_object("intersectionSearch2");
    g_signal_connect(entry1, "changed", G_CALLBACK(handleEntry), (GtkComboBoxText *)intersectionSearch1);
    g_signal_connect(entry2, "changed", G_CALLBACK(handleEntry), (GtkComboBoxText *)intersectionSearch2);

    inter_entry1 = application->get_object("inter_entry1");
    inter_entry2 = application->get_object("inter_entry2");
    searchInter2 = application->get_object("searchInter2");
    searchInter1 = application->get_object("searchInter1");
    g_signal_connect(inter_entry1, "changed", G_CALLBACK(handleEntry), (GtkComboBoxText *)searchInter1);
    g_signal_connect(inter_entry2, "changed", G_CALLBACK(handleEntry), (GtkComboBoxText *)searchInter2);

    intersectionButton1 = application->get_object("intersectionButton1");
    g_signal_connect(intersectionButton1, "clicked", G_CALLBACK(doSearch), application);

    //Default hide path finding widgets
    gtk_widget_hide(GTK_WIDGET(searchInter2));
    gtk_widget_hide(GTK_WIDGET(searchInter1));
    gtk_widget_hide(GTK_WIDGET(intersectionButton));

    initial_setup_complete = true;
}



void handleEntry (GtkEditable *widget, GtkComboBoxText *combo) {
    gtk_combo_box_text_remove_all((GtkComboBoxText *)combo);
    std::vector<std::string> noDupeNames;
    std::vector<std::string> display_inter;
    int c = 0;

    if(!findPath){
        std::string text = (std::string) gtk_editable_get_chars ((GtkEditable *)widget, 0, -1);
        if (text.length() > 3) { //only search after certain number of chars
            std::vector<int> firstSearchOptions = findStreetIdsFromPartialStreetName(text);
            for (int id: firstSearchOptions){
                std::string name = getStreetName(id);
                if (checkVectorDupe(noDupeNames, name)) {noDupeNames.push_back(name);}
                if (c >=5) break;
                c++;
            }
            for (std::string name: noDupeNames) { // only show 5 suggestions
                gtk_combo_box_text_append (combo, "-1", name.c_str());
            }
        }
    }
    else if(findPath){
        //Search Autocomplete for intersection searches 
        std::string text = (std::string) gtk_editable_get_chars ((GtkEditable *)widget, 0, -1);
        std::vector<int> firstSearchOptions;
        std::vector<int> secondSearchOptions;
        std::string street1, street2;
        std::string delimeter = "&";
        std::string display;
        std::string name1, name2;
        std::vector<IntersectionIdx> validInter;
        street1 = text.substr(0, text.find(delimeter));
        street2 = text.substr(text.find(delimeter) + 1);

        firstSearchOptions = findStreetIdsFromPartialStreetName(street1);
        secondSearchOptions = findStreetIdsFromPartialStreetName(street2);

        for (int id1: firstSearchOptions){
            for (int id2: secondSearchOptions){
                validInter = findIntersectionsOfTwoStreets(id1, id2);
                if(!validInter.empty() && street1 != "<unknown>" && street2 != "<unknown>"){
                    //check if intersection name alr exists and put it in display vector
                    for(int inter = 0; inter < validInter.size(); inter++){
                    std::string name = getIntersectionName(validInter[inter]);
                    if (checkVectorDupe(noDupeNames, name)) {noDupeNames.push_back(name);} //Displaying vector
                    }
                }
                if (c >=5) break;
                c++;
            }  
        } 
        int size = noDupeNames.size();
        for(int i = 0; i < std::min(6, size); i++){ //only show a few suggestions
            gtk_combo_box_text_append (combo, "-1", noDupeNames[i].c_str());
        }
    }
}

void clearInterHighlights(){
    //Sets all the highlighted intersections to false
    for (int interx: interHighlights) intersections[interx].highlight = false;
    interHighlights.clear();}

void clearButtonH([[maybe_unused]]GtkButton *button, ezgl::application* app) {
    //Clear button unhighlights intersections
    clearInterHighlights();
    app->refresh_drawing();
}


//Organize intersections of path in their correct order 
void findIntersectionsPath(std::vector<StreetSegmentIdx> ssPath, int interStart){




    searchPathIntersections.push_back(interStart);

   // StreetSegmentInfo streetSegStart  =  allStreetSegments[ssPath[0]];

    int intersectionIterator = interStart;

    for (auto ssBufferPath: ssPath){
        if(allStreetSegments[ssBufferPath].from == intersectionIterator){
            searchPathIntersections.push_back(allStreetSegments[ssBufferPath].to);

            intersectionIterator = allStreetSegments[ssBufferPath].to;

        }
        else{
            searchPathIntersections.push_back(allStreetSegments[ssBufferPath].from);
            intersectionIterator = allStreetSegments[ssBufferPath].from;
        }

    }
}
//cross product calculator, strings describing directions, intersection points on pathway
double angle_Of_Turn(ezgl::point2d TurnEnd1, ezgl::point2d TurnStart2, ezgl::point2d intersection);
//std::vector<IntersectionIdx> searchPathIntersections;

//store directions_path directions in the string vector -UserPromptedDirections
void path_Directions(std::vector<StreetSegmentIdx> directions_path){


   // GeneralActions.push_back(;
    std::vector<std::string> GeneralActions;


    GeneralActions.push_back("Turn right on ");
    GeneralActions.push_back("Turn left on ");
    GeneralActions.push_back("Continue straight ");
    GeneralActions.push_back("Take exit on");
    GeneralActions.push_back("Merge onto ");
    GeneralActions.push_back("Head N on ");
    GeneralActions.push_back("Head S on ");
    GeneralActions.push_back("Head E on ");
    GeneralActions.push_back("Head W on ");
    GeneralActions.push_back("Head NE on ");
    GeneralActions.push_back("Head NW on ");
    GeneralActions.push_back("Head SE on ");
    GeneralActions.push_back("Head SW on ");
    GeneralActions.push_back("Destination is ");
    GeneralActions.push_back("Turn right to ");
    GeneralActions.push_back("Turn left to ");



    double bearingAngle,bearingAngleX, bearingAngleY, turningAngle;
    int NumOfSegs;
    std::string firstDirection;
    StreetSegmentInfo streetSegStart  =  allStreetSegments[directions_path[0]];
    //StreetSegmentInfo streetSegDestination =  getStreetSegmentInfo(directions_path[1]);

    //x = cos(toRadians(la1))*sin(toRadians(la2)) – sin(toRadians(la1))*cos(toRadians(la2))*cos(toRadians(lo2-lo1))

    //bearingAngleX= (cos((streetSegStart.from.latitude())*kDegreeToRadian))*sin((streetSegStart.to.latitude())*kDegreeToRadian)- (sin((streetSegStart.from.latitude())*kDegreeToRadian))*cos((streetSegStart.to.latitude())*kDegreeToRadian)*cos(((streetSegStart.to.longitude())*kDegreeToRadian)-((streetSegStart.from.longitude())*kDegreeToRadian));
    bearingAngleX= intersections[searchPathIntersections[1]].xy_loc.x - intersections[searchPathIntersections[0]].xy_loc.x;
    //y= = sin(toRadians(lo2-lo1)) *  cos(toRadians(la2))
    //bearingAngleY= sin(((streetSegStart.to.longitude())*kDegreeToRadian)-((streetSegStart.from.longitude())*kDegreeToRadian)) * cos((streetSegStart.to.latitude())*kDegreeToRadian);

    bearingAngleY= intersections[searchPathIntersections[1]].xy_loc.y - intersections[searchPathIntersections[0]].xy_loc.y;

    bearingAngle = atan2(bearingAngleX,bearingAngleY);


    int intBufferForLength1;
    std::string stringBufferForLength1;

    intBufferForLength1 = findStreetSegmentLength(directions_path[0]);
    stringBufferForLength1 = std::to_string(intBufferForLength1);

    //Determine first heading direction in terms of cardnial scale
    if(bearingAngle <=20 || bearingAngle >= 340){
        firstDirection.append(GeneralActions[7] + SSInfo[directions_path[0]].name + " towards " + SSInfo[directions_path[1]].name + " for " + stringBufferForLength1 + " meters");
        UserPromptedDirections.push_back(firstDirection);
    }
    else if(bearingAngle >=20 && bearingAngle<= 70){
        firstDirection.append(GeneralActions[9] + getStreetName(directions_path[0]) + " towards " + getStreetName(directions_path[1])+ " for " + stringBufferForLength1 + " meters");
        UserPromptedDirections.push_back(firstDirection);    }
    else if(bearingAngle >=70 && bearingAngle<= 110){
        firstDirection.append(GeneralActions[5] + getStreetName(directions_path[0]) + " towards " + getStreetName(directions_path[1])+ " for " + stringBufferForLength1 + " meters");
        UserPromptedDirections.push_back(firstDirection);    }
    else if(bearingAngle >=110 && bearingAngle<= 160){
        firstDirection.append(GeneralActions[10] + getStreetName(directions_path[0]) + " towards " + getStreetName(directions_path[1])+ " for " + stringBufferForLength1 + " meters");
        UserPromptedDirections.push_back(firstDirection);    }
    else if(bearingAngle >=160 && bearingAngle<= 200){
        firstDirection.append(GeneralActions[8] + getStreetName(directions_path[0]) + " towards " + getStreetName(directions_path[1])+ " for " + stringBufferForLength1 + " meters");
        UserPromptedDirections.push_back(firstDirection);    }
    else if(bearingAngle >=200 && bearingAngle<= 250){
        firstDirection.append(GeneralActions[12] + getStreetName(directions_path[0]) + " towards " + getStreetName(directions_path[1])+ " for " + stringBufferForLength1 + " meters");
        UserPromptedDirections.push_back(firstDirection);    }
    else if(bearingAngle >=250 && bearingAngle<= 290){
        firstDirection.append(GeneralActions[6] + getStreetName(directions_path[0]) + " towards " + getStreetName(directions_path[1])+ " for " + stringBufferForLength1 + " meters");
        UserPromptedDirections.push_back(firstDirection);    }
    else{
        firstDirection.append(GeneralActions[11] + getStreetName(directions_path[0]) + " towards " + getStreetName(directions_path[1]) + " for " + stringBufferForLength1 + " meters" );
        UserPromptedDirections.push_back(firstDirection);    }


    NumOfSegs = directions_path.size();

    for(int parseSegs = 0; parseSegs<NumOfSegs-1; parseSegs++){
        //angle between lines
        turningAngle = 0;
        std::string bufferForUserPromptedDirections;

        std::string stringBufferForLength;
        int intBufferForLength;


        //check if street is different
        if(SSInfo[directions_path[parseSegs]].name != SSInfo[directions_path[parseSegs+1]].name){
            //check if current road is highway or regular road
            if(SSInfo[directions_path[parseSegs]].highway != "motorway" && SSInfo[directions_path[parseSegs]].highway != "motorway_link" && SSInfo[directions_path[parseSegs]].highway != "trunk" && SSInfo[directions_path[parseSegs]].highway != "trunk_link"){
                //if upcoming road is not highway and current is not highway, make regular 'left, right' announcements
                if(SSInfo[directions_path[parseSegs+1]].highway != "motorway" && SSInfo[directions_path[parseSegs+1]].highway != "motorway_link" && SSInfo[directions_path[parseSegs+1]].highway != "trunk" && SSInfo[directions_path[parseSegs+1]].highway != "trunk_link"){

                    ezgl::point2d Turn1, adjoining, Turn2;
                    adjoining = intersections[searchPathIntersections[parseSegs+1]].xy_loc;

                    //check orientation of street, then direction of turn
                    if(SSInfo[directions_path[parseSegs]].from == intersections[searchPathIntersections[parseSegs+1]].xy_loc){
                        if(SSInfo[directions_path[parseSegs]].curvePoints.empty()){
                            Turn1 = SSInfo[directions_path[parseSegs]].to;
                        }
                        else{
                            Turn1 = SSInfo[directions_path[parseSegs]].curvePoints[0];
                        }

                    }
                    else{
                        if(SSInfo[directions_path[parseSegs]].curvePoints.empty()){
                            Turn1 = SSInfo[directions_path[parseSegs]].from;
                        }
                        else{
                            Turn1 = SSInfo[directions_path[parseSegs]].curvePoints.back();
                        }
                    }

                    if(SSInfo[directions_path[parseSegs+1]].from == intersections[searchPathIntersections[parseSegs+1]].xy_loc){
                        if(SSInfo[directions_path[parseSegs+1]].curvePoints.empty()){
                            Turn2 = SSInfo[directions_path[parseSegs+1]].to;
                        }
                        else{
                            Turn2 = SSInfo[directions_path[parseSegs+1]].curvePoints[0];
                        }

                    }
                    else{
                        if(SSInfo[directions_path[parseSegs+1]].curvePoints.empty()){
                            Turn2 = SSInfo[directions_path[parseSegs+1]].from;
                        }
                        else{
                            Turn2 = SSInfo[directions_path[parseSegs+1]].curvePoints.back();
                        }

                    }

                    turningAngle = angle_Of_Turn(Turn1, Turn2, adjoining);

                    intBufferForLength = findStreetSegmentLength(directions_path[parseSegs]);
                    stringBufferForLength = std::to_string(intBufferForLength);

                    if(turningAngle>0){
                        bufferForUserPromptedDirections = GeneralActions[0];
                        bufferForUserPromptedDirections.append(intersections[searchPathIntersections[parseSegs+1]].inter_name + " in " + stringBufferForLength + " meters");

                        UserPromptedDirections.push_back(bufferForUserPromptedDirections);
                    }
                    else if(turningAngle<0){
                        bufferForUserPromptedDirections = GeneralActions[1];
                        bufferForUserPromptedDirections.append(intersections[searchPathIntersections[parseSegs+1]].inter_name + " in " + stringBufferForLength + " meters");

                        UserPromptedDirections.push_back(bufferForUserPromptedDirections);
                    }
                    else{
                        bufferForUserPromptedDirections = GeneralActions[2];
                        bufferForUserPromptedDirections.append(intersections[searchPathIntersections[parseSegs+1]].inter_name + " in " + stringBufferForLength + " meters");

                        UserPromptedDirections.push_back(bufferForUserPromptedDirections);
                    }


                }
                    //if going from street to highway
                else{
                    bufferForUserPromptedDirections = GeneralActions[4];
                    bufferForUserPromptedDirections.append(SSInfo[directions_path[parseSegs+1]].name + " in " + stringBufferForLength + " meters");

                    UserPromptedDirections.push_back(bufferForUserPromptedDirections);

                }
            }
                //if going from highway to street
            else{
                bufferForUserPromptedDirections = GeneralActions[3];
                bufferForUserPromptedDirections.append(SSInfo[directions_path[parseSegs+1]].name + " in " + stringBufferForLength + " meters");

                UserPromptedDirections.push_back(bufferForUserPromptedDirections);
            }
        }
    }
    UserPromptedDirections.push_back(" to arrive at destination");

}


//cross product of 2 lines
double angle_Of_Turn(ezgl::point2d TurnEnd1, ezgl::point2d TurnStart2, ezgl::point2d intersection){

    ezgl::point2d line1, line2;
    double cross;

    // line1 = sqrt((intersection.x - TurnEnd1.x)*(intersection.x - TurnEnd1.x) + (intersection.y - TurnEnd1.y)*(intersection.y - TurnEnd1.y));
    // line2 = sqrt((TurnStart2.x - intersection.x)*(TurnStart2.x - intersection.x) + (TurnStart2.y - intersection.y)*(TurnStart2.y - intersection.y));
    // cross = line1*line2

    line1.x =  (intersection.x - TurnEnd1.x);
    line1.y = (intersection.y - TurnEnd1.y);
    line2.x = (TurnStart2.x - intersection.x);
    line2.y = (TurnStart2.y - intersection.y);

    cross = (line1.x*line2.y) - (line2.x*line1.y);

    return cross;

}


void doSearch([[maybe_unused]]GtkButton *button, [[maybe_unused]]ezgl::application* app) {

    //Finding intersections given street names
    if(!findPath){
        clearInterHighlights();
        std::string street1, street2;
        street1 = (std::string)gtk_editable_get_chars ((GtkEditable *)entry1, 0, -1);
        street2 = (std::string)gtk_editable_get_chars ((GtkEditable *)entry2, 0, -1);
        int found_inter = findInter(street1, street2);
        if(found_inter == 0){
            //Means no intersection was found 
            char Error[6] = "Error";
            char message[40] = {0};
            strcat(message, "\n");
            strcat(message, "\t No intersection found. \t");
            strcat(message, "\n");
            app->create_popup_message(Error, message);
        }
    }
    //Find path mode: parse through input and find intersections
    else if(findPath){

        //Unhighlight clicked intersections
        intersections[prev_click1].highlight = false; 
        intersections[prev_click2].highlight = false;

        //Determine the two intersections for path finding
        std::string intersection1, intersection2; 
        intersection1 = (std::string)gtk_editable_get_chars ((GtkEditable *)inter_entry1, 0, -1);
        intersection2 = (std::string)gtk_editable_get_chars ((GtkEditable *)inter_entry2, 0, -1);

        //Parse the inputs to get individual street names 
        std::stringstream iss(intersection1); 
        std::string street1, street2, street3, street4; 
        std::string delimeter = "&"; 
        street1 = intersection1.substr(0, intersection1.find(delimeter));  
        street2 = intersection1.substr(intersection1.find(delimeter) +1); 

        //Do the same thing for intersection 2 
        street3 = intersection2.substr(0, intersection2.find(delimeter)); 
        street4 = intersection2.substr(intersection2.find(delimeter) +1); 

        //Determine intersections of streets
        int inter1 = findInter(street1, street2);
        int inter2 = findInter(street3, street4);
        if(inter1 == 0 || inter2 == 0){
            //Means nothing entered for intersection 1
            char Error[6] = "Error";
            char message[70] = {0};
            strcat(message, "\n");
            strcat(message, "\t Please enter 2 valid intersections\t");
            strcat(message, "\n");
            app->create_popup_message(Error, message);
        }

        prev_click1 = inter1;
        prev_click2 = inter2;

        //Locations of the pin icons
        carLoc = LatLonToPoint2D(getIntersectionPosition(inter1), worldLatAvg);
        pinLoc = LatLonToPoint2D(getIntersectionPosition(inter2), worldLatAvg);
        app->refresh_drawing();

        //Call shuja findPath here. inter1 and inter2 are intersection ids 

        foundPath = findPathBetweenIntersections(std::make_pair(inter1, inter2), 30.0);

        findIntersectionsPath(foundPath, inter1);

        path_Directions(foundPath);

        GtkWidget *DirText = (GtkWidget *) app->get_object("DirText");
        GtkWidget *DirectionsScroll = (GtkWidget *) app->get_object("DirectionsScroll");
        GtkTextBuffer *directionsBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(DirText));

        gtk_widget_show(DirText);
        gtk_widget_show(DirectionsScroll);
        
        //reset after each search
        gtk_text_buffer_set_text(directionsBuffer, " ", -1);

        for(int instructionsBuffer =0;  instructionsBuffer < UserPromptedDirections.size(); instructionsBuffer++ ){
            gtk_text_buffer_insert_at_cursor(directionsBuffer, UserPromptedDirections[instructionsBuffer].c_str(), -1);
            if(instructionsBuffer != UserPromptedDirections.size()-2)
            gtk_text_buffer_insert_at_cursor(directionsBuffer, "\n" , -1);
            gtk_text_buffer_insert_at_cursor(directionsBuffer, "\n" , -1);
        }

        UserPromptedDirections.clear();




        app->refresh_drawing();

    }
}
 
int findInter(std::string street1, std::string street2){
    std::vector <int> id1, id2;
    //Returning variable
    int foundInter = 0;
    auto filterStreetName = std::remove(street1.begin(), street1.end(), ' ');
    street1.erase(filterStreetName, street1.end());
    transform(street1.begin(), street1.end(), street1.begin(), ::tolower);

    auto filterStreetName2 = std::remove(street2.begin(), street2.end(), ' ');
    street2.erase(filterStreetName2, street2.end());
    transform(street2.begin(), street2.end(), street2.begin(), ::tolower);

    for (auto itr = streetIdsFromNames.begin(); itr != streetIdsFromNames.end(); itr++){
        if (itr -> first == street1) id1.push_back(itr -> second);
        if (itr -> first == street2) id2.push_back(itr -> second);
    }
    
    for (int idx1: id1) {
        for (int idx2: id2) {
            for (int interx: findIntersectionsOfTwoStreets(idx1, idx2)){
                interHighlights.push_back(interx);
                intersections[interx].highlight = true; //for when findPath is off do this
                foundInter = interx;
            }
        }
    }
    appPtr->refresh_drawing();
    return foundInter;
}

void togglePathMode(GtkSwitch*, gboolean state, ezgl::application* app){
    //Hide or show certain widgets if path finding mode is enabled
    if(state){
       findPath = true;
       //Show our findPath widgets
        gtk_widget_show(GTK_WIDGET(searchInter2));
        gtk_widget_show(GTK_WIDGET(searchInter1));
        gtk_widget_show(GTK_WIDGET(intersectionButton));
        //Hide street search widgets
        gtk_widget_hide(GTK_WIDGET(intersectionSearch1));
        gtk_widget_hide(GTK_WIDGET(intersectionSearch2));
        gtk_widget_hide(GTK_WIDGET(clearButton));
        gtk_widget_hide(GTK_WIDGET(intersectionButton1));

    }
    else{
        findPath = false;
        //Hide our findPath widgets
        gtk_widget_hide(GTK_WIDGET(searchInter2));
        gtk_widget_hide(GTK_WIDGET(searchInter1));
        gtk_widget_hide(GTK_WIDGET(intersectionButton));

        GtkWidget *DirText = (GtkWidget *) app->get_object("DirText");
        GtkWidget *DirectionsScroll = (GtkWidget *) app->get_object("DirectionsScroll");
        GtkTextBuffer *directionsBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(DirText));

        gtk_widget_hide(DirText);
        gtk_widget_hide(DirectionsScroll);

        //Show our street search widgets
        gtk_widget_show(GTK_WIDGET(intersectionSearch1));
        gtk_widget_show(GTK_WIDGET(intersectionSearch2));
        gtk_widget_show(GTK_WIDGET(clearButton));
        gtk_widget_show(GTK_WIDGET(intersectionButton1));
    }
    app->refresh_drawing();
}

void toggleScheme(GtkSwitch*, gboolean state, ezgl::application* app) {
    //Change the colour scheme when button enabled
    if (state) {
        darkMode = true;
        current_scheme = dark_mode;
    }
    else 
        current_scheme = light_mode;
    app->refresh_drawing();
}

void draw_toggle(GtkToggleButton *togglebutton, ezgl::application* app) {
    auto widgetName = (std::string)gtk_widget_get_name ((GtkWidget *) togglebutton);
    if (widgetName == (std::string)"feature") {
        draw_settings.features = gtk_toggle_button_get_active(togglebutton);
    } 
    if (widgetName == (std::string)"featurename") {
        draw_settings.feature_names = gtk_toggle_button_get_active(togglebutton);
    } 
    if (widgetName == (std::string)"street") {
        draw_settings.streets = gtk_toggle_button_get_active(togglebutton);
    } 
    if (widgetName == (std::string)"streetname") {
        draw_settings.street_names = gtk_toggle_button_get_active(togglebutton);
    } 
    if (widgetName == (std::string)"icon") {
        draw_settings.poi = gtk_toggle_button_get_active(togglebutton);
    } 
    if (widgetName == (std::string)"poiname") {
        draw_settings.poi_names = gtk_toggle_button_get_active(togglebutton);
    } 
    app->refresh_drawing();
}

void changeMap([[maybe_unused]]GtkButton *button, [[maybe_unused]]gpointer app) {
    const gchar * id = gtk_combo_box_get_active_id ((GtkComboBox*)citySelectBox);
    std::string filename = "/cad2/ece297s/public/maps/" + (std::string) id + ".streets.bin";
    closeMap();
    max_zoom_set = false;
    loadMap(filename);
    cityName = id;
    setupCoords();
    appPtr->change_canvas_world_coordinates("MainCanvas", initial_world);

    appPtr->refresh_drawing();
}

void help_cbk(GtkWidget* /*widget*/, ezgl::application* app){
    //Function to determine what gets displayed in the help window

    char Help[5] = "Help";
    char message[420] = {0};
    strcat(message, "Switch on path mode for navigation view");
    strcat(message, "\n");
    strcat(message, "\n");
    strcat(message, "To use search bar for navigation: \n");
    strcat(message, " \t -> Enter: street name&street name to select 2 intersections \n");
    strcat(message, "\n");
    strcat(message, "To use your mouse for navigation: \n");
    strcat(message, " \t -> Click on a begin and end point");
    strcat(message, "\n");
    strcat(message, "\n");
    strcat(message, "To view the map of another city, navigate to: \n");
    strcat(message, " \t -> City select -> City -> GO");
    strcat(message, "\n");
    strcat(message, "\n");
    strcat(message, "To enable layers or night mode, navigate to: \n");
    strcat(message, " \t -> Settings \n");
    strcat(message, "\n");
    strcat(message, "To find the closest point of interest: \n");
    strcat(message, " \t -> Top Right");

    app->create_popup_message(Help, message);
}

void draw_path(ezgl::renderer *g, std::vector<int> segs) {
    for (int segID: segs) {
        segment_info info1 = SSInfo[segID];
        g->set_color(ezgl::BLUE);
        draw_street_line(g, segID);
    }
}

void draw_main_canvas(ezgl::renderer * g) {
    checkLOD(g);
    drawnFeatures = 0;
    drawnSegs = 0;
    if (true) {
        startDrawTime = std::chrono::high_resolution_clock::now();
    }
    g->set_color(current_scheme.BACKGROUND);
    g->fill_rectangle(initial_world);
    if (LODLevel >= 6) {
        zoom_out(appPtr->get_canvas("MainCanvas"), 5.0 / 3.0);
    }

    else if (screenArea > (initial_world.area()*2) && initial_setup_complete) {
        zoom_in(appPtr->get_canvas("MainCanvas"), 5.0 / 3.0);
    }
    
    else{
        if (draw_settings.features) fill_features(g);
        if (draw_settings.streets) draw_street_segments(g);
        draw_intersections(g);
        if(LODLevel >=2 && findPath){
            clearInterHighlights();
            draw_car(g);
            draw_pin(g);
        }
        if (draw_settings.poi) draw_POI_pins(g);
        if (draw_settings.feature_names) draw_all_feature_names(g);
        draw_path(g, foundPath);

    }

    if (true) {
        auto endDrawTime = std::chrono::high_resolution_clock::now();
        auto totalDrawTime = std::chrono::duration_cast<std::chrono::duration<double>> (endDrawTime-startDrawTime);
        drawTime = totalDrawTime.count();
        getDrawTime = false;
    }

    if (initial_setup_complete) {
        appPtr->update_message("LOD: " + std::to_string(LODLevel) + " | DrawnStreetSegments: " + std::to_string(drawnSegs) + " | DrawnFeatures: " + std::to_string(drawnFeatures) + " | FPS: " + std::to_string(1/(drawTime)) + "");
    }
    g->set_color(ezgl::color(0, 255, 0, 100));
    g->fill_arc(prev_poi, 10*pixelToM, 0.0, 360.0);
    g->set_color(current_scheme.STREET_TEXT);

}

int getLineWidth(std::string highway) { //Uses pixel to meter conversion to ensure roads are the same width at every zoom level
    if (LODLevel < 2) {                       
        if (highway == "motorway") return (round(30/pixelToM));//Road width in meters/pixelToM
        else if (highway == "motorway_link") return (round(25/pixelToM));
        else if (highway == "trunk") return (round(25/pixelToM));
        else if (highway == "trunk_link") return (round(20/pixelToM));
        else return 0;
    }
    else {
        if (highway == "motorway") return (round(30/pixelToM));
        else if (highway == "motorway_link") return (round(25/pixelToM));
        else if (highway == "trunk") return (round(25/pixelToM));
        else if (highway == "trunk_link") return (round(20/pixelToM));
        else if (highway == "primary" || highway == "primary_link") return (round(15/pixelToM));
        else if (highway == "secondary" || highway == "secondary_link") return (round(12/pixelToM));
        else if (highway == "tertiary" || highway == "tertiary_link") return (round(10/pixelToM));
        else if (highway == "residential" || highway == "living_street") return (round(7/pixelToM));
        else return (0);
    }
}

void draw_street_line (ezgl::renderer * g, int segID) {
    StreetSegmentInfo SDBinfo = allStreetSegments[segID]; //Info provided by StreetsDatabaseAPI
    segment_info OURinfo = SSInfo[segID]; //Segment_info struct created by us
    ezgl::point2d from = OURinfo.from;
    ezgl::point2d to = OURinfo.to;
    ezgl::point2d loc_text;
    std::string name = OURinfo.name; 
    double x_bound, x, y;
    bool curve = false;
    x_bound = sqrt(pow((OURinfo.from.x - OURinfo.to.x), 2) + pow((OURinfo.from.y - OURinfo.to.y), 2));
    int width = getLineWidth(OURinfo.highway);
    g->set_line_width(width);
    g->set_line_cap(ezgl::line_cap::round);

    if (SDBinfo.numCurvePoints == 0){
        curve = false;
        if (g->get_visible_world().contains(from) || g->get_visible_world().contains(to)){
            g->draw_line(from, to);
            drawnSegs++;
            //drawing street names 
            if(name != "<unknown>"){
                loc_text.x = (OURinfo.from.x + OURinfo.to.x)/2;
                loc_text.y = (OURinfo.from.y + OURinfo.to.y)/2;       
                //length in x and y direction
                x = (OURinfo.to.x - OURinfo.from.x);
                y = (OURinfo.to.y - OURinfo.from.y);                         
                if (draw_settings.street_names && LODLevel > 2)draw_street_names(g, segID, OURinfo, x_bound, loc_text, x, y, 
                OURinfo.from, OURinfo.to, curve);
            }
        }
    }
    else { //for streets with more than 1 curve point 
        int pointNum = 0;
        ezgl::point2d p1 = from;
        ezgl::point2d p2;
        ezgl::point2d point1;
        ezgl::point2d point2;
        double currDistance; 
        double maxDistance = 0;
        ezgl::point2d text_loc;
        curve = true;

        while (pointNum < SDBinfo.numCurvePoints) {
            p2 = OURinfo.curvePoints[pointNum];
            if (g->get_visible_world().contains(p1) || g->get_visible_world().contains(p2)) {
                g->draw_line(p1, p2);
                drawnSegs++;
            }
            //Compute MaxDistance between curve points of a street segment
            currDistance = sqrt(pow((p1.x - p2.x), 2) + pow((p1.y - p2.y), 2));
            if (currDistance > maxDistance) {
                maxDistance = currDistance;
                point1 = p1;
                point2 = p2;
            }            
            pointNum++;
            p1 = p2;
        }
        g->draw_line(p1, to);
        if(SDBinfo.numCurvePoints > 1 && name != "<unknown>" && draw_settings.street_names){
            //compute text location 
            text_loc.x = (point1.x + point2.x)/2;
            text_loc.y = (point1.y + point2.y)/2;
            x = (point1.x - point2.x);
            y = (point1.y - point2.y); 
            draw_street_names(g, segID, OURinfo, maxDistance, text_loc, x, y, point1, point2, curve);   
        }
    }
}

void draw_street_names(ezgl::renderer * g, [[maybe_unused]]int segID, segment_info &OURinfo, double x_bound, ezgl::point2d text_loc, 
                        double x, double y, ezgl::point2d point1, ezgl::point2d point2, bool curve){ 
   
    //The angle of the name
    double orientation;

    //Accounting for one ways
    if(OURinfo.oneWay){
        //Set orientation of arrow
        if(OURinfo.from.x > OURinfo.to.x)
            OURinfo.name = "<—  " + OURinfo.name;
        
        else if (OURinfo.from.x < OURinfo.to.x)
            OURinfo.name = OURinfo.name + "  —>";
    }

    //Determining Orientation of Street Names
    if(point1.y < point2.y && point1.x > point2.x){
        if(!curve)
            orientation = 180 + (atan2(y, x) * 180/ pi);
        else if(curve)
            orientation = (atan2(y, x) * 180/ pi);
    }
    else if(point1.y > point2.y && point1.x > point2.x){

    if(!curve)
        orientation = 180 + (atan2(y, x) * 180/ pi);
    else if(curve)
        orientation =  (atan2(y, x) * 180/ pi);
    }

    else if(point1.y < point2.y && point1.x < point2.x){
        if(!curve)
            orientation = (atan2(y, x) * 180/ pi);
        else if(curve)
            orientation = 180 + (atan2(y, x) * 180/ pi);
    }
    else if(point1.x < point2.x && point1.y > point2.y){
        if(!curve)
            orientation = (atan2(y, x) * 180/ pi);
        else if(curve)
            orientation = 180 + (atan2(y, x) * 180/ pi);
    }
    else
        orientation = atan2(y,x) * 180/ pi; 

    //Drawing All Street Names 
    g->set_text_rotation(orientation); 
    g->set_color(current_scheme.STREET_TEXT);

    if (cityName == "tehran_iran") g->format_font("DejaVuSans", ezgl::font_slant::normal, ezgl::font_weight::normal, 12);

    else if (cityName == "toronto_canada") g->format_font("monospace", ezgl::font_slant::normal, ezgl::font_weight::normal, 12);

    else g->format_font("NotoSansCJKJP", ezgl::font_slant::normal, ezgl::font_weight::normal, 12);

    g->draw_text(text_loc, OURinfo.name, x_bound, 100); 
    g->set_color(current_scheme.STREET_FILL);      
}

void draw_street_segments(ezgl::renderer * g) {

    g->set_line_width(0);
    g->set_color(current_scheme.STREET_FILL);    
    if (LODLevel > 2) {
        for (int segID: level3StreetSegs) {
            draw_street_line(g, segID);
        }
    }
    g->set_color(current_scheme.STREET_FILL);    
    if (LODLevel > 1) {
        for (int segID: level2StreetSegs) {
            draw_street_line(g, segID);
        }
    }
    g->set_color(current_scheme.STREET_FILL);    
    for (int segID: level1StreetSegs) {
        if (SSInfo[segID].highway == "motorway" || SSInfo[segID].highway == "motorway_link") {g->set_color(current_scheme.HIGHWAYS);}
        else {g->set_color(current_scheme.STREET_FILL);}
        draw_street_line(g, segID);
    }
}

void act_on_mouse_press(ezgl::application * application,[[maybe_unused]] GdkEventButton * event, double x, double y){
    int inter1 = 0;
    int inter2 = 0;
    //Find closest intersection
    if(!findPath){
        intersections[prev_click1].highlight = false; 
        intersections[prev_click2].highlight = false; 
        application->refresh_drawing(); //Force a redraw
        std::cout << "Click:\n x: " << x << " y: " << y << std::endl;
        std::vector<std::string> intersecting_streets;
        clickPos = point2dToLatLon({x, y});
        int closest_inter = findClosestIntersection(clickPos);
        poiGO = true;// Can register click
        std::cout << "Closest Intersection: " << intersections[closest_inter].inter_name <<std::endl;
        intersections[closest_inter].highlight = true;
        if(closest_inter != prev_clicked){ //Reset the last clicked intersection to false
            intersections[prev_clicked].highlight = false; 
        }  
    }
    //If path finding mode is enabled, register begin and end points upon user clicks
    else{
        //Unhighlight intersections entered in search bar
        intersections[prev_click1].highlight = false; 
        intersections[prev_click2].highlight = false; 

        if(clickCount == 0){
            //Store location of the clicked position in clickPos
            clickPos = point2dToLatLon({x, y}); //intersection1
            inter1 = findClosestIntersection(clickPos);
            //Drop a car icon at inter1
            std::cout << "Begin at: " << intersections[inter1].inter_name <<std::endl;
            prev_click1 = inter1;
            carLoc = LatLonToPoint2D(getIntersectionPosition(prev_click1), worldLatAvg);
            application->refresh_drawing(); //Force a redraw
            clickCount++;
        }
        //Register the second click 
        else if(clickCount == 1){

            clickPos = point2dToLatLon({x, y}); //intersection2
            inter2 = findClosestIntersection(clickPos);
            prev_click2 = inter2;

            if(inter1 == inter2){
                clickCount = 1;
            }
            else{
                //Drop a pin at destination
                std::cout << "Destination: " << intersections[inter2].inter_name <<std::endl;
                clickCount++;

                //call shuja function here with prev_click1 and inter2 as start and end points 
                carLoc = LatLonToPoint2D(getIntersectionPosition(prev_click1), worldLatAvg);
                pinLoc = LatLonToPoint2D(getIntersectionPosition(inter2), worldLatAvg);
                foundPath = findPathBetweenIntersections(std::make_pair(prev_click1, inter2), 30.0);
                draw_path1 = true;
                findIntersectionsPath(foundPath, prev_click1);

                path_Directions(foundPath);

                GtkWidget *DirText = (GtkWidget *) application->get_object("DirText");
                GtkWidget *DirectionsScroll = (GtkWidget *) application->get_object("DirectionsScroll");
                GtkTextBuffer *directionsBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(DirText));

                gtk_widget_show(DirText);
                gtk_widget_show(DirectionsScroll);
            
                //reset after each search
                gtk_text_buffer_set_text(directionsBuffer, " ", -1);

            for(int instructionsBuffer =0;  instructionsBuffer < UserPromptedDirections.size(); instructionsBuffer++ ){
                    gtk_text_buffer_insert_at_cursor(directionsBuffer, UserPromptedDirections[instructionsBuffer].c_str(), -1);
                    if(instructionsBuffer != UserPromptedDirections.size()-2){
                    gtk_text_buffer_insert_at_cursor(directionsBuffer, "\n" , -1);
                    gtk_text_buffer_insert_at_cursor(directionsBuffer, "\n" , -1);
                    }
            }
            UserPromptedDirections.clear();
    
                application->refresh_drawing(); //Force a redraw
            }
        }
        else if(clickCount == 2){
            intersections[prev_click2].highlight = false; 
            intersections[prev_click1].highlight = false; 
            clickCount = 0; //Reset clickCount
            //Reset pin and car loc
            carLoc = LatLonToPoint2D(getIntersectionPosition(0), worldLatAvg);
            pinLoc = LatLonToPoint2D(getIntersectionPosition(0), worldLatAvg);
            foundPath.clear();
            application->refresh_drawing(); //Force a redraw
        }
    }
    application->refresh_drawing(); //Force a redraw
}

void display_POI([[maybe_unused]]GtkButton *button, ezgl::application * app){

    if(poiGO){
        const gchar * id = gtk_combo_box_get_active_id ((GtkComboBox*)poiSelectBox);
        int closest_poi;
        closest_poi = findClosestPOI(clickPos, id);
            
        std::cout << "Closest " << id <<  " is: " << getPOIName(closest_poi) << std::endl;
        prev_poi = LatLonToPoint2D(getPOIPosition(closest_poi), worldLatAvg);
        app->refresh_drawing();
    }
    else{
        char Error[6] = "Error";
        char message[40] = {0};
        strcat(message, "\n");
        strcat(message, "\t Please click on an intersection to select it. \t");
        strcat(message, "\n");
        app->create_popup_message(Error, message);
    }
}

void draw_intersections(ezgl::renderer * g) {

    double radius = 10*pixelToM;
    double start_angle = 0.0;
    double end_angle = 360.0;

    for(int inter_id = 0; inter_id < intersections.size(); inter_id++){
       
        if(intersections[inter_id].highlight){
            prev_clicked = inter_id;
            g->set_color(ezgl::RED, 100); 
            g->fill_arc(LatLonToPoint2D(getIntersectionPosition(inter_id), worldLatAvg), radius, start_angle, end_angle);
        }  
    }
}

void draw_feature_line(ezgl::renderer * g, [[maybe_unused]]ezgl::color col, std::vector<ezgl::point2d> &featPoints) {
    int pointNum = 0;
    g->set_line_width(0);
    ezgl::point2d p1 = featPoints.front();
    ezgl::point2d p2;
    if (LODLevel > 0) {
        while (pointNum < featPoints.size()) {
            p2 = featPoints[pointNum];
            if (g->get_visible_world().contains(p1) || g->get_visible_world().contains(p2)) {
                g->draw_line(p1, p2);
            }
            pointNum++;
            p1 = p2;
        }
        g->draw_line(p1, featPoints.back());
    }
}

void draw_feature(ezgl::renderer * g, ezgl::color col, int feature_id){
    if (getNumFeaturePoints(feature_id) > 1){
        std::vector<ezgl::point2d> featPoints = allFeaturePoints2d[feature_id];
        g->set_color(col);
        if (featureInfo[feature_id].closed) {
            g->set_color(col);
            g->fill_poly(featPoints);
            drawnFeatures++;
        }
        else {
            draw_feature_line(g, col, featPoints);
        }
        featPoints.clear();
    }
}

void fill_features(ezgl::renderer * g){

    g->set_color(ezgl::color(228, 225, 210));

    //Draw feature order based on area
    for(int featureID: sortedAreas){
        bool within = checkRectangleOverlap(g->get_visible_world(), featureInfo[featureID].bounds);

        if (within) {
            FeatureType featType = featureInfo[featureID].type;
            switch(featType){

                case ISLAND:  
                    draw_feature(g, current_scheme.ISLAND, featureID); break;
                case PARK: case GREENSPACE: case GOLFCOURSE: 
                    draw_feature(g, current_scheme.PARK, featureID); break;
                case LAKE: case RIVER: 
                    draw_feature(g, current_scheme.LAKE, featureID); break;
                case STREAM: 
                    draw_feature(g, current_scheme.STREAM, featureID); break;
                case UNKNOWN: 
                    draw_feature(g, current_scheme.UNKNOWN, featureID); break;
                case BEACH: 
                    draw_feature(g, current_scheme.BEACH, featureID); break; 
                case BUILDING: if (LODLevel > 3) {
                        draw_feature(g, current_scheme.BUILDING, featureID);
                    } break;
                default: break;
            }
        }
    }
}

void draw_all_feature_names(ezgl::renderer * g){

    if (LODLevel > 2){
        for(int feature_id = 0; feature_id < allFeaturePoints.size(); feature_id++){
            bool within = checkRectangleOverlap(g->get_visible_world(), featureInfo[feature_id].bounds);
            if(within){
                draw_feature_names(g, feature_id);
            }
        }
    }
}

void draw_feature_names(ezgl::renderer * g, int feature_id) {
    if (getFeatureName(feature_id) != "<noname>"){
    
        if(LODLevel == 4){ //Only at building level, draw buildings
            if (featureInfo[feature_id].closed) 
                draw_feature_text(g, feature_id);
        }
        else{
            if (featureInfo[feature_id].closed && getFeatureType(feature_id) != 6) {
                draw_feature_text(g, feature_id);
            }
        } 
    }
}

void draw_feature_text(ezgl::renderer * g, int feature_id){

    ezgl::point2d centroid = featureInfo[feature_id].centroid;
    g->set_color(current_scheme.FEATURE_TEXT);
    g->format_font("monospace", ezgl::font_slant::normal, ezgl::font_weight::normal, 12);
    g->set_text_rotation(0); 
    g->draw_text(centroid, getFeatureName(feature_id), 100, 50);
}

void draw_POI_pins(ezgl::renderer *g){
    ezgl::surface *poiIcon /* = ezgl::renderer::load_png("/homes/a/alyaser1/ece297/work/mapper/restaurant.png")*/;
    ezgl::point2d poiXY;
    std::string path = "/homes/a/alyaser1/ece297/work/mapper/small_icons/";  
   // std::string path = "/homes/m/mobeenar/ece297/work/mapper/";  
    if(LODLevel == 5){

        //loop through all pois and draw associated pins
        for(int poiID = 0; poiID < getNumPointsOfInterest(); poiID++){
            POIinfo OURinfo = allPOIs[poiID];

            poiXY = OURinfo.position;
            if(g->get_visible_world().contains(poiXY)){
                if(OURinfo.type == "restaurant"){
                    poiIcon = ezgl::renderer::load_png((path + "restaurant.png").c_str());
                    g->draw_surface(poiIcon, poiXY, .75);
                    g->set_color(ezgl::DARK_SLATE_BLUE);         
                }

                else if(OURinfo.type == "bar")
                {
                    poiIcon = ezgl::renderer::load_png((path + "drinks.png").c_str());
                    g->draw_surface(poiIcon, poiXY, .75);
                    ezgl::renderer::free_surface(poiIcon);
                    g->set_color(ezgl::DARK_GREEN);
                }

                else if(OURinfo.type == "parking")
                {
                    poiIcon = ezgl::renderer::load_png((path + "parking.png").c_str());
                    g->draw_surface(poiIcon, poiXY, .75);
                    g->set_color(ezgl::MAGENTA);
                    ezgl::renderer::free_surface(poiIcon);
                }

                else if(OURinfo.type == "hospital")
                {
                    poiIcon = ezgl::renderer::load_png((path + "hospital.png").c_str());
                    g->draw_surface(poiIcon, poiXY, .75);
                    g->set_color(ezgl::RED);
                    ezgl::renderer::free_surface(poiIcon);
                }

                else if(OURinfo.type == "cafe")
                {
                    poiIcon = ezgl::renderer::load_png((path + "cafe.png").c_str());
                    g->draw_surface(poiIcon, poiXY, .75);
                    g->set_color(ezgl::DARK_GREEN);
                    ezgl::renderer::free_surface(poiIcon);
                }

                else if(OURinfo.type == "bank")
                {
                    poiIcon = ezgl::renderer::load_png((path + "money-icon.png").c_str());
                    g->draw_surface(poiIcon, poiXY, .75);
                    g->set_color(ezgl::DARK_GREEN);
                    ezgl::renderer::free_surface(poiIcon);
                }
                else
                    g->set_color(current_scheme.FEATURE_TEXT);
                
                //Determine location to place the text
                poiXY.y = poiXY.y + 5;
                //Draw POI name
                g->set_text_rotation(50); 
                g->format_font("monospace", ezgl::font_slant::normal, ezgl::font_weight::normal, 12);
                if (draw_settings.poi_names) g->draw_text(poiXY, OURinfo.name, 100, 100);    
            }
        }
    }
}

//Struct that will hold information about city conditions
typedef struct CityWeatherConditions{
    /*std::string url*/
    char *url = nullptr;
    unsigned int size = 0;
    char *response = nullptr;
} CityWeatherConditions;

//load API data and insert in CityWeatherConditions struct
static size_t write_data(void *buffer, size_t /*size*/, size_t nmemb, void *userp) {

    //save data into temporary struct;
    if (buffer && nmemb && userp) {
        CityWeatherConditions *tempComparator = (CityWeatherConditions *)userp;
        // Writes to struct passed in from main
        tempComparator->response = new char[nmemb + 1];
        strncpy(tempComparator->response, (char *)buffer, nmemb);
        tempComparator->response[nmemb] = '\0'; // NULL-terminate to avoid leaks

        tempComparator->size = nmemb;
    }
    return nmemb;
}

//Loads information from API given API city LatLons
void load_weather_API(double LatCity, double LonCity){

    //Make sure curl library is include
    CURLcode response = curl_global_init(CURL_GLOBAL_ALL);
    if (response != CURLE_OK) {
        std::cout << "ERROR: Unable to initialize libcurl" << std::endl;
        std::cout << curl_easy_strerror(response) << std::endl;
        return;
    }

    //Keep a monitoring handle to debug
    CURL *curlHandle = curl_easy_init();

    if ( !curlHandle ) {
        std::cout << "ERROR: Unable to get easy handle" << std::endl;
        return ;
    } 
    else {
        char errbuf[CURL_ERROR_SIZE] = {0};

        //load latlons into buffer to load api of area
        // std::cout << LatCity <<std::endl;
        // std::cout << LonCity <<std::endl;
        char LatCityBuffer[15];
        char LonCityBuffer[15]; //6
        sprintf(LatCityBuffer, "%lf", LatCity);
        sprintf(LonCityBuffer, "%lf", LonCity);

        //Concatenate weather api url components
        char ApiUrl[] = "https://api.openweathermap.org/data/2.5/weather?lat=";
        strcat(ApiUrl,LatCityBuffer);
        strcat(ApiUrl,"&lon=");

        strcat(ApiUrl,LonCityBuffer);
        strcat(ApiUrl,"&units=metric");
        strcat(ApiUrl,"&appid=bc4dfd7393d9a8cc1391fd479753a69e");

        CityWeatherConditions conditionsTemp;

        //Set up error handling
        response = curl_easy_setopt(curlHandle, CURLOPT_URL, ApiUrl);
        if (response == CURLE_OK) {
            response = curl_easy_setopt(curlHandle, CURLOPT_ERRORBUFFER, errbuf);
        }
        if (response == CURLE_OK) {
            response = curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, write_data);
        }

        //Custom callback
        if (response == CURLE_OK) {
            curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &conditionsTemp);
        }

        conditionsTemp.url = ApiUrl;

        //Check for error in transfer
        if (response != CURLE_OK) {
            std::cout << "ERROR: Unable to set libcurl option" << std::endl;
            std::cout << curl_easy_strerror(response) << std::endl;
        }
        else 
            response = curl_easy_perform(curlHandle);
        
        if (response == CURLE_OK) {
            Relevant_Information_sorting(conditionsTemp.response);
        } else {
            std::cout << "ERROR: res == " << response << std::endl;
            std::cout << errbuf << std::endl;
        }
        if (conditionsTemp.response != nullptr) {
            delete[]conditionsTemp.response;
        }
        curl_easy_cleanup(curlHandle);
        curlHandle = nullptr;
    }
    curl_global_cleanup();
    return;
}

//Store the weather, feels like, and wind speeds from API
bool Relevant_Information_sorting(char* ApiCall){
    
    char ApiCallBuffer[strlen(ApiCall)];
    strncpy(ApiCallBuffer,ApiCall, strlen(ApiCall));

    char tempWeather[20];
    memset(tempWeather,0,sizeof(tempWeather));
    char tempWindSpeeds[20];
    char tempFeelsLike[20];
    memset(tempFeelsLike,0,sizeof(tempFeelsLike));
    memset(tempWindSpeeds,0,sizeof(tempWindSpeeds));
    char tempSnowOrRain[20];
    memset(tempSnowOrRain,0,sizeof(tempSnowOrRain));

    int /*WCount =0*/ FLCount =0, WSCount =0;

    char *temp = strstr(ApiCallBuffer, "temp");
    temp = temp +6;

    int iterateResponse = 0;

    //iterate through API to find weather
    while(*temp != ','){
        tempWeather[iterateResponse] = *temp;
        iterateResponse++;
        temp++;
    }
    
    //Store it in correct form and assign to global variable
    std::string bufferWeather(tempWeather);
    bufferWeather += "\u00B0";
    weather = "Weather: " + bufferWeather + "C";


    //reset to measure feels like
    char *temp2 = strstr(ApiCallBuffer, "feels_like");
    temp2 = temp2 + 11;

    iterateResponse = 0;
    //Iterate through API to find weather feels_like
    while(*temp2 != ','){
        tempFeelsLike[iterateResponse] = *temp2;
        iterateResponse++;
        temp2++;
        FLCount++;

    }
    //Store it in correct form and assign to global variable
    std::string bufferFeels(tempFeelsLike);
    bufferFeels = bufferFeels.substr(0,FLCount);
    bufferFeels += "\u00B0";
    feelsLike = "Feels like: " + bufferFeels + "C";


    //Reset and iterate to find wind speeds
    temp = strstr(ApiCallBuffer, "wind");
    temp = temp + 15;
    iterateResponse = 0;

    //Iterate API to find wind speeds
    while(*temp != ','){
        tempWindSpeeds[iterateResponse] = *temp;
        iterateResponse++;
        temp++;
        WSCount++;
    }
    //Put it in correct form and assign to global variable
    std::string bufferSpeeds(tempWindSpeeds);
    bufferSpeeds= bufferSpeeds.substr(0,WSCount);
    bufferSpeeds += "m/s";
    windSpeeds = "Wind speeds: " + bufferSpeeds;

    //Check for wind and snow, store in global variable
    char *tempSnow = strstr(ApiCallBuffer,"snow");
    char *tempRain = strstr(ApiCallBuffer,"rain");
    if(tempSnow!= nullptr){
        snowOrRain = "Expect snow";
    }
    else if(tempRain!= nullptr){
        snowOrRain = "Expect rain";

    }
    else{
        snowOrRain = "No percipitation";
    }

    //If empty, something went wrong
    if(!windSpeeds.empty()  && !weather.empty() && !feelsLike.empty()){
        return true;
    }


    return false;
}

//preload the weather API when the map loads
void preLoad_API_Data(){

    double LonBuff, LatBuff;
    LonBuff = (worldMaxLon + worldMinLon)/2;
    LatBuff = (worldMaxLat + worldMinLat)/2;
    load_weather_API(LatBuff, LonBuff);
}

//Make weather pop up to display weather
void weather_cbk(GtkWidget* /*widget*/, ezgl::application* app){

    //Globally stored api data is extracted from the Relevant_Information_sorting() function
    char City[50]= "Toronto";
    if(!cityName.empty()){
    strcpy(City, cityName.c_str());
    }
    preLoad_API_Data();

    char bufferWE[150];
    strcat(bufferWE,(weather.c_str()));
    strcat(bufferWE,"\n");
    strcat(bufferWE,(feelsLike.c_str()));
    strcat(bufferWE,"\n");
    strcat(bufferWE,(windSpeeds.c_str()));
    strcat(bufferWE,"\n");
    strcat(bufferWE,(snowOrRain.c_str()));

    //Output the pop up weather info on click
    app->create_popup_message(City, bufferWE);
}

void draw_car(ezgl::renderer *g){
    //Draws the car pin at carLoc

    ezgl::surface *Car;
    std::string path = "/homes/a/alyaser1/ece297/work/mapper/small_icons/";
    if(darkMode == false){
        Car = ezgl::renderer::load_png((path + "FromCar.png").c_str());
        g->draw_surface(Car, carLoc, .75);
    }
    else{
        Car = ezgl::renderer::load_png((path + "FromCarNightMode.png").c_str());
        g->draw_surface(Car, carLoc, .75);
    }
    g->free_surface(Car);
}

void draw_pin(ezgl::renderer *g){
    //Draws the destination pin at pinLoc
  
    ezgl::surface *Pin;
    std::string path = "/homes/a/alyaser1/ece297/work/mapper/small_icons/";
    Pin = ezgl::renderer::load_png((path + "ToPin.png").c_str());
    g->draw_surface(Pin, pinLoc, .75);
    g->free_surface(Pin);
}