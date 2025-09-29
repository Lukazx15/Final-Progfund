#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

int main() {
    int chioce;

    while (1) {
        system("cls");
        printf("Business Contact Management System\n");
        printf("1. Add Contact\n");
        printf("2. Contact List\n");
        printf("3. Delete Contact\n");
        printf("4. Search Contact\n");
        printf("5. Update Contact\n");
        printf("0. Exit\n\n");
        printf("Enter your choice: ");
        scanf("%d", &chioce);

        switch (chioce) {
        case 0:
            printf("Exiting program\n");
            exit(0);

        case 1:
            printf(">> Add Contact selected\n");
            break;

        case 2:
            printf(">> Contact List selected\n");
            break;

        case 3:
            printf(">> Delete Contact selected\n");
            break;

        case 4:
            printf(">> Search Contact selected\n");
            break;

        case 5:
            printf(">> Update Contact selected\n");
            break;

        default:
            printf("Invalid Choice\n");
        }

        printf("Press Any Key To Continue...");
        getch();
    }

    return 0;
}
