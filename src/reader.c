#include "record.h"
#include <fcntl.h>
#include <math.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

int fd;
struct record_s *data = NULL;
int totalRecords;
int size;

struct record_s saved;
struct record_s edited;
int savedIndex = -1;

void openFile(char *filename) {
    fd = open(filename, O_RDWR);
    
    struct stat stat;
    fstat(fd, &stat);

    size = stat.st_size;
    totalRecords = size / sizeof(struct record_s);
}

void closeFile() {
    close(fd);
}

void mapFile() {
    data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
}

void unmapFile() {
    munmap(data, size);
}

void printRecord(struct record_s record) {
    printf("NAME: %s\nADDRESS: %s\nSEMSETER: %d\n", record.name, record.address, record.semester);
}

void list() {
    for (int i = 0; i < totalRecords; i++) {
        printf("RECORD № %d\n", i);
        printRecord(data[i]);
    }
}

void get(int index) {
    if (index >= totalRecords || index < 0) {
        printf("RECORD № %d NOT FOUND!!!", index);
        return;
    }

    memcpy((void*)&saved, (void*)&data[index], sizeof(saved));
    memcpy((void*)&edited, (void*)&saved, sizeof(edited));

    savedIndex = index;
}

void edit() {
    if (savedIndex == -1) {
        printf("Nothing to edit\n");
        return;
    }
    
    printf("Enter new name: ");
    fgets(edited.name, sizeof(edited.name), stdin);
    edited.name[strcspn(edited.name, "\n")] = 0;
    printf("Enter new address: ");
    fgets(edited.address, sizeof(edited.address), stdin);
    edited.address[strcspn(edited.address, "\n")] = 0;
    int code;
    do {
        printf("Enter new semester: ");
        code = scanf("%hhu\n", &edited.semester);
    } while (code != 1);
}

void put() {
    if (savedIndex == -1) {
        printf("Nothing to save.\n");
        return;
    }

    struct flock lock = {
        .l_pid = getpid(),
        .l_start = savedIndex * sizeof(saved),
        .l_len = sizeof(saved),
        .l_type = F_WRLCK,
        .l_whence = SEEK_SET,
    };
    fcntl(fd, F_SETLK, &lock);
    lock.l_type = F_UNLCK;

    if (memcmp(&data[savedIndex], &saved, sizeof(saved)) != 0) {
        fcntl(fd, F_SETLK, &lock);

        printf("RECORD WAS MODIFIED!!!\nNEW CONTENT:\n");
        printRecord(data[savedIndex]);

        char answer[100] = {0};
        do {
            printf("ENTER 'Y' TO SAVE IT ANYWAY OR 'N' TO ABORT SAVING: ");
            scanf("%s\n", (char*)&answer);
        } while (answer[0] != 'Y' || answer[0] != 'N');

        if (answer[0] == 'N')
            return;
    }

    data[savedIndex] = edited;
    fcntl(fd, F_SETLK, &lock);
    savedIndex = -1;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Use reader [file name].\n");
        return 0xDEAD;
    }

    openFile(argv[1]);
    mapFile();

    while (1) {
        printf("COMMAND: ");
        char command[100] = {0};
        fgets(command, 100, stdin);
        int number;

        if (!strcmp(command, "LST\n")) {
            list();
            continue;
        }
        if (sscanf(command, "GET %d\n", &number) == 1) {
            get(number);
            continue;
        }
        if (!strcmp(command, "EDIT\n")) {
            edit();
            continue;
        }
        if (!strcmp(command, "PUT\n")) {
            put();
            continue;
        }
    }

    unmapFile();
    closeFile();
}