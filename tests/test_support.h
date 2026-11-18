// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

#include <functional>
#include <string>

// The event loop and scratch folder the tests run in.
namespace TestSupport
{
const std::string &directory();
// Runs the event loop for `ms` milliseconds.
void wait(int ms);
// Runs the event loop until `done` holds or `timeoutMs` passes.
bool waitUntil(const std::function<bool()> &done, int timeoutMs);
}

#endif
