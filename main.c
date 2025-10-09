#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "test.h"


#ifdef _WIN32
  #include <conio.h>
  #define CLEAR_SCREEN "cls"
  #include <io.h>
  #define DUP    _dup
  #define DUP2   _dup2
  #define FILENO _fileno
#else
  #define DUP    dup
  #define DUP2   dup2
  #define FILENO fileno
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

static void trim_newline(char *s){
    if (!s) return;
    size_t n = strlen(s);
    while (n && (s[n-1] == '\n' || s[n-1] == '\r')) s[--n] = '\0';
}

static void consume_rest_of_line(void){
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { /* discard */ }
}

// ใช้แทน fgets ตรงๆ เพื่อกัน “Read error!”
static int read_line_prompt(const char *prompt, char *out, size_t out_size){
    if (prompt){ printf("%s", prompt); fflush(stdout); }
    if (!fgets(out, out_size, stdin)){
        if (feof(stdin)) {
            // stdin ถูกปิด: ถือว่า “ยกเลิก” แทนที่จะ error
            clearerr(stdin);
            out[0] = '0'; out[1] = '\0';   // โปรโตคอลในแอป: '0' = cancel
            return 0;                      // บอกว่าไม่ได้อ่านค่าปกติ
        }
        // กรณีอื่นๆ เคลียร์ error แล้วส่งสตริงว่าง
        clearerr(stdin);
        out[0] = '\0';
        return 0;
    }
    trim_newline(out);
    return 1;
}

// ถ้าต้องอ่านตัวเลือกเป็นตัวเลข แทน scanf ด้วยฟังก์ชันนี้
static int read_int_choice(const char *prompt, int *out){
    char buf[64];
    if (!read_line_prompt(prompt, buf, sizeof(buf))) return 0;
    if (buf[0] == '\0') return 0;
    char *end = NULL;
    long v = strtol(buf, &end, 10);
    if (end == buf) return 0;
    *out = (int)v;
    return 1;
}

// ==== File path switcher (so E2E doesn't touch real data) ====
static char g_contacts_file[256] = "contacts.csv";
const char* getContactsFile() { return g_contacts_file; }
void setContactsFile(const char* path) {
    if (path && *path) {
        strncpy(g_contacts_file, path, sizeof(g_contacts_file)-1);
        g_contacts_file[sizeof(g_contacts_file)-1] = '\0';
    }
}
static void makeTempPath(char *buf, size_t n) {
    snprintf(buf, n, "%s.tmp", getContactsFile());
}

// ==== Declarations ====
void addContact();
void listContacts();
void deleteContact();
void searchContact();
void updateContact();
void runE2ETests();

void clearInputBuffer();
int  confirmAction(const char *message);
void trimWhitespace(char *str);
void sanitizeInput(char *str);
void escapeCSV(const char *input, char *output, size_t output_size);
void unescapeCSV(char *str);
int  validateEmail(const char *email);
int  validatePhone(const char *phone);

// small CSV parser for 4 fields handling quotes
static void parseCsv4(const char *srcLine,
                      char *f1, size_t n1,
                      char *f2, size_t n2,
                      char *f3, size_t n3,
                      char *f4, size_t n4);

// helpers for tests
static void normalizePhone(const char *in, char *out, size_t out_size);
// static int contactExistsByCompanyCI(const char *filename, const char *company);
// static int contactExistsByPhoneNorm  (const char *filename, const char *phone_raw);
// static int contactExistsByEmailCI    (const char *filename, const char *email_raw);

// // ==== Globals for tests ====
// static int test_passed = 0, test_failed = 0;
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
        printf("7. Run E2E Tests\n");
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
            case 7: runE2ETests();  break;
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
    while (end > start && isspace((unsigned char)end[0])) end--;
    size_t len = (size_t)(end - start + 1);
    memmove(str, start, len);
    str[len] = '\0';
}

// trim-only (keep symbols). This keeps leading '=', '+', etc.
void sanitizeInput(char *str) {
    if (!str) return;
    size_t len = strlen(str);
    while (len > 0 && (str[len-1] == '\r' || str[len-1] == '\n')) str[--len] = '\0';
    trimWhitespace(str);
}


// --- Robust normalization helpers (added) ---
static void toLowerInPlace(char *s) {
    if (!s) return;
    for (char *p = s; *p; ++p) *p = (char)tolower((unsigned char)*p);
}

// Normalize for text keys: trim, lower, keep only [a-z0-9 ] and compress spaces
static void normalizeKey(char *s) {
    if (!s) return;
    trimWhitespace(s);
    toLowerInPlace(s);
    char *r = s, *w = s; int spaced = 1;
    while (*r) {
        unsigned char c = (unsigned char)*r++;
        if (isalnum(c)) { *w++ = (char)c; spaced = 0; }
        else if (isspace(c)) { if (!spaced) { *w++ = ' '; spaced = 1; } }
        // drop punctuation (quotes, commas, etc.)
    }
    if (w > s && *(w-1) == ' ') --w;
    *w = '\0';
}
// Escape a CSV field (RFC4180-ish)
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

    // ต้องมี '@' และไม่อยู่ตัวแรก
    const char *at = strchr(email, '@');
    if (!at || at == email) return 0;

    const char *domain = at + 1;
    if (*domain == '\0') return 0;

    //  ห้ามโดเมนขึ้นต้นด้วย '.'
    if (domain[0] == '.') return 0;            // <-- แก้ D5

    //  ห้ามมี '..' ติดกันทั้งสตริง
    for (const char *p = email; *p; ++p)
        if (p[0] == '.' && p[1] == '.') return 0;

    // ต้องมีจุดในโดเมน และ  ห้ามจบด้วย '.'
    const char *lastdot = strrchr(domain, '.');
    if (!lastdot) return 0;
    if (email[strlen(email) - 1] == '.') return 0;

    // ผ่านเกณฑ์พื้นฐานทั้งหมด
    return 1;
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

// helper: digits-only normalization for phone matching
static void normalizePhone(const char *in, char *out, size_t out_size) {
    if (!in || !out || out_size == 0) return;
    size_t j = 0;
    for (size_t i = 0; in[i] && j + 1 < out_size; i++) {
        if (isdigit((unsigned char)in[i])) out[j++] = in[i];
    }
    out[j] = '\0';
}

// Parse CSV line into 4 fields with quotes support
static void parseCsv4(const char *srcLine,
                      char *f1, size_t n1,
                      char *f2, size_t n2,
                      char *f3, size_t n3,
                      char *f4, size_t n4) {
    f1[0]=f2[0]=f3[0]=f4[0]='\0';
    const char *src = srcLine;
    char *dsts[4] = { f1, f2, f3, f4 };
    size_t caps[4] = { n1, n2, n3, n4 };
    int idx = 0, inq = 0;
    while (*src && idx < 4) {
        if (*src == '"') { inq = !inq; src++; }
        else if (*src == ',' && !inq) {
            // terminate this field
            if (caps[idx] > 0) dsts[idx][ (caps[idx]-1) < strlen(dsts[idx]) ? (caps[idx]-1) : strlen(dsts[idx]) ] = '\0';
            idx++;
            if (idx >= 4) break;
            dsts[idx][0] = '\0';
            src++;
        } else {
            size_t cur_len = strlen(dsts[idx]);
            if (cur_len + 2 < caps[idx]) {
                dsts[idx][cur_len] = *src;
                dsts[idx][cur_len+1] = '\0';
            }
            src++;
        }
    }
    // unescape quotes in each
    unescapeCSV(f1); unescapeCSV(f2); unescapeCSV(f3); unescapeCSV(f4);
}

// ==== Add Contact ====
void addContact() {
    struct Contact c;
    char temp[MAX_FIELD_LEN * 2];

    printf("\n=== Add New Contact ===\n(Enter 0 at any field to cancel)\n\n");

    // Company
    while (1) {
        if (!read_line_prompt("Enter company name: ", temp, sizeof(temp))) {
            printf("[CANCEL] Input canceled.\n"); return;
        }
        if (strcmp(temp, "0") == 0) { printf("[INFO] Add cancelled.\n"); return; }
        sanitizeInput(temp);
        if (!*temp) { printf("[ERROR] Company name cannot be empty!\n"); continue; }
        if (strlen(temp) >= MAX_FIELD_LEN) { printf("[ERROR] Too long.\n"); continue; }
        strncpy(c.company, temp, MAX_FIELD_LEN); c.company[MAX_FIELD_LEN-1] = '\0'; break;
    }

    // Person
    while (1) {
        if (!read_line_prompt("Enter contact person: ", temp, sizeof(temp))) {
            printf("[CANCEL] Input canceled.\n"); return;
        }
        if (strcmp(temp, "0") == 0) { printf("[INFO] Add cancelled.\n"); return; }
        sanitizeInput(temp);
        if (!*temp) { printf("[ERROR] Person name cannot be empty!\n"); continue; }
        if (strlen(temp) >= MAX_FIELD_LEN) { printf("[ERROR] Too long.\n"); continue; }
        strncpy(c.person, temp, MAX_FIELD_LEN); c.person[MAX_FIELD_LEN-1] = '\0'; break;
    }

    // Phone
    while (1) {
        if (!read_line_prompt("Enter phone: ", temp, sizeof(temp))) {
            printf("[CANCEL] Input canceled.\n"); return;
        }
        if (strcmp(temp, "0") == 0) { printf("[INFO] Add cancelled.\n"); return; }
        sanitizeInput(temp);
        if (!*temp) { printf("[ERROR] Phone cannot be empty!\n"); continue; }
        if (!validatePhone(temp)) { printf("[ERROR] Invalid phone.\n"); continue; }
        if (strlen(temp) >= MAX_FIELD_LEN) { printf("[ERROR] Too long.\n"); continue; }
        strncpy(c.phone, temp, MAX_FIELD_LEN); c.phone[MAX_FIELD_LEN-1] = '\0'; break;
    }

    // Email
    while (1) {
        if (!read_line_prompt("Enter email: ", temp, sizeof(temp))) {
            printf("[CANCEL] Input canceled.\n"); return;
        }
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
    FILE *fp = fopen(getContactsFile(), "a");
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

    FILE *fp = fopen(getContactsFile(), "r");
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

        char company[MAX_FIELD_LEN] = "", person[MAX_FIELD_LEN] = "", phone[MAX_FIELD_LEN] = "", email[MAX_FIELD_LEN] = "";
        parseCsv4(line, company, sizeof(company), person, sizeof(person), phone, sizeof(phone), email, sizeof(email));

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

// ==== Delete (by company/person/email exact-insensitive, phone normalized) ====
void deleteContact() {
    char key[MAX_FIELD_LEN];
    printf("\n=== Delete Contact ===\n");
    printf("Enter company / contact person / phone / email to delete (or 0 to cancel): ");
    if (!fgets(key, sizeof(key), stdin)) { printf("[ERROR] Failed to read input!\n"); return; }
    key[strcspn(key, "\n")] = '\0';
    trimWhitespace(key);

    if (strcmp(key, "0") == 0) { printf("[INFO] Delete cancelled.\n"); return; }
    if (!*key) { printf("[ERROR] Keyword cannot be empty!\n"); return; }

    char key_lower[MAX_FIELD_LEN];
    strncpy(key_lower, key, MAX_FIELD_LEN - 1);
    key_lower[MAX_FIELD_LEN - 1] = '\0';
    for (int i = 0; key_lower[i]; i++) key_lower[i] = (char)tolower((unsigned char)key_lower[i]);

    char key_phone_norm[MAX_FIELD_LEN];
    normalizePhone(key, key_phone_norm, sizeof(key_phone_norm));
    int key_is_phone = (int)(strlen(key_phone_norm) > 0);
    int key_is_email = (strchr(key, '@') != NULL);

    char key_norm[MAX_FIELD_LEN];
    strncpy(key_norm, key, MAX_FIELD_LEN - 1); key_norm[MAX_FIELD_LEN - 1] = '\0';
    normalizeKey(key_norm);
FILE *rf = fopen(getContactsFile(), "r");
    if (!rf) { printf("[ERROR] No contacts file found!\n"); return; }

    typedef struct {
        char company[MAX_FIELD_LEN];
        char person [MAX_FIELD_LEN];
        char phone  [MAX_FIELD_LEN];
        char email  [MAX_FIELD_LEN];
        char rawline[MAX_LINE_LEN];
    } Row;

    enum { MAX_MATCH = 1024 };
    Row matches[MAX_MATCH];
    int mcount = 0;

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), rf)) {
        line[strcspn(line, "\n\r")] = '\0';
        if (!*line) continue;

        char company[MAX_FIELD_LEN] = "", person[MAX_FIELD_LEN] = "", phone[MAX_FIELD_LEN] = "", email[MAX_FIELD_LEN] = "";
        parseCsv4(line, company, sizeof(company), person, sizeof(person), phone, sizeof(phone), email, sizeof(email));

        if (!*company && !*person && !*phone && !*email) continue;

        char company_lower[MAX_FIELD_LEN], person_lower[MAX_FIELD_LEN], email_lower[MAX_FIELD_LEN];
        strncpy(company_lower, company, MAX_FIELD_LEN - 1); company_lower[MAX_FIELD_LEN - 1] = '\0';
        strncpy(person_lower , person , MAX_FIELD_LEN - 1); person_lower [MAX_FIELD_LEN - 1] = '\0';
        strncpy(email_lower  , email  , MAX_FIELD_LEN - 1); email_lower  [MAX_FIELD_LEN - 1] = '\0';
        for (int i = 0; company_lower[i]; i++) company_lower[i] = (char)tolower((unsigned char)company_lower[i]);
        for (int i = 0; person_lower [i]; i++) person_lower [i] = (char)tolower((unsigned char)person_lower [i]);
        for (int i = 0; email_lower  [i]; i++) email_lower  [i] = (char)tolower((unsigned char)email_lower  [i]);

        char phone_norm[MAX_FIELD_LEN];
        normalizePhone(phone, phone_norm, sizeof(phone_norm));

        
        int match = 0;
        if (key_is_phone) {
            if (*phone_norm && strcmp(phone_norm, key_phone_norm) == 0) match = 1;
        }
        if (!match && key_is_email) {
            if (*email_lower && strstr(email_lower, key_lower) != NULL) match = 1; // CI substring for email
        }
        if (!match) {
            // Build normalized text keys for robust company/person matching
            char company_norm[MAX_FIELD_LEN]; strncpy(company_norm, company, MAX_FIELD_LEN - 1); company_norm[MAX_FIELD_LEN - 1] = '\0'; normalizeKey(company_norm);
            char person_norm [MAX_FIELD_LEN]; strncpy(person_norm , person , MAX_FIELD_LEN - 1); person_norm [MAX_FIELD_LEN - 1] = '\0'; normalizeKey(person_norm);
            if ((strstr(company_norm, key_norm) != NULL) || (strstr(person_norm, key_norm) != NULL)) {
                match = 1;
            }
        }


        if (match) {
            if (mcount < MAX_MATCH) {
                strncpy(matches[mcount].company, company, MAX_FIELD_LEN - 1);
                strncpy(matches[mcount].person , person , MAX_FIELD_LEN - 1);
                strncpy(matches[mcount].phone  , phone  , MAX_FIELD_LEN - 1);
                strncpy(matches[mcount].email  , email  , MAX_FIELD_LEN - 1);
                matches[mcount].company[MAX_FIELD_LEN-1] = '\0';
                matches[mcount].person [MAX_FIELD_LEN-1] = '\0';
                matches[mcount].phone  [MAX_FIELD_LEN-1] = '\0';
                matches[mcount].email  [MAX_FIELD_LEN-1] = '\0';
                strncpy(matches[mcount].rawline, line, MAX_LINE_LEN - 1);
                matches[mcount].rawline[MAX_LINE_LEN-1] = '\0';
                mcount++;
            }
        }
    }
    fclose(rf);

    if (mcount == 0) {
        printf("\n[INFO] No record matches '%s'.\n", key);
        return;
    }

    int choice_idx = 0;
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

        int sel = 0;
        while (1) {
            printf("Select [0-%d]: ", mcount);
            int choice = -1;
            if (!read_int_choice("Enter your choice: ", &choice)) {
            choice = -1; // จะวิ่งไป default case
            }

            if (sel == 0) { printf("[INFO] Delete cancelled.\n"); return; }
            if (sel >= 1 && sel <= mcount) {
                printf("\n--- Contact to Delete ---\n");
                printf("Company : %s\n", matches[sel-1].company);
                printf("Contact : %s\n", matches[sel-1].person);
                printf("Phone   : %s\n", matches[sel-1].phone);
                printf("Email   : %s\n", matches[sel-1].email);
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

    char tmpfile[300];
    makeTempPath(tmpfile, sizeof(tmpfile));
    rf = fopen(getContactsFile(), "r");
    if (!rf) { printf("[ERROR] Cannot open contacts file!\n"); return; }
    FILE *wf = fopen(tmpfile, "w");
    if (!wf) { fclose(rf); printf("[ERROR] Cannot create temporary file!\n"); return; }

    int deleted = 0;
    while (fgets(line, sizeof(line), rf)) {
        line[strcspn(line, "\n\r")] = '\0';
        if (!*line) continue;
        if (!deleted && strcmp(line, matches[choice_idx - 1].rawline) == 0) {
            deleted = 1; // skip
            continue;
        }
        fprintf(wf, "%s\n", line);
    }
    fclose(rf);
    fclose(wf);

    if (remove(getContactsFile()) != 0) {
        printf("[ERROR] Failed to remove old file!\n");
        remove(tmpfile);
        return;
    }
    if (rename(tmpfile, getContactsFile()) != 0) {
        printf("[ERROR] Failed to rename temporary file!\n");
        return;
    }

    if (deleted) printf("\n[SUCCESS] Contact deleted successfully!\n");
    else         printf("\n[INFO] Nothing was deleted.\n");
}

// ==== Search (case-insensitive; company/person/email = prefix match, phone = substring) ====
void searchContact() {
    char key[MAX_FIELD_LEN];
    printf("\n=== Search Contact ===\n");
    printf("Enter keyword (company/person/phone/email, or 0 to cancel): ");
    if (!fgets(key, sizeof(key), stdin)) { printf("[ERROR] Failed to read input!\n"); return; }
    key[strcspn(key, "\n")] = '\0';
    sanitizeInput(key);
    if (strcmp(key, "0") == 0) { printf("[INFO] Search cancelled.\n"); return; }
    if (!*key) { printf("[ERROR] Search keyword cannot be empty!\n"); return; }

    // เตรียมคีย์เวิร์ด (lowercase / normalize)
    char key_lower[MAX_FIELD_LEN];
    strncpy(key_lower, key, MAX_FIELD_LEN - 1);
    key_lower[MAX_FIELD_LEN - 1] = '\0';
    for (int i = 0; key_lower[i]; i++) key_lower[i] = (char)tolower((unsigned char)key_lower[i]);

    char key_phone_norm[MAX_FIELD_LEN];
    normalizePhone(key, key_phone_norm, sizeof(key_phone_norm));

    int key_is_phone = (int)(strlen(key_phone_norm) > 0);   // ถ้ามีตัวเลขจน normalize แล้วไม่ว่าง
    int key_is_email = (strchr(key, '@') != NULL);          // เดาจาก '@'

    FILE *fp = fopen(getContactsFile(), "r");
    if (!fp) { printf("[ERROR] No contacts file found!\n"); return; }

    char line[MAX_LINE_LEN];
    int found = 0;

    printf("\n--- Search Results ---\n");
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n\r")] = '\0';
        if (!*line) continue;

        char company[MAX_FIELD_LEN] = "", person[MAX_FIELD_LEN] = "", phone[MAX_FIELD_LEN] = "", email[MAX_FIELD_LEN] = "";
        parseCsv4(line, company, sizeof(company), person, sizeof(person), phone, sizeof(phone), email, sizeof(email));
        if (!*company && !*person && !*phone && !*email) continue;

        char company_lower[MAX_FIELD_LEN], person_lower[MAX_FIELD_LEN], email_lower[MAX_FIELD_LEN], phone_norm[MAX_FIELD_LEN];
        strncpy(company_lower, company, MAX_FIELD_LEN - 1); company_lower[MAX_FIELD_LEN - 1] = '\0';
        strncpy(person_lower , person , MAX_FIELD_LEN - 1); person_lower [MAX_FIELD_LEN - 1] = '\0';
        strncpy(email_lower  , email  , MAX_FIELD_LEN - 1); email_lower  [MAX_FIELD_LEN - 1] = '\0';
        for (int i = 0; company_lower[i]; i++) company_lower[i] = (char)tolower((unsigned char)company_lower[i]);
        for (int i = 0; person_lower [i]; i++) person_lower [i] = (char)tolower((unsigned char)person_lower [i]);
        for (int i = 0; email_lower  [i]; i++) email_lower  [i] = (char)tolower((unsigned char)email_lower  [i]);

        normalizePhone(phone, phone_norm, sizeof(phone_norm));

        int match = 0;
        size_t klen = strlen(key_lower);

        if (key_is_phone) {
            if (*phone_norm && strstr(phone_norm, key_phone_norm) != NULL) match = 1;
        } else if (key_is_email) {
            if (*email_lower && klen > 0 && strncmp(email_lower, key_lower, klen) == 0) match = 1;
        } else {
            if (!match && *company_lower && klen > 0 && strncmp(company_lower, key_lower, klen) == 0) match = 1;
            if (!match && *person_lower  && klen > 0 && strncmp(person_lower , key_lower, klen) == 0) match = 1;
        }

        if (match) {
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

// ==== Update (by company, case-insensitive) ====
void updateContact() {
    char key[MAX_FIELD_LEN];

    printf("\n=== Update Contact ===\n");
    if (!read_line_prompt("Enter company name to update (or 0 to cancel): ", key, sizeof(key))) {
        printf("[CANCEL] Input canceled.\n");
        return;
    }
    trimWhitespace(key);
    if (strcmp(key, "0") == 0) { printf("[INFO] Update cancelled.\n"); return; }
    if (!*key) { printf("[ERROR] Company name cannot be empty!\n"); return; }

    char key_norm[MAX_FIELD_LEN];
    strncpy(key_norm, key, MAX_FIELD_LEN - 1);
    key_norm[MAX_FIELD_LEN - 1] = '\0';   // <- FIX: ต้อง \0 ไม่ใช่ ' '
    normalizeKey(key_norm);

    FILE *rf = fopen(getContactsFile(), "r");
    if (!rf) { printf("[ERROR] No contacts file found!\n"); return; }

    char tmpfile[300]; 
    makeTempPath(tmpfile, sizeof(tmpfile));
    FILE *wf = fopen(tmpfile, "w");
    if (!wf) { fclose(rf); printf("[ERROR] Cannot create temporary file!\n"); return; }

    char line[MAX_LINE_LEN];
    int updated = 0;

    while (fgets(line, sizeof(line), rf)) {
        line[strcspn(line, "\n\r")] = '\0';
        if (!*line) continue;

        char company[MAX_FIELD_LEN] = "";
        char person [MAX_FIELD_LEN] = "";
        char phone  [MAX_FIELD_LEN] = "";
        char email  [MAX_FIELD_LEN] = "";
        parseCsv4(line, company, sizeof(company),
                        person,  sizeof(person),
                        phone,   sizeof(phone),
                        email,   sizeof(email));

        char company_norm[MAX_FIELD_LEN];
        strncpy(company_norm, company, MAX_FIELD_LEN - 1);
        company_norm[MAX_FIELD_LEN - 1] = '\0';
        normalizeKey(company_norm);

        if (!updated && *company && strcmp(company_norm, key_norm) == 0) {
            int choice = -1;                      // <- FIX: ไม่ประกาศซ้ำ
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
            if (!read_int_choice("Choice: ", &choice)) choice = 0;

            if (choice == 0) {
                printf("[INFO] Update cancelled.\n");
                // write original
                char e1[MAX_FIELD_LEN*2], e2[MAX_FIELD_LEN*2], e3[MAX_FIELD_LEN*2], e4[MAX_FIELD_LEN*2];
                escapeCSV(company,e1,sizeof(e1)); escapeCSV(person,e2,sizeof(e2));
                escapeCSV(phone,e3,sizeof(e3));   escapeCSV(email,e4,sizeof(e4));
                fprintf(wf, "%s,%s,%s,%s\n", e1,e2,e3,e4);
                continue;
            }

            int  valid_update = 0;
            char new_company[MAX_FIELD_LEN], new_person[MAX_FIELD_LEN];
            char new_phone  [MAX_FIELD_LEN], new_email [MAX_FIELD_LEN];
            strcpy(new_company, company);
            strcpy(new_person,  person);
            strcpy(new_phone,   phone);
            strcpy(new_email,   email);

            switch (choice) {
                case 1: // Company
                    while (1) {
                        if (!read_line_prompt("New Company (0 to cancel): ", buf, sizeof(buf))) { 
                            printf("[CANCEL] Input canceled.\n"); break; 
                        }
                        if (strcmp(buf, "0") == 0) break;
                        sanitizeInput(buf);
                        if (*buf && strlen(buf) < MAX_FIELD_LEN) { 
                            strcpy(new_company, buf); valid_update = 1; break; 
                        }
                        printf("[ERROR] Invalid input! Try again.\n");
                    }
                    break;

                case 2: // Contact Person
                    while (1) {
                        if (!read_line_prompt("New Contact (0 to cancel): ", buf, sizeof(buf))) { 
                            printf("[CANCEL] Input canceled.\n"); break; 
                        }
                        if (strcmp(buf, "0") == 0) break;
                        sanitizeInput(buf);
                        if (*buf && strlen(buf) < MAX_FIELD_LEN) { 
                            strcpy(new_person, buf); valid_update = 1; break; 
                        }
                        printf("[ERROR] Invalid input! Try again.\n");
                    }
                    break;

                case 3: // Phone
                    while (1) {
                        if (!read_line_prompt("New Phone (0 to cancel): ", buf, sizeof(buf))) { 
                            printf("[CANCEL] Input canceled.\n"); break; 
                        }
                        if (strcmp(buf, "0") == 0) break;
                        sanitizeInput(buf);
                        if (validatePhone(buf) && *buf && strlen(buf) < MAX_FIELD_LEN) { 
                            strcpy(new_phone, buf); valid_update = 1; break; 
                        }
                        printf("[ERROR] Invalid phone format! Try again.\n");
                    }
                    break;

                case 4: // Email
                    while (1) {
                        if (!read_line_prompt("New Email (0 to cancel): ", buf, sizeof(buf))) {
                            printf("[CANCEL] Input canceled.\n"); break;
                        }
                        if (strcmp(buf, "0") == 0) break;
                        sanitizeInput(buf);
                        if (validateEmail(buf) && *buf && strlen(buf) < MAX_FIELD_LEN) { 
                            strcpy(new_email, buf); valid_update = 1; break; 
                        }
                        printf("[ERROR] Invalid email format! Try again.\n");
                    }
                    break;

                default:
                    printf("[INFO] Update cancelled.\n");
                    break;
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
                    strcpy(person,  new_person);
                    strcpy(phone,   new_phone);
                    strcpy(email,   new_email);
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

    fclose(rf);
    fclose(wf);

    if (remove(getContactsFile()) != 0) { printf("[ERROR] Failed to remove old file!\n"); remove(tmpfile); return; }
    if (rename(tmpfile, getContactsFile()) != 0) { printf("[ERROR] Failed to rename temporary file!\n"); return; }

    if (updated) printf("\n[SUCCESS] Contact updated successfully!\n");
    else         printf("\n[INFO] No changes made.\n");
}

// ===== E2E helpers (file = contacts.csv) =====
static int saveRowToFile(const char* filename,
                         const char* company, const char* person,
                         const char* phone,   const char* email) {
    FILE *fp = fopen(filename, "a");
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

static int deleteByCompanyCI_File(const char* filename, const char* company) {
    char key_lower[MAX_FIELD_LEN];
    strncpy(key_lower, company, MAX_FIELD_LEN-1);
    key_lower[MAX_FIELD_LEN-1] = '\0';
    for (int i = 0; key_lower[i]; i++) key_lower[i] = (char)tolower((unsigned char)key_lower[i]);

    FILE *rf = fopen(filename, "r"); if (!rf) return 0;
    FILE *wf = fopen("contacts.tmp", "w"); if (!wf) { fclose(rf); return 0; }

    char line[MAX_LINE_LEN]; int deleted = 0;
    while (fgets(line, sizeof(line), rf)) {
        line[strcspn(line, "\n\r")] = '\0'; if (!*line) continue;

        char linecpy[MAX_LINE_LEN]; strncpy(linecpy, line, sizeof(linecpy)-1); linecpy[sizeof(linecpy)-1] = '\0';
        char comp[MAX_FIELD_LEN] = ""; int in_quotes = 0;
        char *src = linecpy, *dst = comp;
        while (*src && (in_quotes || *src != ',') && (size_t)(dst - comp) < MAX_FIELD_LEN-1) {
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

    remove(filename);
    rename("contacts.tmp", filename);
    return deleted;
}

static int deleteByPhoneNorm_File(const char* filename, const char* phone_raw) {
    FILE *rf = fopen(filename, "r"); if (!rf) return 0;
    FILE *wf = fopen("contacts.tmp", "w"); if (!wf) { fclose(rf); return 0; }

    char key_norm[MAX_FIELD_LEN]; normalizePhone(phone_raw, key_norm, sizeof(key_norm));
    int deleted = 0;
    char line[MAX_LINE_LEN];

    while (fgets(line, sizeof(line), rf)) {
        line[strcspn(line, "\n\r")] = '\0'; if (!*line) continue;

        char linecpy[MAX_LINE_LEN]; strncpy(linecpy, line, sizeof(linecpy)-1); linecpy[sizeof(linecpy)-1] = '\0';

        char company[MAX_FIELD_LEN] = "", person[MAX_FIELD_LEN] = "", phone[MAX_FIELD_LEN] = "", email[MAX_FIELD_LEN] = "";
        int field_idx = 0, in_quotes = 0;
        char *fields[4] = {company, person, phone, email};
        char *src = linecpy, *dst = fields[0];

        while (*src && field_idx < 4) {
            if (*src == '"') { in_quotes = !in_quotes; src++; }
            else if (*src == ',' && !in_quotes) { *dst = '\0'; field_idx++; if (field_idx<4) dst = fields[field_idx]; src++; }
            else { *dst++ = *src++; }
        }
        *dst = '\0';
        unescapeCSV(phone);

        char pn[MAX_FIELD_LEN]; normalizePhone(phone, pn, sizeof(pn));

        if (!deleted && *pn && strcmp(pn, key_norm) == 0) { deleted = 1; continue; }
        fprintf(wf, "%s\n", line);
    }

    fclose(rf); fclose(wf);
    remove(filename);
    rename("contacts.tmp", filename);
    return deleted;
}

// ===== E2E helpers =====
static int write_input_script(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    fputs(content, f);
    fclose(f);
    return 1;
}

static int with_stdin_script(const char *script, void (*fn)(void)) {
    const char *tmp = "e2e_stdin.txt";
    if (!write_input_script(tmp, script)) return 0;

    int saved_fd = DUP(FILENO(stdin));
    if (saved_fd < 0) return 0;

    FILE *r = freopen(tmp, "r", stdin);
    if (!r) { /* restore and bail */ DUP2(saved_fd, FILENO(stdin)); return 0; }

    // call the interactive function with prepared inputs
    fn();

    // restore stdin
    DUP2(saved_fd, FILENO(stdin));
#ifdef _WIN32
    _close(saved_fd);
#else
    close(saved_fd);
#endif
    remove(tmp);
    return 1;
}

// ===== E2E TESTS =====
#if 0

void runE2ETests() {
    // === E2E mode: use PRODUCTION file with backup/restore ===
    setContactsFile(getContactsFile());
    char __bak_path[512];
    snprintf(__bak_path, sizeof(__bak_path), "%s.bak", getContactsFile());

    int had_backup = 0;
    { FILE *probe = fopen(getContactsFile(), "r"); if (probe) { fclose(probe); had_backup = 1; } }

    if (had_backup) {
        if (rename(getContactsFile(), __bak_path) != 0) {
            FILE *rf = fopen(getContactsFile(), "r"); FILE *wf = rf ? fopen(__bak_path, "w") : NULL;
            if (rf && wf) { char buf[4096]; size_t n; while ((n = fread(buf,1,sizeof(buf),rf))>0) fwrite(buf,1,n,wf); }
            if (rf) fclose(rf);
            if (wf) fclose(wf);
        }
    }
    { FILE *init = fopen(getContactsFile(), "w"); if (init) fclose(init); }
    char __tmp_path[256]; makeTempPath(__tmp_path, sizeof(__tmp_path));

    int pass = 0, fail = 0;
    #define E2E_ASSERT(cond, name) do{ if (cond){ printf("  [PASS] %s\n", name); pass++; } else { printf("  [FAIL] %s\n", name); fail++; } }while(0)

    printf("\n========================================\n");
    printf("  E2E TESTS - Contact Management\n");
    printf("========================================\n\n");
// --- E1: Add multiple (with comma, quotes, and duplicates) ---
    // DupCo #1
    with_stdin_script(
        "DupCo\n"
        "A\n"
        "090-111-1111\n"
        "a@d.com\n"
        "y\n",
        addContact
    );
    // DupCo #2
    with_stdin_script(
        "DupCo\n"
        "B\n"
        "090-222-2222\n"
        "b@d.com\n"
        "y\n",
        addContact
    );
    // Company, LLC (comma)
    with_stdin_script(
        "Company, LLC\n"
        "John Jr.\n"
        "(081) 234-5678\n"
        "john@company.com\n"
        "y\n",
        addContact
    );
    // Alpha "Inc" (embedded quotes)
    with_stdin_script(
        "Alpha \"Inc\"\n"
        "Alice \"A.\"\n"
        "090-000-0000\n"
        "Alice@Alpha.com\n"
        "y\n",
        addContact
    );

    E2E_ASSERT(countContactsTest(getContactsFile()) == 4, "E1: added 4 rows");
    E2E_ASSERT(contactExistsByCompanyCI(getContactsFile(), "Company, LLC"), "E1.1: comma in company is stored/parsed");
    E2E_ASSERT(contactExistsByCompanyCI(getContactsFile(), "Alpha \"Inc\""), "E1.2: embedded quotes are stored/parsed");

    // --- E2: Search (smoke) ---
    // just run it to ensure no crash; we assert by file state instead
    with_stdin_script("alpha\n", searchContact);
    E2E_ASSERT(contactExistsByEmailCI(getContactsFile(), "alice@alpha.com"), "E2: email case-insensitive exists");

    // --- E3: Update Alpha "Inc" phone -> +66 90 000 0000 ---
    with_stdin_script(
        "Alpha \"Inc\"\n"  // company to update
        "3\n"              // choose Phone
        "+66 90 000 0000\n"
        "y\n",             // confirm save
        updateContact
    );
    E2E_ASSERT(contactExistsByPhoneNorm(getContactsFile(), "+66900000000"), "E3: updated phone (normalized) exists");
    E2E_ASSERT(!contactExistsByPhoneNorm(getContactsFile(), "0900000000"), "E3.1: old phone no longer exists");

    // --- E4: Delete one of duplicates (DupCo) by selecting #2 ---
    with_stdin_script(
        "DupCo\n"  // key matches multiple -> will show list
        "2\n"      // select second record
        "y\n",     // confirm
        deleteContact
    );
    E2E_ASSERT(countContactsTest(getContactsFile()) == 3, "E4: one of duplicates removed");
    E2E_ASSERT(contactExistsByCompanyCI(getContactsFile(), "DupCo"), "E4.1: a DupCo record still remains");

    // --- E5: Delete by phone (Company, LLC) using normalized phone ---
    with_stdin_script(
        "(081) 234-5678\n"
        "y\n",
        deleteContact
    );
    E2E_ASSERT(!contactExistsByCompanyCI(getContactsFile(), "Company, LLC"), "E5: delete by phone removed Company, LLC");
    E2E_ASSERT(countContactsTest(getContactsFile()) == 2, "E5.1: 2 rows left after phone-based delete");

    // --- E6: Delete by company (case-insensitive) Alpha "Inc" ---
    with_stdin_script(
        "alpha \"inc\"\n"
        "y\n",
        deleteContact
    );
    E2E_ASSERT(!contactExistsByCompanyCI(getContactsFile(), "Alpha \"Inc\""), "E6: case-insensitive delete (company)");
    E2E_ASSERT(countContactsTest(getContactsFile()) == 1, "E6.1: 1 row left");

    // --- E7: Final cleanup - delete remaining DupCo ---
    with_stdin_script(
        "DupCo\n"
        "y\n",   // only one match now -> direct confirm
        deleteContact
    );
    E2E_ASSERT(countContactsTest(getContactsFile()) == 0, "E7: empty after final delete");

    // --- E8: Delete non-existent should be no-op ---
    with_stdin_script("NoSuchKey\n", deleteContact);
    E2E_ASSERT(countContactsTest(getContactsFile()) == 0, "E8: delete non-existent is a no-op");

    // Summary
    printf("\n========================================\n");
    printf("  E2E SUMMARY\n");
    printf("========================================\n");
    printf("Passed: %d\n", pass);
    printf("Failed: %d\n", fail);
    printf("Total:  %d\n", pass + fail);
    if (pass + fail > 0) {
        printf("Success Rate: %.1f%%\n", (pass * 100.0) / (pass + fail));
    }
    printf("========================================\n");

    // === Restore original user data ===
    if (had_backup) { remove(getContactsFile()); rename(__bak_path, getContactsFile()); }
}
#endif
