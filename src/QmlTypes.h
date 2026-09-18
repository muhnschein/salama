// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

namespace Salama {

class Core;

// Registers the `harbour.salama 1.0` module: TabModel, HistoryModel, BookmarkModel,
// Settings and EngineMessages as singletons backed by the given Core. Safe to call
// again with another Core (tests); registration itself happens once per process.
void registerQmlTypes(Core *core);

} // namespace Salama
