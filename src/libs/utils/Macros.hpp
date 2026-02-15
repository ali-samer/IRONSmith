// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "utils/Result.hpp"
#include "utils/ScopeGuard.hpp"

#include <QtCore/QtGlobal>

// This header is intentionally boring.
//
// A bunch of code in IRONSmith wants the same handful of guard/early-return idioms,
// but we don't want every file to invent its own micro-style.

// If you need a conditional noexcept (mostly for move operations).
#ifndef UTILS_NOEXCEPT_IF
#	define UTILS_NOEXCEPT_IF(expr) noexcept(noexcept(expr))
#endif

#ifndef UTILS_UNUSED
#	define UTILS_UNUSED(x) Q_UNUSED(x)
#endif

#ifndef UTILS_LIKELY
#	define UTILS_LIKELY(x) Q_LIKELY(x)
#endif

#ifndef UTILS_UNLIKELY
#	define UTILS_UNLIKELY(x) Q_UNLIKELY(x)
#endif

#ifndef UTILS_GUARD
#	define UTILS_GUARD(cond) do { if (!(cond)) return; } while (false)
#endif

#ifndef UTILS_GUARD_RET
#	define UTILS_GUARD_RET(cond, ret) do { if (!(cond)) return (ret); } while (false)
#endif

#ifndef UTILS_GUARD_OK
#	define UTILS_GUARD_OK(cond, msg) do { if (!(cond)) return ::Utils::Result::failure((msg)); } while (false)
#endif

#ifndef UTILS_RETURN_IF
#	define UTILS_RETURN_IF(cond) do { if ((cond)) return; } while (false)
#endif

#ifndef UTILS_RETURN_VAL_IF
#	define UTILS_RETURN_VAL_IF(cond, val) do { if ((cond)) return (val); } while (false)
#endif

#ifndef UTILS_ASSERT
#	define UTILS_ASSERT(cond) Q_ASSERT(cond)
#endif

#ifndef UTILS_ASSERT_MSG
#	define UTILS_ASSERT_MSG(cond, msg) Q_ASSERT_X((cond), Q_FUNC_INFO, (msg))
#endif

#ifndef UTILS_UNREACHABLE
#	define UTILS_UNREACHABLE() Q_UNREACHABLE()
#endif

#define UTILS__JOIN2(a, b) a##b
#define UTILS__JOIN(a, b) UTILS__JOIN2(a, b)

#ifndef UTILS_DEFER
#	define UTILS_DEFER(...) \
		auto UTILS__JOIN(_utils_defer_, __COUNTER__) = ::Utils::makeScopeGuard([&] { __VA_ARGS__; })
#endif
