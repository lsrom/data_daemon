#include "data_monitor.h"
#define _GNU_SOURCE

char log_data_file[255];	// path to log file
int gmt_offset = 0;

#ifdef DEBUG

#define KILOBYTE 1024
#define MEGABYTE (1024 * KILOBYTE)
#define GIGABYTE (1024 * MEGABYTE)

void printMi (unsigned long bytes, char *buffer){
	char *units[] = {"kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};

	unsigned long unit = KILOBYTE;
	int u = -1;

	int i = -1;
	unsigned long tmp = 0;
	do {
		tmp = bytes % (unit == 1024 ? 1024 : (unit / 1024));
		bytes = bytes / unit;
		i++;
	} while (bytes > unit);

	sprintf (buffer, "%lu.%lu %s", bytes, tmp, units[u == -1 ? i : u]);
}
#endif

bool starts_with (const char *str_1, const char *str_2){
	size_t len_str_1 = strlen(str_1);
    size_t len_str_2 = strlen(str_2);

    for (int i = 0; i < len_str_1 - 1; i++){
    	if (str_1[i] != str_2[i]){
    		return false;
    	}
    }

    return true;
}

int read_log (log_line *log_file_lines){
	FILE *log = fopen (log_data_file, "r");

	if (!log){
		return 0;
	}

	int lines = 0;
	while (fscanf(log, "%ld,%lu,%lu", &log_file_lines[lines].timestamp, &log_file_lines[lines].bytes_down, &log_file_lines[lines].bytes_up) == 3) {
        lines++;
    }

    fclose(log);

    return lines;
}

void add_to_log (const time_t timestamp, const ul_t bytes_down, const ul_t bytes_up, const int lines){
	bool same = get_current_data_file(log_data_file);
	FILE *log = fopen (log_data_file, "a");

	if (lines == 0){
		fprintf(log, "%ld,%lu,%lu", timestamp, bytes_down, bytes_up);
	} else {
		fprintf(log, "\n%ld,%lu,%lu", timestamp, bytes_down, bytes_up);
	}

	fclose(log);
}

void modify_log (log_line *log_file_lines, int lines, const time_t timestamp, const ul_t bytes_down, const ul_t bytes_up){
    FILE *log = fopen(log_data_file, "w");

    if (lines != 0){
    	for (int i = 0; i < (lines - 1); i++){
    		fprintf (log, "%ld,%lu,%lu\r\n", log_file_lines[i].timestamp, log_file_lines[i].bytes_down, log_file_lines[i].bytes_up);
    	}
    }

    fprintf (log, "%ld,%lu,%lu", log_file_lines[lines].timestamp, bytes_down, bytes_up);

	fclose(log);
}

bool get_current_data_file (char *buffer){
	time_t t = time(NULL);				// get current time
	struct tm tm = *localtime(&t);		// get localtime struct
	char log_name_format[16];			// buffer for holding log file name
	bool same = true;

	// format log file name
	sprintf(log_name_format, "%d-%d.log", tm.tm_year + 1900, tm.tm_mon + 1);

	if (!strcmp(log_data_file, log_name_format) == 0){
		same = false;
	}

	// format log data location
	sprintf(buffer, "%s%s", LOG_LOCATION, log_name_format);

	return same;
}

time_t get_timestamp (){
	time_t current_time = time(NULL);
	time_t current_date = (current_time + gmt_offset) - (current_time % DAY);

	return current_date;
}

void init (){
	// init the gmt offset for current timezone
	time_t t = time(NULL);
  	struct tm lt = {0};
  	localtime_r(&t, &lt);

  	gmt_offset = lt.tm_gmtoff;
}

int run (const char *interface){
	log_line log_file_lines[31];

	/*if (argc <= 1){
		printf ("Too few arguments.\n");
		exit(1);
	}*/

	get_current_data_file(log_data_file);

	const char delimeter[2] = " ";		// delimeter for dev file
	FILE *dev = NULL;					// /proc/net/dev on most systems
	char buffer[255];					// buffer for reading the dev file
	ul_t bytes_down = 0;		// how many bytes downloaded is written in dev file
	ul_t bytes_up = 0;			// how many bytes uploaded is written in dev file
	ul_t last_bytes_up = 0;		// how many bytes were downloaded in last measurement
	ul_t last_bytes_down = 0;	// how many bytes were uploaded in last measurement

	init(&last_bytes_down, &last_bytes_up, log_file_lines);

	#ifdef DEBUG
	int writes = 0;		// how many writes to the disk the program made
	int rounds = 0;		// how many rounds was program active (rounds * SLEEP_INTERVAL = time duration of program run)
	#endif

	// this loop runs forever, with sleep interval defined in SLEEP_INTERVAL
	while (true){
		dev = fopen(NET_DEV, "r");	// open file everytime, don't hold it open forever

		if (!dev){
			// if file failed to open, print message and try again next time
			//printf ("%s failed to open.\n", NET_DEV);
			syslog(LOG_WARNING, "%s failed to open.\n", NET_DEV);
		} else {
			// read and parse dev file
			int i = -1;
			while(fgets(buffer, 255, (FILE*) dev)) {
				i++;
				if (i < 2){continue;}	// skip  first two lines, they are header

				if (starts_with(&interface[0], buffer)){
					char *token;	// parts of line separated by delimeter
	   
				   	token = strtok(buffer, delimeter);	// get the first token

				   	char *ptr;
				   	bytes_down = strtoul(strtok(NULL, delimeter), &ptr, 10);	// parse bytes downloaded from dev file
				   	
				   	// jump to correct column
				   	for (int i = 0; i < 7; i++){
				   		strtok(NULL, delimeter);
				   	}

				   	bytes_up = strtoul(strtok(NULL, delimeter), &ptr, 10);		// parse bytes uploaded from dev file

				   	//printf ("%u %u\n", bytes_down, bytes_up);		// uncomment for printing down/up amount every round
				   	time_t timestamp = get_timestamp();

				   	// calculate difference between this and last measurement
				   	ul_t delta_down = bytes_down - last_bytes_down;
				   	ul_t delta_up = bytes_up - last_bytes_up;

				   	int lines = read_log(log_file_lines);

				   	if (lines != 0 && log_file_lines[lines - 1].timestamp == timestamp){
				   		#ifdef DEBUG
				   		printf ("Same day (%ld).\n", timestamp);
						#endif
				   		if (delta_down >= DELTA_TRANSFER || delta_up >= DELTA_TRANSFER || (last_bytes_down == 0 || last_bytes_up == 0)){
				   			modify_log(log_file_lines, lines, log_file_lines[lines].timestamp, delta_down + log_file_lines[lines].bytes_down, delta_up + log_file_lines[lines].bytes_up);
				   			
				   			#ifdef DEBUG
				   			writes++;
							#endif

				   			// refresh last values
				   			last_bytes_down = bytes_down;
				   			last_bytes_up = bytes_up;

				   			#ifdef DEBUG
				   			char buffer[32];
				   			printMi(delta_down, buffer);
				   			printf ("Delta down: %s\n", buffer);
				   			printMi(delta_up, buffer);
				   			printf ("Delta up: %s\n", buffer);
							#endif
				   		}

				   	} else {
				   		#ifdef DEBUG
				   		printf ("New day (%ld).\n", timestamp);
						#endif
				   		add_to_log(timestamp, delta_down, delta_up, lines);
				   		
				   		#ifdef DEBUG
				   		writes++;
						#endif

						// refresh last values
				   		last_bytes_down = bytes_down;
				   		last_bytes_up = bytes_up;

				   		#ifdef DEBUG
				   		char buffer[32];
				   		printMi(delta_down, buffer);
				   		printf ("Delta down: %s\n", buffer);
				   		printMi(delta_up, buffer);
				   		printf ("Delta up: %s\n", buffer);
						#endif
				   	}
				}
			}
		}

		fclose(dev);			// don't keep the file open

		sleep(SLEEP_INTERVAL);	// wait before another cycle

		#ifdef DEBUG
		rounds++;
		printf ("Rounds: %4d Writes: %4d\n", rounds, writes);
		#endif
	}
}