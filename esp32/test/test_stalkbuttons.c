#include "unity.h"
#include <stdlib.h>
#include <stdbool.h>

#include "stalkButtons.c"

#define BUTTON_UP   (0x10u)
#define BUTTON_DOWN   (0x20u)
#define BUTTON_RESET   (0x40u)


void setup(void)
{

}

void tearDown(void)
{

}

void test_StalkButtons_Receive_UP(void)
{
    events = STALKBUTTONS_NOEVENT;
    lastButtons = 0;
    StalkButtons_Receive(BUTTON_UP);
    TEST_ASSERT_EQUAL(STALKBUTTONS_NOEVENT, events);
    TEST_ASSERT_EQUAL(BUTTON_UP, lastButtons);
    StalkButtons_Receive(0);
    TEST_ASSERT_EQUAL(STALKBUTTONS_UP, events);
    TEST_ASSERT_EQUAL(0, lastButtons);
}

void test_StalkButtons_Receive_DOWN(void)
{
    events = STALKBUTTONS_NOEVENT;
    lastButtons = 0;
    StalkButtons_Receive(BUTTON_DOWN);
    TEST_ASSERT_EQUAL(STALKBUTTONS_NOEVENT, events);
    TEST_ASSERT_EQUAL(BUTTON_DOWN, lastButtons);
    StalkButtons_Receive(0);
    TEST_ASSERT_EQUAL(STALKBUTTONS_DOWN, events);
    TEST_ASSERT_EQUAL(0, lastButtons);
}

void test_StalkButtons_Receive_RESET(void)
{
    events = STALKBUTTONS_NOEVENT;
    lastButtons = 0;
    StalkButtons_Receive(BUTTON_RESET);
    TEST_ASSERT_EQUAL(STALKBUTTONS_NOEVENT, events);
    TEST_ASSERT_EQUAL(BUTTON_RESET, lastButtons);
    StalkButtons_Receive(0);
    TEST_ASSERT_EQUAL(STALKBUTTONS_RESET, events);
    TEST_ASSERT_EQUAL(0, lastButtons);
}

void test_StalkButtons_Receive_RESET_UP(void)
{
    events = STALKBUTTONS_NOEVENT;
    lastButtons = 0;
    StalkButtons_Receive(BUTTON_RESET | BUTTON_UP);
    TEST_ASSERT_EQUAL(STALKBUTTONS_NOEVENT, events);
    TEST_ASSERT_EQUAL(BUTTON_RESET | BUTTON_UP, lastButtons);
    StalkButtons_Receive(0);
    TEST_ASSERT_EQUAL(STALKBUTTONS_UP | STALKBUTTONS_RESET, events);
    TEST_ASSERT_EQUAL(0, lastButtons);
}

void test_StalkButtons_Receive_RESET_DOWN(void)
{
    events = STALKBUTTONS_NOEVENT;
    lastButtons = 0;
    StalkButtons_Receive(BUTTON_RESET | BUTTON_DOWN);
    TEST_ASSERT_EQUAL(STALKBUTTONS_NOEVENT, events);
    TEST_ASSERT_EQUAL(BUTTON_RESET | BUTTON_DOWN, lastButtons);
    StalkButtons_Receive(0);
    TEST_ASSERT_EQUAL(STALKBUTTONS_DOWN | STALKBUTTONS_RESET, events);
    TEST_ASSERT_EQUAL(0, lastButtons);
}

