#pragma once

#include <QtCore/QObject>

class QMainWindow;
class QWidget;

namespace Core {
namespace Internal { class MainWindow; }

class ICore : public QObject
{
	Q_OBJECT

public:
	explicit ICore(QObject* parent = nullptr) : QObject(parent) {}
	~ICore() override = default;

	virtual void setCentralWidget(QWidget* widget) = 0;

	virtual void open() = 0;

signals:
	void coreAboutToOpen();
	void coreOpened();
};

} // namespace Core