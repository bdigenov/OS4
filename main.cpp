#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <deque>
#include <unistd.h>
#include <ctime>
using namespace std;

void load_site(string site_file);
void load_search(string search_file);
void curl_url(string site);
int search_data(string word, string text);

vector<string> site_urls;
vector<string> search_terms;
deque<string> curl_data;


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
				} else if(!isspace(n[i])){
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
	
	cout << "searchfile: " << search_file << endl;
	cout << "sitefile: " << site_file << endl;
	
	int file_num = 0;

	while (true) {
		int count = 0;
		int start_time = time(NULL);
		
		load_site(site_file);
		load_search(search_file);
		for(int i=0; i<site_urls.size(); i++){
			curl_url(site_urls[i]);
		}
		string data_in;
		int num;
		while(curl_data.size() != 0){
			data_in = curl_data.back();
			curl_data.pop_back();
			for(int i=0; i<search_terms.size(); i++){
				num = search_data(search_terms[i], data_in);
				cout << "Number of " << search_terms[i] << " at URL " << site_urls[count%site_urls.size()] << ":" << num << endl;
			}
			count++;
		}
		file_num++;			//Keep track of which round you are on for file output

		while ((time(NULL) - start_time) < period ) {
			sleep(10);
		}
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
	for (int i=0; i<site_urls.size(); i++){
		cout << site_urls[i] << endl;
	}
	file.close();
	//site_urls.pop_back();
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
	//search_terms.pop_back();
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~CURL CODE~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
	/* out of memory! */
	printf("not enough memory (realloc returned NULL)\n");
	return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

void curl_url(string site)
{
  CURL *curl_handle;
  CURLcode res;

  struct MemoryStruct chunk;

  chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
  chunk.size = 0;    /* no data at this point */

  curl_global_init(CURL_GLOBAL_ALL);

  /* init the curl session */
  curl_handle = curl_easy_init();

  /* specify URL to get */
  curl_easy_setopt(curl_handle, CURLOPT_URL, site.c_str());
  

  /* send all data to this function  */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

  /* we pass our 'chunk' struct to the callback function */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

  /* some servers don't like requests that are made without a user-agent
	 field, so we provide one */
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  
  
  curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);

  /* get it! */
  res = curl_easy_perform(curl_handle);

  /* check for errors */
  if(res != CURLE_OK) {
	fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
  }
  else {
	
	 string data = chunk.memory;
	 
	 //cout << "count of " << word << ": " << count << endl;
	 //cout << data << endl;
	 curl_data.push_front(data);

	//printf("%lu bytes retrieved\n", (long)chunk.size);
  }

  /* cleanup curl stuff */
  curl_easy_cleanup(curl_handle);

  free(chunk.memory);

  /* we're done with libcurl, so clean it up */
  curl_global_cleanup();

}


/*~~~~~~~~~~~~~~~~~~~~~~CURL FINISHED~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

int search_data(string word, string text){
	bool pass = false;
	int count = 0;
	for(int i=0; i<text.size(); i++){
		if(text[i] == word[0]){
			pass = true;
			for(int j=0; j<word.size(); j++){
				if(text[i+j] != word[j]){
					pass = false;
					break;
				}
			}
			if(pass){
				count++;
			}
		}
	}
	return count;
}



























