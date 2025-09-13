# Syslog Analysis Tool

This tool is designed to extract text from syslog and perform calculations on various metrics for each service. The metrics include the period (T), frequency, maximum (WCET), minimum, average, and standard deviations.

![Alt Text](https://github.com/amin-amani/RTES-ECEE-5623/blob/syslog-calculation-tool/SyslogTimeCalculatr/screenshot.PNG)

## Sample Syslog Entry

A sample syslog entry should follow the format below:


```
Dec 17 13:07:42 raspberrypi sequencer[27447]: S1 50 Hz on core 2 for release 11 @ sec=0.229461354
```

## Prerequisites

Qt Framework installed on your system.

## How to build
```
 cd src
 mkdirbuld && cd build
 cmake ..
 make
```
