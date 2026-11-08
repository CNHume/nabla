// Copyright (C) 2017-2026, Christopher N. Hume.  All rights reserved.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, see <https://www.gnu.org/licenses/>.
//
// 2017-07-09 CNHume  Created File class
// 2017-07-04 CNHume  Created modes::Record class
// 2017-06-29 CNHume  Added Command class
// 2015-04-29 CNHume  Added support for isjoin
// 2015-04-25 CNHume  Repurposed infix as join
// 2015-04-18 CNHume  Added parse_uint32()
// 2014-12-18 CNHume  Created file
//
// Purpose:
//
// This program implements a solution to the Longest Common Subsequence
// (LCS) Problem based on the Hunt and Szymanski algorithm.  Please see
// the overview provided in "Doc/LCS Overview.md"
//
#include "Command.h"
#include "files/File.h"
#include "metrics/ProgressBar.h"
#include "metrics/Statistics.h"

#include <cstdlib>                      // for EXIT_FAILURE, EXIT_SUCCESS
#include <format>                       // for format
#include <iostream>                     // for cout, cerr
#include <locale>

using namespace std;
using namespace lcs;

namespace {
// Select a UTF-8 locale when the platform provides one, degrading to the
// environment locale and finally to the always-valid "C" locale.  Each
// std::locale(name) construction can throw std::runtime_error when the
// requested locale is unavailable (e.g. ".UTF-8" in a minimal container, or
// an LC_ALL/LANG such as C.UTF-8 on glibc < 2.35), so guard every step.
std::locale selectLocale() {
  try {
    return std::locale(".UTF-8");
  }
  catch (const std::exception&) {
    // Fall through to the environment locale.
  }
  try {
    return std::locale("");
  }
  catch (const std::exception&) {
    // Fall through to the classic "C" locale.
  }
  return std::locale::classic();
}
}  // namespace

//
// command line application
//
int main(int argc, char* argv[]) {
  auto errorLevel = EXIT_FAILURE;
  // Force the global runtime and standard streams to use UTF-8 when the
  // platform provides it, with graceful fallback (see selectLocale()).
  std::locale::global(selectLocale());
  std::cout.imbue(std::locale());
  std::cerr.imbue(std::locale());

  try {
    Command command;
    command.Parse(argc, argv);

    shared_ptr<Delta> intervals;
    auto pintervals = command.islength ?
      nullptr : &intervals;

    metrics::Statistics statistics;
    metrics::ProgressBar progressBar;
    auto progress = command.isprogress ? &progressBar : nullptr;
    auto length = command.iscorrespondence ?
      files::File::Correspondence(pintervals, &statistics, progress, command) :
      files::File::Difference(pintervals, &statistics, progress, command);

    if (command.isverbose || command.islength)
      cerr << format(
        std::locale(), "LCS Length = {:L}\n", length);
    if (command.isverbose)
      cerr << statistics.DurationReport();

    errorLevel = EXIT_SUCCESS;
  }
  catch (exception& ex) {
    cerr << ex.what() << std::endl;
  }
  catch (...) {
    cout << "exception" << std::endl;
    throw;
  }
#ifdef _DEBUG
  cout << "Press Enter";
  char c;
  cin.getline(&c, 1);
#endif
  return errorLevel;
}
