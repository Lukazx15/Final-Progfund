
// Unit_test.c - centralizes all unit tests; callable from main menu via runUnitTests()
// NOTE: This file has NO main(). Compile together with main.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
  #include <io.h>
  #define DUP    _dup
  #define DUP2   _dup2
  #define FILENO _fileno
#else
  #include <unistd.h>
  #define DUP    dup
  #define DUP2   dup2
  #define FILENO fileno
#endif

#define MAX_FIELD_LEN 100
#define MAX_LINE_LEN  512

// ===== extern functions from main.c =====
extern void listContacts(void);
extern void searchContact(void);
extern void updateContact(void);
extern int  validateEmail(const char*);
extern int  validatePhone(const char*);
extern void escapeCSV(const char*, char*, size_t);
extern void unescapeCSV(char*);
extern void sanitizeInput(char*);
extern void trimWhitespace(char*);
extern const char* getContactsFile(void);
extern void setContactsFile(const char*);

// ===== local counters / asserts =====
static int tests_passed = 0, tests_failed = 0;
#define TEST_ASSERT(cond, name) \
  do { if (cond) { printf("[PASS] %s\n", name); tests_passed++; } \
       else       { printf("[FAIL] %s\n", name); tests_failed++; } } while(0)

// ===== small helpers =====
static void normalizePhone(const char *in, char *out, size_t out_size) {
    if (!in || !out || out_size == 0) return;
    size_t j = 0;
    for (size_t i = 0; in[i] && j + 1 < out_size; i++) {
        if (isdigit((unsigned char)in[i])) out[j++] = in[i];
    }
    out[j] = '\0';
}

static int addRow(const char *filename, const char *company, const char *person, const char *phone, const char *email) {
    FILE *fp = fopen(filename, "a");
    if (!fp) return 0;
    char a[MAX_FIELD_LEN*2], b[MAX_FIELD_LEN*2], c[MAX_FIELD_LEN*2], d[MAX_FIELD_LEN*2];
    escapeCSV(company,a,sizeof(a));
    escapeCSV(person ,b,sizeof(b));
    escapeCSV(phone  ,c,sizeof(c));
    escapeCSV(email  ,d,sizeof(d));
    fprintf(fp, "%s,%s,%s,%s\n", a,b,c,d);
    fclose(fp);
    return 1;
}

static int countRows(const char *filename) {
    FILE *fp = fopen(filename, "r"); if (!fp) return 0;
    char line[MAX_LINE_LEN]; int cnt = 0;
    while (fgets(line,sizeof(line),fp)) { line[strcspn(line, "\r\n")] = '\0'; if (*line) cnt++; }
    fclose(fp); return cnt;
}

static int existsCompanyCI(const char *filename, const char *company) {
    FILE *fp = fopen(filename, "r"); if (!fp) return 0;
    char key_lower[MAX_FIELD_LEN];
    strncpy(key_lower, company, MAX_FIELD_LEN-1); key_lower[MAX_FIELD_LEN-1] = '\0';
    for (int i=0; key_lower[i]; i++) key_lower[i] = (char)tolower((unsigned char)key_lower[i]);

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0'; if (!*line) continue;
        // parse first field (company) with quote handling
        char comp[MAX_FIELD_LEN] = "";
        const char *src = line; char *dst = comp; int inq = 0;
        while (*src && (inq || *src != ',')) {
            if (*src=='"'){ if(inq && *(src+1)=='"'){ if((size_t)(dst-comp)<MAX_FIELD_LEN-1)*dst++='"'; src+=2; }
                            else { inq=!inq; src++; } }
            else { if ((size_t)(dst-comp) < MAX_FIELD_LEN-1) *dst++ = *src++; }
        }
        *dst = '\0'; unescapeCSV(comp);

        char low[MAX_FIELD_LEN]; strncpy(low,comp,MAX_FIELD_LEN-1); low[MAX_FIELD_LEN-1]='\0';
        for (int i=0; low[i]; i++) low[i] = (char)tolower((unsigned char)low[i]);
        if (strcmp(low, key_lower) == 0) { fclose(fp); return 1; }
    }
    fclose(fp); return 0;
}

static int existsPhoneNorm(const char *filename, const char *phone_raw) {
    FILE *fp = fopen(filename, "r"); if (!fp) return 0;
    char key[MAX_FIELD_LEN]; normalizePhone(phone_raw, key, sizeof(key));
    char line[MAX_LINE_LEN];
    while (fgets(line,sizeof(line),fp)) {
        line[strcspn(line,"\r\n")] = '\0'; if (!*line) continue;
        // extract 3rd field (phone)
        char fields[4][MAX_FIELD_LEN]={{0}}; int idx=0, inq=0;
        const char *src=line; char *dst = fields[0];
        while (*src && idx<4){
            if (*src=='"'){ inq=!inq; src++; }
            else if (*src==',' && !inq){ *dst='\0'; idx++; if(idx<4) dst = fields[idx]; src++; }
            else { if((size_t)(dst-fields[idx])<MAX_FIELD_LEN-1) *dst++ = *src++; }
        }
        *dst = '\0'; unescapeCSV(fields[2]);
        char pn[MAX_FIELD_LEN]; normalizePhone(fields[2], pn, sizeof(pn));
        if (*pn && strcmp(pn, key)==0){ fclose(fp); return 1; }
    }
    fclose(fp); return 0;
}

// ===== IO redirection =====
static int capture_stdout_to_file(const char *outfile, void (*fn)(void), const char *stdin_script) {
    int ok = 1;
    int saved_in = -1;
    const char *tmp_in = "ut_in.txt";
    if (stdin_script) {
        FILE *w = fopen(tmp_in,"w"); if(!w) return 0;
        fputs(stdin_script, w); fclose(w);
        saved_in = DUP(FILENO(stdin)); if(saved_in<0){ remove(tmp_in); return 0; }
        if(!freopen(tmp_in,"r",stdin)){ DUP2(saved_in,FILENO(stdin)); remove(tmp_in); return 0; }
    }

    int saved_out = DUP(FILENO(stdout));
    if (saved_out < 0) ok = 0;
    FILE *wf = freopen(outfile, "w", stdout);
    if (!wf) { if(saved_out>=0) DUP2(saved_out, FILENO(stdout)); ok = 0; }

    if (ok) fn();

    if (saved_out >= 0) DUP2(saved_out, FILENO(stdout));
#ifdef _WIN32
    if (saved_out >= 0) _close(saved_out);
#else
    if (saved_out >= 0) close(saved_out);
#endif

    if (stdin_script) {
        DUP2(saved_in, FILENO(stdin));
#ifdef _WIN32
        _close(saved_in);
#else
        close(saved_in);
#endif
        remove(tmp_in);
    }
    return ok;
}

static int file_contains(const char *path, const char *needle) {
    FILE *f = fopen(path, "r"); if (!f) return 0;
    int found=0; char buf[4096];
    while (fgets(buf,sizeof(buf),f)) {
        if (strstr(buf, needle)) { found=1; break; }
    }
    fclose(f); return found;
}

// ===== Test Cases =====
static void test_add_basics() {
    remove("test_contacts.csv");
    TEST_ASSERT(addRow("test_contacts.csv","Company, LLC","John Jr.","(081) 234-5678","john@company.com"), "ADD1 write row with comma");
    TEST_ASSERT(existsCompanyCI("test_contacts.csv","Company, LLC"), "ADD1.1 exists by company");
    TEST_ASSERT(validateEmail("ok@x.com")==1,  "ADD2 email valid");
    TEST_ASSERT(validateEmail("bad")==0,       "ADD3 email invalid");
    TEST_ASSERT(validatePhone("081-234-5678")==1, "ADD4 phone valid");
}

static void test_list_all_and_filter() {
    setContactsFile("test_contacts.csv");
    remove("test_contacts.csv");
    addRow("test_contacts.csv","Alpha Co","A","081","a@x");
    addRow("test_contacts.csv","Beta Co","B","082","b@x");

    // Show all (Enter)
    capture_stdout_to_file("out_list_all.txt", listContacts, "\n");
    TEST_ASSERT(file_contains("out_list_all.txt","Alpha Co"), "LIST1 shows Alpha");
    TEST_ASSERT(file_contains("out_list_all.txt","Beta Co"),  "LIST1 shows Beta");

    // Filter 'alp'
    capture_stdout_to_file("out_list_filter.txt", listContacts, "alp\n");
    TEST_ASSERT(file_contains("out_list_filter.txt","Alpha Co"), "LIST2 filter keeps Alpha");
    TEST_ASSERT(!file_contains("out_list_filter.txt","Beta Co"), "LIST2 filter hides Beta");
}

static void test_search_company_and_phone() {
    setContactsFile("test_contacts.csv");
    remove("test_contacts.csv");
    addRow("test_contacts.csv","Zebra Ltd","Zed","0812345678","z@x");
    addRow("test_contacts.csv","Zoom Inc","Zoo","0900000000","zz@x");

    capture_stdout_to_file("out_search_comp.txt", searchContact, "ze\n");
    TEST_ASSERT(file_contains("out_search_comp.txt","Zebra Ltd"), "SEARCH1 company prefix");

    capture_stdout_to_file("out_search_phone.txt", searchContact, "812345\n");
    TEST_ASSERT(file_contains("out_search_phone.txt","Zebra Ltd"), "SEARCH2 phone substring");
}

static void test_update_phone_and_email() {
    setContactsFile("test_contacts.csv");
    remove("test_contacts.csv");
    addRow("test_contacts.csv","Alpha \"Inc\"","Alice","090-000-0000","alice@alpha.com");

    // 3 = Phone, 4 = Email (based on typical menu in updateContact)
    // Update phone
    const char *script1 =
        "Alpha \"Inc\"\n"  // company
        "3\n"              // choose phone
        "+66 90 000 0000\n"
        "y\n";
    capture_stdout_to_file("out_update_phone.txt", updateContact, script1);
    TEST_ASSERT(existsPhoneNorm("test_contacts.csv","+66900000000"), "UPDATE1 phone changed");

    // Update email
    const char *script2 =
        "Alpha \"Inc\"\n"
        "4\n"
        "alice@alpha.com\n"
        "y\n";
    capture_stdout_to_file("out_update_email.txt", updateContact, script2);
    TEST_ASSERT(existsCompanyCI("test_contacts.csv","Alpha \"Inc\""), "UPDATE2 row still exists");
}

// ===== public entry =====
void runUnitTests(void) {
    const char* original = getContactsFile();
    setContactsFile("test_contacts.csv");

    tests_passed = tests_failed = 0;
    printf("\n========================================\n");
    printf("  UNIT TESTS - Contact Management\n");
    printf("========================================\n\n");

    printf("Group: Add\n");
    test_add_basics();

    printf("\nGroup: List\n");
    test_list_all_and_filter();

    printf("\nGroup: Search\n");
    test_search_company_and_phone();

    printf("\nGroup: Update\n");
    test_update_phone_and_email();

    printf("\n========================================\n");
    printf("  TEST SUMMARY\n");
    printf("========================================\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);
    if (tests_passed + tests_failed > 0) {
        printf("Success Rate: %.1f%%\n", (tests_passed * 100.0) / (tests_passed + tests_failed));
    }
    printf("========================================\n");

    // cleanup temp files and restore
    remove("out_list_all.txt");
    remove("out_list_filter.txt");
    remove("out_search_comp.txt");
    remove("out_search_phone.txt");
    remove("out_update_phone.txt");
    remove("out_update_email.txt");
    remove("test_contacts.csv");
    remove("ut_in.txt");
    setContactsFile(original);
}
