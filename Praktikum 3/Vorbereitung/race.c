/*
	Quellcode zum Praktikum "Echtzeitsysteme" im Fachbereich Elektrotechnik
	und Informatik der Hochschule Niederrhein.

	ACHTUNG: Version zur Vorbereitung zum Praktikum 3!

	Fuer die Generierung wird die Realzeitbibliothek "rt" benoetigt.
*/

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <time.h>
#include <sys/time.h>
#include <asm/types.h>


static void set_speed(int fd, unsigned char Speed);

static unsigned char basis_speed_in = 0x00, basis_speed_out = 0x00;
static unsigned long last_time;
static int fdc = 0;


typedef struct _liste {
	int type;
	int length;
	struct _liste* next;
} streckenliste;

static streckenliste* root = NULL; 

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


static void exithandler(int signr) {
	if (fdc) set_speed(fdc, 0x0);
	exit(0);
}


static void set_speed(int fd, unsigned char speed) {
	int ret;
	char buffer[256];

	printf("new speed: 0x%x\n", speed );
	
	if (fd > 0)	ret = write(fd, &speed, sizeof(speed));
	
	if (ret < 0) {
		read(fd, buffer, sizeof(buffer));
		printf("%s\n", buffer);
		close(fd);
		exit( -1 );
	}
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


static inline int is_sling(__u16 state) {
	if ((state & 0xf000) != 0x0000) return 0;

	// Es liegt eine Auslenkung vor...
	// TODO: Die Auslenkung wird abhaengig von der Spur
	//       in auslenkung_innen bzw. auslenkung_aussen abgespeichert

	return 1;
}


static void exploration(int fdc) {
	add_to_liste(0x1000, 100);
}


static void tracking(int rounds_to_go) {
	struct _liste* position = root;
	int rounds = 0;
	__u16 state_act;
	unsigned long time_act;

	do {
		do {
			state_act = read_with_time(fdc, &time_act);
		} while (is_sling(state_act));

		state_act = read_with_time(fdc, &time_act);
		
		if ((state_act & 0xf000) != position->type) {
			printf("wrong position 0x%04x (0x%04x)\n", state_act, position->type);
		}

		if ((state_act & 0xf000) == 0x1000) { // Start/Ziel
			rounds++;
			rounds_to_go--;
			printf("\n---> Runde: %d\n", rounds );
			last_time = time_act;
		}

		position = position->next;
	} while (rounds_to_go);
}


int main(int argc, char **argv) {
	int rounds_to_go = 10;
	struct sigaction new_action;

	fdc = open("/dev/ezsv3", O_RDWR);
	if (fdc < 0) {
		perror("/dev/ezsv3");
		return -1;
	}

	if (argc > 1) {
		basis_speed_in = basis_speed_out= (unsigned char) strtoul(argv[1], NULL, 0);
		
		if (argc > 2) {
			rounds_to_go = (unsigned int) strtoul(argv[2], NULL, 0);
		}
	}

	new_action.sa_handler = exithandler;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = 0;
	sigaction(SIGINT, &new_action, NULL);

	set_speed(fdc, basis_speed_in);
	exploration(fdc);
	tracking(rounds_to_go);
	set_speed(fdc, 0x0);
	
	return 0;
}
