#ifndef CUSTOMDOCUMENT_H
#define CUSTOMDOCUMENT_H

#include <QtGui>
#include <QStringList>
#include <QTextEdit>

class ScreenshotBlocks;

class CustomDocument : public QTextEdit
{
    Q_OBJECT

public:
    explicit CustomDocument(QWidget *parent);
    QString toPlainText() const;
    void setText(const QString &text);
    ScreenshotBlocks *screenshots() const { return m_screenshots; }
    void setDocumentPadding(int left, int top, int right, int bottom);
    bool eventFilter(QObject *obj, QEvent *event) override;
    bool openLinkAtCursorPosition();
    QString getMarkdownUrlAtPosition(const QString &text, int position);
    bool isValidUrl(const QString &urlString);
    void openUrl(const QString &urlString);
    QMap<QString, QString> parseMarkdownUrlsFromText(const QString &text);
    QUrl getUrlUnderMouse();
    void moveBlockUp();
    void moveBlockDown();
signals:
    void resized();
    void mouseMoved();

    // QWidget interface
protected:
    bool canInsertFromMimeData(const QMimeData *source) const override;
    void insertFromMimeData(const QMimeData *source) override;
    QMimeData *createMimeDataFromSelection() const override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

    QStringList _ignoredClickUrlSchemata;
private:
    ScreenshotBlocks *m_screenshots;
    QWidget *m_calloutFrames=nullptr;
    void prepareTextInsertion();
};

#endif // CUSTOMDOCUMENT_H
