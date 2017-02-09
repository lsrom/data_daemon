#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>

#define KRED  "\x1B[31m"	// red output color
#define RESET "\x1B[0m"

#define ul_t unsigned long

static unsigned short notifications_displayed = 0;
static unsigned short notifications_ran = 0;

#define DELIMETER " "
/* Definitions of byte multiples. These are actually Mibi units (MiB, GiB,..) not MB, GB,.. */
#define KILOBYTE 1024
#define MEGABYTE (1024 * KILOBYTE)
#define GIGABYTE (1024 * MEGABYTE)

#define SECOND 1
#define MINUTE 60 * SECOND
#define HOUR 60 * MINUTE

/* ------------------------------------------------------------------------- */
/* SETTINGS */
#ifdef DEBUG
// DEBUG MODE
#define DELTA_TRANSFER 50 * MEGABYTE 	// after how many bytes you want to write to log in debug mode
#define SLEEP_INTERVAL MINUTE		// seconds between checks of transfered data in debug mode
#define LOG_LOCATION "/data/git/data_daemon/test_log/"	// where to put the file with log in debug mode
static ul_t TRANSFER_LIMIT = 15 * (ul_t)GIGABYTE; 							// this is data limit per day
#define TRANSFER_WARNING (	TRANSFER_LIMIT * 0.1)		// this can be either fraction of TRANSFER_LIMIT or byte amount
#define NOTIFICATIONS_LIMIT 5			// how many times should the notifications pop up
#define NOTIFICATIONS_PAUSE	2	// how many sleep intervals between notification pop up
#define NOTIFICATIONS_EXCEEDED_LIMIT 5	// how many times should notification pop up when data limit is already exceeded
#define NOTIFICATIONS_EXCEEDED_PAUSE 1	// how many sleep intervals between notification pop up when data limit is already exceeded
#else
// NORMAL MODE
#define DELTA_TRANSFER 50 * MEGABYTE 	// after how many bytes you want to write to log?
#define SLEEP_INTERVAL MINUTE		// seconds between checks of transfered data
#define LOG_LOCATION "/data/logs/data_transferred/"	// where to put the file with log
static ul_t TRANSFER_LIMIT = 15 * (ul_t)GIGABYTE; 						// this is data limit per day
#define TRANSFER_WARNING (TRANSFER_LIMIT * 0.1)			// this can be either fraction of TRANSFER_LIMIT or byte amount
#define NOTIFICATIONS_LIMIT 10			// how many times should the notifications pop up
#define NOTIFICATIONS_PAUSE	3			// how many sleep intervals between notification pop up
#define NOTIFICATIONS_EXCEEDED_LIMIT 5	// how many times should notification pop up when data limit is already exceeded
#define NOTIFICATIONS_EXCEEDED_PAUSE 1	// how many sleep intervals between notification pop up when data limit is already exceeded
#endif
/* ------------------------------------------------------------------------- */

/* Length of day in seconds. */
#define DAY 86400

/* Where is virtual file 'dev' for bandwidth info. */
#define NET_DEV "/proc/net/dev"

typedef struct {
    int year;
    int month;
    int day;
} date;

/* for log lines */
typedef struct log_line {
	date timestamp;		// local date
	ul_t bytes_down;	// how many bytes was downloaded this calendar day
	ul_t bytes_up;		// how many bytes was uploaded this calendar day
} log_line;

/* for /proc/net/dev file */
struct ifinfo {
    char name[16];
    ul_t r_bytes, r_pkt, r_err, r_drop, r_fifo, r_frame;
    unsigned int r_compr, r_mcast;
    ul_t x_bytes, x_pkt, x_err, x_drop, x_fifo, x_coll;
    unsigned int x_carrier, x_compr;
} ifc;

/**
 * Check if 'str_1' is the beginning of 'str_2'. If so, returns TRUE, otherwise FALSE.
 */
bool starts_with (const char *str_1, const char *str_2);

/**
 * Read file with log and parse it's content into the 'log_file_lines' structure. 
 * Location of the log file is set in 'log_data_file' property.
 */
int read_log (log_line *log_file_lines);

/**
 * Appends new entry to the log file, creating new line. This is called every time new day starts, 
 * regardless whether it is after midnight during the run of the program or after boot and new start of the program.
 * This means that downloaded and upload data amount will be always cunted for one calendar day.
 */
void add_to_log (const date *timestamp, const ul_t bytes_down, const ul_t bytes_up, const int lines);

/**
 * Changes last line if log file. No new line is added. Is called during the day, logging changing values.
 */
void modify_log (log_line *log_file_lines, int lines, const ul_t bytes_down, const ul_t bytes_up);

/**
 * Creates absolute location of the log file and puts it into @buffer.
 */
bool get_current_data_file (char *buffer);

/**
 * Show notification to the user.
 */
void notification (const ul_t down_total, const ul_t up_total);

/**
 * TODO
 */
void get_timestamp(date *d);

/**
TODO
 */
void init ();

int run (const char *interface);
