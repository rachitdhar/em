/* *************************************************

Tests

The list of categories that need to be tested are:

- Basic tokenization
- Function definitions
- Declarations
- Literals
- Function calls
- if-else
- for
- while
- break
- continue
- return
- Assignment
- Binary operations
- Unary operations

Basic rules to follow

I'll try to write at least four kinds of tests
for each category above. Broadly, they can cover
the following ideas:

    1. Basic syntax                   (Positive test)
    2. General uses                   (Positive test)
    3. Complex scenarios / Edge cases (Positive test)
    4. Scenarios that should fail     (Negative test)

************************************************* */

#include <fstream>
#include <stdio.h>
#include <string>


// list of [ <test_name>, <test_output_file_name> ]
const std::string tests[][2] = {
    {"tokenization_1",  ""},
    {"tokenization_2",  ""},
    {"tokenization_3",  ""},
    {"tokenization_4",  ""},
    {"funcdef_1",       ""},
    {"funcdef_2",       ""},
    {"funcdef_3",       ""},
    {"funcdef_4",       ""},
    {"declarations_1",  ""},
    {"declarations_2",  ""},
    {"declarations_3",  ""},
    {"declarations_4",  ""},
    {"literals_1",      ""},
    {"literals_2",      ""},
    {"literals_3",      ""},
    {"literals_4",      ""},
    {"calls_1",         ""},
    {"calls_2",         ""},
    {"calls_3",         ""},
    {"calls_4",         ""},
    {"ifelse_1",        ""},
    {"ifelse_2",        ""},
    {"ifelse_3",        ""},
    {"ifelse_4",        ""},
    {"forloop_1",       ""},
    {"forloop_2",       ""},
    {"forloop_3",       ""},
    {"forloop_4",       ""},
    {"whileloop_1",     ""},
    {"whileloop_2",     ""},
    {"whileloop_3",     ""},
    {"whileloop_4",     ""},
    {"break_1",         ""},
    {"break_2",         ""},
    {"break_3",         ""},
    {"break_4",         ""},
    {"continue_1",      ""},
    {"continue_2",      ""},
    {"continue_3",      ""},
    {"continue_4",      ""},
    {"return_1",        ""},
    {"return_2",        ""},
    {"return_3",        ""},
    {"return_4",        ""},
    {"assignment_1",    ""},
    {"assignment_2",    ""},
    {"assignment_3",    ""},
    {"assignment_4",    ""},
    {"binaryops_1",     ""},
    {"binaryops_2",     ""},
    {"binaryops_3",     ""},
    {"binaryops_4",     ""},
    {"unaryops_1",      ""},
    {"unaryops_2",      ""},
    {"unaryops_3",      ""},
    {"unaryops_4",      ""}
};


// create blank test files (just one time use)
void create_blank_tests()
{
    for (const auto& test : tests) {
        std::string filename = test[0] + ".em";

        std::ofstream outfile(filename);
        if (!outfile) {
	    break;
        }
        outfile.close();
    }
}


// to run the tests
int main()
{
    //create_blank_tests();
    return 0;
}
