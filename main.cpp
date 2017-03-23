#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

using namespace std;


vector<string> site_urls;
vector<string> search_terms;


int main(int argc, char** argv){
	string paramfile;
	if(argc == 2){
		paramfile = argv[1];
	}else{
		cout << "Please enter a parameter file" << endl;
		return 0;
	}
	
	string n;
	string param ("");
	string val ("");
	int period = 180;
	int num_fetch = 1;
	int num_parse = 1;
	string search_file = "Search.txt";
	string site_file = "Sites.txt";
	
	bool first;
	
	ifstream file(paramfile.c_str());
	if(file.is_open()){
		while(!file.eof()){
			getline(file, n);
			first = true;
			for(int i=0; i<n.size(); i++){
				if(n[i] == '='){
					first = false;
				}else if(first){
					param += n[i];
				} else {
					val += n[i];
				}
			}
			if(param.compare("PERIOD_FETCH") == 0){
				period = atoi(val.c_str());
			} else if(param.compare("NUM_FETCH") == 0){
				num_fetch = atoi(val.c_str());
			} else if(param.compare("NUM_PARSE") == 0){
				num_parse = atoi(val.c_str());
			} else if(param.compare("SEARCH_FILE") == 0){
				search_file = val;
			} else if(param.compare("SITE_FILE") == 0){
				site_file = val;
			} else if(param.compare("") == 0) {
				//do nothing but don't print the error either
			} else {
				cout << "Invalid parameter argument" << endl;
			}
			param = "";
			val = "";
		}
	}
	file.close();
	
	load_site(site_file);
	load_search(search_file);
	for(int i=0; i<site_urls.size(); i++){
		curl_url(site_urls[i]);
	}
	
}

void load_site(string site_file){				//todo: make sure file is there
	string text;
	ifstream file(site_file.c_str());
	if(file.is_open()){
		while(!file.eof()){
			getline(file, text);
			site_urls.push_back(text);
		}
	}
	
	file.close();
	site_urls.pop_back();
}

void load_search(string search_file){			//todo: make sure file is there
	string text;
	ifstream file(search_file.c_str());
	if(file.is_open()){
		while(!file.eof()){
			getline(file, text);
			search_terms.push_back(text);
		}
	}
	
	file.close();
	search_terms.pop_back();
}

