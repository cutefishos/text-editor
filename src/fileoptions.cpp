// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "fileoptions.h"

#include "base/strings.h"

#include <map>

namespace strings = cutefish::base::strings;

std::optional<Indentation> FileOptions::detectIndentation(const std::vector<std::string> &lines)
{
    int tabs = 0;
    int spaces = 0;
    std::map<int, int> steps;
    int previous = 0;
    const size_t limit = std::min<size_t>(lines.size(), 2000);
    for (size_t i = 0; i < limit; ++i) {
        const std::string &line = lines[i];
        if (strings::trimmed(line).empty())
            continue;
        if (line[0] == '\t') {
            ++tabs;
            previous = 0;
            continue;
        }
        int indent = 0;
        while (indent < int(line.size()) && line[size_t(indent)] == ' ')
            ++indent;
        // A " * " comment continuation is not indentation.
        if (indent > 0 && indent < int(line.size()) && line[size_t(indent)] == '*')
            continue;
        if (indent > 0)
            ++spaces;
        const int step = indent - previous;
        if (step >= 2 && step <= 8)
            ++steps[step];
        previous = indent;
    }

    if (tabs + spaces < 3)
        return std::nullopt;
    if (tabs > spaces)
        return Indentation{false, 0};

    int best = 0;
    int bestCount = 0;
    for (const auto &[step, count] : steps) {
        if (count > bestCount) {
            best = step;
            bestCount = count;
        }
    }
    if (best == 0)
        return std::nullopt;
    return Indentation{true, best};
}
