// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

namespace Salama {

// Roles are scoped enums; Qt wants int.
template <typename Role> constexpr int roleId(Role role)
{
    return static_cast<int>(role);
}

} // namespace Salama
