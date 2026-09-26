// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
#pragma once

namespace Salama {

// Every model's roles are a scoped enum, and Qt hands a role about as an int: data()'s
// argument, roleNames()' keys, the list dataChanged() carries.
template <typename Role> constexpr int roleId(Role role)
{
    return static_cast<int>(role);
}

} // namespace Salama
