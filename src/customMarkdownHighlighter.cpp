#include "customMarkdownHighlighter.h"
#include "editorsettingsoptions.h"

CustomMarkdownHighlighter::CustomMarkdownHighlighter(QTextDocument *parent, HighlightingOptions highlightingOptions)
    : MarkdownHighlighter(parent, highlightingOptions)
{
    setListsColor(QColor(35, 131, 226)); // accent color

    _formats[static_cast<HighlighterState>(HighlighterState::HorizontalRuler)].clearBackground();
}

void CustomMarkdownHighlighter::setHeaderColors(QColor color)
{
    // set all header colors to the same color
    for (int i = HighlighterState::H1; i <= HighlighterState::H6; ++i) {
        _formats[static_cast<HighlighterState>(i)].setForeground(color);
    }
}

void CustomMarkdownHighlighter::setFontSize(qreal fontSize)
{
    // change font size for all formats
    for (auto &format : _formats) {
        format.setFontPointSize(fontSize);
    }

    // change header sizes
    for (int i = HighlighterState::H1; i <= HighlighterState::H6; ++i) {
        // H1 => size = fontSize * 1.6
        // H6 => size = fontSize * 1.1
        qreal size = fontSize * (1.6 - (i - HighlighterState::H1) * 0.1);
        _formats[static_cast<HighlighterState>(i)].setFontPointSize(size);
    }

    qreal codeBlockFontSize = fontSize - 4;
    _formats[static_cast<HighlighterState>(HighlighterState::InlineCodeBlock)].setFontPointSize(codeBlockFontSize);
}

void CustomMarkdownHighlighter::setListsColor(QColor color)
{
    _formats[static_cast<HighlighterState>(HighlighterState::CheckBoxUnChecked)].setForeground(color);
    _formats[static_cast<HighlighterState>(HighlighterState::CheckBoxChecked)].setForeground(QColor(90, 113, 140));
    _formats[static_cast<HighlighterState>(HighlighterState::List)].setForeground(color);
    _formats[static_cast<HighlighterState>(HighlighterState::BlockQuote)].setForeground(color);
}

void CustomMarkdownHighlighter::setTheme(Theme::Value theme, QColor textColor, qreal fontSize)
{
    setHeaderColors(textColor);
    setFontSize(fontSize);

    auto &icBlock = _formats[static_cast<HighlighterState>(HighlighterState::InlineCodeBlock)];
    switch (theme) {
    case Theme::Light:
        icBlock.setBackground(QColor(239, 241, 243));
        icBlock.setForeground(QColor(42, 46, 51));
        break;
    case Theme::Dark:
        icBlock.setBackground(QColor(52, 57, 66));
        icBlock.setForeground(QColor(230, 237, 243));
        break;
    case Theme::Sepia:
        icBlock.setBackground(QColor(239, 241, 243));
        icBlock.setForeground(QColor(42, 46, 51));
        break;
    }
}

void CustomMarkdownHighlighter::highlightBlock(const QString &text) {
    // Keep parser state for code fences. Basic toolbar styles are independent of
    // the optional full Markdown highlighting switch.
    MarkdownHighlighter::highlightBlock(text);
    if(!m_markdownEnabled)setFormat(0,text.size(),QTextCharFormat());
    if(isCodeBlock(currentBlockState()) || isCodeBlockEnd(currentBlockState()))return;
    const auto hide=[this](int start,int length,bool compact){QTextCharFormat style;style.setForeground(Qt::transparent);if(compact)style.setFontPointSize(1);setFormat(start,length,style);};
    {
        const auto insideCode=[&text](int position){auto spans=QRegularExpression(R"((`+)(.*?)\1)").globalMatch(text);while(spans.hasNext()){auto span=spans.next();if(position>=span.capturedStart()&&position<span.capturedEnd())return true;}return false;};
        const auto inlineStyle=[this,hide,insideCode,&text](const QRegularExpression &pattern,bool bold){auto matches=pattern.globalMatch(text);while(matches.hasNext()){auto match=matches.next();if(insideCode(match.capturedStart()))continue;for(int i=match.capturedStart(1);i<match.capturedEnd(1);++i){auto style=format(i);if(bold)style.setFontWeight(QFont::Bold);else style.setFontItalic(true);setFormat(i,1,style);}hide(match.capturedStart(),match.capturedStart(1)-match.capturedStart(),true);hide(match.capturedEnd(1),match.capturedEnd()-match.capturedEnd(1),true);}};
        inlineStyle(QRegularExpression(R"(\*\*([^*\n]+)\*\*)"),true);
        inlineStyle(QRegularExpression(R"((?<!\*)\*([^*\n]+)\*(?!\*))"),false);
        static const QRegularExpression underline("<u>(.*?)</u>");auto matches=underline.globalMatch(text);while(matches.hasNext()){auto match=matches.next();if(insideCode(match.capturedStart()))continue;for(int i=match.capturedStart(1);i<match.capturedEnd(1);++i){auto style=format(i);style.setFontUnderline(true);setFormat(i,1,style);}hide(match.capturedStart(),3,true);hide(match.capturedEnd()-4,4,true);}
        const auto bullet=QRegularExpression(R"(^((?:> )?\s*)[-+*]\s+(?!\[[ xX]\]))").match(text);
        if(bullet.hasMatch()){
            QTextCharFormat marker;marker.setForeground(Qt::transparent);marker.setFontLetterSpacingType(QFont::AbsoluteSpacing);marker.setFontLetterSpacing(8);
            setFormat(bullet.capturedLength(1),1,marker);
        }
        auto header=QRegularExpression(R"(^(?:> )?(#{1,6})\s+(.+)$)").match(text);if(header.hasMatch()){const auto key=static_cast<HighlighterState>(HighlighterState::H1+header.captured(1).size()-1);setFormat(header.capturedStart(2),header.capturedLength(2),_formats[key]);hide(header.capturedStart(1),header.capturedStart(2)-header.capturedStart(1),true);}
    }
    if(text.startsWith(">")){
        bool callout=false;for(auto block=currentBlock();block.isValid()&&block.text().startsWith('>');block=block.previous())if(block.text().startsWith("> [!NOTE]")){callout=true;break;}
        if(callout){if(text.startsWith("> [!NOTE]")){QTextCharFormat header;header.setForeground(Qt::transparent);header.setFontPointSize(11);setFormat(0,text.size(),header);}else hide(0,text.startsWith("> ")?2:1,true);}
    }
}
