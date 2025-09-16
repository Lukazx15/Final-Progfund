#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>

struct Contact {
    char company[50];
    char person[50];
    char phone[50];
    char email[50];
} c;

FILE *fp;

void addContact();

int main() {
    int choice;

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
        scanf("%d", &choice);
        getchar();

        switch (choice) {
        case 0:
            printf("Exiting program\n");
            exit(0);

        case 1:
            addContact();
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

        printf("Press Any Key To Continue:");
        getch();
    }

    return 0;
}

void addContact() {
    fp = fopen("contacts.csv", "a");
    if (!fp) {
        printf("Error opening file!\n");
        return;
    }

    printf("Enter company name: ");
    fgets(c.company, sizeof(c.company), stdin);
    c.company[strcspn(c.company, "\n")] = 0;

    printf("Enter contact person name: ");
    fgets(c.person, sizeof(c.person), stdin);
    c.person[strcspn(c.person, "\n")] = 0;

    printf("Enter phone number: ");
    fgets(c.phone, sizeof(c.phone), stdin);
    c.phone[strcspn(c.phone, "\n")] = 0;

    printf("Enter email: ");
    fgets(c.email, sizeof(c.email), stdin);
    c.email[strcspn(c.email, "\n")] = 0;

    
    fprintf(fp, "%s,%s,%s,%s\n", c.company, c.person, c.phone, c.email);
    fclose(fp);

    printf("Contact Added Successfully\n");
}
