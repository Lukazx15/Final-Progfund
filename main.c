#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
    #include <conio.h>
    #define CLEAR_SCREEN "cls"
#else
    #define CLEAR_SCREEN "clear"
    int getch(void) {
        int ch;
        system("stty -echo -icanon");
        ch = getchar();
        system("stty echo icanon");
        return ch;
    }
#endif

#define MAX_FIELD_LEN 100
#define MAX_LINE_LEN 512

struct Contact {
    char company[MAX_FIELD_LEN];
    char person[MAX_FIELD_LEN];
    char phone[MAX_FIELD_LEN];
    char email[MAX_FIELD_LEN];
};

void addContact();
void listContacts();
void deleteContact();
void searchContact();
void updateContact();
void sanitizeInput(char *str);
void escapeCSV(const char *input, char *output, size_t output_size);
void unescapeCSV(char *str);
void trimWhitespace(char *str);
int validateEmail(const char *email);
int validatePhone(const char *phone);
void clearInputBuffer();

int main() {
    int choice;
    while (1) {
        system(CLEAR_SCREEN);
        printf("===========================================\n");
        printf("  Business Contact Management System\n");
        printf("===========================================\n");
        printf("1. Add Contact\n");
        printf("2. Contact List\n");
        printf("3. Delete Contact\n");
        printf("4. Search Contact\n");
        printf("5. Update Contact\n");
        printf("0. Exit\n");
        printf("===========================================\n");
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) {
            choice = -1;
            clearInputBuffer();
        } else {
            clearInputBuffer();
        }
        switch (choice) {
            case 0:
                printf("\nThank you for using the system. Goodbye!\n");
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
                printf("\n[ERROR] Invalid choice! Please try again.\n");
        }
        printf("\nPress any key to continue...");
        getch();
    }
    return 0;
}

void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void trimWhitespace(char *str) {
    if (!str || *str == '\0') return;
    char *start = str;
    while (isspace((unsigned char)*start)) start++;
    if (*start == '\0') {
        *str = '\0';
        return;
    }
    char *end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    size_t len = (end - start) + 1;
    memmove(str, start, len);
    str[len] = '\0';
}

void sanitizeInput(char *str) {
    if (!str || *str == '\0') return;
    while (*str && (*str == '=' || *str == '+' || *str == '-' || *str == '@' || *str == '\t')) {
        memmove(str, str + 1, strlen(str));
    }
    trimWhitespace(str);
}

void escapeCSV(const char *input, char *output, size_t output_size) {
    if (!input || !output || output_size == 0) return;
    size_t j = 0;
    int needs_quotes = 0;
    for (size_t i = 0; i < strlen(input); i++) {
        if (input[i] == ',' || input[i] == '"' || input[i] == '\n' || input[i] == '\r') {
            needs_quotes = 1;
            break;
        }
    }
    if (needs_quotes && j < output_size - 1) output[j++] = '"';
    for (size_t i = 0; input[i] && j < output_size - 2; i++) {
        if (input[i] == '"' && j < output_size - 3) {
            output[j++] = '"';
            output[j++] = '"';
        } else if (j < output_size - 2) {
            output[j++] = input[i];
        }
    }
    if (needs_quotes && j < output_size - 1) output[j++] = '"';
    output[j] = '\0';
}

void unescapeCSV(char *str) {
    if (!str || *str == '\0') return;
    size_t len = strlen(str);
    if (len >= 2 && str[0] == '"' && str[len-1] == '"') {
        memmove(str, str + 1, len - 1);
        str[len - 2] = '\0';
        len -= 2;
    }
    char *read = str;
    char *write = str;
    while (*read) {
        if (*read == '"' && *(read + 1) == '"') {
            *write++ = '"';
            read += 2;
        } else {
            *write++ = *read++;
        }
    }
    *write = '\0';
}

int validateEmail(const char *email) {
    if (!email || strlen(email) == 0) return 0;
    const char *at = strchr(email, '@');
    const char *dot = strrchr(email, '.');
    return (at != NULL && dot != NULL && at < dot && at > email && dot < email + strlen(email) - 1);
}

int validatePhone(const char *phone) {
    if (!phone || strlen(phone) == 0) return 0;
    int has_digit = 0;
    for (size_t i = 0; phone[i]; i++) {
        if (isdigit((unsigned char)phone[i])) {
            has_digit = 1;
        } else if (phone[i] != '-' && phone[i] != '+' && phone[i] != '(' && 
                   phone[i] != ')' && phone[i] != ' ' && phone[i] != '.') {
            return 0;
        }
    }
    return has_digit;
}

void addContact() {
    struct Contact c;
    char temp[MAX_FIELD_LEN * 2];
    printf("\n--- Add New Contact ---\n");
    printf("Enter company name: ");
    if (!fgets(temp, sizeof(temp), stdin)) { printf("[ERROR] Failed to read input!\n"); return; }
    temp[strcspn(temp, "\n")] = '\0';
    sanitizeInput(temp);
    if (strlen(temp) == 0) { printf("[ERROR] Company name cannot be empty!\n"); return; }
    if (strlen(temp) >= MAX_FIELD_LEN) { printf("[ERROR] Company name is too long (max %d chars)!\n", MAX_FIELD_LEN - 1); return; }
    strncpy(c.company, temp, MAX_FIELD_LEN - 1); c.company[MAX_FIELD_LEN - 1] = '\0';

    printf("Enter contact person name: ");
    if (!fgets(temp, sizeof(temp), stdin)) { printf("[ERROR] Failed to read input!\n"); return; }
    temp[strcspn(temp, "\n")] = '\0';
    sanitizeInput(temp);
    if (strlen(temp) == 0) { printf("[ERROR] Person name cannot be empty!\n"); return; }
    if (strlen(temp) >= MAX_FIELD_LEN) { printf("[ERROR] Person name is too long (max %d chars)!\n", MAX_FIELD_LEN - 1); return; }
    strncpy(c.person, temp, MAX_FIELD_LEN - 1); c.person[MAX_FIELD_LEN - 1] = '\0';

    printf("Enter phone number: ");
    if (!fgets(temp, sizeof(temp), stdin)) { printf("[ERROR] Failed to read input!\n"); return; }
    temp[strcspn(temp, "\n")] = '\0';
    sanitizeInput(temp);
    if (strlen(temp) == 0) { printf("[ERROR] Phone number cannot be empty!\n"); return; }
    if (!validatePhone(temp)) { printf("[ERROR] Invalid phone number format!\n"); return; }
    if (strlen(temp) >= MAX_FIELD_LEN) { printf("[ERROR] Phone number is too long (max %d chars)!\n", MAX_FIELD_LEN - 1); return; }
    strncpy(c.phone, temp, MAX_FIELD_LEN - 1); c.phone[MAX_FIELD_LEN - 1] = '\0';

    printf("Enter email: ");
    if (!fgets(temp, sizeof(temp), stdin)) { printf("[ERROR] Failed to read input!\n"); return; }
    temp[strcspn(temp, "\n")] = '\0';
    sanitizeInput(temp);
    if (strlen(temp) == 0) { printf("[ERROR] Email cannot be empty!\n"); return; }
    if (!validateEmail(temp)) { printf("[ERROR] Invalid email format!\n"); return; }
    if (strlen(temp) >= MAX_FIELD_LEN) { printf("[ERROR] Email is too long (max %d chars)!\n", MAX_FIELD_LEN - 1); return; }
    strncpy(c.email, temp, MAX_FIELD_LEN - 1); c.email[MAX_FIELD_LEN - 1] = '\0';

    FILE *fp = fopen("contacts.csv", "a");
    if (!fp) { printf("[ERROR] Cannot open file for writing!\n"); return; }

    char esc_company[MAX_FIELD_LEN * 2];
    char esc_person[MAX_FIELD_LEN * 2];
    char esc_phone[MAX_FIELD_LEN * 2];
    char esc_email[MAX_FIELD_LEN * 2];

    escapeCSV(c.company, esc_company, sizeof(esc_company));
    escapeCSV(c.person, esc_person, sizeof(esc_person));
    escapeCSV(c.phone, esc_phone, sizeof(esc_phone));
    escapeCSV(c.email, esc_email, sizeof(esc_email));

    fprintf(fp, "%s,%s,%s,%s\n", esc_company, esc_person, esc_phone, esc_email);
    fclose(fp);
    printf("\n[SUCCESS] Contact added successfully!\n");
}

void listContacts() {
    char filter[MAX_FIELD_LEN];
    printf("\n--- Contact List ---\n");
    printf("Enter keyword to search company (or press Enter to show all): ");
    if (!fgets(filter, sizeof(filter), stdin)) { printf("[ERROR] Failed to read input!\n"); return; }
    filter[strcspn(filter, "\n")] = '\0';
    trimWhitespace(filter);

    FILE *fp = fopen("contacts.csv", "r");
    if (!fp) { printf("[INFO] No contacts file found or cannot open.\n"); return; }

    char line[MAX_LINE_LEN];
    int count = 0;
    int use_filter = (int)(strlen(filter) > 0);

    char filter_lower[MAX_FIELD_LEN] = "";
    if (use_filter) {
        strncpy(filter_lower, filter, MAX_FIELD_LEN - 1);
        filter_lower[MAX_FIELD_LEN - 1] = '\0';
        for (int i = 0; filter_lower[i]; i++) filter_lower[i] = (char)tolower((unsigned char)filter_lower[i]);
    }

    printf("\n%-4s | %-20s | %-20s | %-15s | %-30s\n", "No.", "Company", "Contact", "Phone", "Email");
    printf("------------------------------------------------------------------------------------------------\n");

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n\r")] = '\0';
        if (strlen(line) == 0) continue;

        char linecpy[MAX_LINE_LEN];
        strncpy(linecpy, line, sizeof(linecpy) - 1);
        linecpy[sizeof(linecpy) - 1] = '\0';

        char company[MAX_FIELD_LEN] = "";
        char person[MAX_FIELD_LEN] = "";
        char phone[MAX_FIELD_LEN] = "";
        char email[MAX_FIELD_LEN] = "";

        char *fields[4] = {company, person, phone, email};
        int field_idx = 0;
        int in_quotes = 0;
        char *src = linecpy;
        char *dst = fields[0];

        while (*src && field_idx < 4) {
            if (*src == '"') { in_quotes = !in_quotes; src++; }
            else if (*src == ',' && !in_quotes) { *dst = '\0'; field_idx++; if (field_idx < 4) dst = fields[field_idx]; src++; }
            else { *dst++ = *src++; }
        }
        *dst = '\0';

        unescapeCSV(company);
        unescapeCSV(person);
        unescapeCSV(phone);
        unescapeCSV(email);

        if (strlen(company) == 0 || strlen(person) == 0) continue;

        if (use_filter) {
            char company_lower[MAX_FIELD_LEN];
            strncpy(company_lower, company, MAX_FIELD_LEN - 1);
            company_lower[MAX_FIELD_LEN - 1] = '\0';
            for (int i = 0; company_lower[i]; i++) company_lower[i] = (char)tolower((unsigned char)company_lower[i]);
            if (strstr(company_lower, filter_lower) == NULL) continue;
        }

        count++;
        printf("%-4d | %-20.20s | %-20.20s | %-15.15s | %-30.30s\n",
               count, company, person, phone, email);
    }
    fclose(fp);

    if (count == 0) {
        if (use_filter) printf("[INFO] No contacts found with keyword '%s'.\n", filter);
        else printf("[INFO] No contacts in the system.\n");
    } else {
        printf("------------------------------------------------------------------------------------------------\n");
        printf("Total: %d contact(s) displayed\n", count);
    }
}

void deleteContact() {
    char key[MAX_FIELD_LEN];
    printf("\n--- Delete Contact ---\n");
    printf("Enter company name to delete: ");
    if (!fgets(key, sizeof(key), stdin)) { printf("[ERROR] Failed to read input!\n"); return; }
    key[strcspn(key, "\n")] = '\0';
    trimWhitespace(key);
    if (strlen(key) == 0) { printf("[ERROR] Company name cannot be empty!\n"); return; }

    FILE *rf = fopen("contacts.csv", "r");
    if (!rf) { printf("[ERROR] No contacts file found!\n"); return; }

    FILE *wf = fopen("contacts.tmp", "w");
    if (!wf) { fclose(rf); printf("[ERROR] Cannot create temporary file!\n"); return; }

    char line[MAX_LINE_LEN];
    int deleted = 0;

    while (fgets(line, sizeof(line), rf)) {
        line[strcspn(line, "\n\r")] = '\0';
        if (strlen(line) == 0) continue;

        char linecpy[MAX_LINE_LEN];
        strncpy(linecpy, line, sizeof(linecpy) - 1);
        linecpy[sizeof(linecpy) - 1] = '\0';

        char company[MAX_FIELD_LEN] = "";
        char *src = linecpy;
        char *dst = company;
        int in_quotes = 0;

        while (*src && *src != ',' && (size_t)(dst - company) < MAX_FIELD_LEN - 1) {
            if (*src == '"') { in_quotes = !in_quotes; src++; }
            else { *dst++ = *src++; }
        }
        *dst = '\0';

        unescapeCSV(company);

        if (!deleted && strlen(company) > 0 && strcmp(company, key) == 0) {
            deleted = 1;
            continue;
        }
        fprintf(wf, "%s\n", line);
    }

    fclose(rf);
    fclose(wf);

    if (remove("contacts.csv") != 0) { printf("[ERROR] Failed to remove old file!\n"); remove("contacts.tmp"); return; }
    if (rename("contacts.tmp", "contacts.csv") != 0) { printf("[ERROR] Failed to rename temporary file!\n"); return; }

    if (deleted) printf("\n[SUCCESS] Contact deleted successfully!\n");
    else printf("\n[INFO] Company '%s' not found.\n", key);
}

void searchContact() {
    char key[MAX_FIELD_LEN];
    printf("\n--- Search Contact ---\n");
    printf("Enter keyword (company or person name): ");
    if (!fgets(key, sizeof(key), stdin)) { printf("[ERROR] Failed to read input!\n"); return; }
    key[strcspn(key, "\n")] = '\0';
    trimWhitespace(key);
    if (strlen(key) == 0) { printf("[ERROR] Search keyword cannot be empty!\n"); return; }

    FILE *fp = fopen("contacts.csv", "r");
    if (!fp) { printf("[ERROR] No contacts file found!\n"); return; }

    char line[MAX_LINE_LEN];
    int found = 0;

    printf("\n--- Search Results ---\n");
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n\r")] = '\0';
        if (strlen(line) == 0) continue;

        char linecpy[MAX_LINE_LEN];
        strncpy(linecpy, line, sizeof(linecpy) - 1);
        linecpy[sizeof(linecpy) - 1] = '\0';

        char company[MAX_FIELD_LEN] = "";
        char person[MAX_FIELD_LEN] = "";
        char phone[MAX_FIELD_LEN] = "";
        char email[MAX_FIELD_LEN] = "";

        char *fields[4] = {company, person, phone, email};
        int field_idx = 0;
        int in_quotes = 0;
        char *src = linecpy;
        char *dst = fields[0];

        while (*src && field_idx < 4) {
            if (*src == '"') { in_quotes = !in_quotes; src++; }
            else if (*src == ',' && !in_quotes) { *dst = '\0'; field_idx++; if (field_idx < 4) dst = fields[field_idx]; src++; }
            else { *dst++ = *src++; }
        }
        *dst = '\0';

        unescapeCSV(company);
        unescapeCSV(person);
        unescapeCSV(phone);
        unescapeCSV(email);

        if (strlen(company) == 0) continue;

        if (strstr(company, key) != NULL || strstr(person, key) != NULL) {
            printf("\nCompany : %s\n", company);
            printf("Contact : %s\n", person);
            printf("Phone   : %s\n", phone);
            printf("Email   : %s\n", email);
            printf("-----------------------------------\n");
            found = 1;
        }
    }

    if (!found) printf("[INFO] No matching contacts found.\n");
    fclose(fp);
}

void updateContact() {
    char key[MAX_FIELD_LEN];
    printf("\n--- Update Contact ---\n");
    printf("Enter company name to update: ");
    if (!fgets(key, sizeof(key), stdin)) { printf("[ERROR] Failed to read input!\n"); return; }
    key[strcspn(key, "\n")] = '\0';
    trimWhitespace(key);
    if (strlen(key) == 0) { printf("[ERROR] Company name cannot be empty!\n"); return; }

    FILE *rf = fopen("contacts.csv", "r");
    if (!rf) { printf("[ERROR] No contacts file found!\n"); return; }

    FILE *wf = fopen("contacts.tmp", "w");
    if (!wf) { fclose(rf); printf("[ERROR] Cannot create temporary file!\n"); return; }

    char line[MAX_LINE_LEN];
    int updated = 0;

    while (fgets(line, sizeof(line), rf)) {
        line[strcspn(line, "\n\r")] = '\0';
        if (strlen(line) == 0) continue;

        char linecpy[MAX_LINE_LEN];
        strncpy(linecpy, line, sizeof(linecpy) - 1);
        linecpy[sizeof(linecpy) - 1] = '\0';

        char company[MAX_FIELD_LEN] = "";
        char person[MAX_FIELD_LEN] = "";
        char phone[MAX_FIELD_LEN] = "";
        char email[MAX_FIELD_LEN] = "";

        char *fields[4] = {company, person, phone, email};
        int field_idx = 0;
        int in_quotes = 0;
        char *src = linecpy;
        char *dst = fields[0];

        while (*src && field_idx < 4) {
            if (*src == '"') { in_quotes = !in_quotes; src++; }
            else if (*src == ',' && !in_quotes) { *dst = '\0'; field_idx++; if (field_idx < 4) dst = fields[field_idx]; src++; }
            else { *dst++ = *src++; }
        }
        *dst = '\0';

        unescapeCSV(company);
        unescapeCSV(person);
        unescapeCSV(phone);
        unescapeCSV(email);

        if (!updated && strlen(company) > 0 && strcmp(company, key) == 0) {
            int choice;
            char buf[MAX_FIELD_LEN];
            printf("\nFound contact:\n");
            printf("Company : %s\n", company);
            printf("Contact : %s\n", person);
            printf("Phone   : %s\n", phone);
            printf("Email   : %s\n", email);
            printf("\nUpdate which field?\n");
            printf("1. Company\n");
            printf("2. Contact Person\n");
            printf("3. Phone\n");
            printf("4. Email\n");
            printf("Choice: ");
            if (scanf("%d", &choice) != 1) choice = 0;
            clearInputBuffer();

            int valid_update = 1;
            switch (choice) {
                case 1:
                    printf("New Company: ");
                    if (fgets(buf, sizeof(buf), stdin)) {
                        buf[strcspn(buf, "\n")] = '\0';
                        sanitizeInput(buf);
                        if (strlen(buf) > 0 && strlen(buf) < MAX_FIELD_LEN) {
                            strncpy(company, buf, MAX_FIELD_LEN - 1);
                            company[MAX_FIELD_LEN - 1] = '\0';
                        } else valid_update = 0;
                    }
                    break;
                case 2:
                    printf("New Contact: ");
                    if (fgets(buf, sizeof(buf), stdin)) {
                        buf[strcspn(buf, "\n")] = '\0';
                        sanitizeInput(buf);
                        if (strlen(buf) > 0 && strlen(buf) < MAX_FIELD_LEN) {
                            strncpy(person, buf, MAX_FIELD_LEN - 1);
                            person[MAX_FIELD_LEN - 1] = '\0';
                        } else valid_update = 0;
                    }
                    break;
                case 3:
                    printf("New Phone: ");
                    if (fgets(buf, sizeof(buf), stdin)) {
                        buf[strcspn(buf, "\n")] = '\0';
                        sanitizeInput(buf);
                        if (validatePhone(buf) && strlen(buf) > 0 && strlen(buf) < MAX_FIELD_LEN) {
                            strncpy(phone, buf, MAX_FIELD_LEN - 1);
                            phone[MAX_FIELD_LEN - 1] = '\0';
                        } else { printf("[ERROR] Invalid phone format!\n"); valid_update = 0; }
                    }
                    break;
                case 4:
                    printf("New Email: ");
                    if (fgets(buf, sizeof(buf), stdin)) {
                        buf[strcspn(buf, "\n")] = '\0';
                        sanitizeInput(buf);
                        if (validateEmail(buf) && strlen(buf) > 0 && strlen(buf) < MAX_FIELD_LEN) {
                            strncpy(email, buf, MAX_FIELD_LEN - 1);
                            email[MAX_FIELD_LEN - 1] = '\0';
                        } else { printf("[ERROR] Invalid email format!\n"); valid_update = 0; }
                    }
                    break;
                default:
                    printf("[INFO] Update cancelled.\n");
                    valid_update = 0;
                    break;
            }
            if (valid_update) updated = 1;
        }

        char esc_company[MAX_FIELD_LEN * 2];
        char esc_person[MAX_FIELD_LEN * 2];
        char esc_phone[MAX_FIELD_LEN * 2];
        char esc_email[MAX_FIELD_LEN * 2];

        escapeCSV(company, esc_company, sizeof(esc_company));
        escapeCSV(person, esc_person, sizeof(esc_person));
        escapeCSV(phone, esc_phone, sizeof(esc_phone));
        escapeCSV(email, esc_email, sizeof(esc_email));

        fprintf(wf, "%s,%s,%s,%s\n", esc_company, esc_person, esc_phone, esc_email);
    }

    fclose(rf);
    fclose(wf);

    if (remove("contacts.csv") != 0) { printf("[ERROR] Failed to remove old file!\n"); remove("contacts.tmp"); return; }
    if (rename("contacts.tmp", "contacts.csv") != 0) { printf("[ERROR] Failed to rename temporary file!\n"); return; }

    if (updated) printf("\n[SUCCESS] Contact updated successfully!\n");
    else printf("\n[INFO] No matching record to update.\n");
}
