# Data Daemon

Simple Linux daemon to measure transferred data over selected interface.

## Setup
For now, you need to edit the `data_monitor.h` and set the location of log file. Open it in your editor and edit the line with LOG_LOCATION macro to your desired location. In the future, this will be in a config file and there won't be any need to edit the source code.  
Also in `data_monitor.h` there are DELTA_TRANSFER and SLEEP_INTERVAL macros which define how long should the daemon sleep between measurements and after what change in transferred data it should write this change to the disk. The bigger these numbers, the bigger sleeping time and delay in getting to know the real transferred amount. If you need to know the transferred amount precisely all the time, set these values low. If you don't, set them big. The values will always get saved on daemon exit, so you won't lose any data.  
Setting these values bigger also protects your SSD because the daemon will not write as often.

## Compilation
Use `gcc data_daemon.c data_monitor.* -o data_daemon -std=c11` to compile data_daemon into binary and `gcc data_printer.c -o data_printer` to compile printer.

## Use
You need to make the data_daemon start on boot. This might be different for every distro so use Google if you don't know how to do it. In my case, I use Fedora 25 and this is how I did it:  
`sudo vim /etc/rc.d/rc.local`  
Type in:
```
#!/bin/sh -e
/path/to/data_daemon
exit 0
```
Make sure it is executable: `sudo chmod 775 /etc/rc.d/rc.local`  
And finally enable rc services: `sudo systemctl enable rc-local.service`
And it's done. Now it should start on boot.  

Data_printer is there to print the values from log file in human readable format. Use `data_printer --help` to learn more.