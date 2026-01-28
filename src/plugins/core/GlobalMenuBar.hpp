#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVector>

namespace Core {

class GlobalMenuBarItem
{
public:
	GlobalMenuBarItem() = default;
	GlobalMenuBarItem(QString id, QString title)
		: m_id(std::move(id)), m_title(std::move(title)) {}

	const QString& id() const { return m_id; }
	const QString& title() const { return m_title; }

	bool isValid() const { return !m_id.isEmpty() && !m_title.isEmpty(); }

private:
	QString m_id;
	QString m_title;
};

class GlobalMenuBar final : public QObject
{
	Q_OBJECT

public:
	explicit GlobalMenuBar(QObject* parent = nullptr);

	const QVector<GlobalMenuBarItem>& items() const { return m_items; }

	bool addItem(const GlobalMenuBarItem& item);
	bool addItem(QString id, QString title);

	bool removeItem(const QString& id);
	void clear();

	int indexOf(const QString& id) const;
	const GlobalMenuBarItem* itemById(const QString& id) const;

	QString activeId() const { return m_activeId; }
	bool setActiveId(const QString& id);

	signals:
		void changed();
	void activeChanged(const QString& id);

private:
	bool containsId(const QString& id) const;

private:
	QVector<GlobalMenuBarItem> m_items;
	QString m_activeId;
};

} // namespace Core