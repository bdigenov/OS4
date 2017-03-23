#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <curl/curl.h>
#include <deque>
#include <unistd.h>
#include <ctime>
#include <cerrno>
#include <time.h>
#include <pthread.h>
using namespace std;

int load_site(string site_file);
int load_search(string search_file);
void curl_url(string site);
int search_data(string word, string text);

deque<string> site_urls;
vector<string> search_terms;
deque<string> curl_data;
vector<string> file_lines;

pthread_mutex_t urlmutex;
pthread_cond_t urlcond;


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
	int period = 180 * CLOCKS_PER_SEC;
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
				if(atoi(val.c_str()) < 600 && atoi(val.c_str()) > 0) period = atoi(val.c_str())*CLOCKS_PER_SEC;
			} else if(param.compare("NUM_FETCH") == 0){
				if(atoi(val.c_str()) < 8 && atoi(val.c_str()) > -1) num_fetch = atoi(val.c_str());
			} else if(param.compare("NUM_PARSE") == 0){
				if(atoi(val.c_str()) < 8 && atoi(val.c_str()) > -1) num_parse = atoi(val.c_str());
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
	
	int file_num = 1;
	if(load_search(search_file)==1) return EXIT_FAILURE;
	while (true) {
		int count = 0;
		clock_t start_time = clock();
		
		//Create proper output file and redirect all output to that file 
		stringstream ss;
		ss << file_num;
		string fn = ss.str();
		fn+=".csv";
		ofstream outfile(fn.c_str());
		streambuf *coutbuf = cout.rdbuf();
		cout.rdbuf(outfile.rdbuf());
		
		if(load_site(site_file)== 1) return EXIT_FAILURE;
		curl_url();
		/*string site_in;
		int size = site_urls.size();
		while(site_urls.size() != 0){
			site_in = site_urls.back();
			site_urls.pop_back();
			curl_url(site_in);
		}*/
		string data_in;
		int num;
		while(curl_data.size() != 0){
			data_in = curl_data.back();
			curl_data.pop_back();
			for(int i=0; i<search_terms.size(); i++){
				num = search_data(search_terms[i], data_in);
				cout << time(0) << "," << search_terms[i] << "," << "URL HERE" << "," << num << endl; //THIS LINE PRINTS TO THE FILE 
			}
			count++;
		}
		file_num++;			//Keep track of which round you are on for file output

		outfile.close();//Keep track of which round you are on for file output

		while ((clock() - start_time) < period ) {
			//sleep(1);
		}
	}
}

int load_site(string site_file){				//todo: make sure file is there
	string text;
	ifstream file(site_file.c_str());
	pthread_mutex_lock(&urlmutex);
	if(file.is_open()){
		while(!file.eof()){
			getline(file, text);
			site_urls.push_front(text);
		}		
		file.close();
	}
	else {
		cout << "ERROR: Could not open site file " << strerror(errno) << endl;
		return 1;
	}	
	pthread_cond_broadcast(&urlcond);
	pthread_mutex_unlock(&urlmutex);
	//site_urls.pop_back();
	return 0;
}

int load_search(string search_file){			//todo: make sure file is there
	string text;
	ifstream file(search_file.c_str());
	if(file.is_open()){
		while(!file.eof()){
			getline(file, text);
			search_terms.push_back(text);
		}
		file.close();
	}
	else {
		cout << "ERROR: Could not open search file " << strerror(errno) << endl;
		return 1;
	}			
	
	//search_terms.pop_back();
	return 0;
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

void curl_url()
{
  CURL *curl_handle;
  CURLcode res;

  struct MemoryStruct chunk;

  chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
  chunk.size = 0;    /* no data at this point */

  curl_global_init(CURL_GLOBAL_ALL);

  /* init the curl session */
  curl_handle = curl_easy_init();
  
  pthread_mutex_lock(&urlmutex);
  while(site_urls.empty()){
	  pthread_cond_wait(&urlcond, &urlmutex);
  }
  site = site_urls.back();
  site_urls.pop_back();
  pthread_cond_broadcast(&urlcond);
  pthread_mutex_unlock(&urlmutex);

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



























