// PW: < folgt >

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
	return 0;
}


/****************************************************************/
/* Falls das eigene Fahrzeug mit dem Gegner in einer            */
/* Gefahrenstelle ist, bremst das Fahrzeug, bis der Gegner die  */
/* Gefahrenstelle verlassen hat oder bis die Wartezeit          */
/* abgelaufen ist.                                              */
/* Return: 0                                                    */
/****************************************************************/
int collision_check(int fd) {
	return 0;
}
