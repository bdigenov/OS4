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
#include <tuple>
#include <iomanip>
#include <signal.h>
using namespace std;

int load_site(string site_file);
int load_search(string search_file);
void * curl_url(void*);
void * search_data(void*);
void clean_exit(int sig);

deque<string> site_urls;			//Stores all site_urls
vector<string> search_terms;		//Stores all search terms
deque<string> curl_data;			//Stores response string from URL's after libcurl
bool PROGRAM_CONTINUE = true;		//Flag that will handle clean exits for SIGHUP/ctr-c

deque<tuple<string, string, vector<string> > > url_tuple_data;	//Used to store URL, URL_DATA, and list of search terms for all URLS

pthread_mutex_t urlmutex;
pthread_cond_t urlcond;
pthread_mutex_t datamutex;
pthread_cond_t datacond;



int main(int argc, char** argv){
	//Handle signals cleanly
	signal(SIGINT,clean_exit);
	signal(SIGHUP,clean_exit);
	
	//Check that config file was given
	string paramfile;
	if(argc == 2){
		paramfile = argv[1];
	}else{
		cout << "Please enter a parameter file" << endl;
		return 0;
	}
	
	//Set defaults
	string n;
	string param ("");
	string val ("");
	int period = 180 * CLOCKS_PER_SEC;
	int num_fetch = 1;
	int num_parse = 1;
	string search_file = "Search.txt";
	string site_file = "Sites.txt";
	
	bool first;
	
	//Read and process config file 
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
				if(atoi(val.c_str()) < 8 && atoi(val.c_str()) > 0) num_fetch = atoi(val.c_str());
			} else if(param.compare("NUM_PARSE") == 0){
				if(atoi(val.c_str()) < 8 && atoi(val.c_str()) > 0) num_parse = atoi(val.c_str());
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
	
	int curlthreadcount = 0;
	
	//Error check that search file may be opened 
	if(load_search(search_file)==1) return EXIT_FAILURE;	
	
	//Initialize arrays of fetching and parsing pthreads
	pthread_t fetchers[num_fetch];								
	pthread_t parsers[num_parse];								
	
	//Create fetching and parsing threads and fill the phtread arrays
	for(int i=0; i<num_fetch; i++){
		pthread_create(&fetchers[i],0,curl_url, NULL);
	}
	for(int i=0; i<num_parse; i++){
		pthread_create(&parsers[i],0,search_data, NULL);
	}
	
	int file_num = 1;

	while (PROGRAM_CONTINUE) {
		int count = 0;
		clock_t start_time = clock();
		
		//Create proper XX.csv output file and redirect cout to that file 
		stringstream ss;
		ss << file_num;
		string fn = ss.str();
		fn+=".csv";
		ofstream outfile(fn.c_str());
		streambuf *coutbuf = cout.rdbuf();
		cout.rdbuf(outfile.rdbuf());
		
		//Error check that site file may be opened 
		if(load_site(site_file)== 1) return EXIT_FAILURE;
		
		
		while ((clock() - start_time) < period ) {
			//Keep waiting if there is time left in period
		}

		
		outfile.close();
		file_num++;
	}
	exit(0);
}


//FUNCTIONS

//Load URLS into site_urls vector from site file 
int load_site(string site_file){				
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
	return 0;
}

//Load search terms from search file
int load_search(string search_file){			
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

void * curl_url(void * unused)
{
  CURL *curl_handle;
  CURLcode res;

  struct MemoryStruct chunk;
  
  while(true){

	  chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
	  chunk.size = 0;    /* no data at this point */

	  curl_global_init(CURL_GLOBAL_ALL);

	  /* init the curl session */
	  curl_handle = curl_easy_init();
	  
	  string site;
	  
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
		 
		 //curl_data.push_front(data);
		 
		 //POPULATE TUPLE
		pthread_mutex_lock(&datamutex);
		url_tuple_data.push_front(make_tuple(site, data, search_terms));
		pthread_cond_broadcast(&datacond);
		pthread_mutex_unlock(&datamutex);

		//printf("%lu bytes retrieved\n", (long)chunk.size);
	  }

	  /* cleanup curl stuff */
	  curl_easy_cleanup(curl_handle);

	  free(chunk.memory);

	  /* we're done with libcurl, so clean it up */
	  curl_global_cleanup();
  }
}


/*~~~~~~~~~~~~~~~~~~~~~~CURL FINISHED~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void * search_data(void * unused){
	
	string url_copy;
	string data_copy;
	string word;
	
	while(true){
	
		pthread_mutex_lock(&datamutex);
		while(url_tuple_data.empty()){
			pthread_cond_wait(&datacond, &datamutex);
		}
		url_copy = get<0>(url_tuple_data.back());
		data_copy = get<1>(url_tuple_data.back());
		word = get<2>(url_tuple_data.back()).back();
		get<2>(url_tuple_data.back()).pop_back();
		if(get<2>(url_tuple_data.back()).size() == 0){
			url_tuple_data.pop_back();
		}
		
		
		bool pass = false;
		int count = 0;
		for(int i=0; i<data_copy.size(); i++){
			if(data_copy[i] == word[0]){
				pass = true;
				for(int j=0; j<word.size(); j++){
					if(data_copy[i+j] != word[j]){
						pass = false;
						break;
					}
				}
				if(pass){
					count++;
				}
			}
		}
		
		time_t rawtime;
		struct tm * timeinfo;
		char buffer[80];
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		strftime(buffer,sizeof(buffer),"%d-%m-%Y %I:%M:%S",timeinfo);
		string str(buffer);
		url_copy.pop_back();
		str = str + "," + url_copy + "," + word + "," + to_string(count);
		cout << str << endl;
		
		pthread_cond_broadcast(&datacond);
		pthread_mutex_unlock(&datamutex);
	}
}

//Handles signal cleanly by switching the PROGRAM_CONTINUE flag
void clean_exit(int sig) {
	cout << "ENTERED CLEAN EXIT" << endl;
	PROGRAM_CONTINUE = false;
}
























