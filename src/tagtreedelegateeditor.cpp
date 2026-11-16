#include "tagtreedelegateeditor.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QDebug>
#include <QTreeView>
#include <QMouseEvent>
#include <QResizeEvent>
#include "pushbuttontype.h"
#include "nodetreemodel.h"
#include "nodetreeview.h"
#include "labeledittype.h"
#include "notelistview.h"
#include "fontloader.h"

TagTreeDelegateEditor::TagTreeDelegateEditor(QTreeView *view, const QStyleOptionViewItem &option, const QModelIndex &index, QListView *listView,
                                             QWidget *parent)
    : QWidget(parent),
      m_option(option),
      m_index(index),
#ifdef __APPLE__
      m_displayFont(QFont(QStringLiteral("SF Pro Text")).exactMatch() ? QStringLiteral("SF Pro Text") : QStringLiteral("Roboto")),
#elif _WIN32
      m_displayFont(QFont(QStringLiteral("Segoe UI")).exactMatch() ? QStringLiteral("Segoe UI") : QStringLiteral("Roboto")),
#else
      m_displayFont(QStringLiteral("Roboto")),
#endif
#ifdef __APPLE__
      m_titleFont(m_displayFont, 13, QFont::DemiBold),
#else
      m_titleFont(m_displayFont, 10, QFont::DemiBold),
#endif
      m_titleColor(26, 26, 26),
      m_titleSelectedColor(255, 255, 255),
      m_activeColor(68, 138, 201),
      m_hoverColor(207, 207, 207),
      m_view(view),
      m_listView(listView), m_theme(Theme::Light)
{
    setContentsMargins(0, 0, 0, 0);
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(22, 0, 8, 0);
    layout->setSpacing(0);
    setLayout(layout);
    layout->addSpacing(27);

    m_label = new LabelEditType(this);
    m_label->setFont(m_titleFont);
    m_label->setMinimumWidth(0);
    QSizePolicy labelPolicy;
    labelPolicy.setVerticalPolicy(QSizePolicy::Expanding);
    labelPolicy.setHorizontalPolicy(QSizePolicy::Expanding);
    m_label->setSizePolicy(labelPolicy);
    m_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    connect(m_label, &LabelEditType::editingStarted, this, [this] {
        auto *treeView = static_cast<NodeTreeView *>(m_view);
        treeView->setIsEditing(true);
    });
    connect(m_label, &LabelEditType::editingFinished, this, [this](const QString &label) {
        auto *treeView = static_cast<NodeTreeView *>(m_view);
        treeView->onRenameTagFinished(label);
        treeView->setIsEditing(false);
    });
    connect(static_cast<NodeTreeView *>(m_view), &NodeTreeView::renameTagRequested, m_label, &LabelEditType::openEditor);
    layout->addWidget(m_label);
    auto *tree = static_cast<NodeTreeView *>(m_view);
    const auto addAction = [this, layout, tree](const QString &glyph, const QString &tip, NodeTreeView::RowAction action) {
        auto *button = new PushButtonType(this);
        button->setFixedSize(20, 25);
        button->setFont(font_loader::loadFont("Font Awesome 6 Free Solid", "", 9));
        button->setText(glyph);
        button->setToolTip(tip);
        button->setCursor(Qt::PointingHandCursor);
        connect(button, &QPushButton::clicked, tree, [tree, index = m_index, action] { tree->triggerRowAction(index, action); });
        layout->addWidget(button);
        m_actions.append(button);
    };
    addAction(QString::fromUtf8(u8"\uf53f"), tr("Change tag color"), NodeTreeView::RowAction::Color);
    addAction(QString::fromUtf8(u8"\uf303"), tr("Rename tag"), NodeTreeView::RowAction::Rename);
    addAction(QString::fromUtf8(u8"\uf1f8"), tr("Delete tag"), NodeTreeView::RowAction::Delete);
    m_count = new QLabel(this);
    m_count->setFixedWidth(32);
    m_count->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_count);
    connect(m_view, &QTreeView::expanded, this, [this](const QModelIndex &) { update(); });
}

void TagTreeDelegateEditor::updateDelegate()
{
    auto displayName = m_index.data(NodeItem::Roles::DisplayText).toString();
    QFontMetrics fm(m_titleFont);

    QString labelStyle;

    if (m_view->selectionModel()->selectedIndexes().contains(m_index)) {
        labelStyle = QStringLiteral("QLabel{color: rgb(%1, %2, %3);}")
                             .arg(QString::number(m_titleSelectedColor.red()), QString::number(m_titleSelectedColor.green()),
                                  QString::number(m_titleSelectedColor.blue()));
    } else {
        labelStyle = QStringLiteral("QLabel{color: rgb(%1, %2, %3);}")
                             .arg(QString::number(m_titleColor.red()), QString::number(m_titleColor.green()), QString::number(m_titleColor.blue()));
    }
    m_label->setText(displayName);
    m_count->setText(QString::number(m_index.data(NodeItem::ChildCount).toInt()));
    const QString color = m_view->selectionModel()->isSelected(m_index) ? "white" : m_titleColor.name();
    m_count->setStyleSheet("QLabel {color:" + color + ";}");
    for (int i = 0; i < m_actions.size(); ++i)
        m_actions[i]->setStyleSheet("QPushButton {border:0; padding:0; color:" + color + ";} QPushButton:hover {color:" + (i == 2 ? QString("#d34d4d") : QString("#448ac9")) + ";}");


    if (m_label->styleSheet() != labelStyle) {
        m_label->setStyleSheet(labelStyle);
    }
}

void TagTreeDelegateEditor::paintEvent(QPaintEvent *event)
{
    updateDelegate();
    QPainter painter(this);
    if (m_view->selectionModel()->selectedIndexes().contains(m_index)) {
        painter.fillRect(rect(), QBrush(m_activeColor));
    } else {
        auto const *listView = static_cast<NoteListView *>(m_listView);
        if (listView->isDragging()) {
            if (m_theme == Theme::Dark) {
                painter.fillRect(rect(), QBrush(QColor(35, 52, 69)));
            } else {
                painter.fillRect(rect(), QBrush(QColor(180, 208, 233)));
            }
        } else {
            painter.fillRect(rect(), QBrush(m_hoverColor));
        }
    }

    auto iconRect = QRect(rect().x() + 22, rect().y() + ((rect().height() - 14) / 2), 16, 16);
    auto tagColor = m_index.data(NodeItem::Roles::TagColor).toString();
    painter.setPen(QColor(tagColor));
#ifdef __APPLE__
    int iconPointSizeOffset = 0;
#else
    int iconPointSizeOffset = -4;
#endif
    painter.setFont(font_loader::loadFont("Font Awesome 6 Free Solid", "", 16 + iconPointSizeOffset));
    painter.drawText(iconRect, u8"\uf111"); // fa-circle
    QWidget::paintEvent(event);
}

void TagTreeDelegateEditor::mouseDoubleClickEvent(QMouseEvent *event)
{
    auto iconRect = QRect(rect().x() + 10, rect().y() + ((rect().height() - 14) / 2), 14, 14);
    if (iconRect.contains(event->position().toPoint())) {
        static_cast<NodeTreeView *>(m_view)->onChangeTagColorAction();
    } else if (m_label->geometry().contains(event->position().toPoint())) {
        m_label->openEditor();
    } else {
        QWidget::mouseDoubleClickEvent(event);
    }
}

void TagTreeDelegateEditor::setTheme(Theme::Value theme)
{
    m_theme = theme;
    switch (theme) {
    case Theme::Light: {
        m_hoverColor = QColor(247, 247, 247);
        m_titleColor = QColor(26, 26, 26);
        break;
    }
    case Theme::Dark: {
        m_hoverColor = QColor(25, 25, 25);
        m_titleColor = QColor(212, 212, 212);
        break;
    }
    case Theme::Sepia: {
        m_hoverColor = QColor(251, 240, 217);
        m_titleColor = QColor(26, 26, 26);
        break;
    }
    }
}

void TagTreeDelegateEditor::resizeEvent(QResizeEvent *event)
{
    for (auto *button : m_actions) button->setVisible(event->size().width() >= 150);
    QWidget::resizeEvent(event);
}
