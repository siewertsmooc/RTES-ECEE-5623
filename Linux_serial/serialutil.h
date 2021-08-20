/*******************************************************************************
 *
 *  FILE NAME:     serialutil.h
 *
 *  FILE DESCRIPTION:
 *
 *  serialutil.h
 *
 *
 *  modification history
 *  --------------------
 *  Oct 18, 2001 - created, Sam Siewert
 *
 ******************************************************************************/
#ifndef _SERIALUTIL_H_
#define _SERIALUTIL_H_

#include <termio.h>
#include <unistd.h>

#define MAX_INPUT_LENGTH 4096

#define CMD_LINE_INPUT_MODE 0
#define HEX_INPUT_MODE 1
#define TELETYPE_INPUT_MODE 2

static int serialdev_set_raw(struct termios *serialdev);
static int serialdev_set_speed(struct termios *serialdev, char *speed);
static int serialdev_set_databits(struct termios *serialdev, char *databits);
static int serialdev_set_parity(struct termios *serialdev, char *parity);
static int serialdev_set_stopbits(struct termios *serialdev, char *stopbits);

/* test shell interface function */

void print_menu(void);

void initiate_serial_session(char *session_name, int type);

int initialize_serial_device(char *port_device_file);

ssize_t serial_readn(int fd, void *vptr, size_t n);

ssize_t serial_writen(int fd, const void *vptr, size_t n);

#endif
