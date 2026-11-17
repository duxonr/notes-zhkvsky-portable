#pragma once
#include <QPushButton>
#include <QPainter>
#include <QPainterPath>

// Vector icons avoid relying on glyphs missing from the bundled subset font.
enum class RowIcon { FolderPlus, Pencil, Trash, Palette };
class RowActionButton : public QPushButton
{
public:
    RowActionButton(RowIcon icon, QWidget *parent) : QPushButton(parent), m_icon(icon) { }
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QColor baseColor = palette().color(QPalette::ButtonText);
        if (underMouse()) {
            p.setPen(Qt::NoPen);
            p.setBrush(baseColor.lightness() > 128 ? QColor(255,255,255,28) : QColor(0,0,0,16));
            p.drawRoundedRect(rect().adjusted(1,3,-1,-3), 3, 3);
        }
        p.translate((width() - 14) / 2.0, (height() - 14) / 2.0);
        QColor color = baseColor;
        if (underMouse() && m_icon == RowIcon::Trash)
            color = baseColor.lightness() > 128 ? QColor("#ffb4ab") : QColor("#c93636");
        p.setPen(QPen(color, 1.3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        switch (m_icon) {
        case RowIcon::FolderPlus: {
            QPainterPath path;
            path.moveTo(1, 3); path.lineTo(5, 3); path.lineTo(7, 5); path.lineTo(13, 5);
            path.lineTo(13, 12); path.lineTo(1, 12); path.closeSubpath();
            p.drawPath(path); p.drawLine(7, 7, 7, 10); p.drawLine(5, 8, 9, 8); break;
        }
        case RowIcon::Pencil: {
            QPainterPath path;
            path.moveTo(2, 12); path.lineTo(3, 8); path.lineTo(10, 1); path.lineTo(13, 4);
            path.lineTo(6, 11); path.closeSubpath(); p.drawPath(path); p.drawLine(8, 3, 11, 6); break;
        }
        case RowIcon::Trash:
            p.drawLine(2, 3, 12, 3); p.drawLine(5, 1, 9, 1);
            p.drawRect(QRectF(3.5, 4, 7, 8)); p.drawLine(6, 6, 6, 10); p.drawLine(8, 6, 8, 10); break;
        case RowIcon::Palette: {
            QPainterPath path;
            path.moveTo(12, 9); path.cubicTo(14, 3, 9, 0, 5, 1);
            path.cubicTo(0, 2, 0, 9, 3, 12); path.cubicTo(5, 14, 9, 13, 8, 10);
            path.cubicTo(7, 8, 10, 8, 12, 9); p.drawPath(path);
            p.setBrush(color); p.drawEllipse(QPointF(4, 5), .7, .7);
            p.drawEllipse(QPointF(7, 3.5), .7, .7); p.drawEllipse(QPointF(10, 5), .7, .7); break;
        }
        }
    }
private:
    RowIcon m_icon;
};
