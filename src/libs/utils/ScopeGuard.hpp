#pragma once

#include <type_traits>
#include <utility>

namespace Utils {

template <typename F>
class ScopeGuard final {
public:
	using Func = F;

	explicit ScopeGuard(Func&& f)
		: m_f(std::move(f))
		, m_active(true)
	{}

	ScopeGuard(ScopeGuard&& other) noexcept
		: m_f(std::move(other.m_f))
		, m_active(other.m_active)
	{
		other.m_active = false;
	}

	ScopeGuard(const ScopeGuard&) = delete;
	ScopeGuard& operator=(const ScopeGuard&) = delete;
	ScopeGuard& operator=(ScopeGuard&&) = delete;

	~ScopeGuard() noexcept
	{
		if (m_active)
			m_f();
	}

	void dismiss() noexcept { m_active = false; }

private:
	Func m_f;
	bool m_active = false;
};

template <typename F>
auto makeScopeGuard(F&& f)
{
	using Fn = std::decay_t<F>;
	return ScopeGuard<Fn>(std::forward<F>(f));
}

} // namespace Utils
