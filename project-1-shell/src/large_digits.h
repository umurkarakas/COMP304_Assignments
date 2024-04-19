// large_digits.h

#ifndef LARGE_DIGITS_H
#define LARGE_DIGITS_H

#define NUM_DIGITS 10
#define DIGIT_HEIGHT 5
#define DIGIT_WIDTH 4

char largeDigits[NUM_DIGITS][DIGIT_HEIGHT][DIGIT_WIDTH +1 ] = {
    {
        " ** ",
        "*  *",
        "*  *",
        "*  *",
        " ** "
    }, // 0
    {
        "  * ",
        "  * ",
        "  * ",
        "  * ",
        "  * "
    }, // 1
    {
        " ** ",
        "   *",
        " ** ",
        "*   ",
        " ** "
    }, // 2
    {
        " ** ",
        "   *",
        " ** ",
        "   *",
        " ** "
    }, // 3
    {
        " *  ",
        "* * ",
        "*** ",
        "  * ",
        "  * "
    }, // 4
    {
        " ** ",
        "*   ",
        " ** ",
        "   *",
        " ** "
    }, // 5
    {
        " ** ",
        "*   ",
        " ***",
        "*  *",
        " ** "
    }, // 6
    {
        " ***",
        "   *",
        "   *",
        "  * ",
        " *  "
    }, // 7
    {
        " ** ",
        "*  *",
        " ** ",
        "*  *",
        " ** "
    }, // 8
    {
        " ** ",
        "*  *",
        " ***",
        "   *",
        " ** "
    }  // 9
};
#endif
