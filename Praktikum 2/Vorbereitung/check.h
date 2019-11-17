#ifndef _CHECK_H
#define _CHECK_H


extern int _clock_nanosleep(clockid_t clock_id, int flags, const struct timespec* request,
								struct timespec* remain);

extern int _clock_gettime(clockid_t clk_id, struct timespec* tp);

extern int set_speed(int fd, int value);


/// Single linked list structure
struct _liste {
	int type;
	int length;
	struct _liste* next;
};


#define clock_nanosleep _clock_nanosleep
#define clock_gettime _clock_gettime

#endif
