#ifndef TESTS_H
#define TESTS_H

// entry ของ unit test
void runUnitTests(void);

// ฟังก์ชันที่ E2E ใน main.c เรียกใช้
int countContactsTest(const char *filename);
int contactExistsByCompanyCI(const char *filename, const char *company);
int contactExistsByEmailCI  (const char *filename, const char *email_raw);
int contactExistsByPhoneNorm(const char *filename, const char *phone_raw);

#endif