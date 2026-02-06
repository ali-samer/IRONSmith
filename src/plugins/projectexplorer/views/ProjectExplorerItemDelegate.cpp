#include "projectexplorer/views/ProjectExplorerItemDelegate.hpp"

#include "projectexplorer/ProjectExplorerModel.hpp"
#include "projectexplorer/search/ProjectExplorerSearchMatcher.hpp"

#include <QtCore/QRect>
#include <QtWidgets/QApplication>
#include <QtGui/QFontMetrics>
#include <QtGui/QPainter>
#include <QtWidgets/QStyle>

#include <algorithm>

namespace ProjectExplorer::Internal {

namespace {

struct HighlightInfo {
    QString text;
    int start = -1;
    int length = 0;
};

HighlightInfo buildHighlight(const QString& text, const QString& query, const QFontMetrics& metrics, int width)
{
    HighlightInfo info;
    info.text = metrics.elidedText(text, Qt::ElideRight, width);
    const auto match = ProjectExplorer::Internal::ProjectExplorerSearchMatcher::match(
        info.text, query, Qt::CaseInsensitive);
    if (match.matched) {
        info.start = match.start;
        info.length = match.length;
    }
    return info;
}

void drawHighlightedText(QPainter* painter,
                         const QRect& rect,
                         const QFont& font,
                         const QPalette& palette,
                         const HighlightInfo& highlight,
                         bool selected)
{
    if (!painter || rect.isEmpty())
        return;

    painter->setFont(font);
    const QFontMetrics fm(font);

    const QString text = highlight.text;
    if (text.isEmpty())
        return;

    const QColor textColor = palette.color(selected ? QPalette::HighlightedText : QPalette::Text);
    QColor highlightColor(255, 170, 70);
    highlightColor.setAlphaF(selected ? 0.35 : 0.25);

    const int start = highlight.start;
    const int length = highlight.length;
    if (start < 0 || length <= 0) {
        painter->setPen(textColor);
        painter->drawText(rect, Qt::AlignVCenter | Qt::AlignLeft, text);
        return;
    }

    const QString prefix = text.left(start);
    const QString match = text.mid(start, length);
    const QString suffix = text.mid(start + length);

    const int prefixWidth = fm.horizontalAdvance(prefix);
    const int matchWidth = fm.horizontalAdvance(match);

    const int x = rect.left();
    const int y = rect.top();
    const int h = rect.height();

    const QRect highlightRect(x + prefixWidth, y, matchWidth, h);
    painter->fillRect(highlightRect, highlightColor);

    painter->setPen(textColor);
    painter->drawText(QRect(x, y, rect.width(), h), Qt::AlignVCenter | Qt::AlignLeft, prefix);
    painter->drawText(QRect(x + prefixWidth, y, rect.width() - prefixWidth, h),
                      Qt::AlignVCenter | Qt::AlignLeft, match);
    painter->drawText(QRect(x + prefixWidth + matchWidth, y,
                            rect.width() - prefixWidth - matchWidth, h),
                      Qt::AlignVCenter | Qt::AlignLeft, suffix);
}

} // namespace

ProjectExplorerItemDelegate::ProjectExplorerItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void ProjectExplorerItemDelegate::setSearchText(const QString& text)
{
    m_searchText = text;
}

QSize ProjectExplorerItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    return QStyledItemDelegate::sizeHint(option, index);
}

void ProjectExplorerItemDelegate::paint(QPainter* painter,
                                        const QStyleOptionViewItem& option,
                                        const QModelIndex& index) const
{
    if (!index.isValid()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    const int kind = index.data(ProjectExplorerModel::KindRole).toInt();
    const bool isRoot = (kind == static_cast<int>(ProjectExplorerModel::NodeKind::Root));

    const QString rootPath = index.data(ProjectExplorerModel::RootPathRole).toString();

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
    const QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);
    if (textRect.isEmpty())
        return;

    const QString name = opt.text;
    const int spacing = 8;

    QFont primaryFont = opt.font;
    QFont secondaryFont = opt.font;
    secondaryFont.setPointSize(std::max(8, secondaryFont.pointSize() - 1));

    const QFontMetrics fmPrimary(primaryFont);
    const QFontMetrics fmSecondary(secondaryFont);

    const int available = textRect.width();
    if (available <= 0)
        return;

    if (!isRoot) {
        if (m_searchText.trimmed().isEmpty()) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        const HighlightInfo info = buildHighlight(name, m_searchText, fmPrimary, available);
        if (info.start < 0) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);
        if (!opt.icon.isNull()) {
            const QRect iconRect = style->subElementRect(QStyle::SE_ItemViewItemDecoration, &opt, opt.widget);
            opt.icon.paint(painter, iconRect, opt.decorationAlignment);
        }

        drawHighlightedText(painter, textRect, primaryFont, opt.palette, info,
                            opt.state & QStyle::State_Selected);
        return;
    }

    if (rootPath.isEmpty()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);
    if (!opt.icon.isNull()) {
        const QRect iconRect = style->subElementRect(QStyle::SE_ItemViewItemDecoration, &opt, opt.widget);
        opt.icon.paint(painter, iconRect, opt.decorationAlignment);
    }

    QString nameText = name;
    int nameWidth = fmPrimary.horizontalAdvance(nameText);

    if (nameWidth > available) {
        const HighlightInfo info = buildHighlight(name, m_searchText, fmPrimary, available);
        painter->save();
        drawHighlightedText(painter, textRect, primaryFont, opt.palette, info,
                            opt.state & QStyle::State_Selected);
        painter->restore();
        return;
    }

    const int remaining = available - nameWidth - spacing;
    QString pathText;
    if (remaining > 0)
        pathText = fmSecondary.elidedText(rootPath, Qt::ElideMiddle, remaining);

    painter->save();

    const QColor primaryColor = opt.palette.color(opt.state & QStyle::State_Selected
                                                      ? QPalette::HighlightedText
                                                      : QPalette::Text);
    QColor secondaryColor = primaryColor;
    secondaryColor.setAlphaF(0.6);

    painter->setFont(primaryFont);
    painter->setPen(primaryColor);
    const QRect nameRect(textRect.left(), textRect.top(), nameWidth, textRect.height());
    const HighlightInfo nameHighlight = buildHighlight(nameText, m_searchText, fmPrimary, nameWidth);
    if (nameHighlight.start >= 0) {
        drawHighlightedText(painter, nameRect, primaryFont, opt.palette, nameHighlight,
                            opt.state & QStyle::State_Selected);
    } else {
        painter->drawText(nameRect, Qt::AlignVCenter | Qt::AlignLeft, nameText);
    }

    if (!pathText.isEmpty()) {
        painter->setFont(secondaryFont);
        painter->setPen(secondaryColor);
        painter->drawText(QRect(textRect.left() + nameWidth + spacing,
                                textRect.top(),
                                remaining,
                                textRect.height()),
                          Qt::AlignVCenter | Qt::AlignLeft, pathText);
    }

    painter->restore();
}

} // namespace ProjectExplorer::Internal
