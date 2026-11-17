#pragma once
#include <QPainter>
#include <QFontMetrics>
#include <QtMath>
namespace note_row {
inline int metadataWidth(int width) { return qBound(64, width / 3, 112); }
inline int dotColumnWidth(int width, int count) { return count == 0 ? 0 : qMin(qMax(1, (metadataWidth(width)-2) / 16), count) * 16 + 2; }
inline void paint(QPainter *p, double x, double y, double width, const QString &title,
                  const QString &date, const QString &content, const QString &folder,
                  bool showFolder, QFont titleFont, const QFont &textFont,
                  const QColor &titleColor, const QColor &dateColor, const QColor &contentColor,
                  const QImage &folderIcon)
{
    titleFont.setWeight(QFont::DemiBold);
    const QFontMetrics tf(titleFont), df(textFont);
    const int lineHeight = qMax(tf.height(), df.height());
    const QRectF header(x+20, y+10, qMax(0.0,width-40), lineHeight);
    const double metadataLeft = header.right() - metadataWidth(int(width));
    // Reserve the delete target even when hidden, so the date never jumps on hover.
    const int dateWidth = qMin(df.horizontalAdvance(date), qMax(0, metadataWidth(int(width))-24));
    const QRectF dateRect(header.right()-24-dateWidth, header.top(), dateWidth, lineHeight);
    const QRectF titleRect(header.left(),header.top(),qMax(0.0,metadataLeft-header.left()-10),lineHeight);
    p->setFont(titleFont);p->setPen(titleColor);
    p->drawText(titleRect,Qt::AlignLeft|Qt::AlignVCenter,tf.elidedText(title,Qt::ElideRight,int(titleRect.width())));
    QColor accent(128,128,128,90);p->setPen(QPen(accent,1));
    p->drawLine(QPointF(titleRect.left(),header.bottom()+3),QPointF(titleRect.left()+qMin(30.0,titleRect.width()),header.bottom()+3));
    p->setFont(textFont);p->setPen(dateColor);
    p->drawText(dateRect,Qt::AlignRight|Qt::AlignVCenter,df.elidedText(date,Qt::ElideRight,dateWidth));
    p->setPen(contentColor);
    const double contentY=header.bottom()+9;
    p->drawText(QRectF(x+20,contentY,qMax(0.0,metadataLeft-x-30),df.height()),Qt::AlignLeft|Qt::AlignVCenter,df.elidedText(content,Qt::ElideRight,qMax(0,int(metadataLeft-x-30))));
    if(showFolder){
        const double folderY=contentY+df.height()+6;
        p->drawImage(QRectF(x+20,folderY,16,16),folderIcon);
        p->drawText(QRectF(x+40,folderY,qMax(0.0,metadataLeft-x-50),qMax(16,df.height())),Qt::AlignLeft|Qt::AlignVCenter,df.elidedText(folder,Qt::ElideRight,qMax(0,int(metadataLeft-x-50))));
    }
}
}
