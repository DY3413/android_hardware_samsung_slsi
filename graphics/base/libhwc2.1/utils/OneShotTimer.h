/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _ONESHOTTIMER_H
#define _ONESHOTTIMER_H

#include <android-base/thread_annotations.h>
#include <semaphore.h>
#include <chrono>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <log/log.h>

class OneShotTimer {
  public:
    using Interval = std::chrono::milliseconds;
    using ResetCallback = std::function<void()>;
    using TimeoutCallback = std::function<void()>;
    // Enum to track in what state is the timer.
    enum class TimerState {
        // The internal timer thread has been destroyed, and no state is
        // tracked.
        // Possible state transitions: RESET
        STOPPED = 0,
        // An external thread has just reset this timer.
        // If there is a reset callback, then that callback is fired.
        // Possible state transitions: STOPPED, WAITING
        RESET = 1,
        // This timer is waiting for the timeout interval to expire.
        // Possible state transaitions: STOPPED, RESET, IDLE
        WAITING = 2,
        // The timeout interval has expired, so we are sleeping now.
        // Possible state transaitions: STOPPED, RESET
        IDLE = 3
    };

    OneShotTimer(const Interval &interval, const ResetCallback &resetCallback,
                 const TimeoutCallback &timeoutCallback);
    ~OneShotTimer();

    // Initializes and turns on the idle timer.
    void start();
    // Stops the idle timer and any held resources.
    void stop();
    // Stops the idle timer and change interval in timer. After that start the timer.
    void setInterval(const Interval &interval);
    // Resets the wakeup time and fires the reset callback.
    void reset();
    // Return true if current TimerState is WAITING
    bool isTimerRunning();

  private:
    // Function that loops until the condition for stopping is met.
    void loop();

    // Checks whether mResetTriggered and mStopTriggered were set and updates
    // mState if so.
    TimerState checkForResetAndStop(TimerState state);

    // Thread waiting for timer to expire.
    std::thread mThread;

    // Semaphore to keep mThread synchronized.
    sem_t mSemaphore;

    // Interval after which timer expires.
    Interval mInterval;

    // Callback that happens when timer resets.
    const ResetCallback mResetCallback;

    // Callback that happens when timer expires.
    const TimeoutCallback mTimeoutCallback;

    // After removing lock guarding mState, the state can be now accessed at
    // any time. Keep a bool if the reset or stop were requested, and occasionally
    // check in the main loop if they were.
    std::atomic<bool> mResetTriggered = false;
    std::atomic<bool> mStopTriggered = false;
    std::atomic<bool> mWaiting = false;
    std::atomic<std::chrono::steady_clock::time_point> mLastResetTime;
};
#endif  //  _ONESHOTTIMER_H
