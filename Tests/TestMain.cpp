#include <juce_core/juce_core.h>
#include <cstdio>

int main (int, char**)
{
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);
    runner.runAllTests();

    int numFailures = 0;
    int numTests = runner.getNumResults();

    for (int i = 0; i < numTests; ++i)
    {
        const auto* result = runner.getResult (i);
        numFailures += result->failures;
    }

    std::printf ("\n=== DNA Orbit test summary: %d suite(s) run, %d failure(s) ===\n", numTests, numFailures);

    return numFailures == 0 ? 0 : 1;
}
