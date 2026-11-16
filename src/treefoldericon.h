#pragma once
#include <QPushButton>
#include <QPainter>
#include <QPainterPath>

inline void drawTreeFolderIcon(QPainter &p, const QRect &rect, QColor color, bool nested, bool halo)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    p.translate(rect.center().x() - 7, rect.center().y() - 7);
    QPainterPath path;
    path.moveTo(1, 3); path.lineTo(5, 3); path.lineTo(7, 5); path.lineTo(13, 5);
    path.lineTo(13, 12); path.lineTo(1, 12); path.closeSubpath();
    if (nested) { path.moveTo(4, 6); path.lineTo(4, 9); path.lineTo(8, 9); }
    p.setBrush(Qt::NoBrush);
    if (halo) {
        const QColor contrast = color.lightness() > 128 ? QColor(20,20,20) : QColor(250,250,250);
        p.setPen(QPen(contrast, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPath(path);
    }
    p.setPen(QPen(color, 1.3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPath(path);
    p.restore();
}

class TreeFolderIconButton : public QPushButton
{
public:
    explicit TreeFolderIconButton(bool nested, QWidget *parent) : QPushButton(parent), m_nested(nested) { }
    void setAppearance(QColor color, bool halo) {
        if (m_color != color || m_halo != halo) { m_color = color; m_halo = halo; update(); }
    }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); drawTreeFolderIcon(p, rect(), m_color, m_nested, m_halo);
    }
private:
    QColor m_color;
    bool m_nested;
    bool m_halo = false;
};
