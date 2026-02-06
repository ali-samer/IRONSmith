#pragma once

#include <algorithm>

#include <QString>
#include <QStringList>

namespace Utils {

struct Result {
	bool ok = true;
	QStringList errors;

	static Result success() { return Result{}; }

	static Result failure(const QString& msg)
	{
		Result r;
		r.ok = false;
		r.errors.push_back(msg);
		return r;
	}

	static Result failure(QStringList msgs)
	{
		Result r;
		r.ok = false;
		r.errors = std::move(msgs);
		return r;
	}

	void addError(const QString& msg)
	{
		ok = false;
		errors.push_back(msg);
	}

	explicit operator bool() const { return ok; }
};
} // namespace Utils
