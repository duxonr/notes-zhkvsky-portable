#pragma once
#include <QPushButton>
#include <QPainter>
#include <QEnterEvent>

// Keep the count's geometry fixed while exposing its existing context menu.
class RowCounterButton : public QPushButton
{
public:
    explicit RowCounterButton(QWidget *parent) : QPushButton(parent)
    {
        setFixedWidth(32);
        setFlat(true);
        setCursor(Qt::PointingHandCursor);
        setToolTip(tr("More actions"));
        setAccessibleName(tr("More actions"));
        QFont counterFont = font();
        counterFont.setPointSize(9);
        counterFont.setWeight(QFont::DemiBold);
        setFont(counterFont);
    }
protected:
    void enterEvent(QEnterEvent *event) override
    {
        m_hovered = true;
        update();
        QPushButton::enterEvent(event);
    }
    void leaveEvent(QEvent *event) override
    {
        m_hovered = false;
        update();
        QPushButton::leaveEvent(event);
    }
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QColor color = palette().color(QPalette::ButtonText);
        if (m_hovered) {
            p.setPen(Qt::NoPen);
            p.setBrush(color);
            const QPointF center = rect().center();
            for (int dx : {-5, 0, 5}) p.drawEllipse(center + QPointF(dx, 0), 1.2, 1.2);
        } else {
            p.setPen(color);
            p.setFont(font());
            p.drawText(rect(), Qt::AlignCenter, text());
        }
    }
private:
    bool m_hovered = false;
};
