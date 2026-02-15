// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QtCore/QObject>
#include <QtCore/QStringList>

class QWidget;

namespace Core {

class IProjectService : public QObject
{
	Q_OBJECT

public:
	explicit IProjectService(QObject* parent = nullptr) : QObject(parent) {}
	~IProjectService() override = default;

public slots:
	virtual void newDesign() = 0;
	virtual void openDialog(QWidget* parent) = 0;
	virtual void save() = 0;
	virtual void saveAsDialog(QWidget* parent) = 0;

public:
	virtual QStringList recentProjects() const = 0;

signals:
	void recentProjectsChanged();
};

} // namespace Core
