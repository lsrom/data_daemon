#include "data_monitor.h"
#define _GNU_SOURCE

char log_data_file[255];	// path to log file
int gmt_offset = 0;

bool is_timestamp_equal (date *d1, date *d2){
	if (d1->year != d2->year){
		return false;
	}

	if (d1->month != d2->month){
		return false;
	}

	if (d1->day != d2->day){
		return false;
	}

	return true;
}

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
	while (fscanf(log, "%d-%d-%d,%lu,%lu", &log_file_lines[lines].timestamp.year, &log_file_lines[lines].timestamp.month, &log_file_lines[lines].timestamp.day, &log_file_lines[lines].bytes_down, &log_file_lines[lines].bytes_up) == 5) {
        lines++;
    }

    fclose(log);

    return lines;
}

void add_to_log (const date *timestamp, const ul_t bytes_down, const ul_t bytes_up, const int lines){
	bool same = get_current_data_file(log_data_file);
	FILE *log = fopen (log_data_file, "a");

	if (lines == 0){
		fprintf(log, "%d-%d-%d,%lu,%lu", timestamp->year, timestamp->month, timestamp->day, bytes_down, bytes_up);
	} else {
		fprintf(log, "\n%d-%d-%d,%lu,%lu", timestamp->year, timestamp->month, timestamp->day, bytes_down, bytes_up);
	}

	fclose(log);
}

void modify_log (log_line *log_file_lines, int lines, const ul_t bytes_down, const ul_t bytes_up){
    FILE *log = fopen(log_data_file, "w");

    if (lines != 0){
    	for (int i = 0; i < (lines - 1); i++){
    		fprintf (log, "%d-%d-%d,%lu,%lu\n", log_file_lines[i].timestamp.year, log_file_lines[i].timestamp.month, log_file_lines[i].timestamp.day, log_file_lines[i].bytes_down, log_file_lines[i].bytes_up);
    	}
    }

    fprintf (log, "%d-%d-%d,%lu,%lu\n", log_file_lines[lines - 1].timestamp.year, log_file_lines[lines - 1].timestamp.month, log_file_lines[lines - 1].timestamp.day, bytes_down, bytes_up);

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

void get_timestamp (date *d){
	time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    d->year = tm.tm_year + 1900;
    d->month = tm.tm_mon + 1;
    d->day = tm.tm_mday;
}

void notification (const ul_t down_total, const ul_t up_total){
	ul_t total = down_total + up_total;

	char buffer[255];			// for message to the user
	char down_buffer[32];		// for human readable amount of bytes downloaded
	char up_buffer[32];			// for human readable amount of bytes uploaded
	char total_buffer[32];		// for human readable amount of bytes in total

	// make byte amount human readable
	printMi(down_total, down_buffer);
	printMi(up_total, up_buffer);
	printMi(total, total_buffer);

	// when aproaching transfer limit
	if ((total > (TRANSFER_LIMIT - TRANSFER_WARNING)) && (notifications_displayed < NOTIFICATIONS_LIMIT) && (notifications_ran % NOTIFICATIONS_PAUSE == 0)){
		// create shel command to show ntification and play sound effect
		sprintf (buffer, "notify-send \"Data Limit Warning\" \"%s down, %s up\nTotal: %s\"; paplay notify_sound.wav", down_buffer, up_buffer, total_buffer);

		system(buffer);					// show notification
		notifications_displayed++;		// increment amount of notifications displayed
	}

	// if next round will be over transfer limit, set notifications_displayed to zero 
	// because every following notification will have different count
	if ((total + DELTA_TRANSFER) > TRANSFER_LIMIT){
		notifications_displayed = 0;
	}

	notifications_ran++;		// increment number of times this function has ran
}

void init (){
	// init the gmt offset for current timezone
	time_t t = time(NULL);
  	struct tm lt = {0};
  	localtime_r(&t, &lt);

  	gmt_offset = lt.tm_gmtoff;
}

void get_bytes_transferred (ul_t *bytes_down, ul_t *bytes_up, char *buffer){
	char *token;	// parts of line separated by delimeter
	   
	token = strtok(buffer, DELIMETER);	// get the first token

	char *ptr;
	*bytes_down = strtoul(strtok(NULL, DELIMETER), &ptr, 10);	// parse bytes downloaded from dev file
				   	
	// jump to correct column
	for (int i = 0; i < 7; i++){
		strtok(NULL, DELIMETER);
	}

	*bytes_up = strtoul(strtok(NULL, DELIMETER), &ptr, 10);		// parse bytes uploaded from dev file
}

int run (){
	log_line log_file_lines[31];

	get_current_data_file(log_data_file);

	FILE *dev = NULL;					// /proc/net/dev on most systems
	char buffer[255];					// buffer for reading the dev file
	ul_t bytes_down = 0;		// how many bytes downloaded is written in dev file
	ul_t bytes_up = 0;			// how many bytes uploaded is written in dev file
	ul_t last_bytes_up = 0;		// how many bytes were downloaded in last measurement
	ul_t last_bytes_down = 0;	// how many bytes were uploaded in last measurement
	date timestamp;

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

				if (starts_with(INTERFACE, buffer)){
					get_bytes_transferred(&bytes_down, &bytes_up, buffer);

				   	get_timestamp(&timestamp);

				   	// calculate difference between this and last measurement
				   	ul_t delta_down = bytes_down - last_bytes_down;
				   	ul_t delta_up = bytes_up - last_bytes_up;

				   	ul_t down_total = 0;
				   	ul_t up_total = 0;

				   	int lines = read_log(log_file_lines);

				   	if (lines != 0 && is_timestamp_equal(&log_file_lines[lines - 1].timestamp, &timestamp)){
				   		#ifdef DEBUG
				   		printf ("Same day (%d-%d-%d).\n", timestamp.year, timestamp.month, timestamp.day);
						#endif

						down_total = delta_down + log_file_lines[lines - 1].bytes_down;
				   		up_total = delta_up + log_file_lines[lines - 1].bytes_up;

				   		if (delta_down >= DELTA_TRANSFER || delta_up >= DELTA_TRANSFER || (last_bytes_down == 0 || last_bytes_up == 0)){
				   			modify_log(log_file_lines, lines, down_total, up_total);

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

				   		notification (down_total, up_total);
				   	} else {
				   		#ifdef DEBUG
				   		printf ("New day (%d-%d-%d).\n", timestamp.year, timestamp.month, timestamp.day);
						#endif
				   		add_to_log(&timestamp, delta_down, delta_up, lines);
				   		
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
		printf (KRED "Rounds: %4d Writes: %4d\n\n" RESET, rounds, writes);
		#endif
	}
}