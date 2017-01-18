#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define PROGRAM_NAME "data_printer"

#define KILOBYTE 1024
#define MEGABYTE (1024 * KILOBYTE)
#define GIGABYTE (1024 * MEGABYTE)

// for log
typedef struct log_line {
	unsigned long timestamp;		// unix timestamp % 86400 to give the start of day
	unsigned long bytes_down;	// how many bytes was downloaded this calendar day
	unsigned long bytes_up;		// how many bytes was uploaded this calendar day
} log_line;

char read_location[255];
bool display_all = false;
bool display_total = false;
bool display_less = false;
bool display_csv = false;
char *convert_to_unit = NULL;
bool no_units = false;
log_line log_file_lines[31];

int read_file (){
	FILE *file = fopen(read_location, "r");

	int lines = 0;
	while (fscanf(file, "%lu,%lu,%lu", &log_file_lines[lines].timestamp, &log_file_lines[lines].bytes_down, &log_file_lines[lines].bytes_up) == 3) {
        lines++;
    }

    fclose(file);

    return lines;
}

void printMi (unsigned long bytes, char *buffer){
	char *units[] = {"kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};

	unsigned long unit = KILOBYTE;
	int u = -1;

	if (convert_to_unit != NULL){
		switch (convert_to_unit[1]){
			case 'k':
			unit = KILOBYTE;
			u = 0;
			break;

			case 'm':
			unit = MEGABYTE;
			u = 1;
			break;

			case 'g':
			unit = GIGABYTE;
			u = 2;
			break;
		}
	}

	int i = -1;
	unsigned long tmp = 0;
	do {
		tmp = bytes % (unit == 1024 ? 1024 : (unit / 1024));
		bytes = bytes / unit;
		i++;
	} while (bytes > unit);

	if (no_units){
		sprintf (buffer, "%lu.%lu", bytes, tmp);
	} else {
		sprintf (buffer, "%lu.%lu %s", bytes, tmp, units[u == -1 ? i : u]);
	}
}

void pretty_print (const unsigned long bytes_down, const unsigned long bytes_up){
	char buffer[32];
	char str_down[7];
	char str_up[5];
	char str_total[9];

	if (display_less){
		strcpy(str_down, "");
		strcpy(str_up, "");
		strcpy(str_total, "");
	}

	if (display_csv){
		strcpy(str_down, "");
		strcpy(str_up, ",");
		strcpy(str_total, ",");
	} else {
		strcpy(str_down, "Down: ");
		strcpy(str_up, " Up: ");
		strcpy(str_total, " Total: ");
	}

	printMi(bytes_down, buffer);
	printf ("%s%s", str_down, buffer);
		
	printMi(bytes_up, buffer);
	printf ("%s%s", str_up, buffer);

	if (display_total){
		printMi(bytes_down + bytes_up, buffer);
		printf ("%s%s", str_total, buffer);
	}

	printf ("\n");
}

void get_current_data_file (char *buffer){
	time_t t = time(NULL);				// get current time
	struct tm tm = *localtime(&t);		// get localtime struct
	char log_name_format[16];			// buffer for holding log file name

	// format log file name
	sprintf(log_name_format, "%d-%d.log", tm.tm_year + 1900, tm.tm_mon + 1);

	// format log data location
	sprintf(buffer, "%s%s", LOG_LOCATION, log_name_format);
}

void display_help (){
	fprintf (stderr, "Usage: %s [%s]...\n%s\n\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n "
		, PROGRAM_NAME, "OPTIONS"
		, "Print data amount from log file to stdout. Parameter -p mandatory."
		, "  -p, --path\tmust be followed by a path to single file with data logs, it's mandatory parameter"
		, "  -t, --total\tprint total amount for data (download + upload)"
		, "  -l, --less\tprint only numbers with units without the Down/Up/Total strings"
		, "  -L, --LESS\tprint only number with no units"
		, "  -c, --csv\tprint in .csv format"
		, "  -kb, -mb, -gb\tprint in kilobytes, megabytes or gigabytes respectively, these are mutually exclusive"
		, "  -h, --help\tdisplay usage information");
}

int main (int argc, char **argv){
	get_current_data_file(read_location);

	if (argc == 1){
		fprintf(stderr, "%s\n", "Parameter -p is mandatory. Use --help for more information.");
		return 0;
	} else {
		for (int i = 1; i < argc; i++){
			if ((strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--path") == 0) && argc > (i + 1)){
				strcpy(read_location, argv[i + 1]);
				i++;
			} else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--all") == 0){
				display_all = true;
			} else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--total") == 0){
				display_total = true;
			} else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--less") == 0){
				display_less = true;
			} else if (strcmp(argv[i], "-L") == 0 || strcmp(argv[i], "--LESS") == 0){
				display_less = true;
				no_units = true;
			} else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--csv") == 0){
				display_csv = true;
			} else if (strcmp(argv[i], "-kb") == 0 || strcmp(argv[i], "-mb") == 0 || strcmp(argv[i], "-gb") == 0){
				convert_to_unit = argv[i];
			} else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0){
				display_help();
				return 0;
			}
		}
	}

	int lines = read_file ();

	if (display_all){
		for (int i = 0; i < lines; i++){
			pretty_print(log_file_lines[i].bytes_down, log_file_lines[i].bytes_up);
		}
	} else {
		pretty_print(log_file_lines[lines - 1].bytes_down, log_file_lines[lines - 1].bytes_up);
	}

	exit(0);
}