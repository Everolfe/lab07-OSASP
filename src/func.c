#include <stdbool.h>
#include "func.h"

void clear_input() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void menu(void) {
    record_t current_record;
    int record_index = -1;
    char ch;

    bool IN_PROGRESS = true;
    do {
        printf("==========================\n"
            "Select an action:\n"
            "l - LST\n"
            "g - GET\n"
            "p - PUT\n"
            "q - QUIT\n"
            "==========================\n");
        printf("Command: ");

        ch = getchar();
        clear_input();
        switch (ch) {
            case 'l':
                for (int i = 0; i < MAX_RECORDS; i++) {
                    print_record(i);
                }
                break;
            case 'g':
                printf("Enter the record number: ");
                scanf("%d", &record_index);
                print_record(record_index);
                break;
            case 'p':
                if (record_index == -1) {
                    printf("Get the record with the GET option before using the PUT option.\n");
                    break;
                }
                record_t new_record;

                memset(&new_record, 0, sizeof(new_record));
                
                get_record(record_index, &current_record);
            
                printf("Enter the new name for the record: ");
                scanf("%s", new_record.name);

                printf("Enter the new address for the record: ");
                scanf("%s", new_record.address);

                printf("Enter the new semester for the record: ");
                scanf("%d", &new_record.semester);

                save_record(&current_record, &new_record, record_index);
                break;
            case 'q':
                IN_PROGRESS = false;
                break;
            default:
                printf("Wrong command\n");
                fflush(stdin);
                break;

        }
        getchar();
    } while (IN_PROGRESS);
}

void print_record(int record_index) {
    if (record_index < 0 || record_index >= MAX_RECORDS) {
        printf("Invalid record index.\n");
        return;
    }

    record_t record;
    struct flock fl;
    off_t offset = record_index * sizeof(record);

    // Пытаемся получить блокировку чтения без ожидания
    fl.l_type = F_RDLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = offset;
    fl.l_len = sizeof(record);
    fl.l_pid = getpid();

    if (fcntl(fd, F_SETLK, &fl) == -1) {
        if (errno == EAGAIN || errno == EACCES) {
            printf("Record %d is locked for writing. Cannot display now.\n", record_index);
        } else {
            perror("Error trying to lock record for reading");
        }
        return;
    }

    // Читаем запись
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("lseek");
        fl.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &fl);
        return;
    }

    if (read(fd, &record, sizeof(record)) != sizeof(record)) {
        perror("read");
        fl.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &fl);
        return;
    }

    // Снимаем блокировку
    fl.l_type = F_UNLCK;
    if (fcntl(fd, F_SETLK, &fl) == -1) {
        perror("Error unlocking record");
    }

    // Выводим информацию
    printf("==========================\n"
           "Record    %d:\n"
           "Name:     %s;\n"
           "Address:  %s;\n"
           "Semester: %d.\n"
           "==========================\n\n", 
           record_index, record.name, record.address, record.semester);
}

void get_record(int record_index, record_t *record) {
    if (record_index < 0 || record_index >= MAX_RECORDS) {
        printf("Invalid record index.\n");
        return;
    }

    struct flock fl;
    off_t offset = record_index * sizeof(*record);

    // Пытаемся получить блокировку чтения с ожиданием
    fl.l_type = F_RDLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = offset;
    fl.l_len = sizeof(*record);
    fl.l_pid = getpid();

    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        perror("Error locking record for reading");
        return;
    }

    // Читаем запись
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("lseek");
        fl.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &fl);
        return;
    }

    if (read(fd, record, sizeof(*record)) != sizeof(*record)) {
        perror("read");
        fl.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &fl);
        return;
    }

    // Снимаем блокировку
    fl.l_type = F_UNLCK;
    if (fcntl(fd, F_SETLK, &fl) == -1) {
        perror("Error unlocking record");
    }
}

void modify_record(int record_index, record_t *record) {
    off_t offset = record_index * sizeof(*record);
    lseek(fd, offset, SEEK_SET);

    write(fd, record, sizeof(*record));
}

void save_record(record_t *record, record_t *new_record, int record_index) {
    if (record_index < 0 || record_index >= MAX_RECORDS) {
        printf("Invalid record number.\n");
        return;
    }

    while (true) {
        struct flock fl = {0};
        fl.l_type = F_WRLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start = record_index * sizeof(*record);
        fl.l_len = sizeof(*record);

        printf("Trying to acquire write lock for record %d...\n", record_index);
        if (fcntl(fd, F_SETLKW, &fl) == -1) {
            perror("fcntl - write lock");
            exit(EXIT_FAILURE);
        }
        printf("Write lock acquired for record %d.\n", record_index);

        // Имитация длительной работы
        sleep(5);

        record_t current_record;
        lseek(fd, record_index * sizeof(current_record), SEEK_SET);
        if (read(fd, &current_record, sizeof(current_record)) != sizeof(current_record)) {
            perror("read");
            fl.l_type = F_UNLCK;
            fcntl(fd, F_SETLK, &fl);
            return;
        }

        if (memcmp(&current_record, record, sizeof(record_t)) != 0) {
            printf("WARNING: Record %d was modified by another process.\n", record_index);
            printf("Current contents:\n");
            printf("Name: %s\n", current_record.name);
            printf("Address: %s\n", current_record.address);
            printf("Semester: %d\n", current_record.semester);
            printf("Your modified record was NOT saved.\n");

            fl.l_type = F_UNLCK;
            fcntl(fd, F_SETLK, &fl);

            char answer;
            printf("Do you want to retry saving with updated data? (y/n): ");
            scanf(" %c", &answer);
            clear_input();

            if (answer == 'y' || answer == 'Y') {
                // Обновляем локальную копию исходной записи
                memcpy(record, &current_record, sizeof(record_t));
                continue;  // повторяем попытку сохранения
            } else {
                printf("Save operation aborted by user.\n");
                return;
            }
        }

        lseek(fd, record_index * sizeof(current_record), SEEK_SET);
        if (write(fd, new_record, sizeof(*new_record)) != sizeof(*new_record)) {
            perror("write");
        } else {
            printf("Record %d successfully saved.\n", record_index);
        }

        fl.l_type = F_UNLCK;
        if (fcntl(fd, F_SETLK, &fl) == -1) {
            perror("fcntl - unlock write");
            exit(EXIT_FAILURE);
        }
        printf("Write lock released for record %d.\n", record_index);
        break; // сохранение прошло успешно
    }
}




void fill_file(int fd) {
    record_t arrayRecords[MAX_RECORDS];
    for (int i = 0; i < MAX_RECORDS; i++) {
        sprintf(arrayRecords[i].name, "Name%d", i);
        sprintf(arrayRecords[i].address, "Address%d", i);
        arrayRecords[i].semester = i;
    }
    write(fd, arrayRecords, sizeof(arrayRecords));
}