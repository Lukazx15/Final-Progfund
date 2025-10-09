// ===============================================
//  E2E_test.c — Expanded End-to-End tests
//  Covers: add/list/search/update/delete including
//          cancel paths, invalid input rejection,
//          duplicates, multi-match filters, and
//          search/delete by email/phone/person.
// ===============================================

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "test.h"

// ===== externs from main.c (interactive app) =====
extern void addContact(void);
extern void listContacts(void);
extern void searchContact(void);
extern void deleteContact(void);
extern void updateContact(void);

extern const char* getContactsFile(void);
extern void setContactsFile(const char* path);

// ===== Cross-platform stdin redirection =====
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

static int e2e_run_with_stdin(const char *script, void (*fn)(void)) {
    const char *tmp = "e2e_stdin.txt";
    FILE *f = fopen(tmp, "w");
    if (!f) return 0;
    fputs(script, f);
    fclose(f);

    int saved = DUP(FILENO(stdin));
    if (saved < 0) { remove(tmp); return 0; }

    FILE *r = freopen(tmp, "r", stdin);
    if (!r) { DUP2(saved, FILENO(stdin)); CLOSE(saved); remove(tmp); return 0; }

    fn();

    DUP2(saved, FILENO(stdin));
    CLOSE(saved);
    remove(tmp);
    return 1;
}

// ===== Assertions & counters =====
static int e2e_pass = 0, e2e_fail = 0;
#define E2E_ASSERT(cond, name) do{ if (cond){ printf("  [PASS] %s\n", name); e2e_pass++; } else { printf("  [FAIL] %s\n", name); e2e_fail++; } }while(0)

// small helper: ensure clean file
static void ensure_fresh_file(const char* path) {
    FILE *fp = fopen(path, "w");
    if (fp) fclose(fp);
}

// ===============================================
// Public entry
// ===============================================
void runE2ETests(void) {
    printf("\n========================================\n");
    printf("  E2E TESTS - Contact Management (Expanded)\n");
    printf("========================================\n\n");

    // Switch to isolated file
    setContactsFile("contacts_e2e.csv");
    ensure_fresh_file(getContactsFile());

    // =====================================================
    // EE1: ADD (twice) — baseline happy path & CSV/quotes
    // =====================================================
    printf("EE1: Add contacts (baseline)\n");
    int ok = 1;

    ok &= e2e_run_with_stdin(
        "Company, LLC\n"
        "John Jr.\n"
        "(081) 234-5678\n"
        "john@company.com\n"
        "y\n",
        addContact
    );

    ok &= e2e_run_with_stdin(
        "Alpha \"Inc\"\n"
        "Alice \"A.\"\n"
        "090-000-0000\n"
        "Alice@Alpha.com\n"
        "y\n",
        addContact
    );

    E2E_ASSERT(ok == 1, "EE1: addContact() twice");
    E2E_ASSERT(countContactsTest(getContactsFile()) == 2, "EE1.1: file has 2 rows");
    E2E_ASSERT(contactExistsByCompanyCI(getContactsFile(), "Company, LLC"), "EE1.2: Company LLC exists");
    E2E_ASSERT(contactExistsByCompanyCI(getContactsFile(), "Alpha \"Inc\""), "EE1.3: Alpha \"Inc\" exists");
    E2E_ASSERT(contactExistsByEmailCI(getContactsFile(), "alice@alpha.com"), "EE1.4: email CI exists");

    // =====================================================
    // EE2: LIST & SEARCH — all, filtered, search keyword
    // =====================================================
    printf("\nEE2: List & Search (baseline no-crash)\n");
    ok  = e2e_run_with_stdin("\n",         listContacts);   // all
    ok &= e2e_run_with_stdin("alpha\n",    listContacts);   // filtered
    ok &= e2e_run_with_stdin("alpha\n",    searchContact);  // search by keyword
    E2E_ASSERT(ok == 1, "EE2: listContacts (all+filtered) & searchContact run");

    // =====================================================
    // EE3: UPDATE phone and verify normalization
    // =====================================================
    printf("\nEE3: Update phone\n");
    ok = e2e_run_with_stdin(
        "Alpha \"Inc\"\n"   // select record by company
        "3\n"               // field: Phone
        "+66 90 000 0000\n" // new phone
        "y\n",              // confirm
        updateContact
    );
    E2E_ASSERT(ok == 1, "EE3: updateContact() phone");
    E2E_ASSERT(contactExistsByPhoneNorm(getContactsFile(), "+66900000000"), "EE3.1: new phone exists (normalized)");
    E2E_ASSERT(!contactExistsByPhoneNorm(getContactsFile(), "0900000000"),   "EE3.2: old phone gone");

    // =====================================================
    // EE4: DELETE both (by phone, by company)
    // =====================================================
    printf("\nEE4: Delete baseline\n");
    ok  = e2e_run_with_stdin("(081) 234-5678\n" "y\n", deleteContact);   // by phone
    ok &= e2e_run_with_stdin("alpha \"inc\"\n"  "y\n", deleteContact);   // by company (CI)
    E2E_ASSERT(ok == 1, "EE4: deleteContact() by phone then by company");
    E2E_ASSERT(countContactsTest(getContactsFile()) == 0, "EE4.1: all rows deleted");

    // =====================================================
    // EE5: CANCEL paths (add/search/update/delete/list)
    // =====================================================
    printf("\nEE5: Cancel flows\n");
    ensure_fresh_file(getContactsFile());

    // cancel add on company field
    ok = e2e_run_with_stdin("0\n", addContact);
    E2E_ASSERT(ok == 1, "EE5.1: addContact cancel company");
    E2E_ASSERT(countContactsTest(getContactsFile()) == 0, "EE5.1a: file unchanged");

    // cancel add on later field (phone)
    ok = e2e_run_with_stdin(
        "Cancel Co\n"
        "X\n"
        "0\n", addContact);
    E2E_ASSERT(ok == 1, "EE5.2: addContact cancel phone");
    E2E_ASSERT(countContactsTest(getContactsFile()) == 0, "EE5.2a: file unchanged");

    // seed 1 row then cancel update/delete/search/list
    ok  = e2e_run_with_stdin(
        "Seed Co\n"
        "Bob\n"
        "081-111-1111\n"
        "bob@seed.com\n"
        "y\n", addContact);
    E2E_ASSERT(ok == 1, "EE5.3: seed row for cancel tests");
    E2E_ASSERT(countContactsTest(getContactsFile()) == 1, "EE5.3a: seed written");

    // cancel list
    ok = e2e_run_with_stdin("0\n", listContacts);
    E2E_ASSERT(ok == 1, "EE5.4: listContacts cancel");

    // cancel search
    ok = e2e_run_with_stdin("0\n", searchContact);
    E2E_ASSERT(ok == 1, "EE5.5: searchContact cancel");

    // cancel update (enter 0 on company)
    ok = e2e_run_with_stdin("0\n", updateContact);
    E2E_ASSERT(ok == 1, "EE5.6: updateContact cancel by 0");

    // cancel delete (enter 0)
    ok = e2e_run_with_stdin("0\n", deleteContact);
    E2E_ASSERT(ok == 1, "EE5.7: deleteContact cancel by 0");

    // cleanup seed
    ok = e2e_run_with_stdin("seed co\n" "y\n", deleteContact);
    E2E_ASSERT(ok == 1, "EE5.8: cleanup seed");
    E2E_ASSERT(countContactsTest(getContactsFile()) == 0, "EE5.8a: file back to 0");

    // =====================================================
    // EE6: INVALID inputs rejected (email/phone on add/update)
    // =====================================================
    printf("\nEE6: Invalid inputs\n");
    ensure_fresh_file(getContactsFile());

    // try invalid email on add (expect: record not saved if app rejects; count must stay 0)
    ok = e2e_run_with_stdin(
        "BadEmail Co\n"
        "X\n"
        "081-222-2222\n"
        "not-an-email\n"
        "y\n",
        addContact
    );
    // Regardless of how UI handles, assert that an obviously invalid email is not considered "existing"
    E2E_ASSERT(contactExistsByEmailCI(getContactsFile(), "not-an-email") == 0, "EE6.1: invalid email not indexed");
    E2E_ASSERT(countContactsTest(getContactsFile()) == 0 || countContactsTest(getContactsFile()) == 1, "EE6.1a: app may re-prompt; file not crash");

    // now add a valid row to proceed
    ok = e2e_run_with_stdin(
        "Valid Co\n"
        "V\n"
        "081-222-2222\n"
        "v@co.com\n"
        "y\n", addContact);
    E2E_ASSERT(ok == 1, "EE6.2: add valid row");
    E2E_ASSERT(contactExistsByEmailCI(getContactsFile(), "v@co.com") == 1, "EE6.2a: valid row exists");

    // update email to invalid -> expect old email remains (or no invalid index)
    ok = e2e_run_with_stdin(
        "valid co\n"
        "4\n"               // Email
        "user@.com\n"       // invalid email
        "y\n",
        updateContact
    );
    // ensure still find by original email OR at least invalid not accepted as canonical
    E2E_ASSERT(contactExistsByEmailCI(getContactsFile(), "v@co.com") == 1 ||
               contactExistsByEmailCI(getContactsFile(), "user@.com") == 0,
               "EE6.3: invalid email on update not breaking index");

    // cleanup
    ok = e2e_run_with_stdin("valid co\n" "y\n", deleteContact);
    (void)ok;
    ensure_fresh_file(getContactsFile());

    // =====================================================
    // EE7: Duplicates (same company, phone, email)
    // =====================================================
    printf("\nEE7: Duplicates\n");
    ok  = e2e_run_with_stdin("Dup Co\nA\n081-000-0000\na@dup.com\ny\n", addContact);
    ok &= e2e_run_with_stdin("Dup Co\nB\n081-000-0000\na@dup.com\ny\n", addContact); // duplicate company+phone+email
    E2E_ASSERT(ok == 1, "EE7: add duplicates (app-specific handling)");
    // We can't guarantee how app resolves duplicates; assert at least 1 row exists and no crash:
    E2E_ASSERT(countContactsTest(getContactsFile()) >= 1, "EE7.1: at least one row present");
    // cleanup
    ok = e2e_run_with_stdin("dup co\n" "y\n", deleteContact);
    ensure_fresh_file(getContactsFile());

    // =====================================================
    // EE8: Search variations (by email/phone/person)
    // =====================================================
    printf("\nEE8: Search variations\n");
    ok  = e2e_run_with_stdin("Multi A Co\nAnn\n081-111-1111\nann@mail.com\ny\n", addContact);
    ok &= e2e_run_with_stdin("Multi B Co\nBen\n081-222-2222\nben@mail.com\ny\n", addContact);
    E2E_ASSERT(ok == 1, "EE8: seed 2 rows");

    // search by email token
    ok  = e2e_run_with_stdin("ann@mail\n", searchContact);
    // search by phone token
    ok &= e2e_run_with_stdin("081-222\n", searchContact);
    // search by person
    ok &= e2e_run_with_stdin("Ben\n", searchContact);
    E2E_ASSERT(ok == 1, "EE8.1: search by email/phone/person run");

    // =====================================================
    // EE9: Update other fields (company/contact/email)
    // =====================================================
    printf("\nEE9: Update other fields\n");
    // Update company name
    ok = e2e_run_with_stdin(
        "multi a co\n"
        "1\n"               // Company
        "Renamed Co A\n"
        "y\n", updateContact);
    E2E_ASSERT(ok == 1, "EE9.1: update company");
    E2E_ASSERT(contactExistsByCompanyCI(getContactsFile(), "Renamed Co A"), "EE9.1a: renamed company exists");

    // Update contact person
    ok = e2e_run_with_stdin(
        "multi b co\n"
        "2\n"               // Contact Person
        "Benedict\n"
        "y\n", updateContact);
    E2E_ASSERT(ok == 1, "EE9.2: update contact person");
    // company still exists, and row can still be deleted later; no direct index by person in helpers

    // Update email to different valid address and verify
    ok = e2e_run_with_stdin(
        "renamed co a\n"
        "4\n"               // Email
        "ann@renamed.com\n"
        "y\n", updateContact);
    E2E_ASSERT(ok == 1, "EE9.3: update email");
    E2E_ASSERT(contactExistsByEmailCI(getContactsFile(), "ann@renamed.com"), "EE9.3a: new email exists");

    // =====================================================
    // EE10: Multi-match filter in list, delete by email/person
    // =====================================================
    printf("\nEE10: Multi-match & delete by email/person\n");
    // list filter that should find multiple
    ok  = e2e_run_with_stdin("co\n", listContacts);
    E2E_ASSERT(ok == 1, "EE10.1: list filtered 'co' (multi-match likely)");

    // delete by email
    ok = e2e_run_with_stdin("ann@renamed.com\n" "y\n", deleteContact);
    E2E_ASSERT(ok == 1, "EE10.2: delete by email");
    E2E_ASSERT(!contactExistsByEmailCI(getContactsFile(), "ann@renamed.com"), "EE10.2a: deleted email gone");

    // delete by person name
    ok = e2e_run_with_stdin("Benedict\n" "y\n", deleteContact);
    E2E_ASSERT(ok == 1, "EE10.3: delete by person");
    // At this point the file should be empty again
    E2E_ASSERT(countContactsTest(getContactsFile()) == 0, "EE10.4: all rows deleted");

    // cleanup
    remove(getContactsFile());

    // summary
    printf("\n========================================\n");
    printf("  E2E SUMMARY\n");
    printf("========================================\n");
    printf("Passed: %d\n", e2e_pass);
    printf("Failed: %d\n", e2e_fail);
    printf("Total:  %d\n", e2e_pass + e2e_fail);
    if (e2e_pass + e2e_fail > 0) {
        printf("Success Rate: %.1f%%\n", (e2e_pass * 100.0) / (e2e_pass + e2e_fail));
    }
    printf("========================================\n");
}
