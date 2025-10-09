// ===============================================
//  Unit_test.c  —  Consolidated & Structured A–H
//  Covers: addContact / listContacts / searchContact
//          updateContact / deleteContact
//  Plus: CSV escaping/unescaping, email/phone validation,
//        existence checks, file boundary cases
// ===============================================

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "test.h"   // prototypes: runUnitTests, countContactsTest, contactExistsBy* ...

// ===== extern (from main.c) =====
extern void addContact(void);
extern void listContacts(void);
extern void searchContact(void);
extern void deleteContact(void);
extern void updateContact(void);

extern const char* getContactsFile(void);
extern void setContactsFile(const char* path);

// helpers provided by main.c
extern void escapeCSV(const char *input, char *output, size_t output_size);
extern void unescapeCSV(char *str);
extern int  validateEmail(const char *email);
extern int  validatePhone(const char *phone);

// ===== local config (mirror main.c) =====
#define MAX_FIELD_LEN 100
#define MAX_LINE_LEN  512

// ===============================================
// Internal helpers (local use)
// ===============================================
static void normalizePhone(const char *in, char *out, size_t out_size) {
    if (!in || !out || out_size == 0) return;
    size_t j = 0;
    for (size_t i = 0; in[i] && j + 1 < out_size; i++) {
        if (isdigit((unsigned char)in[i])) out[j++] = in[i];
    }
    out[j] = '\0';
}

// ==== Minimal CSV writer used for tests ====
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

// ==== Public helpers (shared with E2E in main.c) ====
int countContactsTest(const char *filename) {
    FILE *fp = fopen(filename, "r"); if (!fp) return 0;
    char line[MAX_LINE_LEN]; int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n\r")] = '\0';
        if (*line) count++;
    }
    fclose(fp);
    return count;
}

// Extract first CSV field (Company) with quote support and compare CI
int contactExistsByCompanyCI(const char *filename, const char *company) {
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
        char *src = linecpy, *dst = comp; 
        int in_quotes = 0;

        while (*src && (in_quotes || *src != ',')) {
            if (*src == '"') {
                if (in_quotes && *(src + 1) == '"') {
                    size_t L = (size_t)(dst - comp);
                    if (L < MAX_FIELD_LEN - 1) *dst++ = '"';
                    src += 2;
                } else { in_quotes = !in_quotes; src++; }
            } else {
                size_t L = (size_t)(dst - comp);
                if (L < MAX_FIELD_LEN - 1) *dst++ = *src;
                src++;
            }
        }
        *dst = '\0';
        unescapeCSV(comp);

        char comp_lower[MAX_FIELD_LEN];
        strncpy(comp_lower, comp, MAX_FIELD_LEN-1); comp_lower[MAX_FIELD_LEN-1] = '\0';
        for (int i = 0; comp_lower[i]; i++) comp_lower[i] = (char)tolower((unsigned char)comp_lower[i]);

        if (strcmp(comp_lower, key_lower) == 0) { fclose(fp); return 1; }
    }
    fclose(fp);
    return 0;
}

int contactExistsByPhoneNorm(const char *filename, const char *phone_raw) {
    FILE *fp = fopen(filename, "r"); if (!fp) return 0;
    char key_norm[MAX_FIELD_LEN]; normalizePhone(phone_raw, key_norm, sizeof(key_norm));
    char line[MAX_LINE_LEN];

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n\r")] = '\0'; if (!*line) continue;

        char linecpy[MAX_LINE_LEN];
        strncpy(linecpy, line, sizeof(linecpy)-1); linecpy[sizeof(linecpy)-1] = '\0';

        char company[MAX_FIELD_LEN] = "", person[MAX_FIELD_LEN] = "", phone[MAX_FIELD_LEN] = "", email[MAX_FIELD_LEN] = "";
        char *fields[4] = {company, person, phone, email};
        int field_idx = 0, in_quotes = 0;
        char *src = linecpy, *dst = fields[0];

        while (*src && field_idx < 4) {
            if (*src == '"') { in_quotes = !in_quotes; src++; }
            else if (*src == ',' && !in_quotes) { *dst = '\0'; field_idx++; if (field_idx<4) dst = fields[field_idx]; src++; }
            else { *dst++ = *src++; }
        }
        *dst = '\0';

        unescapeCSV(phone);
        char pn[MAX_FIELD_LEN]; normalizePhone(phone, pn, sizeof(pn));
        if (*pn && strcmp(pn, key_norm) == 0) { fclose(fp); return 1; }
    }
    fclose(fp);
    return 0;
}

int contactExistsByEmailCI(const char *filename, const char *email_raw) {
    FILE *fp = fopen(filename, "r"); if (!fp) return 0;

    char key_lower[MAX_FIELD_LEN];
    strncpy(key_lower, email_raw, MAX_FIELD_LEN-1); key_lower[MAX_FIELD_LEN-1] = '\0';
    for (int i=0; key_lower[i]; i++) key_lower[i] = (char)tolower((unsigned char)key_lower[i]);

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n\r")] = '\0'; if (!*line) continue;

        char linecpy[MAX_LINE_LEN];
        strncpy(linecpy, line, sizeof(linecpy)-1); linecpy[sizeof(linecpy)-1] = '\0';

        char company[MAX_FIELD_LEN] = "", person[MAX_FIELD_LEN] = "", phone[MAX_FIELD_LEN] = "", email[MAX_FIELD_LEN] = "";
        char *fields[4] = {company, person, phone, email};
        int field_idx = 0, in_quotes = 0;
        char *src = linecpy, *dst = fields[0];

        while (*src && field_idx < 4) {
            if (*src == '"') { in_quotes = !in_quotes; src++; }
            else if (*src == ',' && !in_quotes) { *dst = '\0'; field_idx++; if (field_idx<4) dst = fields[field_idx]; src++; }
            else { *dst++ = *src++; }
        }
        *dst = '\0';

        unescapeCSV(email);
        char email_lower[MAX_FIELD_LEN];
        strncpy(email_lower, email, MAX_FIELD_LEN-1); email_lower[MAX_FIELD_LEN-1] = '\0';
        for (int i=0; email_lower[i]; i++) email_lower[i] = (char)tolower((unsigned char)email_lower[i]);
        if (*email_lower && strcmp(email_lower, key_lower) == 0) { fclose(fp); return 1; }
    }
    fclose(fp);
    return 0;
}

// ===============================================
// Cross-platform stdin redirection (for menu funcs)
// ===============================================
#ifdef _WIN32
  #include <io.h>
  #define DUP    _dup
  #define DUP2   _dup2
  #define FILENO _fileno
  #define CLOSE  _close
#else
  #include <unistd.h>
  #define DUP    dup
  #define DUP2   dup2
  #define FILENO fileno
  #define CLOSE  close
#endif

static int run_with_stdin_script(const char *script, void (*fn)(void)) {
    const char *tmp = "unit_stdin.txt";
    FILE *f = fopen(tmp, "w");
    if (!f) return 0;
    fputs(script, f);
    fclose(f);

    int saved_fd = DUP(FILENO(stdin));
    if (saved_fd < 0) { remove(tmp); return 0; }

    FILE *r = freopen(tmp, "r", stdin);
    if (!r) { DUP2(saved_fd, FILENO(stdin)); CLOSE(saved_fd); remove(tmp); return 0; }

    fn(); // call interactive function

    DUP2(saved_fd, FILENO(stdin));
    CLOSE(saved_fd);
    remove(tmp);
    return 1;
}

// ===============================================
// Test counter & macro
// ===============================================
static int test_passed = 0, test_failed = 0;
#define TEST_ASSERT(cond, name) do { \
    if (cond){ printf("  [PASS] %s\n", name); test_passed++; } \
    else { printf("  [FAIL] %s\n", name); test_failed++; } \
} while(0)

// ===============================================
// Main entry: runUnitTests()
// ===============================================
void runUnitTests(void) {
    test_passed = test_failed = 0;

    printf("\n========================================\n");
    printf("  UNIT TESTS - Contact Management\n");
    printf("========================================\n\n");

    // -----------------------------
    // Group A: Add (CSV basics)
    // -----------------------------
    printf("Group A: Add Functions\n");
    remove("test_contacts.csv");
    int ok = addContactTestHelper("Google", "John Doe", "081-234-5678", "john@google.com");
    TEST_ASSERT(ok && contactExistsByCompanyCI("test_contacts.csv", "Google"), "A1: add one basic row");

    remove("test_contacts.csv");
    addContactTestHelper("Google", "John", "081-111-1111", "john@google.com");
    addContactTestHelper("Microsoft", "Jane", "082-222-2222", "jane@microsoft.com");
    addContactTestHelper("Apple", "Bob", "083-333-3333", "bob@apple.com");
    TEST_ASSERT(countContactsTest("test_contacts.csv") == 3, "A2: add three rows total");

    remove("test_contacts.csv");
    ok = addContactTestHelper("Company, LLC", "John", "081-234-5678", "john@company.com");
    TEST_ASSERT(ok && contactExistsByCompanyCI("test_contacts.csv", "Company, LLC"), "A3: comma in company handled");

    remove("test_contacts.csv");
    ok = addContactTestHelper("Alpha \"Inc\"", "Alice", "090-000-0000", "alice@alpha.com");
    TEST_ASSERT(ok && contactExistsByCompanyCI("test_contacts.csv", "Alpha \"Inc\""), "A4: embedded quotes handled");

    remove("test_contacts.csv");
    addContactTestHelper("TelCo", "Tom", "(081) 234-5678", "Tom@Mail.COM");
    TEST_ASSERT(contactExistsByPhoneNorm("test_contacts.csv", "0812345678"), "A5: phone normalize match");
    TEST_ASSERT(contactExistsByEmailCI("test_contacts.csv", "tom@mail.com"), "A6: email CI match");

    TEST_ASSERT(validateEmail("test@example.com") == 1, "A7: validateEmail ok");
    TEST_ASSERT(validateEmail("invalid.email") == 0,      "A8: validateEmail fail");
    TEST_ASSERT(validatePhone("081-234-5678") == 1,       "A9: validatePhone ok");
    TEST_ASSERT(validatePhone("abc-def-ghij") == 0,       "A10: validatePhone fail");

    char input1[64] = "   =malicious \t"; (void)input1;

    // -----------------------------
    // Group B: Delete (inline demo)
    // -----------------------------
    printf("\nGroup B: Delete Functions\n");
    remove("test_contacts.csv");
    addContactTestHelper("Google", "John", "081-111-1111", "john@google.com");
    addContactTestHelper("Microsoft", "Jane", "082-222-2222", "jane@microsoft.com");
    // simulate delete by rewriting file (remove first Google)
    {
        char key_lower[MAX_FIELD_LEN];
        strncpy(key_lower, "GOOGLE", MAX_FIELD_LEN-1); key_lower[MAX_FIELD_LEN-1] = '\0';
        for (int i=0; key_lower[i]; ++i) key_lower[i] = (char)tolower((unsigned char)key_lower[i]);

        FILE *rf = fopen("test_contacts.csv", "r");
        FILE *wf = fopen("test_contacts.tmp", "w");
        int deleted = 0;
        if (rf && wf) {
            char line[MAX_LINE_LEN];
            while (fgets(line, sizeof(line), rf)) {
                line[strcspn(line, "\n\r")] = '\0'; if (!*line) continue;

                char linecpy[MAX_LINE_LEN];
                strncpy(linecpy, line, sizeof(linecpy)-1); linecpy[sizeof(linecpy)-1] = '\0';

                char comp[MAX_FIELD_LEN] = "";
                int inq = 0; char *src = linecpy, *dst = comp;
                while (*src && (inq || *src != ',')) {
                    if (*src == '"') {
                        if (inq && *(src+1) == '"') { size_t L=(size_t)(dst-comp); if (L<MAX_FIELD_LEN-1) *dst++ = '"'; src += 2; }
                        else { inq = !inq; src++; }
                    } else { size_t L=(size_t)(dst-comp); if (L<MAX_FIELD_LEN-1) *dst++ = *src; src++; }
                }
                *dst = '\0';
                unescapeCSV(comp);

                char comp_lower[MAX_FIELD_LEN];
                strncpy(comp_lower, comp, MAX_FIELD_LEN-1); comp_lower[MAX_FIELD_LEN-1] = '\0';
                for (int i=0;i<MAX_FIELD_LEN && comp_lower[i];++i) comp_lower[i]=(char)tolower((unsigned char)comp_lower[i]);

                if (!deleted && strcmp(comp_lower, key_lower)==0) { deleted = 1; continue; }
                fprintf(wf, "%s\n", line);
            }
        }
        if (rf) fclose(rf);
        if (wf) fclose(wf);
        remove("test_contacts.csv");
        rename("test_contacts.tmp", "test_contacts.csv");

        TEST_ASSERT(!contactExistsByCompanyCI("test_contacts.csv", "Google"), "B1: delete by company (CI)");
    }

    // -----------------------------
    // Group C: CSV & Quotes Robustness
    // -----------------------------
    printf("\nGroup C: CSV & Quotes Robustness\n");
    remove("test_contacts.csv");
    ok = addContactTestHelper("He said: \"\"Hello\"\"", "Q", "099-999-9999", "q@x.com");
    TEST_ASSERT(ok && contactExistsByCompanyCI("test_contacts.csv", "He said: \"\"Hello\"\""),
                "C1: double-quoted quotes preserved & matched");

    remove("test_contacts.csv");
    ok = addContactTestHelper("\"Quoted Co.\"", "W", "088-888-8888", "w@y.com");
    TEST_ASSERT(ok && contactExistsByCompanyCI("test_contacts.csv", "\"Quoted Co.\""),
                "C2: leading & trailing quotes handled");

    remove("test_contacts.csv");
    ok = addContactTestHelper("A, B, C, D, Co.", "E", "087-777-7777", "e@z.com");
    TEST_ASSERT(ok && contactExistsByCompanyCI("test_contacts.csv", "A, B, C, D, Co."),
                "C3: multiple commas in company");

    // -----------------------------
    // Group D: Email / Phone Validate edges
    // -----------------------------
    printf("\nGroup D: Email / Phone Validate\n");
    TEST_ASSERT(validateEmail("a@b.c") == 1, "D1: minimal valid email");
    TEST_ASSERT(validateEmail("User.Name+tag@sub.domain.co") == 1, "D2: complex email should pass");
    TEST_ASSERT(validateEmail("user@domain") == 0, "D3: missing TLD");
    TEST_ASSERT(validateEmail("@domain.com") == 0, "D4: missing local");
    TEST_ASSERT(validateEmail("user@.com") == 0, "D5: dot right after @");
    TEST_ASSERT(validateEmail("user@domain.") == 0, "D6: trailing dot");
    TEST_ASSERT(validateEmail("userdomain.com") == 0, "D7: missing @");

    TEST_ASSERT(validatePhone("+66 81.234-5678") == 1, "D8: phone with + and dots");
    TEST_ASSERT(validatePhone("(081) 234 5678") == 1,   "D9: phone with parentheses");
    TEST_ASSERT(validatePhone("abc-1234") == 0,         "D10: letters not allowed");
    TEST_ASSERT(validatePhone("++--()") == 0,           "D11: only symbols no digits");

    // -----------------------------
    // Group E: Normalization Equivalence
    // -----------------------------
    printf("\nGroup E: Normalization Equivalence\n");
    remove("test_contacts.csv");
    ok = addContactTestHelper("Norm Co", "N", "081-000-0000", "Case@Mail.COM");
    TEST_ASSERT(ok, "E0: setup row");
    TEST_ASSERT(contactExistsByEmailCI("test_contacts.csv", "case@mail.com"), "E1: email CI match");
    TEST_ASSERT(!contactExistsByEmailCI("test_contacts.csv", "case@mail.comm"), "E2: similar but not same");
    TEST_ASSERT(contactExistsByPhoneNorm("test_contacts.csv", "+66810000000"), "E3: phone normalized +66 == 0");
    TEST_ASSERT(!contactExistsByPhoneNorm("test_contacts.csv", "0810000001"),  "E4: different number no match");

    // -----------------------------
    // Group F: Field Length Boundaries
    // -----------------------------
    printf("\nGroup F: Field Length Boundaries\n");
    char longName[MAX_FIELD_LEN + 20];
    for (int i = 0; i < MAX_FIELD_LEN - 1; ++i) longName[i] = 'X';
    longName[MAX_FIELD_LEN - 1] = '\0';

    remove("test_contacts.csv");
    ok = addContactTestHelper(longName, "P", "086-666-6666", "p@p.com");
    TEST_ASSERT(ok, "F1: add row with company at MAX_FIELD_LEN-1");
    TEST_ASSERT(contactExistsByCompanyCI("test_contacts.csv", longName), "F2: match company at boundary length");

    char overName[MAX_FIELD_LEN + 20];
    for (int i = 0; i < MAX_FIELD_LEN + 10; ++i) overName[i] = 'Y';
    overName[MAX_FIELD_LEN + 10] = '\0';
    remove("test_contacts.csv");
    ok = addContactTestHelper(overName, "Q", "085-555-5555", "q@q.com");
    TEST_ASSERT(ok && countContactsTest("test_contacts.csv") == 1, "F3: oversize field written safely");

    // -----------------------------
    // Group G: File Absence & Empty File
    // -----------------------------
    printf("\nGroup G: File Absence & Empty File\n");
    remove("test_contacts.csv");
    TEST_ASSERT(countContactsTest("no_such_contacts.csv") == 0, "G1: missing file returns 0");
    { FILE *fp = fopen("test_contacts.csv", "w"); if (fp) fclose(fp); }
    TEST_ASSERT(countContactsTest("test_contacts.csv") == 0, "G2: empty file returns 0");

    // -----------------------------
    // Group H: Menu Functions (add/list/search/update/delete)
    // -----------------------------
    printf("\nGroup H: Menu Functions (add/list/search/update/delete)\n");

    // Use separate file for menu tests (not to touch user's real data)
    setContactsFile("test_unit.csv");
    { FILE *init = fopen(getContactsFile(), "w"); if (init) fclose(init); }

    // H1) ADD x2
    int ok_script = 1;
    ok_script &= run_with_stdin_script(
        "Company, LLC\n"
        "John Jr.\n"
        "(081) 234-5678\n"
        "john@company.com\n"
        "y\n",
        addContact
    );
    ok_script &= run_with_stdin_script(
        "Alpha \"Inc\"\n"
        "Alice \"A.\"\n"
        "090-000-0000\n"
        "Alice@Alpha.com\n"
        "y\n",
        addContact
    );
    TEST_ASSERT(ok_script == 1, "H1: addContact() twice via scripted stdin");
    TEST_ASSERT(countContactsTest(getContactsFile()) == 2, "H1.1: 2 rows added");
    TEST_ASSERT(contactExistsByCompanyCI(getContactsFile(), "Company, LLC"), "H1.2: exists Company, LLC");
    TEST_ASSERT(contactExistsByCompanyCI(getContactsFile(), "Alpha \"Inc\""), "H1.3: exists Alpha \"Inc\"");
    TEST_ASSERT(contactExistsByEmailCI(getContactsFile(), "alice@alpha.com"), "H1.4: email CI ok");

    // H2) LIST (all + filter) — assert non-crash (state checked by exists* above)
    ok_script  = run_with_stdin_script("\n", listContacts);      // show all
    ok_script &= run_with_stdin_script("alpha\n", listContacts); // filtered
    TEST_ASSERT(ok_script == 1, "H2: listContacts() runs (all + filtered)");

    // H3) SEARCH
    ok_script = run_with_stdin_script("alpha\n", searchContact);
    TEST_ASSERT(ok_script == 1, "H3: searchContact() runs with keyword 'alpha'");

    // H4) UPDATE phone of Alpha "Inc" -> +66 90 000 0000
    ok_script = run_with_stdin_script(
        "Alpha \"Inc\"\n"
        "3\n"
        "+66 90 000 0000\n"
        "y\n",
        updateContact
    );
    TEST_ASSERT(ok_script == 1, "H4: updateContact() phone for Alpha \"Inc\"");
    TEST_ASSERT(contactExistsByPhoneNorm(getContactsFile(), "+66900000000"), "H4.1: new phone exists (normalized)");
    TEST_ASSERT(!contactExistsByPhoneNorm(getContactsFile(), "0900000000"),   "H4.2: old phone gone");

    // H5) DELETE by phone (Company, LLC) then by company (Alpha "Inc")
    ok_script  = run_with_stdin_script("(081) 234-5678\n" "y\n", deleteContact);
    ok_script &= run_with_stdin_script("alpha \"inc\"\n"  "y\n", deleteContact);
    TEST_ASSERT(ok_script == 1, "H5: deleteContact() by phone then by company");
    TEST_ASSERT(countContactsTest(getContactsFile()) == 0, "H5.1: all rows deleted");

    // cleanup
    remove(getContactsFile());
    remove("test_contacts.csv");
    remove("test_contacts.tmp");

    // =========================================
    // Summary
    // =========================================
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
}
