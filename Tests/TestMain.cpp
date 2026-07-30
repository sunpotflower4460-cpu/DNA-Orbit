#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
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

    const int exitCode = numFailures == 0 ? 0 : 1;

    // This is a plain console runner, not a juce::JUCEApplicationBase, so
    // nothing else calls JUCE's normal shutdown path. Some tests exercise
    // juce::Timer, which lazily creates a DeletedAtShutdown-derived
    // ShutdownDetector singleton; without this it is reported as a leak by
    // every future sanitizer/leak-detector run even though it holds no
    // application data.
    juce::DeletedAtShutdown::deleteAll();

    return exitCode;
}
