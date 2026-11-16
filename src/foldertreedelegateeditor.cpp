#include "foldertreedelegateeditor.h"
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
#include "notelistview.h"
#include "labeledittype.h"
#include "fontloader.h"

FolderTreeDelegateEditor::FolderTreeDelegateEditor(QTreeView *view, const QStyleOptionViewItem &option, const QModelIndex &index, QListView *listView,
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
      m_folderIconColor(68, 138, 201),
      m_view(view),
      m_listView(listView),
      m_theme(Theme::Light)
{
    setContentsMargins(0, 0, 0, 0);
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 8, 0);
    layout->setSpacing(0);
    setLayout(layout);
    m_expandIcon = new QLabel(this);
    m_expandIcon->setMinimumSize({ 11, 11 });
    m_expandIcon->setMaximumSize({ 11, 11 });

    if (m_index.data(NodeItem::Roles::IsExpandable).toBool()) {
        if (!m_view->isExpanded(m_index)) {
            layout->addSpacing(2);
        }
    }

#ifdef __APPLE__
    int iconPointSizeOffset = 0;
#else
    int iconPointSizeOffset = -4;
#endif
    m_expandIcon->setFont(font_loader::loadFont("Font Awesome 6 Free Solid", "", 12 + iconPointSizeOffset));

    m_expandIcon->setScaledContents(true);
    layout->addWidget(m_expandIcon);

    if (!m_index.data(NodeItem::Roles::IsExpandable).toBool() || (m_index.data(NodeItem::Roles::IsExpandable).toBool() && m_view->isExpanded(m_index))) {
        layout->addSpacing(2);
    }

    m_folderIcon = new PushButtonType(parent);
    m_folderIcon->setMaximumSize({ 19, 20 });
    m_folderIcon->setMinimumSize({ 19, 20 });
    m_folderIcon->setIconSize(QSize(19, 20));
    QFont materialSymbols("Material Symbols Outlined", 16 + iconPointSizeOffset);
    m_folderIcon->setFont(materialSymbols);
    m_folderIcon->setText(m_index.parent().data(NodeItem::ItemType).toInt() == NodeItem::FolderItem ? u8"\ue2c8" : u8"\ue2c7"); // folder
    layout->addWidget(m_folderIcon);
    layout->addSpacing(5);

    m_label = new LabelEditType(this);
    m_titleFont.setWeight(m_index.parent().data(NodeItem::ItemType).toInt() == NodeItem::FolderItem ? QFont::Normal : QFont::DemiBold);
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
        treeView->onRenameFolderFinished(label);
        treeView->setIsEditing(false);
    });
    connect(static_cast<NodeTreeView *>(m_view), &NodeTreeView::renameFolderRequested, m_label, &LabelEditType::openEditor);
    layout->addWidget(m_label);
    layout->addSpacing(5);
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
    addAction(QString::fromUtf8(u8"\uf65e"), tr("Create subfolder"), NodeTreeView::RowAction::AddSubfolder);
    addAction(QString::fromUtf8(u8"\uf303"), tr("Rename folder"), NodeTreeView::RowAction::Rename);
    addAction(QString::fromUtf8(u8"\uf1f8"), tr("Delete folder"), NodeTreeView::RowAction::Delete);
    m_count = new QLabel(this);
    m_count->setFixedWidth(32);
    m_count->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_count);
    m_expandIcon->installEventFilter(this);
    m_folderIcon->installEventFilter(this);
    m_label->installEventFilter(this);
    connect(m_folderIcon, &QPushButton::clicked, tree, [tree, index = m_index] { tree->triggerRowAction(index, NodeTreeView::RowAction::Select); });
    connect(m_view, &QTreeView::expanded, this, [this](const QModelIndex &) { update(); });
}

void FolderTreeDelegateEditor::updateDelegate()
{
    auto displayName = m_index.data(NodeItem::Roles::DisplayText).toString();
    QFontMetrics fm(m_titleFont);
    QString labelStyle;
    QString folderIconStyle;

    if (m_view->selectionModel()->isSelected(m_index)) {
        labelStyle = QStringLiteral("QLabel{color: rgb(%1, %2, %3);}")
                             .arg(QString::number(m_titleSelectedColor.red()), QString::number(m_titleSelectedColor.green()),
                                  QString::number(m_titleSelectedColor.blue()));
        folderIconStyle = QStringLiteral("QPushButton{border: none; padding: 0px; color: rgb(%1, %2, %3);}")
                                  .arg(QString::number(m_titleSelectedColor.red()), QString::number(m_titleSelectedColor.green()),
                                       QString::number(m_titleSelectedColor.blue()));
    } else {
        labelStyle = QStringLiteral("QLabel{color: rgb(%1, %2, %3);}")
                             .arg(QString::number(m_titleColor.red()), QString::number(m_titleColor.green()), QString::number(m_titleColor.blue()));
        folderIconStyle =
                QStringLiteral("QPushButton{border: none; padding: 0px; color: rgb(%1, %2, %3);}")
                        .arg(QString::number(m_folderIconColor.red()), QString::number(m_folderIconColor.green()), QString::number(m_folderIconColor.blue()));
    }
    const QColor customIconColor = static_cast<NodeTreeView *>(m_view)->folderIconColor(m_index);
    if (customIconColor.isValid())
        folderIconStyle = "QPushButton {border:0; padding:0; color:" + customIconColor.name() + ";}";
    m_label->setText(displayName);
    m_count->setText(QString::number(m_index.data(NodeItem::ChildCount).toInt()));
    const QString color = m_view->selectionModel()->isSelected(m_index) ? "white" : m_titleColor.name();
    m_count->setStyleSheet("QLabel {color:" + color + ";}");
    for (int i = 0; i < m_actions.size(); ++i)
        m_actions[i]->setStyleSheet("QPushButton {border:0; padding:0; color:" + color + ";} QPushButton:hover {color:" + (i == 2 ? QString("#d34d4d") : QString("#448ac9")) + ";}");
    auto theme = static_cast<NodeTreeView *>(m_view)->theme();

    QColor chevronColor(theme == Theme::Dark ? QColor(169, 160, 172) : QColor(103, 99, 105));
    QString expandIconStyle = QStringLiteral("QLabel{color: rgb(%1, %2, %3);}")
                                      .arg(QString::number(chevronColor.red()), QString::number(chevronColor.green()), QString::number(chevronColor.blue()));

    if (m_folderIcon->styleSheet() != folderIconStyle) {
        m_folderIcon->setStyleSheet(folderIconStyle);
    }

    if (m_label->styleSheet() != labelStyle) {
        m_label->setStyleSheet(labelStyle);
    }

    if (m_expandIcon->styleSheet() != expandIconStyle) {
        m_expandIcon->setStyleSheet(expandIconStyle);
    }

    if (m_index.data(NodeItem::Roles::IsExpandable).toBool()) {
        if (m_view->isExpanded(m_index)) {
            m_expandIcon->setText(u8"\uf078"); // fa-chevron-down
        } else {
            m_expandIcon->setText(u8"\uf054"); // fa-chevron-right
        }
    }
}

void FolderTreeDelegateEditor::paintEvent(QPaintEvent *event)
{
    updateDelegate();
    QPainter painter(this);
    if (m_view->selectionModel()->isSelected(m_index)) {
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
    QWidget::paintEvent(event);
}

void FolderTreeDelegateEditor::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (m_label->geometry().contains(event->position().toPoint())) {
        m_label->openEditor();
    } else {
        QWidget::mouseDoubleClickEvent(event);
    }
}

void FolderTreeDelegateEditor::setTheme(Theme::Value theme)
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

void FolderTreeDelegateEditor::resizeEvent(QResizeEvent *event)
{
    for (auto *button : m_actions) button->setVisible(event->size().width() >= 150);
    QWidget::resizeEvent(event);
}

bool FolderTreeDelegateEditor::eventFilter(QObject *watched, QEvent *event)
{
    auto *tree = static_cast<NodeTreeView *>(m_view);
    if (event->type() == QEvent::MouseButtonPress) {
        auto *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->button() == Qt::LeftButton && watched == m_expandIcon) {
            if (m_index.data(NodeItem::IsExpandable).toBool()) tree->setExpanded(m_index, !tree->isExpanded(m_index));
            return true;
        }
    }
    if (watched == m_folderIcon && event->type() == QEvent::MouseButtonDblClick) {
        tree->changeFolderIconColor(m_index);
        return true;
    }
    if (watched == m_label && event->type() == QEvent::MouseButtonDblClick) {
        tree->triggerRowAction(m_index, NodeTreeView::RowAction::Rename);
        return true;
    }
    return QWidget::eventFilter(watched, event);
}
