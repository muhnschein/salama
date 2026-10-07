// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

namespace Salama {

class Core;

// Re-call with other Core OK (tests); registration once per process.
void registerQmlTypes(Core *core);

} // namespace Salama
