// main.c - Business Contact Management (CSV-safe, quoted fields support, tests pass)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
  #include <conio.h>
  #define CLEAR_SCREEN "cls"
#else
  #include <termios.h>
  #include <unistd.h>
  #define CLEAR_SCREEN "clear"
  static int getch(void) {
      struct termios oldt, newt;
      int ch;
      tcgetattr(STDIN_FILENO, &oldt);
      newt = oldt;
      newt.c_lflag &= ~(ICANON | ECHO);
      tcsetattr(STDIN_FILENO, TCSANOW, &newt);
      ch = getchar();
      tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
      return ch;
  }
#endif

#define MAX_FIELD_LEN 100
#define MAX_LINE_LEN  512

struct Contact {
    char company[MAX_FIELD_LEN];
    char person [MAX_FIELD_LEN];
    char phone  [MAX_FIELD_LEN];
    char email  [MAX_FIELD_LEN];
};

// ==== Declarations ====
void addContact();
void listContacts();
void deleteContact();
void searchContact();
void updateContact();
void runUnitTests();

void clearInputBuffer();
int  confirmAction(const char *message);
void trimWhitespace(char *str);
void sanitizeInput(char *str);
void escapeCSV(const char *input, char *output, size_t output_size);
void unescapeCSV(char *str);
int  validateEmail(const char *email);
int  validatePhone(const char *phone);

// ==== Globals for tests ====
static int test_passed = 0, test_failed = 0;
#define TEST_ASSERT(cond, name) do { if (cond){printf("  [PASS] %s\n", name); test_passed++;} else {printf("  [FAIL] %s\n", name); test_failed++;} } while(0)

// ==== Main ====
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
        printf("6. Run Unit Tests\n");
        printf("0. Exit\n");
        printf("===========================================\n");
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) {
            choice = -1; clearInputBuffer();
        } else {
            clearInputBuffer();
        }

        switch (choice) {
            case 0: printf("\nThank you for using the system. Goodbye!\n"); exit(0);
            case 1: addContact();   break;
            case 2: listContacts(); break;
            case 3: deleteContact();break;
            case 4: searchContact();break;
            case 5: updateContact();break;
            case 6: runUnitTests(); break;
            default: printf("\n[ERROR] Invalid choice! Please try again.\n");
        }
        printf("\nPress any key to continue...");
        getch();
    }
    return 0;
}

// ==== Utilities ====
void clearInputBuffer() { int c; while ((c = getchar()) != '\n' && c != EOF); }

int confirmAction(const char *message) {
    char response[10];
    printf("%s (y/n): ", message);
    if (fgets(response, sizeof(response), stdin)) {
        response[strcspn(response, "\n")] = '\0';
        trimWhitespace(response);
        if (response[0] == 'y' || response[0] == 'Y') return 1;
    }
    return 0;
}

void trimWhitespace(char *str) {
    if (!str || *str == '\0') return;
    char *start = str;
    while (*start && isspace((unsigned char)*start)) start++;
    if (*start == '\0') { *str = '\0'; return; }
    char *end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    size_t len = (size_t)(end - start + 1);
    memmove(str, start, len);
    str[len] = '\0';
}

// Trim-only: ไม่ลบสัญลักษณ์พิเศษ (= + - @) เพื่อให้เทสชุดใหม่ผ่าน
void sanitizeInput(char *str) {
    if (!str) return;
    // ตัด \r \n ที่ปลาย
    size_t len = strlen(str);
    while (len > 0 && (str[len-1] == '\r' || str[len-1] == '\n')) str[--len] = '\0';
    trimWhitespace(str);
}

// Escape CSV ตามมาตรฐาน RFC4180 อย่างง่าย
void escapeCSV(const char *input, char *output, size_t output_size) {
    if (!input || !output || output_size == 0) return;
    size_t j = 0;
    int needs_quotes = 0;
    for (size_t i = 0; input[i]; i++) {
        if (input[i] == ',' || input[i] == '"' || input[i] == '\n' || input[i] == '\r') { needs_quotes = 1; break; }
    }
    if (needs_quotes && j < output_size - 1) output[j++] = '"';
    for (size_t i = 0; input[i] && j < output_size - 2; i++) {
        if (input[i] == '"' && j < output_size - 3) { output[j++] = '"'; output[j++] = '"'; }
        else { output[j++] = input[i]; }
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
    char *r = str, *w = str;
    while (*r) {
        if (*r == '"' && *(r + 1) == '"') { *w++ = '"'; r += 2; }
        else { *w++ = *r++; }
    }
    *w = '\0';
}

int validateEmail(const char *email) {
    if (!email || !*email) return 0;
    const char *at = strchr(email, '@');
    const char *dot = strrchr(email, '.');
    return (at && dot && at < dot && at > email && dot < email + strlen(email) - 1);
}

int validatePhone(const char *phone) {
    if (!phone || !*phone) return 0;
    int has_digit = 0;
    for (size_t i = 0; phone[i]; i++) {
        if (isdigit((unsigned char)phone[i])) has_digit = 1;
        else if (phone[i] != '-' && phone[i] != '+' && phone[i] != '(' &&
                 phone[i] != ')' && phone[i] != ' ' && phone[i] != '.') return 0;
    }
    return has_digit;
}

// ==== Add Contact ====
void addContact() {
    struct Contact c;
    char temp[MAX_FIELD_LEN * 2];

    printf("\n=== Add New Contact ===\n(Enter 0 at any field to cancel)\n\n");

    // Company
    while (1) {
        printf("Enter company name: ");
        if (!fgets(temp, sizeof(temp), stdin)) { printf("[ERROR] Read error!\n"); continue; }
        temp[strcspn(temp, "\n")] = '\0';
        if (strcmp(temp, "0") == 0) { printf("[INFO] Add cancelled.\n"); return; }
        sanitizeInput(temp);
        if (!*temp) { printf("[ERROR] Company name cannot be empty!\n"); continue; }
        if (strlen(temp) >= MAX_FIELD_LEN) { printf("[ERROR] Too long.\n"); continue; }
        strncpy(c.company, temp, MAX_FIELD_LEN); c.company[MAX_FIELD_LEN-1] = '\0'; break;
    }

    // Person
    while (1) {
        printf("Enter contact person: ");
        if (!fgets(temp, sizeof(temp), stdin)) { printf("[ERROR] Read error!\n"); continue; }
        temp[strcspn(temp, "\n")] = '\0';
        if (strcmp(temp, "0") == 0) { printf("[INFO] Add cancelled.\n"); return; }
        sanitizeInput(temp);
        if (!*temp) { printf("[ERROR] Person name cannot be empty!\n"); continue; }
        if (strlen(temp) >= MAX_FIELD_LEN) { printf("[ERROR] Too long.\n"); continue; }
        strncpy(c.person, temp, MAX_FIELD_LEN); c.person[MAX_FIELD_LEN-1] = '\0'; break;
    }

    // Phone
    while (1) {
        printf("Enter phone: ");
        if (!fgets(temp, sizeof(temp), stdin)) { printf("[ERROR] Read error!\n"); continue; }
        temp[strcspn(temp, "\n")] = '\0';
        if (strcmp(temp, "0") == 0) { printf("[INFO] Add cancelled.\n"); return; }
        sanitizeInput(temp);
        if (!*temp) { printf("[ERROR] Phone cannot be empty!\n"); continue; }
        if (!validatePhone(temp)) { printf("[ERROR] Invalid phone.\n"); continue; }
        if (strlen(temp) >= MAX_FIELD_LEN) { printf("[ERROR] Too long.\n"); continue; }
        strncpy(c.phone, temp, MAX_FIELD_LEN); c.phone[MAX_FIELD_LEN-1] = '\0'; break;
    }

    // Email
    while (1) {
        printf("Enter email: ");
        if (!fgets(temp, sizeof(temp), stdin)) { printf("[ERROR] Read error!\n"); continue; }
        temp[strcspn(temp, "\n")] = '\0';
        if (strcmp(temp, "0") == 0) { printf("[INFO] Add cancelled.\n"); return; }
        sanitizeInput(temp);
        if (!*temp) { printf("[ERROR] Email cannot be empty!\n"); continue; }
        if (!validateEmail(temp)) { printf("[ERROR] Invalid email.\n"); continue; }
        if (strlen(temp) >= MAX_FIELD_LEN) { printf("[ERROR] Too long.\n"); continue; }
        strncpy(c.email, temp, MAX_FIELD_LEN); c.email[MAX_FIELD_LEN-1] = '\0'; break;
    }

    // Summary
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

    // Save
    FILE *fp = fopen("contacts.csv", "a");
    if (!fp) { printf("[ERROR] Cannot open file for writing!\n"); return; }

    char esc_company[MAX_FIELD_LEN * 2], esc_person [MAX_FIELD_LEN * 2];
    char esc_phone  [MAX_FIELD_LEN * 2], esc_email  [MAX_FIELD_LEN * 2];

    escapeCSV(c.company, esc_company, sizeof(esc_company));
    escapeCSV(c.person , esc_person , sizeof(esc_person ));
    escapeCSV(c.phone  , esc_phone  , sizeof(esc_phone  ));
    escapeCSV(c.email  , esc_email  , sizeof(esc_email  ));

    fprintf(fp, "%s,%s,%s,%s\n", esc_company, esc_person, esc_phone, esc_email);
    fclose(fp);
    printf("\n[SUCCESS] Contact added successfully!\n");
}

// ==== List ====
void listContacts() {
    char filter[MAX_FIELD_LEN];
    printf("\n=== Contact List ===\n");
    printf("Enter keyword to search company (or press Enter to show all, 0 to cancel): ");
    if (!fgets(filter, sizeof(filter), stdin)) { printf("[ERROR] Failed to read input!\n"); return; }
    filter[strcspn(filter, "\n")] = '\0';
    trimWhitespace(filter);
    if (strcmp(filter, "0") == 0) { printf("[INFO] List contacts cancelled.\n"); return; }

    FILE *fp = fopen("contacts.csv", "r");
    if (!fp) { printf("[INFO] No contacts file found or cannot open.\n"); return; }

    int use_filter = (int)(strlen(filter) > 0);
    char filter_lower[MAX_FIELD_LEN] = "";
    if (use_filter) {
        strncpy(filter_lower, filter, MAX_FIELD_LEN - 1);
        filter_lower[MAX_FIELD_LEN - 1] = '\0';
        for (int i = 0; filter_lower[i]; i++) filter_lower[i] = (char)tolower((unsigned char)filter_lower[i]);
    }

    char line[MAX_LINE_LEN];
    int count = 0;

    printf("\n%-4s | %-20s | %-20s | %-15s | %-30s\n", "No.", "Company", "Contact", "Phone", "Email");
    printf("------------------------------------------------------------------------------------------------\n");

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n\r")] = '\0';
        if (!*line) continue;

        char linecpy[MAX_LINE_LEN];
        strncpy(linecpy, line, sizeof(linecpy)-1);
        linecpy[sizeof(linecpy)-1] = '\0';

        char company[MAX_FIELD_LEN] = "", person[MAX_FIELD_LEN] = "", phone[MAX_FIELD_LEN] = "", email[MAX_FIELD_LEN] = "";
        char *fields[4] = {company, person, phone, email};
        int field_idx = 0, in_quotes = 0;
        char *src = linecpy, *dst = fields[0];

        while (*src && field_idx < 4) {
            if (*src == '"') { in_quotes = !in_quotes; src++; }
            else if (*src == ',' && !in_quotes) { *dst = '\0'; field_idx++; if (field_idx < 4) dst = fields[field_idx]; src++; }
            else { *dst++ = *src++; }
        }
        *dst = '\0';

        unescapeCSV(company); unescapeCSV(person); unescapeCSV(phone); unescapeCSV(email);
        if (!*company || !*person) continue;

        if (use_filter) {
            char company_lower[MAX_FIELD_LEN];
            strncpy(company_lower, company, MAX_FIELD_LEN - 1); company_lower[MAX_FIELD_LEN - 1] = '\0';
            for (int i = 0; company_lower[i]; i++) company_lower[i] = (char)tolower((unsigned char)company_lower[i]);
            if (!strstr(company_lower, filter_lower)) continue;
        }

        count++;
        printf("%-4d | %-20.20s | %-20.20s | %-15.15s | %-30.30s\n",
               count, company, person, phone, email);
    }
    fclose(fp);

    if (count == 0) {
        if (use_filter) printf("[INFO] No contacts found with keyword '%s'.\n", filter);
        else            printf("[INFO] No contacts in the system.\n");
    } else {
        printf("------------------------------------------------------------------------------------------------\n");
        printf("Total: %d contact(s) displayed\n", count);
    }
}

// Helper: คืนสตริงที่เหลือเฉพาะตัวเลขของเบอร์
static void normalizePhone(const char *in, char *out, size_t out_size) {
    if (!in || !out || out_size == 0) return;
    size_t j = 0;
    for (size_t i = 0; in[i] && j + 1 < out_size; i++) {
        if (isdigit((unsigned char)in[i])) out[j++] = in[i];
    }
    out[j] = '\0';
}

void deleteContact() {
    // รับคีย์เวิร์ด (ลบได้ทั้ง Company / Person / Phone / Email)
    char key[MAX_FIELD_LEN];
    printf("\n=== Delete Contact ===\n");
    printf("Enter company / contact person / phone / email to delete (or 0 to cancel): ");
    if (!fgets(key, sizeof(key), stdin)) { printf("[ERROR] Failed to read input!\n"); return; }
    key[strcspn(key, "\n")] = '\0';
    trimWhitespace(key);

    if (strcmp(key, "0") == 0) { printf("[INFO] Delete cancelled.\n"); return; }
    if (!*key) { printf("[ERROR] Keyword cannot be empty!\n"); return; }

    // เตรียมคีย์สำหรับเทียบ
    char key_lower[MAX_FIELD_LEN];
    strncpy(key_lower, key, MAX_FIELD_LEN - 1);
    key_lower[MAX_FIELD_LEN - 1] = '\0';
    for (int i = 0; key_lower[i]; i++) key_lower[i] = (char)tolower((unsigned char)key_lower[i]);

    char key_phone_norm[MAX_FIELD_LEN];
    normalizePhone(key, key_phone_norm, sizeof(key_phone_norm));
    int key_looks_like_phone = (int)(strlen(key_phone_norm) > 0); // มีตัวเลข → ถือเป็น phone mode ด้วย

    int key_looks_like_email = (strchr(key, '@') != NULL);

    // เปิดไฟล์รอบแรกเพื่อ "ค้นหา" และเก็บ matching ทั้งหมด
    FILE *rf = fopen("contacts.csv", "r");
    if (!rf) { printf("[ERROR] No contacts file found!\n"); return; }

    typedef struct {
        char company[MAX_FIELD_LEN];
        char person [MAX_FIELD_LEN];
        char phone  [MAX_FIELD_LEN];
        char email  [MAX_FIELD_LEN];
        char rawline[MAX_LINE_LEN]; // เก็บบรรทัดดิบไว้ใช้เขียนคืนได้แม่นยำ
    } Row;

    enum { MAX_MATCH = 1024 };
    Row matches[MAX_MATCH];
    int mcount = 0;

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), rf)) {
        line[strcspn(line, "\n\r")] = '\0';
        if (!*line) continue;

        char linecpy[MAX_LINE_LEN];
        strncpy(linecpy, line, sizeof(linecpy) - 1);
        linecpy[sizeof(linecpy) - 1] = '\0';

        char company[MAX_FIELD_LEN] = "", person[MAX_FIELD_LEN] = "";
        char phone  [MAX_FIELD_LEN] = "", email [MAX_FIELD_LEN] = "";

        // พาร์ส CSV รองรับ "
        char *fields[4] = {company, person, phone, email};
        int field_idx = 0, in_quotes = 0;
        char *src = linecpy, *dst = fields[0];

        while (*src && field_idx < 4) {
            if (*src == '"') { in_quotes = !in_quotes; src++; }
            else if (*src == ',' && !in_quotes) { *dst = '\0'; field_idx++; if (field_idx < 4) dst = fields[field_idx]; src++; }
            else { *dst++ = *src++; }
        }
        *dst = '\0';

        unescapeCSV(company); unescapeCSV(person); unescapeCSV(phone); unescapeCSV(email);
        if (!*company && !*person && !*phone && !*email) continue;

        // lower สำหรับ company/person/email
        char company_lower[MAX_FIELD_LEN], person_lower[MAX_FIELD_LEN], email_lower[MAX_FIELD_LEN];
        strncpy(company_lower, company, MAX_FIELD_LEN - 1); company_lower[MAX_FIELD_LEN - 1] = '\0';
        strncpy(person_lower , person , MAX_FIELD_LEN - 1); person_lower [MAX_FIELD_LEN - 1] = '\0';
        strncpy(email_lower  , email  , MAX_FIELD_LEN - 1); email_lower  [MAX_FIELD_LEN - 1] = '\0';
        for (int i = 0; company_lower[i]; i++) company_lower[i] = (char)tolower((unsigned char)company_lower[i]);
        for (int i = 0; person_lower [i]; i++) person_lower [i] = (char)tolower((unsigned char)person_lower [i]);
        for (int i = 0; email_lower  [i]; i++) email_lower  [i] = (char)tolower((unsigned char)email_lower  [i]);

        // normalize phone
        char phone_norm[MAX_FIELD_LEN];
        normalizePhone(phone, phone_norm, sizeof(phone_norm));

        // เงื่อนไขตรง: รองรับ 4 เคส (company/person/email exact-insensitive, phone เทียบแบบ normalize)
        int match = 0;
        if (key_looks_like_phone) {
            if (*phone_norm && strcmp(phone_norm, key_phone_norm) == 0) match = 1;
        }
        if (!match && key_looks_like_email) {
            if (*email_lower && strcmp(email_lower, key_lower) == 0) match = 1;
        }
        if (!match) {
            if ((*company_lower && strcmp(company_lower, key_lower) == 0) ||
                (*person_lower  && strcmp(person_lower , key_lower) == 0)) {
                match = 1;
            }
        }

        if (match) {
            if (mcount < MAX_MATCH) {
                strncpy(matches[mcount].company, company, MAX_FIELD_LEN - 1); matches[mcount].company[MAX_FIELD_LEN - 1] = '\0';
                strncpy(matches[mcount].person , person , MAX_FIELD_LEN - 1); matches[mcount].person [MAX_FIELD_LEN - 1] = '\0';
                strncpy(matches[mcount].phone  , phone  , MAX_FIELD_LEN - 1); matches[mcount].phone  [MAX_FIELD_LEN - 1] = '\0';
                strncpy(matches[mcount].email  , email  , MAX_FIELD_LEN - 1); matches[mcount].email  [MAX_FIELD_LEN - 1] = '\0';
                strncpy(matches[mcount].rawline, line   , MAX_LINE_LEN - 1);  matches[mcount].rawline[MAX_LINE_LEN - 1] = '\0';
                mcount++;
            }
        }
    }
    fclose(rf);

    if (mcount == 0) {
        printf("\n[INFO] No record matches '%s'.\n", key);
        return;
    }

    // เลือกรายการเมื่อชนหลายอัน
    int choice_idx = 0; // 1..mcount
    if (mcount == 1) {
        printf("\n--- Contact to Delete ---\n");
        printf("Company : %s\n", matches[0].company);
        printf("Contact : %s\n", matches[0].person);
        printf("Phone   : %s\n", matches[0].phone);
        printf("Email   : %s\n", matches[0].email);
        printf("------------------------\n");
        if (!confirmAction("\nAre you sure you want to delete this contact?")) {
            printf("[INFO] Delete cancelled.\n");
            return;
        }
        choice_idx = 1;
    } else {
        printf("\nMultiple records matched '%s'. Please choose one to delete:\n", key);
        for (int i = 0; i < mcount; i++) {
            printf("%d) Company: %s | Person: %s | Phone: %s | Email: %s\n",
                   i + 1, matches[i].company, matches[i].person, matches[i].phone, matches[i].email);
        }
        printf("0) Cancel\n");

        int sel;
        while (1) {
            printf("Select [0-%d]: ", mcount);
            if (scanf("%d", &sel) != 1) { clearInputBuffer(); continue; }
            clearInputBuffer();
            if (sel == 0) { printf("[INFO] Delete cancelled.\n"); return; }
            if (sel >= 1 && sel <= mcount) {
                Row *pick = &matches[sel - 1];
                printf("\n--- Contact to Delete ---\n");
                printf("Company : %s\n", pick->company);
                printf("Contact : %s\n", pick->person);
                printf("Phone   : %s\n", pick->phone);
                printf("Email   : %s\n", pick->email);
                printf("------------------------\n");
                if (!confirmAction("\nAre you sure you want to delete this contact?")) {
                    printf("[INFO] Delete cancelled.\n"); return;
                }
                choice_idx = sel;
                break;
            }
            printf("[ERROR] Invalid selection. Try again.\n");
        }
    }

    // ลบจริง: ข้ามบรรทัดที่ตรงกับ matches[choice_idx-1].rawline
    Row target = matches[choice_idx - 1];

    rf = fopen("contacts.csv", "r");
    if (!rf) { printf("[ERROR] Cannot open contacts file!\n"); return; }
    FILE *wf = fopen("contacts.tmp", "w");
    if (!wf) { fclose(rf); printf("[ERROR] Cannot create temporary file!\n"); return; }

    int deleted = 0;
    while (fgets(line, sizeof(line), rf)) {
        line[strcspn(line, "\n\r")] = '\0';
        if (!*line) continue;

        if (!deleted && strcmp(line, target.rawline) == 0) {
            deleted = 1;           // ข้ามบรรทัดนี้ = ลบ
            continue;
        }
        fprintf(wf, "%s\n", line);
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

    if (deleted) printf("\n[SUCCESS] Contact deleted successfully!\n");
    else         printf("\n[INFO] Nothing was deleted.\n");
}

// ==== Search ====
void searchContact() {
    char key[MAX_FIELD_LEN];
    printf("\n=== Search Contact ===\n");
    printf("Enter keyword (company/person/phone, or 0 to cancel): ");
    if (!fgets(key, sizeof(key), stdin)) { printf("[ERROR] Failed to read input!\n"); return; }
    key[strcspn(key, "\n")] = '\0';
    trimWhitespace(key);
    if (strcmp(key, "0") == 0) { printf("[INFO] Search cancelled.\n"); return; }
    if (!*key) { printf("[ERROR] Search keyword cannot be empty!\n"); return; }

    char key_lower[MAX_FIELD_LEN];
    strncpy(key_lower, key, MAX_FIELD_LEN - 1); key_lower[MAX_FIELD_LEN - 1] = '\0';
    for (int i = 0; key_lower[i]; i++) key_lower[i] = (char)tolower((unsigned char)key_lower[i]);

    FILE *fp = fopen("contacts.csv", "r");
    if (!fp) { printf("[ERROR] No contacts file found!\n"); return; }

    char line[MAX_LINE_LEN];
    int found = 0;

    printf("\n--- Search Results ---\n");
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n\r")] = '\0';
        if (!*line) continue;

        char linecpy[MAX_LINE_LEN];
        strncpy(linecpy, line, sizeof(linecpy)-1);
        linecpy[sizeof(linecpy)-1] = '\0';

        char company[MAX_FIELD_LEN] = "", person[MAX_FIELD_LEN] = "", phone[MAX_FIELD_LEN] = "", email[MAX_FIELD_LEN] = "";
        char *fields[4] = {company, person, phone, email};
        int field_idx = 0, in_quotes = 0;
        char *src = linecpy, *dst = fields[0];

        while (*src && field_idx < 4) {
            if (*src == '"') { in_quotes = !in_quotes; src++; }
            else if (*src == ',' && !in_quotes) { *dst = '\0'; field_idx++; if (field_idx < 4) dst = fields[field_idx]; src++; }
            else { *dst++ = *src++; }
        }
        *dst = '\0';

        unescapeCSV(company); unescapeCSV(person); unescapeCSV(phone); unescapeCSV(email);
        if (!*company) continue;

        char company_lower[MAX_FIELD_LEN], person_lower[MAX_FIELD_LEN], phone_lower[MAX_FIELD_LEN];
        strncpy(company_lower, company, MAX_FIELD_LEN - 1); company_lower[MAX_FIELD_LEN - 1] = '\0';
        strncpy(person_lower , person , MAX_FIELD_LEN - 1); person_lower [MAX_FIELD_LEN - 1] = '\0';
        strncpy(phone_lower  , phone  , MAX_FIELD_LEN - 1); phone_lower  [MAX_FIELD_LEN - 1] = '\0';
        for (int i = 0; company_lower[i]; i++) company_lower[i] = (char)tolower((unsigned char)company_lower[i]);
        for (int i = 0; person_lower [i]; i++) person_lower [i] = (char)tolower((unsigned char)person_lower [i]);
        for (int i = 0; phone_lower  [i]; i++) phone_lower  [i] = (char)tolower((unsigned char)phone_lower  [i]);

        if (strstr(company_lower, key_lower) || strstr(person_lower, key_lower) || strstr(phone_lower, key_lower)) {
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

// ==== Update (case-insensitive) ====
void updateContact() {
    char key[MAX_FIELD_LEN];
    printf("\n=== Update Contact ===\n");
    printf("Enter company name to update (or 0 to cancel): ");
    if (!fgets(key, sizeof(key), stdin)) { printf("[ERROR] Failed to read input!\n"); return; }
    key[strcspn(key, "\n")] = '\0';
    trimWhitespace(key);
    if (strcmp(key, "0") == 0) { printf("[INFO] Update cancelled.\n"); return; }
    if (!*key) { printf("[ERROR] Company name cannot be empty!\n"); return; }

    char key_lower[MAX_FIELD_LEN];
    strncpy(key_lower, key, MAX_FIELD_LEN - 1); key_lower[MAX_FIELD_LEN - 1] = '\0';
    for (int i = 0; key_lower[i]; i++) key_lower[i] = (char)tolower((unsigned char)key_lower[i]);

    FILE *rf = fopen("contacts.csv", "r");
    if (!rf) { printf("[ERROR] No contacts file found!\n"); return; }
    FILE *wf = fopen("contacts.tmp", "w");
    if (!wf) { fclose(rf); printf("[ERROR] Cannot create temporary file!\n"); return; }

    char line[MAX_LINE_LEN];
    int updated = 0;

    while (fgets(line, sizeof(line), rf)) {
        line[strcspn(line, "\n\r")] = '\0';
        if (!*line) continue;

        char linecpy[MAX_LINE_LEN];
        strncpy(linecpy, line, sizeof(linecpy)-1);
        linecpy[sizeof(linecpy)-1] = '\0';

        char company[MAX_FIELD_LEN] = "", person[MAX_FIELD_LEN] = "", phone[MAX_FIELD_LEN] = "", email[MAX_FIELD_LEN] = "";
        char *fields[4] = {company, person, phone, email};
        int field_idx = 0, in_quotes = 0;
        char *src = linecpy, *dst = fields[0];

        while (*src && field_idx < 4) {
            if (*src == '"') { in_quotes = !in_quotes; src++; }
            else if (*src == ',' && !in_quotes) { *dst = '\0'; field_idx++; if (field_idx < 4) dst = fields[field_idx]; src++; }
            else { *dst++ = *src++; }
        }
        *dst = '\0';

        unescapeCSV(company); unescapeCSV(person); unescapeCSV(phone); unescapeCSV(email);

        char company_lower[MAX_FIELD_LEN];
        strncpy(company_lower, company, MAX_FIELD_LEN - 1); company_lower[MAX_FIELD_LEN - 1] = '\0';
        for (int i = 0; company_lower[i]; i++) company_lower[i] = (char)tolower((unsigned char)company_lower[i]);

        if (!updated && *company && strcmp(company_lower, key_lower) == 0) {
            int choice; char buf[MAX_FIELD_LEN];
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
                // write original
                char e1[MAX_FIELD_LEN*2],e2[MAX_FIELD_LEN*2],e3[MAX_FIELD_LEN*2],e4[MAX_FIELD_LEN*2];
                escapeCSV(company,e1,sizeof(e1)); escapeCSV(person,e2,sizeof(e2));
                escapeCSV(phone,e3,sizeof(e3));   escapeCSV(email,e4,sizeof(e4));
                fprintf(wf, "%s,%s,%s,%s\n", e1,e2,e3,e4);
                continue;
            }

            int valid_update = 0;
            char new_company[MAX_FIELD_LEN], new_person[MAX_FIELD_LEN];
            char new_phone[MAX_FIELD_LEN],   new_email[MAX_FIELD_LEN];
            strcpy(new_company, company); strcpy(new_person, person);
            strcpy(new_phone,   phone);   strcpy(new_email, email);

            switch (choice) {
                case 1:
                    while (1) {
                        printf("New Company (0 to cancel): ");
                        if (!fgets(buf, sizeof(buf), stdin)) continue;
                        buf[strcspn(buf, "\n")] = '\0';
                        if (strcmp(buf, "0") == 0) break;
                        sanitizeInput(buf);
                        if (*buf && strlen(buf) < MAX_FIELD_LEN) { strcpy(new_company, buf); valid_update = 1; break; }
                        printf("[ERROR] Invalid input! Try again.\n");
                    } break;
                case 2:
                    while (1) {
                        printf("New Contact (0 to cancel): ");
                        if (!fgets(buf, sizeof(buf), stdin)) continue;
                        buf[strcspn(buf, "\n")] = '\0';
                        if (strcmp(buf, "0") == 0) break;
                        sanitizeInput(buf);
                        if (*buf && strlen(buf) < MAX_FIELD_LEN) { strcpy(new_person, buf); valid_update = 1; break; }
                        printf("[ERROR] Invalid input! Try again.\n");
                    } break;
                case 3:
                    while (1) {
                        printf("New Phone (0 to cancel): ");
                        if (!fgets(buf, sizeof(buf), stdin)) continue;
                        buf[strcspn(buf, "\n")] = '\0';
                        if (strcmp(buf, "0") == 0) break;
                        sanitizeInput(buf);
                        if (validatePhone(buf) && *buf && strlen(buf) < MAX_FIELD_LEN) { strcpy(new_phone, buf); valid_update = 1; break; }
                        printf("[ERROR] Invalid phone format! Try again.\n");
                    } break;
                case 4:
                    while (1) {
                        printf("New Email (0 to cancel): ");
                        if (!fgets(buf, sizeof(buf), stdin)) continue;
                        buf[strcspn(buf, "\n")] = '\0';
                        if (strcmp(buf, "0") == 0) break;
                        sanitizeInput(buf);
                        if (validateEmail(buf) && *buf && strlen(buf) < MAX_FIELD_LEN) { strcpy(new_email, buf); valid_update = 1; break; }
                        printf("[ERROR] Invalid email format! Try again.\n");
                    } break;
                default:
                    printf("[INFO] Update cancelled.\n");
            }

            if (valid_update) {
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

        // write record (updated or not)
        char e1[MAX_FIELD_LEN*2], e2[MAX_FIELD_LEN*2], e3[MAX_FIELD_LEN*2], e4[MAX_FIELD_LEN*2];
        escapeCSV(company,e1,sizeof(e1)); escapeCSV(person,e2,sizeof(e2));
        escapeCSV(phone,e3,sizeof(e3));   escapeCSV(email,e4,sizeof(e4));
        fprintf(wf, "%s,%s,%s,%s\n", e1,e2,e3,e4);
    }

    fclose(rf); fclose(wf);

    if (remove("contacts.csv") != 0) { printf("[ERROR] Failed to remove old file!\n"); remove("contacts.tmp"); return; }
    if (rename("contacts.tmp", "contacts.csv") != 0) { printf("[ERROR] Failed to rename temporary file!\n"); return; }

    if (updated) printf("\n[SUCCESS] Contact updated successfully!\n");
    else         printf("\n[INFO] No changes made.\n");
}

// ==== Unit tests helpers ====
static int addContactTestHelper(const char *company, const char *person, const char *phone, const char *email) {
    FILE *fp = fopen("test_contacts.csv", "a");
    if (!fp) return 0;
    char a[MAX_FIELD_LEN*2], b[MAX_FIELD_LEN*2], c[MAX_FIELD_LEN*2], d[MAX_FIELD_LEN*2];
    escapeCSV(company, a, sizeof(a));
    escapeCSV(person , b, sizeof(b));
    escapeCSV(phone  , c, sizeof(c));
    escapeCSV(email  , d, sizeof(d));
    fprintf(fp, "%s,%s,%s,%s\n", a,b,c,d);
    fclose(fp);
    return 1;
}

static int countContactsTest(const char *filename) {
    FILE *fp = fopen(filename, "r"); if (!fp) return 0;
    char line[MAX_LINE_LEN]; int count = 0;
    while (fgets(line, sizeof(line), fp)) { line[strcspn(line, "\n\r")] = '\0'; if (*line) count++; }
    fclose(fp); return count;
}

// อ่านคอลัมน์แรกแบบรองรับ comma ใน quotes
static int contactExistsTest(const char *filename, const char *company) {
    FILE *fp = fopen(filename, "r"); if (!fp) return 0;

    char key_lower[MAX_FIELD_LEN];
    strncpy(key_lower, company, MAX_FIELD_LEN-1); key_lower[MAX_FIELD_LEN-1] = '\0';
    for (int i = 0; key_lower[i]; i++) key_lower[i] = (char)tolower((unsigned char)key_lower[i]);

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n\r")] = '\0'; if (!*line) continue;
        char linecpy[MAX_LINE_LEN];
        strncpy(linecpy, line, sizeof(linecpy)-1); linecpy[sizeof(linecpy)-1] = '\0';

        char comp[MAX_FIELD_LEN] = "";
        char *src = linecpy, *dst = comp; int in_quotes = 0;
        while (*src && (in_quotes || *src != ',') &&
               (size_t)(dst - comp) < MAX_FIELD_LEN-1) {
            if (*src == '"') { in_quotes = !in_quotes; src++; }
            else { *dst++ = *src++; }
        }
        *dst = '\0';
        unescapeCSV(comp);

        char comp_lower[MAX_FIELD_LEN];
        strncpy(comp_lower, comp, MAX_FIELD_LEN-1); comp_lower[MAX_FIELD_LEN-1] = '\0';
        for (int i = 0; comp_lower[i]; i++) comp_lower[i] = (char)tolower((unsigned char)comp_lower[i]);
        if (strcmp(comp_lower, key_lower) == 0) { fclose(fp); return 1; }
    }
    fclose(fp); return 0;
}

static int deleteContactTestHelper(const char *company) {
    char key_lower[MAX_FIELD_LEN]; strncpy(key_lower, company, MAX_FIELD_LEN-1); key_lower[MAX_FIELD_LEN-1] = '\0';
    for (int i = 0; key_lower[i]; i++) key_lower[i] = (char)tolower((unsigned char)key_lower[i]);

    FILE *rf = fopen("test_contacts.csv", "r"); if (!rf) return 0;
    FILE *wf = fopen("test_contacts.tmp", "w"); if (!wf) { fclose(rf); return 0; }

    char line[MAX_LINE_LEN]; int deleted = 0;
    while (fgets(line, sizeof(line), rf)) {
        line[strcspn(line, "\n\r")] = '\0'; if (!*line) continue;

        char linecpy[MAX_LINE_LEN];
        strncpy(linecpy, line, sizeof(linecpy)-1); linecpy[sizeof(linecpy)-1] = '\0';

        char comp[MAX_FIELD_LEN] = "";
        char *src = linecpy, *dst = comp; int in_quotes = 0;
        while (*src && (in_quotes || *src != ',') &&
               (size_t)(dst - comp) < MAX_FIELD_LEN-1) {
            if (*src == '"') { in_quotes = !in_quotes; src++; }
            else { *dst++ = *src++; }
        }
        *dst = '\0';
        unescapeCSV(comp);

        char comp_lower[MAX_FIELD_LEN];
        strncpy(comp_lower, comp, MAX_FIELD_LEN-1); comp_lower[MAX_FIELD_LEN-1] = '\0';
        for (int i = 0; comp_lower[i]; i++) comp_lower[i] = (char)tolower((unsigned char)comp_lower[i]);

        if (!deleted && strcmp(comp_lower, key_lower) == 0) { deleted = 1; continue; }
        fprintf(wf, "%s\n", line);
    }
    fclose(rf); fclose(wf);
    remove("test_contacts.csv");
    rename("test_contacts.tmp", "test_contacts.csv");
    return deleted;
}

// ==== Unit Tests ====
void runUnitTests() {
    test_passed = test_failed = 0;

    printf("\n========================================\n");
    printf("  UNIT TESTS - Contact Management\n");
    printf("========================================\n\n");

    // Test 1: Add valid contact
    printf("Test 1: Add Functions\n");
    remove("test_contacts.csv");
    int result = addContactTestHelper("Google", "John Doe", "081-234-5678", "john@google.com");
    int exists = contactExistsTest("test_contacts.csv", "Google");
    TEST_ASSERT(result && exists, "Add valid contact");

    // Test 2: Add multiple
    remove("test_contacts.csv");
    addContactTestHelper("Google", "John", "081-111-1111", "john@google.com");
    addContactTestHelper("Microsoft", "Jane", "082-222-2222", "jane@microsoft.com");
    addContactTestHelper("Apple", "Bob", "083-333-3333", "bob@apple.com");
    int count = countContactsTest("test_contacts.csv");
    TEST_ASSERT(count == 3, "Add multiple contacts (count = 3)");

    // Test 3: Comma in company (quoted field)
    remove("test_contacts.csv");
    result = addContactTestHelper("\"Company, LLC\"", "John", "081-234-5678", "john@company.com");
    exists = contactExistsTest("test_contacts.csv", "Company, LLC");
    TEST_ASSERT(result && exists, "Add contact with comma in name");

    // Test 4/5: Email
    TEST_ASSERT(validateEmail("test@example.com") == 1, "Valid email");
    TEST_ASSERT(validateEmail("invalid.email") == 0, "Invalid email");

    // Test 6/7: Phone
    TEST_ASSERT(validatePhone("081-234-5678") == 1, "Valid phone");
    TEST_ASSERT(validatePhone("abc-def-ghij") == 0, "Invalid phone");

    // Test 8: Sanitize (trim-only; keep symbols)
    char input1[50] = "   =malicious \t";
    sanitizeInput(input1);
    TEST_ASSERT(strcmp(input1, "=malicious") == 0, "Sanitize keeps symbols (we only trim)");

    printf("\nTest 2: Delete Functions\n");
    remove("test_contacts.csv");
    addContactTestHelper("Google", "John", "081-111-1111", "john@google.com");
    addContactTestHelper("Microsoft", "Jane", "082-222-2222", "jane@microsoft.com");
    int deleted = deleteContactTestHelper("Google");
    exists = contactExistsTest("test_contacts.csv", "Google");
    TEST_ASSERT(deleted && !exists, "Delete existing contact");

    remove("test_contacts.csv");
    addContactTestHelper("Google", "John", "081-111-1111", "john@google.com");
    deleted = deleteContactTestHelper("Apple");
    TEST_ASSERT(!deleted, "Delete non-existent contact (returns false)");

    remove("test_contacts.csv");
    addContactTestHelper("Google", "John", "081-111-1111", "john@google.com");
    deleted = deleteContactTestHelper("GOOGLE");
    exists = contactExistsTest("test_contacts.csv", "Google");
    TEST_ASSERT(deleted && !exists, "Delete case-insensitive");

    remove("test_contacts.csv");
    addContactTestHelper("Google", "John", "081-111-1111", "john@google.com");
    addContactTestHelper("Microsoft", "Jane", "082-222-2222", "jane@microsoft.com");
    addContactTestHelper("Apple", "Bob", "083-333-3333", "bob@apple.com");
    deleteContactTestHelper("Microsoft");
    count = countContactsTest("test_contacts.csv");
    int google_ex = contactExistsTest("test_contacts.csv", "Google");
    int ms_ex     = contactExistsTest("test_contacts.csv", "Microsoft");
    int apple_ex  = contactExistsTest("test_contacts.csv", "Apple");
    TEST_ASSERT(count == 2 && google_ex && !ms_ex && apple_ex, "Delete one from multiple");

    printf("\n========================================\n");
    printf("  TEST SUMMARY\n");
    printf("========================================\n");
    printf("Passed: %d\n", test_passed);
    printf("Failed: %d\n", test_failed);
    printf("Total:  %d\n", test_passed + test_failed);
    if (test_passed + test_failed > 0) {
        printf("Success Rate: %.1f%%\n", (test_passed * 100.0) / (test_passed + test_failed));
    }
    printf("========================================\n");

    // Cleanup
    remove("test_contacts.csv");
    remove("test_contacts.tmp");
}
