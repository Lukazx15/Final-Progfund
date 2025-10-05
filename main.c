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
int confirmAction(const char *message);

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

int confirmAction(const char *message) {
    char response[10];
    printf("%s (y/n): ", message);
    if (fgets(response, sizeof(response), stdin)) {
        response[strcspn(response, "\n")] = '\0';
        trimWhitespace(response);
        if (strlen(response) > 0 && (response[0] == 'y' || response[0] == 'Y')) {
            return 1;
        }
    }
    return 0;
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
    
    printf("\n=== Add New Contact ===\n");
    printf("(Enter 0 at any field to cancel)\n\n");
    
    // Company name - loop until valid
    while (1) {
        printf("Enter company name: ");
        if (!fgets(temp, sizeof(temp), stdin)) {
            printf("[ERROR] Failed to read input!\n");
            continue;
        }
        temp[strcspn(temp, "\n")] = '\0';
        
        // Check for exit
        if (strcmp(temp, "0") == 0) {
            printf("[INFO] Add contact cancelled.\n");
            return;
        }
        
        sanitizeInput(temp);
        
        if (strlen(temp) == 0) {
            printf("[ERROR] Company name cannot be empty! Please try again.\n");
            continue;
        }
        if (strlen(temp) >= MAX_FIELD_LEN) {
            printf("[ERROR] Company name is too long (max %d chars)! Please try again.\n", MAX_FIELD_LEN - 1);
            continue;
        }
        
        strncpy(c.company, temp, MAX_FIELD_LEN - 1);
        c.company[MAX_FIELD_LEN - 1] = '\0';
        break;
    }

    // Contact person - loop until valid
    while (1) {
        printf("Enter contact person name: ");
        if (!fgets(temp, sizeof(temp), stdin)) {
            printf("[ERROR] Failed to read input!\n");
            continue;
        }
        temp[strcspn(temp, "\n")] = '\0';
        
        // Check for exit
        if (strcmp(temp, "0") == 0) {
            printf("[INFO] Add contact cancelled.\n");
            return;
        }
        
        sanitizeInput(temp);
        
        if (strlen(temp) == 0) {
            printf("[ERROR] Person name cannot be empty! Please try again.\n");
            continue;
        }
        if (strlen(temp) >= MAX_FIELD_LEN) {
            printf("[ERROR] Person name is too long (max %d chars)! Please try again.\n", MAX_FIELD_LEN - 1);
            continue;
        }
        
        strncpy(c.person, temp, MAX_FIELD_LEN - 1);
        c.person[MAX_FIELD_LEN - 1] = '\0';
        break;
    }

    // Phone - loop until valid
    while (1) {
        printf("Enter phone number: ");
        if (!fgets(temp, sizeof(temp), stdin)) {
            printf("[ERROR] Failed to read input!\n");
            continue;
        }
        temp[strcspn(temp, "\n")] = '\0';
        
        // Check for exit
        if (strcmp(temp, "0") == 0) {
            printf("[INFO] Add contact cancelled.\n");
            return;
        }
        
        sanitizeInput(temp);
        
        if (strlen(temp) == 0) {
            printf("[ERROR] Phone number cannot be empty! Please try again.\n");
            continue;
        }
        if (!validatePhone(temp)) {
            printf("[ERROR] Invalid phone number format! Please try again.\n");
            continue;
        }
        if (strlen(temp) >= MAX_FIELD_LEN) {
            printf("[ERROR] Phone number is too long (max %d chars)! Please try again.\n", MAX_FIELD_LEN - 1);
            continue;
        }
        
        strncpy(c.phone, temp, MAX_FIELD_LEN - 1);
        c.phone[MAX_FIELD_LEN - 1] = '\0';
        break;
    }

    // Email - loop until valid
    while (1) {
        printf("Enter email: ");
        if (!fgets(temp, sizeof(temp), stdin)) {
            printf("[ERROR] Failed to read input!\n");
            continue;
        }
        temp[strcspn(temp, "\n")] = '\0';
        
        // Check for exit
        if (strcmp(temp, "0") == 0) {
            printf("[INFO] Add contact cancelled.\n");
            return;
        }
        
        sanitizeInput(temp);
        
        if (strlen(temp) == 0) {
            printf("[ERROR] Email cannot be empty! Please try again.\n");
            continue;
        }
        if (!validateEmail(temp)) {
            printf("[ERROR] Invalid email format! Please try again.\n");
            continue;
        }
        if (strlen(temp) >= MAX_FIELD_LEN) {
            printf("[ERROR] Email is too long (max %d chars)! Please try again.\n", MAX_FIELD_LEN - 1);
            continue;
        }
        
        strncpy(c.email, temp, MAX_FIELD_LEN - 1);
        c.email[MAX_FIELD_LEN - 1] = '\0';
        break;
    }

    // Show summary and confirm
    printf("\n--- Contact Summary ---\n");
    printf("Company : %s\n", c.company);
    printf("Contact : %s\n", c.person);
    printf("Phone   : %s\n", c.phone);
    printf("Email   : %s\n", c.email);
    printf("----------------------\n");
    
    if (!confirmAction("\nDo you want to save this contact?")) {
        printf("[INFO] Contact not saved.\n");
        return;
    }

    // Save to file
    FILE *fp = fopen("contacts.csv", "a");
    if (!fp) {
        printf("[ERROR] Cannot open file for writing!\n");
        return;
    }

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
    printf("\n=== Contact List ===\n");
    printf("Enter keyword to search company (or press Enter to show all, 0 to cancel): ");
    if (!fgets(filter, sizeof(filter), stdin)) {
        printf("[ERROR] Failed to read input!\n");
        return;
    }
    filter[strcspn(filter, "\n")] = '\0';
    trimWhitespace(filter);
    
    // Check for exit
    if (strcmp(filter, "0") == 0) {
        printf("[INFO] List contacts cancelled.\n");
        return;
    }

    FILE *fp = fopen("contacts.csv", "r");
    if (!fp) {
        printf("[INFO] No contacts file found or cannot open.\n");
        return;
    }

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
    printf("\n=== Delete Contact ===\n");
    printf("Enter company name to delete (or 0 to cancel): ");
    if (!fgets(key, sizeof(key), stdin)) {
        printf("[ERROR] Failed to read input!\n");
        return;
    }
    key[strcspn(key, "\n")] = '\0';
    trimWhitespace(key);
    
    // Check for exit
    if (strcmp(key, "0") == 0) {
        printf("[INFO] Delete cancelled.\n");
        return;
    }
    
    if (strlen(key) == 0) {
        printf("[ERROR] Company name cannot be empty!\n");
        return;
    }

    FILE *rf = fopen("contacts.csv", "r");
    if (!rf) {
        printf("[ERROR] No contacts file found!\n");
        return;
    }

    FILE *wf = fopen("contacts.tmp", "w");
    if (!wf) {
        fclose(rf);
        printf("[ERROR] Cannot create temporary file!\n");
        return;
    }

    char line[MAX_LINE_LEN];
    int deleted = 0;
    int found = 0;

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
            found = 1;
            printf("\n[FOUND] Company: %s\n", company);
            
            fclose(rf);
            fclose(wf);
            remove("contacts.tmp");
            
            if (!confirmAction("Are you sure you want to delete this contact?")) {
                printf("[INFO] Delete cancelled.\n");
                return;
            }
            
            // Reopen and redo
            rf = fopen("contacts.csv", "r");
            wf = fopen("contacts.tmp", "w");
            deleted = 1;
            continue;
        }
        fprintf(wf, "%s\n", line);
    }

    fclose(rf);
    fclose(wf);

    if (deleted) {
        if (remove("contacts.csv") != 0) {
            printf("[ERROR] Failed to remove old file!\n");
            remove("contacts.tmp");
            return;
        }
        if (rename("contacts.tmp", "contacts.csv") != 0) {
            printf("[ERROR] Failed to rename temporary file!\n");
            return;
        }
        printf("\n[SUCCESS] Contact deleted successfully!\n");
    } else {
        remove("contacts.tmp");
        printf("\n[INFO] Company '%s' not found.\n", key);
    }
}

void searchContact() {
    char key[MAX_FIELD_LEN];
    printf("\n=== Search Contact ===\n");
    printf("Enter keyword (company or person name, or 0 to cancel): ");
    if (!fgets(key, sizeof(key), stdin)) {
        printf("[ERROR] Failed to read input!\n");
        return;
    }
    key[strcspn(key, "\n")] = '\0';
    trimWhitespace(key);
    
    // Check for exit
    if (strcmp(key, "0") == 0) {
        printf("[INFO] Search cancelled.\n");
        return;
    }
    
    if (strlen(key) == 0) {
        printf("[ERROR] Search keyword cannot be empty!\n");
        return;
    }

    FILE *fp = fopen("contacts.csv", "r");
    if (!fp) {
        printf("[ERROR] No contacts file found!\n");
        return;
    }

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
    printf("\n=== Update Contact ===\n");
    printf("Enter company name to update (or 0 to cancel): ");
    if (!fgets(key, sizeof(key), stdin)) {
        printf("[ERROR] Failed to read input!\n");
        return;
    }
    key[strcspn(key, "\n")] = '\0';
    trimWhitespace(key);
    
    // Check for exit
    if (strcmp(key, "0") == 0) {
        printf("[INFO] Update cancelled.\n");
        return;
    }
    
    if (strlen(key) == 0) {
        printf("[ERROR] Company name cannot be empty!\n");
        return;
    }

    FILE *rf = fopen("contacts.csv", "r");
    if (!rf) {
        printf("[ERROR] No contacts file found!\n");
        return;
    }

    FILE *wf = fopen("contacts.tmp", "w");
    if (!wf) {
        fclose(rf);
        printf("[ERROR] Cannot create temporary file!\n");
        return;
    }

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
            
            printf("\n--- Current Contact ---\n");
            printf("Company : %s\n", company);
            printf("Contact : %s\n", person);
            printf("Phone   : %s\n", phone);
            printf("Email   : %s\n", email);
            printf("----------------------\n");
            
            printf("\nUpdate which field?\n");
            printf("1. Company\n");
            printf("2. Contact Person\n");
            printf("3. Phone\n");
            printf("4. Email\n");
            printf("0. Cancel\n");
            printf("Choice: ");
            
            if (scanf("%d", &choice) != 1) choice = 0;
            clearInputBuffer();
            
            if (choice == 0) {
                printf("[INFO] Update cancelled.\n");
                fprintf(wf, "%s\n", line);
                continue;
            }

            int valid_update = 0;
            char new_company[MAX_FIELD_LEN], new_person[MAX_FIELD_LEN];
            char new_phone[MAX_FIELD_LEN], new_email[MAX_FIELD_LEN];
            
            strcpy(new_company, company);
            strcpy(new_person, person);
            strcpy(new_phone, phone);
            strcpy(new_email, email);
            
            switch (choice) {
                case 1:
                    while (1) {
                        printf("New Company (0 to cancel): ");
                        if (fgets(buf, sizeof(buf), stdin)) {
                            buf[strcspn(buf, "\n")] = '\0';
                            if (strcmp(buf, "0") == 0) break;
                            sanitizeInput(buf);
                            if (strlen(buf) > 0 && strlen(buf) < MAX_FIELD_LEN) {
                                strcpy(new_company, buf);
                                valid_update = 1;
                                break;
                            }
                            printf("[ERROR] Invalid input! Try again.\n");
                        }
                    }
                    break;
                case 2:
                    while (1) {
                        printf("New Contact (0 to cancel): ");
                        if (fgets(buf, sizeof(buf), stdin)) {
                            buf[strcspn(buf, "\n")] = '\0';
                            if (strcmp(buf, "0") == 0) break;
                            sanitizeInput(buf);
                            if (strlen(buf) > 0 && strlen(buf) < MAX_FIELD_LEN) {
                                strcpy(new_person, buf);
                                valid_update = 1;
                                break;
                            }
                            printf("[ERROR] Invalid input! Try again.\n");
                        }
                    }
                    break;
                case 3:
                    while (1) {
                        printf("New Phone (0 to cancel): ");
                        if (fgets(buf, sizeof(buf), stdin)) {
                            buf[strcspn(buf, "\n")] = '\0';
                            if (strcmp(buf, "0") == 0) break;
                            sanitizeInput(buf);
                            if (validatePhone(buf) && strlen(buf) > 0 && strlen(buf) < MAX_FIELD_LEN) {
                                strcpy(new_phone, buf);
                                valid_update = 1;
                                break;
                            }
                            printf("[ERROR] Invalid phone format! Try again.\n");
                        }
                    }
                    break;
                case 4:
                    while (1) {
                        printf("New Email (0 to cancel): ");
                        if (fgets(buf, sizeof(buf), stdin)) {
                            buf[strcspn(buf, "\n")] = '\0';
                            if (strcmp(buf, "0") == 0) break;
                            sanitizeInput(buf);
                            if (validateEmail(buf) && strlen(buf) > 0 && strlen(buf) < MAX_FIELD_LEN) {
                                strcpy(new_email, buf);
                                valid_update = 1;
                                break;
                            }
                            printf("[ERROR] Invalid email format! Try again.\n");
                        }
                    }
                    break;
                default:
                    printf("[INFO] Update cancelled.\n");
                    break;
            }
            
            if (valid_update) {
                // Show new data preview
                printf("\n--- Updated Contact Preview ---\n");
                printf("Company : %s\n", new_company);
                printf("Contact : %s\n", new_person);
                printf("Phone   : %s\n", new_phone);
                printf("Email   : %s\n", new_email);
                printf("-------------------------------\n");
                
                if (confirmAction("\nDo you want to save these changes?")) {
                    strcpy(company, new_company);
                    strcpy(person, new_person);
                    strcpy(phone, new_phone);
                    strcpy(email, new_email);
                    updated = 1;
                    printf("[SUCCESS] Changes will be saved.\n");
                } else {
                    printf("[INFO] Changes discarded.\n");
                }
            }
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

    if (remove("contacts.csv") != 0) {
        printf("[ERROR] Failed to remove old file!\n");
        remove("contacts.tmp");
        return;
    }
    if (rename("contacts.tmp", "contacts.csv") != 0) {
        printf("[ERROR] Failed to rename temporary file!\n");
        return;
    }

    if (updated) {
        printf("\n[SUCCESS] Contact updated successfully!\n");
    } else {
        printf("\n[INFO] No changes made.\n");
    }
}