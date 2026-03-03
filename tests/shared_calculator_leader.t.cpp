#include <gtest/gtest.h>
#include <shared_calculator_leader.h>

#include <thread>

/**
 * Unit tests for the Leader class public interface.
 * Just tests basic functionality of the Leader class
 */

TEST(SharedCalculatorLeaderTest, RunSuccessfullyCreatesAndSubmitsEvents) {
  // GIVEN: A leader instance
  Calculator::Leader leaderUnderTest;

  // WHEN: Run() executes in a background thread for 3 seconds
  std::thread runThread(
      [&leaderUnderTest]() { EXPECT_NO_THROW(leaderUnderTest.Run()); });
  std::this_thread::sleep_for(std::chrono::seconds(3));

  // THEN: Events should have been created and state should have changed
  const auto [value1, index1] = leaderUnderTest.GetCurrentValueAndIndex();
  EXPECT_GT(index1, 0);
  EXPECT_NE(value1, 0);

  runThread.detach();
}

TEST(SharedCalculatorLeaderTest,
     GetCurrentValueAndIndexReturnsDifferentValuesAsEventsAreApplied) {
  // GIVEN: A leader instance
  Calculator::Leader leaderUnderTest;

  // WHEN: We get the initial state
  auto [initialValue, initialIndex] = leaderUnderTest.GetCurrentValueAndIndex();

  // THEN: Both should start at 0
  EXPECT_EQ(initialValue, 0);
  EXPECT_EQ(initialIndex, 0);

  // WHEN: The leader runs for 1 second
  std::thread leaderThread(
      [&leaderUnderTest]() { EXPECT_NO_THROW(leaderUnderTest.Run()); });
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // THEN: Value and index should have updated
  auto [updatedValue, updatedIndex] = leaderUnderTest.GetCurrentValueAndIndex();
  EXPECT_NE(updatedValue, initialValue)
      << "Could possibly be false positive since values might coincidentally "
         "be the same";
  EXPECT_GT(updatedIndex, initialIndex);

  leaderThread.detach();
}

TEST(SharedCalculatorLeaderTest,
     WaitForUpdatesFromIndexReturnsEmptyIfTimeoutReached) {
  // GIVEN: A leader instance in a background thread
  Calculator::Leader leaderUnderTest;
  std::thread leaderThread(
      [&leaderUnderTest]() { EXPECT_NO_THROW(leaderUnderTest.Run()); });

  // WHEN: We wait for updates with 0ms timeout immediately
  auto updates =
      leaderUnderTest.WaitForUpdatesFromIndex(0, std::chrono::milliseconds(0));

  // THEN: Should return empty (timeout expires before any events)
  EXPECT_FALSE(updates.has_value());

  leaderThread.detach();
}

TEST(SharedCalculatorLeaderTest,
     WaitForUpdatesFromIndexReturnsEmptyIfLeaderNeverRuns) {
  // GIVEN: A leader instance that does not run
  Calculator::Leader leaderUnderTest;

  // WHEN: We wait for updates with a 100ms timeout
  auto updates = leaderUnderTest.WaitForUpdatesFromIndex(
      0, std::chrono::milliseconds(100));

  // THEN: Should return empty (no events generated)
  EXPECT_FALSE(updates.has_value());
}

TEST(SharedCalculatorLeaderTest,
     WaitForUpdatesFromIndexReturnsEventsGeneratedAfterGivenIndex) {
  // GIVEN: A leader running in a background thread
  Calculator::Leader leaderUnderTest;
  std::thread leaderThread(
      [&leaderUnderTest]() { EXPECT_NO_THROW(leaderUnderTest.Run()); });

  // WHEN: We wait for some events to be generated
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // AND: We get the current state and wait for new events
  auto [currentValue, currentIndex] = leaderUnderTest.GetCurrentValueAndIndex();
  auto updates = leaderUnderTest.WaitForUpdatesFromIndex(
      currentIndex, std::chrono::milliseconds(500));

  // THEN: Should have returned new events after the current index
  EXPECT_TRUE(updates.has_value());
  EXPECT_GT(updates->size(), 0);

  leaderThread.detach();
}