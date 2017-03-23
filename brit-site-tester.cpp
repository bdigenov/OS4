//Brittany DiGenova
//OS Project 04 
//Site-Tester will check sites for presencs of keywords on a timer
//24 March 2017

#include <iostream>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <map>
#include <vector>
#include <cerrno>
#include <curl/curl.h>

using namespace std;

struct SEO {
    string site;
    map<string, int> search_count;
    time_t time;
};

//Declare Constants
int NUM_ARGS = 5;
int PERIOD_FETCH=180;
int NUM_FETCH=1;
int NUM_PARSE=1;
string SEARCH_FILE = "Search.txt";
string SITE_FILE = "Sites.txt";
vector<SEO> search_results;         //Vector of structs storing overall search info

//Curl Function to get response
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

//Declare Functions
void read_config(const char*);
int read_search_file(string, vector<string>&);
int read_site_file(string, vector<string> &);
bool valid_url(string);
string curl_to_string(string);
int word_appearances(string, string);

int main( int argc, char* argv[]) {
    //File Variables
    vector<string> searches;            //Stores the search strings
    vector<string> sites;               //Stores the search sites
    map<string,string> site_content;    //Stores a map of sites and the content

    //Read from config file if one is specified
    if (argc >= 2) {
        const char* file = argv[1];
        read_config(file); 
    }

    //Read search file 
    if (read_search_file(SEARCH_FILE, searches) == 1) return EXIT_FAILURE;

    //Read site file
    if (read_site_file(SITE_FILE, sites) == 1 ) return EXIT_FAILURE;
    
    //Update site_content dictionary with each site and a string of its content
    for (int i =0; i < sites.size(); i++) {
        site_content.insert(pair<string,string>(sites[i],curl_to_string(sites[i])));
    }

    for (int i =0; i < sites.size(); i++) {
        search_results.push_back(SEO());    //push new struct into vector
        search_results[i].site = sites[i];
        for (int j = 0; j < searches.size(); j++) {
            //For every keyword, create a new map key,value store for that word and # of times it appears in site sites[i]
            search_results[i].search_count.insert(pair<string,int>(searches[j],word_appearances(searches[j],site_content[sites[i]])));
        }
    }
    cout << site_content[sites[1]] << endl;
    cout << search_results[1].site << endl;
    cout << site_content[search_results[0].site] << endl;
    cout << "FOR " << search_results[0].site << " PHRASE " << searches[0] << " APPEARS " << search_results[0].search_count[searches[0]] << " TIMES" << endl;

    return 0;
}


//NOTE THIS FUNCTION MESSES UP THE STRINGS!!! FOR SEARCH AND SITE
void read_config(const char* file) {
    //Set keywords to look for and initialize filestream/variables
    const char* args[] = {"PERIOD_FETCH", "NUM_FETCH", "NUM_PARSE", "SEARCH_FILE", "SITE_FILE"};
    fstream fp;
    char* values[10]; 
    char* token;
    char line[256];

    //Process inputs from config file and update global variables
    fp.open(file);
    while(!fp.eof()){
        fp.getline(line, 256);
        token = strtok(line, "=\n");

        int i = 0;
        while (token != NULL) {
            values[i] = token;
            token = strtok(NULL, "=\n");
            i++;
        }

        //MAKE CHECKS THAT VALUES ARE IN A REASONABLE RANGE
        for (int i = 0; i < NUM_ARGS; i++ ) {
            if (strcmp(args[i], values[0])==0) {
                cout << "MATCH" << endl;
                if (i == 0 ) PERIOD_FETCH = atoi(values[1]);
                else if (i == 1 ) NUM_FETCH = atoi(values[1]);
                else if (i == 2) NUM_PARSE = atoi(values [1]);
                else if (i == 3) {
                    string sf = string(values[1]);
                    SEARCH_FILE = sf;
                }
                else if (i == 4 ) {
                    string sf = string(values[1]);
                    SITE_FILE = sf;
                }
                break;
            }
            else if(i == (NUM_ARGS - 1)) cout << "ERROR: " << values[0] << " is not a valid arg, skipping..." << endl;
        }
    }
    fp.close();
}

//READS AND PROCESSES PROPER STRINGS TO SEARCH VECTOR 
int read_search_file(string file, vector<string> &sv) {
    ifstream fp;
    string line;
    fp.open(file.c_str());

    //If file could not be opened, handle error 
    if (fp.is_open()) {
        while (getline(fp,line) != NULL) {
            sv.push_back(line);
        }
        fp.close();
        return 0;
    }

    else{
        cout << "ERROR: CANNOT OPEN SEARCH FILE CLOSING PROGRAM" << endl;
        cout << strerror(errno) << endl;
        return 1;
    }
}

//READS AND PROCESS PROPER URLS TO SITE VECTOR
int read_site_file(string file, vector<string> &sv){
    ifstream fp;
    string line;
    fp.open(file.c_str());

    //If file could not be opened, handle error 
    if (fp.is_open()) {
        while (getline(fp,line) != NULL) {
            if (valid_url(line)) sv.push_back(line);
        }
        fp.close();
        return 0;
    }

    else{
        cout << "ERROR: CANNOT OPEN FILE CLOSING PROGRAM" << endl;
        cout << strerror(errno) << endl;
        return 1;
    }   
}

bool valid_url(string URL) {
    string request_type;
    string http = "http";
    char* url_c = new char[URL.size() + 1];
    copy(URL.begin(), URL.end(), url_c);
    url_c[URL.size()] = '\0';

    request_type = strtok(url_c, ":");
    delete[] url_c;

    if (strcmp(request_type.c_str(), http.c_str()) == 0) return true;

    return false;
}

//Return string of website content from inputted URL
string curl_to_string(string URL) {
  CURL *curl;
  CURLcode res;
  string readBuffer;

  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
  }
  cout << readBuffer;
  return readBuffer;
}

//Returns the number of times a word appears in the given site
int word_appearances(string word, string URL_content){
    int count = 0;
    return count;
}
