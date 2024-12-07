#include <Arduino.h>
#include <unity.h>
#include <WiFiClientSecure.h>

String STR_TO_TEST;
WiFiClientSecure client;

void setUp(void) {
    // set stuff up here
    STR_TO_TEST = "Hello, world!";
    WiFi.begin("SSID", "PASSWORD");
}

void tearDown(void) {
    // clean stuff up here
    STR_TO_TEST = "";
}

void test_wifi_instance(void) {
    TEST_ASSERT_NOT_NULL(&client);
}

void test_wifi_connecting(void) {
    TEST_ASSERT_EQUAL(WL_DISCONNECTED, WiFi.status());
}

void test_wifi_connected(void) {
    TEST_ASSERT_EQUAL(WL_CONNECTED, WiFi.status());
}

void test_string_concat(void) {
    String hello = "Hello, ";
    String world = "world!";
    TEST_ASSERT_EQUAL_STRING(STR_TO_TEST.c_str(), (hello + world).c_str());
}

void test_string_substring(void) {
    TEST_ASSERT_EQUAL_STRING("Hello", STR_TO_TEST.substring(0, 5).c_str());
}

void test_string_index_of(void) {
    TEST_ASSERT_EQUAL(7, STR_TO_TEST.indexOf('w'));
}

void test_string_equal_ignore_case(void) {
    TEST_ASSERT_TRUE(STR_TO_TEST.equalsIgnoreCase("HELLO, WORLD!"));
}

void test_string_to_upper_case(void) {
    STR_TO_TEST.toUpperCase();
    TEST_ASSERT_EQUAL_STRING("HELLO, WORLD!", STR_TO_TEST.c_str());
}

void test_string_replace(void) {
    STR_TO_TEST.replace('!', '?');
    TEST_ASSERT_EQUAL_STRING("Hello, world?", STR_TO_TEST.c_str());
}

void setup()
{
    delay(2000); // service delay
    UNITY_BEGIN();

    RUN_TEST(test_string_concat);
    RUN_TEST(test_string_substring);
    RUN_TEST(test_string_index_of);
    RUN_TEST(test_string_equal_ignore_case);
    RUN_TEST(test_string_to_upper_case);
    RUN_TEST(test_string_replace);

    RUN_TEST(test_wifi_instance);
    RUN_TEST(test_wifi_connecting);
    delay(5000);
    RUN_TEST(test_wifi_connected);

    UNITY_END(); // stop unit testing
}

void loop()
{
}