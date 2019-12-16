/*
	Quellcode zum Praktikum "Echtzeitsysteme" im Fachbereich Elektrotechnik
	und Informatik der Hochschule Niederrhein.

	Fuer die Generierung wird die Realzeitbibliothek "rt" benoetigt.
*/

#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <time.h>
#include <sys/time.h>
#include <asm/types.h>

#define make_seconds(a) (a / 1000000)
#define make_mseconds(a) ((a - ((a / 1000000) * 1000000)) / 1000)

// Anfang Praktikum 3
static unsigned char ausl_in = -1, ausl_out = -1;

// Anfang Praktikum 2
struct timespec eintritt_gegner, eintritt_user;

unsigned long timespec_to_ulong_microseconds(struct timespec* ptr) {
	unsigned long res = ptr->tv_sec * 1000000;		// Seconds to micro seconds
	res += (unsigned long)(ptr->tv_nsec / 1000);	// Nano seconds to micro seconds
	return res;
}
// Ende Praktikum 2


static void set_speed(int fd, int Speed);

static unsigned char basis_speed_in = 0x24, basis_speed_out = 0x24;
static unsigned long last_time;
static int fdc = 0;


typedef struct _liste {
	int type;
	int length;
	struct _liste* next;
} streckenliste;

static streckenliste* root = NULL;

// Anfang Praktikum 2
struct _liste* gegner_pos = NULL;
// Ende Praktikum 2

static struct _liste* add_to_liste(int type, int length) {
	struct _liste *ptr, *lptr;

	lptr = malloc(sizeof(struct _liste));
	if (lptr == NULL) {
		printf("out of memory\n");
		return NULL;
	}

	lptr->type = type;
	lptr->length = length;
	lptr->next = root;

	if (root == NULL) {
		root = lptr;
		lptr->next = lptr;
		return root;
	}

	for (ptr = root; ptr->next != root; ptr = ptr->next) {}
	
	ptr->next = lptr;
	return lptr->next;
}

static void print_liste(void) {
	struct _liste* lptr = root;
	int length_sum = 0;

	do {
		printf("0x%04x - %d [mm]\n", lptr->type, lptr->length);
		length_sum += lptr->length;
		lptr = lptr->next;
	} while (lptr != root);

	printf("sum: %d\n", length_sum);
}


static void exithandler(int signr) {
	if (fdc) set_speed(fdc, 0x0);
	exit(0);
}


static void set_speed(int fd, int speed) {
	printf("new speed: 0x%x\n", speed);

	if (fd > 0) write(fd, &speed, sizeof(speed));
}


static __u16 read_with_time(int fd, unsigned long* time1) {
	struct timespec timestamp;
	__u16 state;
	ssize_t ret;

	ret = read(fd, &state, sizeof(state));
	if (ret < 0) {
		perror("read");
		return -1;
	}

	clock_gettime(CLOCK_MONOTONIC, &timestamp);
	*time1 = (timestamp.tv_sec * 1000000) + (timestamp.tv_nsec / 1000);
	return state;
}


// Praktikum 3
static inline int is_sling(__u16 state) {
	if ((state & 0xf000) != 0x0000) return 0;

	// Auslenkung testen
	if ((state & 0x0800) == 0x0000) {
		// Innen
		ausl_in = (state & 0x000f);
	} else {
		// Aussen
		ausl_out = (state & 0x000f);
	}

	return 1;
}


static char* decode(__u16 status) {
	if ((status & 0xf000) == 0x0000)		return "Auslenkungssensor";
	if ((status & 0xf000) == 0x1000)		return "Start/Ziel";
	if ((status & 0xf000) == 0x2000)		return "Gefahrenstelle";
	if ((status & 0xf000) == 0x3000)		return "NICHT KODIERT";
	if ((status & 0xf000) == 0x4000)		return "NICHT KODIERT";
	if ((status & 0xf000) == 0x5000)		return "NICHT KODIERT";
	
	if ((status & 0xf000) == 0x6000) {
		if ((status & 0x0400) == 0x0400)	return "Brückenende";
		else								return "Brückenanfang";
	}

	if ((status & 0xf000) == 0x7000)		return "NICHT KODIERT";
	if ((status & 0xf000) == 0x8000)		return "Kurve";
	if ((status & 0xf000) == 0x9000)		return "NICHT KODIERT";
	if ((status & 0xf000) == 0xa000)		return "NICHT KODIERT";
	if ((status & 0xf000) == 0xb000)		return "NICHT KODIERT";
	if ((status & 0xf000) == 0xc000)		return "NICHT KODIERT";
	if ((status & 0xf000) == 0xd000)		return "NICHT KODIERT";
	if ((status & 0xf000) == 0xe000)		return "NICHT KODIERT";
	if ((status & 0xf000) == 0xf000)		return "NICHT KODIERT";
	
	return "TYP UNBEKANNT";
}


static void exploration(int fdc) {
	__u16 state_old, state_act;
	unsigned long time_act, time_old;
	int rounds = 0, length, length_sum = 0, v;

	do {
		state_old = read_with_time(fdc, &time_old);
	} while ((state_old & 0xf000) != 0x1000);	// fahre bis Start
	
	while (rounds < 2) {
		do {
			state_act = read_with_time(fdc, &time_act);
		} while (is_sling(state_act));
		
		printf("old/act: 0x%x/0x%x\t%6.6ld [ms] - ",
			state_old, state_act,
			(time_act - time_old) / 1000);
		
		if (state_act == state_old) {
			v = 22000000 / (time_act - time_old);
			
			if ((state_act & 0xf000) == 0x1000) {
				rounds++;
				last_time = time_act;

				printf("Round: %d - %d [mm]\n", rounds, length_sum);
				
				length_sum = 0;
			}

			printf("v=0,%d\n", v);
		} else {
			length = v * (time_act - time_old) / 1000000;
			length_sum += length;

			printf("%s: length=%d\n", decode(state_act), length);
			
			add_to_liste(state_act, length);
		}
		
		state_old = state_act;
		time_old = time_act;
	}
}


void fahre_segment(unsigned int type, unsigned int laenge) {
	// Überprüfen, ob aussen oder innen
	bool innen = true;
	unsigned char speed = basis_speed_in;
	if ((type & 0x0800) == 0x0800) {
		innen = false;
		speed = basis_speed_out;
	}

	// Überprüfen, welches Segment vorliegt
	switch (type & 0xf000) {
	case 0x1000:
		// Start/Ziel
		speed = 0xff;
		break;
	case 0x2000:
		// Kreuzungsstelle
		speed = 0xc0;
		break;
	case 0x6000:
		// Brücke
		if ((type & 0x0400) == 0x0000) {
			// Anfang
			speed = 0xff;
		} else {
			// Ende
			speed -= 10;
		}
		break;
	case 0x8000:
		// Enge Kurve
		if (innen) {
			speed -= 30;
		} else {
			speed -= 10;
		}
		break;
	case 0x0000:
		// Auslenkung
		if (innen) {
			speed -= 40;
		} else {
			speed -=20;
		}
		break;
	}

	set_speed(fdc, speed);
}


static void tracking(int rounds_to_go) {
	struct _liste* position = root;
	int rounds = 0;
	__u16 state_act;
	unsigned long time_act, besttime = (-1), meantime = 0;

	do {
		bool kurve = false;
		ausl_in = -1;
		ausl_out = -1;
		
		// Wird solange ausgeführt wie Auslenkung vorhanden
		do {
			state_act = read_with_time(fdc, &time_act);

			// Auslenkung beachten
			if (kurve && (ausl_in != -1 || ausl_out != -1)) {
				// Auslenkung innen oder aussen vorhanden!
				printf("Auslenkung erkannt, Geschwindigkeit wird EINMAL angepasst\n");
				
				if (ausl_in != -1) {
					if (ausl_in > 4) fahre_segment(0x0000, 0);
				} else {
					if (ausl_out > 4) fahre_segment(0x0800, 0);
				}
				kurve = false;
			} else {
				kurve = true;
			}
		} while (is_sling(state_act));
		kurve = false;

		state_act = read_with_time(fdc, &time_act);
		
		printf("0x%04x (expected 0x%04x)\n", state_act, position->type);
		
		if (state_act != position->type) {
			printf("wrong position 0x%04x (0x%04x)\n", state_act, position->type);

			// Wenn Fahrzeug von Bahn genommen und an neuer Stelle aufgesetzt:
			// => Richtige Position in Liste finden!
			do {
				position = position->next;
			} while (state_act != position->type);
		}
		
		if ((state_act & 0xf000) == 0x1000) {	// Start/Ziel
			rounds++;
			rounds_to_go--;
			
			if (last_time) {
				if ((time_act - last_time) < besttime) besttime = time_act - last_time;
				meantime += time_act - last_time;
				
				printf("\n---> Runde: %d - Zeit: %ld.%03lds "
					"(Beste: %ld.%03lds, "
					"Mean: %ld.%03lds)\n",
					rounds,
				
					make_seconds((time_act - last_time)),
					make_mseconds((time_act - last_time)),
					make_seconds(besttime),
					make_mseconds(besttime),
					make_seconds(meantime / rounds),
					make_mseconds(meantime / rounds)
				);
			}
			
			last_time = time_act;
		}

		// Anfang Praktikum 2 (Kollisionszeit erkennen)
		if ((gegner_pos->type & 0xf000) == 0x2000 && position == gegner_pos) {
			struct timespec sleeptime;
			sleeptime.tv_sec = 0;
			sleeptime.tv_nsec = 10000000;
	
			clock_gettime(CLOCK_MONOTONIC, &eintritt_user);
			unsigned long time_in_us = timespec_to_ulong_microseconds(&eintritt_user);
			unsigned long gegnerzeit_in_us = timespec_to_ulong_microseconds(&eintritt_gegner);
			unsigned long diff_zeit_in_us = time_in_us-gegnerzeit_in_us;
	
			// Await enemy leaving the collision zone
			while (((gegner_pos->type & 0xf000) == 0x2000) && diff_zeit_in_us < 2500000) {
				printf("%4.4x - %4.4x - diff_zeit_in_us: %ld\n", position->type,
						gegner_pos->type, diff_zeit_in_us);
				
				set_speed(fdc, 0x0b);
				clock_nanosleep(CLOCK_MONOTONIC, 0, &sleeptime, NULL);
	
				// Get new time difference
				clock_gettime(CLOCK_MONOTONIC, &eintritt_user);
				time_in_us = timespec_to_ulong_microseconds(&eintritt_user);
				diff_zeit_in_us = time_in_us-gegnerzeit_in_us;
			}
	
			set_speed(fdc, 0x0c);
		}
		// Ende Praktikum 2

		// Anfang Praktikum 3 (Geschwindigkeitsanpassung)
		fahre_segment(position->type, position->length);
		// Ende Praktikum 3
		
		position = position->next;
	} while(rounds_to_go);
}


void* enemy_thread(void) {
	int fde = open("/dev/Carrera.other", O_RDONLY);
	if (fde < 0) {
		perror("/dev/Carrera.other");
		return -1;
	}

	// Anfang Praktikum 2
	gegner_pos = root;
	ssize_t ret;	
	__u16 alt, aktuell = 0;

	if ((ret = read(fde, &alt, sizeof(alt))) < 0) {
		perror("Enemy thread: read -> alt");
		return NULL;	
	}

	for (;;) {
		do {
			if ((ret = read(fde, &aktuell, sizeof(aktuell))) < 0) {
				perror("Enemy thread: read -> aktuell");
				return NULL;
			}

			struct timespec deadline;
			deadline.tv_sec = 0;
			deadline.tv_nsec = 10000000;

			clock_nanosleep(CLOCK_MONOTONIC, 0, &deadline, NULL);
		} while (alt == aktuell || (aktuell & 0xf000) == 0x0000);

		alt = aktuell;
		clock_gettime(CLOCK_MONOTONIC, &eintritt_gegner);

		while ((gegner_pos->type | 0x0800) != (aktuell | 0x0800)) {
			gegner_pos = gegner_pos->next;		
		}
	}
	// Ende Praktikum 2
}


int main(int argc, char** argv) {
	int rounds_to_go = 10;
	struct sigaction new_action;

	fdc = open( "/dev/Carrera", O_RDWR );
	if (fdc < 0) {
		perror("/dev/Carrera");
		return -1;
	}

	if (argc > 1) {
		basis_speed_in = basis_speed_out = (unsigned char) strtoul(argv[1], NULL, 0);
		
		if (argc > 2) rounds_to_go = (unsigned int) strtoul(argv[2], NULL, 0);
	}

	new_action.sa_handler = exithandler;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = 0;
	sigaction(SIGINT, &new_action, NULL);
	
	// Erforschung mit bestimmter Geschwindigkeit
	set_speed(fdc, 0xa0);	
	exploration(fdc);
	
	print_liste();

	// Auf Start vorbereiten!
	set_speed(fdc, 0x0);
	printf("Rennen starten (Enter drücken)!");
	char g = getchar();
	set_speed(fdc, basis_speed_in);
	
	// Gegner-Thread starten
	pthread_t gegner;
	pthread_create(&gegner, NULL, enemy_thread, NULL);

	tracking(rounds_to_go);
	
	set_speed(fdc, 0x0);
	
	return 0;
}
