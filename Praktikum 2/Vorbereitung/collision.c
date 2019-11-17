// PW: ezsJr3h44

#include <stdio.h>
#include <time.h>

#include "check.h"


extern unsigned long gegnerzeit_in_us;
extern unsigned long time_in_us;
extern unsigned short state_act;
extern struct timespec aktuellezeit, gegnerzeit;
extern struct _liste *eigenessegment, *gegnersegment;


/****************************************************************/
/* Umwandlung eines Zeitstempels in Mikrosekunden und Rückgabe  */
/* des Ergebnis als unsigned long.                              */
/****************************************************************/
unsigned long timespec_to_ulong_microseconds(struct timespec* ptr) {
	unsigned long res = ptr->tv_sec * 1000000;		// Seconds to micro seconds
	res += (unsigned long)(ptr->tv_nsec / 1000);	// Nano seconds to micro seconds
	return res;
}


/****************************************************************/
/* Falls das eigene Fahrzeug mit dem Gegner in einer            */
/* Gefahrenstelle ist, bremst das Fahrzeug, bis der Gegner die  */
/* Gefahrenstelle verlassen hat oder bis die Wartezeit          */
/* abgelaufen ist.                                              */
/* Return: 0                                                    */
/****************************************************************/
int collision_check(int fd) {
	struct timespec sleeptime;
	unsigned long time_in_us;

	// Check if car is in collision zone
	if ((state_act & 0xf000) == 0x2000) {
		sleeptime.tv_sec = 0;
		sleeptime.tv_nsec = 114000000;

		// Get time for us and enemy + difference
		clock_gettime(CLOCK_MONOTONIC, &aktuellezeit);
		time_in_us = timespec_to_ulong_microseconds(&aktuellezeit);
		gegnerzeit_in_us = timespec_to_ulong_microseconds(&gegnerzeit);
		unsigned long diff_zeit_in_us = time_in_us-gegnerzeit_in_us;

		// Await enemy leaving the collision zone
		while (eigenessegment->type == gegnersegment->type && diff_zeit_in_us < 15000000) {
			printf("%4.4x - %4.4x - diff_zeit_in_us: %ld\n", eigenessegment->type,
					gegnersegment->type, diff_zeit_in_us);
			
			// Brake
			set_speed(fd, 0x0b);
			clock_nanosleep(CLOCK_MONOTONIC, 0, &sleeptime, NULL);

			// Get new time difference
			clock_gettime(CLOCK_MONOTONIC, &aktuellezeit);
			time_in_us = timespec_to_ulong_microseconds(&aktuellezeit);
			diff_zeit_in_us = time_in_us-gegnerzeit_in_us;
		}

		// Release brake
		set_speed(fd, 0x0c);
	}

	return 0;
}
