// PW: 5a338851

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

typedef unsigned short __u16;


int main(int argc, char** argv, char** envp) {
	printf("Starting follow...\n");

	int fd;
	ssize_t ret;
	__u16 status;

	if ((fd = open("/dev/carrera-38", O_RDWR)) < 0) {
		printf("Fehler beim Öffnen!\nKorrekter Name der Gerätedatei?\n");
		return -1;
	}

	if ((ret = write(fd, "0x63", 5)) < 0) {
		printf("Fehler beim Schreiben!\nStimmt die Geschwindigkeit 0x63?\n");
		return -1;
	}

	for (;;) {
		if ((ret = read(fd, &status, sizeof(status))) < 0) {
			printf("Fehler beim Lesen!\n");
			return -1;
		}

		printf("Status: %4.4x\n", status);
	}

	close(fd);
	return 0;
}
