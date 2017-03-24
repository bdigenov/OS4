//Partners: Brittany DiGenova, Collin Klenke (bdigenov, cklenke)

//site-tester
This program takes in a config file as its only argument. It parses that config file to see if any 
of the default variables should be updated. The heart of the code is in a while loop that will continue
running until the program is interrupted. At the end of this while loop is another while loop that stalls
the next loop from running until the "PERIOD" amount of time has passed. Most of the functions are simple 
parsers. The curl code uses libcurl to retrieve site text and search_data counts the number of matches for 
key words in that text.

Site Urls, search terms, and website text are stored in global vectors. These are made thread safe and accessed
by the threads to speed up processing. The num_fetch threads place content in the curl_data and search terms vector 
and the parse threads consume this content in order to return counts which are ultimately formatted and outputted 
to a XX.csv file, where XX is the number of times the main while loop has been executed. Ctrl-c/SIGHUP is handled
by using a global bool called PROGRAM_CONTINUE that turns to false when ctrl-c is inputted, allowing the current loop to 
execute before the program exits