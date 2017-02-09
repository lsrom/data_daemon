#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <syslog.h>

#include "data_monitor.h"

#include <errno.h>

#define PID_STR_LEN 10
#define PID_LOCATION "/tmp/data_daemon.pid"
#define DAEMON_NAME "data_daemon"
#define WORKING_DIR ""

static int pid_file;

void unlock_pid_file (){
	if (lockf(pid_file, F_ULOCK, 0) == 0){
		syslog(LOG_INFO, "Unlocking successfull.\n");
	} else {
		syslog(LOG_INFO, "Unlocking failed.\n");
	}
}

void signal_handler (int sig){
	switch (sig){
		case SIGHUP:
			// todo - reload settings
			break;
		case SIGINT:
			// todo force read and write to log
			// end server
			unlock_pid_file();
			closelog();
			exit(0);
		break;
	}
}

void setup_signals (){
	struct sigaction psa;
    psa.sa_handler = signal_handler;
    sigaction(SIGINT, &psa, NULL);
    sigaction(SIGHUP, &psa, NULL);
}

void start_log (){
	// open log
	// "daemon" identifies this process in the log file
	// LOG_PID says that pid of this daemon will be included in the log
	// LOG_DAEMON is class of the messages that will be logged
	openlog(DAEMON_NAME, LOG_PID, LOG_DAEMON);
}

static void daemonize (){
	if (getppid() == 1){
		return;		// already a daemon
	}

	pid_t pid;

	pid = fork();

	// fork unsuccessfull - parent ends here, no child existed
	if (pid < 0){
		exit(1);
	}

	// pid > 0 means it is now containing the pid of new process
	// parent ends here, child continues
	// because of shared memory space and pointers to stack, child process 
	// will continue to run even after the exit() is called
	if (pid > 0){
		exit(0);
	}

	// if successfull, new session id is returned
	// this step ensures that the process is detached from command line 
	// and thus cannot accept any input from it
	if (setsid() < 0){
		// errno now indicates what error has occured
		exit(1);
	}

	int lff = open (PID_LOCATION, O_RDONLY);

	if (lockf(lff, F_TEST, 0) == 0){
		syslog(LOG_INFO, "PID file already locked by this process.\n");
	} else if (lockf(lff, F_TEST, 0) == -1){
		syslog(LOG_INFO, "PID file locked by another process.\n");
	}

	// write process id to file
	// this can ensure mutual exlusion - every new process of this program 
	// can check for the file and if it's there then it will know that it's 
	// instance is already running
	pid_file = open(PID_LOCATION, O_CREAT|O_WRONLY, 0777);

	if (!pid_file){
		exit(1);		// cannot open file
	}

	/*if (flock(lf, LOCK_EX|LOCK_NB ) == -1) {
    if (errno == EWOULDBLOCK) {
        syslog(LOG_INFO, "locked\n");
    } else {
        // error
    } // end if
} else {
    flock(lf, LOCK_UN );
    syslog(LOG_INFO, "unlocked\n");
} // end if*/

	// for locking files on linux, 'lockf' is preffered over 'flock' because it 
	// is POSIX compliant and is wrap for fnctl(), where 'flock' is separately implemented 
	// function which may not work on ext4 filesystem
	if (lockf(pid_file, F_LOCK, 0) < 0){
		syslog(LOG_WARNING, "Failed to lock PID file. Maybe permission issue.\n");
		exit(0);		// cannot lock file - only first instance continues
	}

	char pid_str[PID_STR_LEN];	// max 9 numbers long
	sprintf (pid_str, "%d\n", getpid());	// write pid to the string
	write(pid_file, pid_str, strlen(pid_str));	// write pid to the lockfile

	// set new file permissions
	// numbers here are the complement to standard linux permissions
	// 0 means 7, 2 means 5 and 5 means 2 -> all must add up to 7
	umask(0);

	// close open descriptors
	// NOTE!!! if process cannot write into file, it's because of this
	for (int i = getdtablesize(); i >= 0; i--){
		close(i);
	}

	// change working directory of the process
	chdir(WORKING_DIR);

	setup_signals();
}

int main (void){
	start_log();

	#ifdef DEBUG
	printf("DEBUG mode.\n");
	setup_signals();
	#else
	daemonize();
	#endif

	syslog(LOG_INFO, "Daemon started.\n");

	run();

	syslog(LOG_INFO, "Daemon ending.\n");

	unlock_pid_file();
	closelog();
	exit(0);
}