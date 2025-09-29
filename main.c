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
void listContacts();
void deleteContact();
void searchContact();
void updateContact();

int main() {
    int choice;

    while (1) {
        system("cls");
        printf("Business Contact Management System\n");
        printf("1. Add Contact\n");
        printf("2. Contact List\n");
        printf("3. Delete Contact\n");
        printf("4. Search Contact (contains with strstr)\n");
        printf("5. Update Contact (fixed)\n");
        printf("0. Exit\n\n");
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) choice = -1;
        getchar();

        switch (choice) {
        case 0:
            printf("Exiting program\n");
            exit(0);
        case 1:
            addContact();
            break;
        case 2:
            listContacts();
            break;
        case 3:
            deleteContact();
            break;
        case 4:
            searchContact();
            break;
        case 5:
            updateContact();
            break;
        default:
            printf("Invalid Choice\n");
        }

        printf("\nPress Any Key To Continue...");
        getch();
    }
    return 0;
}

// Add 
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

// List 
void listContacts(){
    FILE *rf = fopen("contacts.csv", "r");
    if (!rf){
        printf("No file or cannot open contacts.csv\n");
        return;
    }

    char line[512];
    int i = 0;

    printf("\nNo. | Company | Contact | Phone | Email\n");
    printf("------------------------------------------------------------\n");

    while (fgets(line, sizeof(line), rf)){
        char *company = strtok(line, ",");
        char *person  = strtok(NULL, ",");
        char *phone   = strtok(NULL, ",");
        char *email   = strtok(NULL, ",\n");

        if (!company || !person || !phone || !email) continue;
        printf("%3d) %s | %s | %s | %s\n", ++i, company, person, phone, email);
    }

    if (i == 0) printf("No contacts in file.\n");
    fclose(rf);
}

// Delete by Company name
void deleteContact(){
    char key[100];
    printf("Enter company name to delete (exact): ");
    fgets(key, sizeof(key), stdin);
    key[strcspn(key, "\n")] = 0;

    FILE *rf = fopen("contacts.csv", "r");
    if (!rf){ printf("No file or cannot open contacts.csv\n"); return; }
    FILE *wf = fopen("contacts.tmp", "w");
    if (!wf){ fclose(rf); printf("Cannot create temp file.\n"); return; }

    char line[512];
    int deleted = 0;

    while (fgets(line, sizeof(line), rf)){
        char linecpy[512];
        strncpy(linecpy, line, sizeof(linecpy));
        linecpy[sizeof(linecpy)-1] = 0;

        char *company = strtok(linecpy, ",");
        char *person  = strtok(NULL, ",");
        char *phone   = strtok(NULL, ",");
        char *email   = strtok(NULL, ",\n");

        if (company && person && phone && email){
            if (!deleted && strcmp(company, key) == 0){
                deleted = 1;
                continue;
            }
        }
        fputs(line, wf);
    }

    fclose(rf); fclose(wf);
    remove("contacts.csv");
    rename("contacts.tmp", "contacts.csv");

    if (deleted) printf("Deleted successfully.\n");
    else         printf("Company not found.\n");
}

// Search
void searchContact(){
    char key[100];
    printf("Enter keyword (company/contact) to search: ");
    fgets(key, sizeof(key), stdin);
    key[strcspn(key, "\n")] = 0;

    FILE *rf = fopen("contacts.csv", "r");
    if (!rf){ printf("No file or cannot open contacts.csv\n"); return; }

    char line[512];
    int found = 0;

    printf("\nSearch results:\n");
    printf("------------------------------------------------------------\n");
    while (fgets(line, sizeof(line), rf)){
        char linecpy[512];
        strncpy(linecpy, line, sizeof(linecpy));
        linecpy[sizeof(linecpy)-1] = 0;

        char *company = strtok(linecpy, ",");
        char *person  = strtok(NULL, ",");
        char *phone   = strtok(NULL, ",");
        char *email   = strtok(NULL, ",\n");

        if (!company || !person || !phone || !email) continue;

        if ((strstr(company, key) != NULL) || (strstr(person, key) != NULL)){
            printf("Company: %s\nContact: %s\nPhone  : %s\nEmail  : %s\n",
                   company, person, phone, email);
            printf("------------------------------------------------------------\n");
            found = 1;
        }
    }
    if (!found) printf("No matching record.\n");
    fclose(rf);
}

// Update
void updateContact(){
    char key[100];
    printf("Enter keyword (company/contact) to update: ");
    fgets(key, sizeof(key), stdin);
    key[strcspn(key, "\n")] = 0;

    FILE *rf = fopen("contacts.csv", "r");
    if (!rf){ printf("No file or cannot open contacts.csv\n"); return; }
    FILE *wf = fopen("contacts.tmp", "w");
    if (!wf){ fclose(rf); printf("Cannot create temp file.\n"); return; }

    char line[512];
    int updated = 0;

    while (fgets(line, sizeof(line), rf)){
        char linecpy[512];
        strncpy(linecpy, line, sizeof(linecpy));
        linecpy[sizeof(linecpy)-1] = 0;


        char *t_company = strtok(linecpy, ",");
        char *t_person  = strtok(NULL, ",");
        char *t_phone   = strtok(NULL, ",");
        char *t_email   = strtok(NULL, ",\n");

        if (!t_company || !t_person || !t_phone || !t_email){
            fputs(line, wf);
            continue;
        }


        char company[50], person[50], phone[50], email[50];
        strncpy(company, t_company, sizeof(company)-1); company[sizeof(company)-1]=0;
        strncpy(person , t_person , sizeof(person )-1); person [sizeof(person )-1]=0;
        strncpy(phone  , t_phone  , sizeof(phone  )-1); phone  [sizeof(phone  )-1]=0;
        strncpy(email  , t_email  , sizeof(email  )-1); email  [sizeof(email  )-1]=0;

        if (!updated && (strstr(company, key) != NULL || strstr(person, key) != NULL)){
            int choice;
            char buf[100];
            printf("\nFound: %s, %s, %s, %s\n", company, person, phone, email);
            printf("Update which field? (1=Company, 2=Contact, 3=Phone, 4=Email): ");
            if (scanf("%d", &choice) != 1) choice = 0;
            getchar();

            switch (choice){
                case 1:
                    printf("New Company: ");
                    fgets(buf, sizeof(buf), stdin);
                    buf[strcspn(buf,"\n")] = 0;
                    strncpy(company, buf, sizeof(company)-1);
                    company[sizeof(company)-1]=0;
                    break;
                case 2:
                    printf("New Contact: ");
                    fgets(buf, sizeof(buf), stdin);
                    buf[strcspn(buf,"\n")] = 0;
                    strncpy(person, buf, sizeof(person)-1);
                    person[sizeof(person)-1]=0;
                    break;
                case 3:
                    printf("New Phone  : ");
                    fgets(buf, sizeof(buf), stdin);
                    buf[strcspn(buf,"\n")] = 0;
                    strncpy(phone, buf, sizeof(phone)-1);
                    phone[sizeof(phone)-1]=0;
                    break;
                case 4:
                    printf("New Email  : ");
                    fgets(buf, sizeof(buf), stdin);
                    buf[strcspn(buf,"\n")] = 0;
                    strncpy(email, buf, sizeof(email)-1);
                    email[sizeof(email)-1]=0;
                    break;
                default:
                    printf("Skip update.\n");
                    break;
            }
            fprintf(wf, "%s,%s,%s,%s\n", company, person, phone, email);
            updated = 1;
        } else {

            fputs(line, wf);
        }
    }

    fclose(rf); fclose(wf);
    remove("contacts.csv");
    rename("contacts.tmp", "contacts.csv");

    if (updated) printf("Updated successfully.\n");
    else         printf("No matching record to update.\n");
}
