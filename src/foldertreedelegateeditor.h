#ifndef FOLDERTREEDELEGATEEDITOR_H
#define FOLDERTREEDELEGATEEDITOR_H

#include <QWidget>
#include <QStyleOptionViewItem>
#include <QPersistentModelIndex>
#include <QFont>
#include "editorsettingsoptions.h"

class QTreeView;
class QPushButton;
class TreeFolderIconButton;
class QLabel;
class RowCounterButton;
class PushButtonType;
class LabelEditType;
class QListView;

class FolderTreeDelegateEditor : public QWidget
{
    Q_OBJECT
public:
    explicit FolderTreeDelegateEditor(QTreeView *view, const QStyleOptionViewItem &option, const QModelIndex &index, QListView *listView,
                                      QWidget *parent = nullptr);
    void setTheme(Theme::Value theme);

private:
    QStyleOptionViewItem m_option;
    QPersistentModelIndex m_index;
    QString m_displayFont;
    QFont m_titleFont;
    QColor m_titleColor;
    QColor m_titleSelectedColor;
    QColor m_activeColor;
    QColor m_hoverColor;
    QColor m_folderIconColor;
    QTreeView *m_view;
    QListView *m_listView;
    LabelEditType *m_label;
    TreeFolderIconButton *m_folderIcon;
    QPixmap m_expanded;
    QPixmap m_notExpanded;
    QLabel *m_expandIcon;
    QList<QPushButton *> m_actions;
    RowCounterButton *m_count;
    Theme::Value m_theme;
    void updateDelegate();

    // QWidget interface
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
};

#endif // FOLDERTREEDELEGATEEDITOR_H
