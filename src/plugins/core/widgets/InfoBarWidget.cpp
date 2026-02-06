#include "core/widgets/InfoBarWidget.hpp"

#include <QtWidgets/QHBoxLayout>

#include "core/StatusBarField.hpp"
#include "core/widgets/StatusBarFieldWidget.hpp"

namespace Core {

static bool isNonEmptyId(const QString& id)
{
    return !id.trimmed().isEmpty();
}

InfoBarWidget::InfoBarWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("InfoBar");
    setAttribute(Qt::WA_StyledBackground, true);

    m_root = new QHBoxLayout(this);
    m_root->setContentsMargins(8, 0, 8, 0);
    m_root->setSpacing(0);

    auto* leftHost = new QWidget(this);
    leftHost->setObjectName("InfoBarLeftHost");
    leftHost->setAttribute(Qt::WA_StyledBackground, true);

    m_left = new QHBoxLayout(leftHost);
    m_left->setContentsMargins(0, 0, 0, 0);
    m_left->setSpacing(12);

    // Right zone
    auto* rightHost = new QWidget(this);
    rightHost->setObjectName("InfoBarRightHost");
    rightHost->setAttribute(Qt::WA_StyledBackground, true);

    m_right = new QHBoxLayout(rightHost);
    m_right->setContentsMargins(0, 0, 0, 0);
    m_right->setSpacing(12);

    m_root->addWidget(leftHost, 0);
    m_root->addStretch(1);
    m_root->addWidget(rightHost, 0);
}

StatusBarField* InfoBarWidget::field(const QString& id) const noexcept
{
    return m_fields.value(id, nullptr);
}

bool InfoBarWidget::hasField(const QString& id) const noexcept
{
    return m_fields.contains(id);
}

StatusBarField* InfoBarWidget::ensureField(const QString& id)
{
    if (!isNonEmptyId(id))
        return nullptr;

    if (auto* f = field(id))
        return f;

    auto* f = new StatusBarField(id, this);
    setField(f);
    return f;
}

void InfoBarWidget::setField(StatusBarField* field)
{
    if (!field)
        return;

    const QString id = field->id();
    if (!isNonEmptyId(id)) {
        Q_ASSERT(false);
        return;
    }

    if (auto* existing = m_fields.value(id, nullptr)) {
        if (existing == field)
            return;

        removeField(id);
    }

    if (field->parent() != this)
        field->setParent(this);

    m_fields.insert(id, field);

    addFieldWidget(field);

    connect(field, &StatusBarField::sideChanged, this, &InfoBarWidget::onFieldSideChanged);

    emit fieldAdded(field);
}

void InfoBarWidget::removeField(const QString& id)
{
    auto* f = m_fields.value(id, nullptr);
    if (!f)
        return;

    removeFieldWidget(id);

    m_fields.remove(id);

    f->deleteLater();

    emit fieldRemoved(id);
}

void InfoBarWidget::clear()
{
    const auto keys = m_fields.keys();
    for (const auto& id : keys)
        removeField(id);
}

void InfoBarWidget::onFieldSideChanged(StatusBarField::Side)
{
    auto* f = qobject_cast<StatusBarField*>(sender());
    if (!f)
        return;

    const QString id = f->id();
    auto* w = m_fieldWidgets.value(id, nullptr);
    if (!w)
        return;

    if (w->parentWidget() && w->parentWidget()->layout())
        w->parentWidget()->layout()->removeWidget(w);

    if (f->side() == StatusBarField::Side::Left) {
        m_left->addWidget(w);
    } else {
        m_right->addWidget(w);
    }

    w->show();
}

void InfoBarWidget::addFieldWidget(StatusBarField* field)
{
    const QString id = field->id();
    if (m_fieldWidgets.contains(id))
        return;

    auto* w = new StatusBarFieldWidget(field, this);

    m_fieldWidgets.insert(id, w);

    if (field->side() == StatusBarField::Side::Left)
        m_left->addWidget(w);
    else
        m_right->addWidget(w);
}

void InfoBarWidget::removeFieldWidget(const QString& id)
{
    auto* w = m_fieldWidgets.value(id, nullptr);
    if (!w)
        return;

    if (w->parentWidget() && w->parentWidget()->layout())
        w->parentWidget()->layout()->removeWidget(w);

    m_fieldWidgets.remove(id);
    w->deleteLater();
}

} // namespace Core
