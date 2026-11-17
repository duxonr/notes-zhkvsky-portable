#pragma once

#include "3rdParty/qmarkdowntextedit/markdownhighlighter.h"
#include "editorsettingsoptions.h"

class CustomMarkdownHighlighter : public MarkdownHighlighter
{
public:
    explicit CustomMarkdownHighlighter(QTextDocument *parent, HighlightingOptions highlightingOptions = HighlightingOption::None);
    ~CustomMarkdownHighlighter() override = default;

protected:
    void highlightBlock(const QString &text) override;
public:
    void setFontSize(qreal fontSize);
    void setHeaderColors(QColor color);
    void setListsColor(QColor color);

    bool markdownEnabled() const { return m_markdownEnabled; }
    void setMarkdownEnabled(bool enabled) { m_markdownEnabled=enabled; rehighlight(); }
    void setTheme(Theme::Value theme, QColor textColor, qreal fontSize);
private:
    bool m_markdownEnabled=false;
};
