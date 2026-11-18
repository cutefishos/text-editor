// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef FILEOPTIONS_H
#define FILEOPTIONS_H

#include <optional>
#include <string>
#include <vector>

struct Indentation
{
    bool insertSpaces = true;
    int width = 4;
};

namespace FileOptions
{
// The indentation most lines use, or nothing when too few lines are indented.
std::optional<Indentation> detectIndentation(const std::vector<std::string> &lines);
}

#endif
