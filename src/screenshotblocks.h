#pragma once

#include <QObject>
#include <QTextObjectInterface>
#include <QTextCursor>
#include <QJsonObject>
#include <QCache>
#include <QImage>
#include <QFuture>
#include <QPointer>
#include "editorsettingsoptions.h"

class CustomDocument;
class QFrame;
class QLineEdit;
class QTextEdit;

// The SQLite representation is versioned text, never a pointer or an absolute path.
class ScreenshotBlocks : public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)
public:
    enum { ObjectType = QTextFormat::UserObject + 381, Metadata = QTextFormat::UserProperty + 381 };
    explicit ScreenshotBlocks(CustomDocument *editor);
    ~ScreenshotBlocks() override;
    QSizeF intrinsicSize(QTextDocument *, int, const QTextFormat &) override;
    void drawObject(QPainter *, const QRectF &, QTextDocument *, int, const QTextFormat &) override;
    static QString dataRoot();
    static QString marker(const QJsonObject &);
    static QJsonObject parseMarker(const QString &);
    static QString readableText(const QString &);
    static QString exportMarkdown(const QString &, const QString &directory, QString *error);
    static QString serialize(const QTextCursor &);
    static void insertSource(QTextCursor &, const QString &, bool cloneIds = false);
    void load(const QString &);
    void insertImage(const QImage &, const QString &file = QString());
    void chooseImage();
    void setTheme(Theme::Value, QColor foreground);
    void closeEditor();
    bool handleMouse(QMouseEvent *);
    void updateMetadata(const QTextCursor &, const QJsonObject &);
    QList<QTextCursor> cards() const;
    QRect cardRect(const QTextCursor &) const;
    int pendingJobs() const { return m_pending.size(); }
    void waitForWrites();
    QJsonObject metadata(const QTextCursor &) const;
    void removeCard(QTextCursor);
    void editCard(QTextCursor, bool description);
    bool descriptionOverflows(const QString &, qreal width) const;
signals:
    void imageError(const QString &message);
private:
    struct Result { QImage thumbnail; QString error; };
    void requestThumbnail(const QJsonObject &);
    void startJob(const QJsonObject &, const QImage &, const QString &, bool import);
    QColor surface(bool hovered) const;
    CustomDocument *m_editor;
    QCache<QString, QImage> m_thumbnails{32 * 1024}; // KiB, not original image count
    QSet<QString> m_pending;
    QSet<QString> m_missing;
    QList<QFuture<Result>> m_writes;
    QPointer<QFrame> m_popup;
    QPointer<QLineEdit> m_title;
    QTextCursor m_editCursor;
    QColor m_foreground{26,26,26};
    Theme::Value m_theme = Theme::Light;
};
