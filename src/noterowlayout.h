#pragma once
#include <QPainter>
#include <QFontMetrics>
#include <QtMath>
namespace note_row {
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
    // Reserve the delete target even when hidden, so the date never jumps on hover.
    const int dateWidth = qMin(df.horizontalAdvance(date), qMax(0, int(header.width())-80));
    const QRectF dateRect(header.right()-24-dateWidth, header.top(), dateWidth, lineHeight);
    const QRectF titleRect(header.left(),header.top(),qMax(0.0,dateRect.left()-header.left()-12),lineHeight);
    p->setFont(titleFont);p->setPen(titleColor);
    p->drawText(titleRect,Qt::AlignLeft|Qt::AlignVCenter,tf.elidedText(title,Qt::ElideRight,int(titleRect.width())));
    QColor accent("#448ac9");accent.setAlpha(190);p->setPen(QPen(accent,1.5));
    p->drawLine(QPointF(titleRect.left(),header.bottom()+3),QPointF(titleRect.left()+qMin(30.0,titleRect.width()),header.bottom()+3));
    p->setFont(textFont);p->setPen(dateColor);
    p->drawText(dateRect,Qt::AlignRight|Qt::AlignVCenter,df.elidedText(date,Qt::ElideRight,dateWidth));
    p->setPen(contentColor);
    const double contentY=header.bottom()+9;
    p->drawText(QRectF(x+20,contentY,qMax(0.0,width-40),df.height()),Qt::AlignLeft|Qt::AlignVCenter,df.elidedText(content,Qt::ElideRight,qMax(0,int(width)-40)));
    if(showFolder){
        const double folderY=contentY+df.height()+6;
        p->drawImage(QRectF(x+20,folderY,16,16),folderIcon);
        p->drawText(QRectF(x+40,folderY,qMax(0.0,width-60),qMax(16,df.height())),Qt::AlignLeft|Qt::AlignVCenter,df.elidedText(folder,Qt::ElideRight,qMax(0,int(width)-60)));
    }
}
}
