#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>

#define ul_t unsigned long

/* ------------------------------------------------------------------------- */
/* SETTINGS */
#define DELTA_TRANSFER 50 * MB 	// after how many bytes you want to write to log?
#define SLEEP_INTERVAL 60		// seconds between checks of transfered data

#ifdef DEBUG
#define LOG_LOCATION "/data/git/data_daemon/test_log/"	// where to put the file with log in debug mode
#elif
#define LOG_LOCATION "/data/logs/data_transferred/"	// where to put the file with log
#endif
/* ------------------------------------------------------------------------- */

/* Definitions of byte multiples. These are actually Mibi units (MiB, GiB,..) not MB, GB,.. */
#define kB 1024
#define MB (1024 * kB)
#define GB (1024 * MB)

/* Length of day in seconds. I don't care about days with leap seconds. */
#define DAY 86400

/* Where is virtual file 'dev' for bandwidth info. */
#define NET_DEV "/proc/net/dev"

/* for log lines */
typedef struct log_line {
	ul_t timestamp;	// unix timestamp % 86400 to give the start of day
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
void add_to_log (const time_t timestamp, const ul_t bytes_down, const ul_t bytes_up, const int lines);

/**
 * Changes last line if log file. No new line is added. Is called during the day, logging changing values.
 */
void modify_log (log_line *log_file_lines, int lines, const time_t timestamp, const ul_t bytes_down, const ul_t bytes_up);

/**
 * Creates absolute location of the log file and puts it into @buffer.
 */
bool get_current_data_file (char *buffer);


/**
 * TODO
 */
time_t get_timestamp();

/**
TODO
 */
void init ();

int run (const char *interface);
