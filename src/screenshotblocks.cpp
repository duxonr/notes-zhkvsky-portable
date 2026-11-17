#include "screenshotblocks.h"
#include "customDocument.h"
#include "isolatedtest.h"
#include <QAbstractTextDocumentLayout>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextFragment>
#include <QPainter>
#include <QTextLayout>
#include <QImageReader>
#include <QSaveFile>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QDesktopServices>
#include <QLineEdit>
#include <QFrame>
#include <QVBoxLayout>
#include <QGuiApplication>
#include <QScreen>
#include <QMessageBox>
#include <QScrollBar>
#include <QMenu>
#include <QCryptographicHash>
#include <QStyle>
#include <QLabel>
#include <QClipboard>
#include <QMimeData>
#include <QBuffer>
#include <QTimer>
#include <QApplication>
#include <QProcess>

namespace {
// A compact card previews the start after a paste without moving the insertion cursor.
class CardDescriptionEdit final : public QTextEdit {
public:
    using QTextEdit::QTextEdit;
protected:
    void insertFromMimeData(const QMimeData *source) override {
        QTextEdit::insertFromMimeData(source);
        showBeginning();
        QTimer::singleShot(0,this,[this]{showBeginning();});
    }
private:
    void showBeginning(){verticalScrollBar()->setValue(0);horizontalScrollBar()->setValue(0);}
};
const QString prefix = QStringLiteral("[!notes-screenshot:v1:");
const QRegularExpression token(QStringLiteral("\\[!notes-screenshot:v1:([A-Za-z0-9_-]+)\\]"));
const QRegularExpression uuid(QStringLiteral("^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$"));
bool inCallout(QTextBlock block) {
    for(;block.isValid()&&block.text().startsWith('>');block=block.previous())if(block.text().startsWith("> [!NOTE]"))return true;
    return false;
}
QRectF photoOpenButton(const QRectF &card) {return QRectF(card.left()+8+qMin(120.0,card.width()*.25)-24,card.top()+36,24,24);}
QString imageFilter() {
    return QImageReader::supportedImageFormats().contains("webp")
        ? QObject::tr("Изображения (*.png *.jpg *.jpeg *.webp)") : QObject::tr("Изображения (*.png *.jpg *.jpeg)");
}
bool valid(const QJsonObject &o) {
    const auto id=o["id"].toString();
    const QString path=o["original"].toString();
    const QString asset=QFileInfo(path).completeBaseName();
    return uuid.match(id).hasMatch() && uuid.match(asset).hasMatch() && QRegularExpression("^Attachments/"+asset+"\\.(png|jpg|jpeg|webp)$").match(path).hasMatch()
        && o["thumbnail"].toString()=="Thumbnails/"+asset+".png" && o["title"].isString() && o["description"].isString();
}
QString safePath(const QString &relative) {
    if(relative.contains("..") || QDir::isAbsolutePath(relative)) return {};
    const auto root=ScreenshotBlocks::dataRoot();
    const auto path=QDir(root).filePath(relative);
    if(QFileInfo(root).isSymLink() || QFileInfo(QFileInfo(path).absolutePath()).isSymLink() || QFileInfo(path).isSymLink()) return {};
    return path;
}
bool writeImage(const QString &path,const QImage &image) {
    QSaveFile file(path);return !path.isEmpty() && file.open(QIODevice::WriteOnly) && image.save(&file,"PNG") && file.commit();
}
bool copyAtomic(const QString &source,const QString &target) {
    if(source==target)return QFileInfo::exists(source);
    QFile from(source);QSaveFile to(target);
    if(target.isEmpty() || !from.open(QIODevice::ReadOnly) || !to.open(QIODevice::WriteOnly))return false;
    while(!from.atEnd()) {auto bytes=from.read(1024*1024);if(bytes.isEmpty()&&from.error()!=QFile::NoError)return false;if(to.write(bytes)!=bytes.size())return false;}
    return to.commit();
}
QTextCharFormat objectFormat(const QJsonObject &o) {
    QTextCharFormat f;f.setObjectType(ScreenshotBlocks::ObjectType);f.setProperty(ScreenshotBlocks::Metadata,QJsonDocument(o).toJson(QJsonDocument::Compact));return f;
}
QString twoLines(const QString &text,const QFont &font,qreal width,bool *overflow=nullptr) {
    QString normalized=text;normalized.replace(QChar('\n'),QChar::LineSeparator);
    QTextLayout layout(normalized,font);
    layout.beginLayout();QString result;int consumed=0;
    for(int i=0;i<2;++i){auto line=layout.createLine();if(!line.isValid())break;line.setLineWidth(qMax(1.0,width));
        auto part=text.mid(line.textStart(),line.textLength()).trimmed();consumed=line.textStart()+line.textLength();
        if(i)result+='\n';if(i==1 && consumed<text.size())part=QFontMetrics(font).elidedText(part+QStringLiteral("…"),Qt::ElideRight,int(width));result+=part;
    }
    layout.endLayout();if(overflow)*overflow=consumed<text.size();return result;
}
}

QString ScreenshotBlocks::dataRoot() {
#ifdef NOTES_PORTABLE
    return QDir(portableRoot()).filePath("Data");
#else
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#endif
}
QString ScreenshotBlocks::marker(const QJsonObject &o) {
    return valid(o)?prefix+QString::fromLatin1(QJsonDocument(o).toJson(QJsonDocument::Compact).toBase64(QByteArray::Base64UrlEncoding|QByteArray::OmitTrailingEquals))+']':QString();
}
QJsonObject ScreenshotBlocks::parseMarker(const QString &text) {
    auto m=token.match(text);if(!m.hasMatch()||m.capturedLength()!=text.size())return {};
    auto o=QJsonDocument::fromJson(QByteArray::fromBase64(m.captured(1).toLatin1(),QByteArray::Base64UrlEncoding)).object();return valid(o)?o:QJsonObject();
}
QString ScreenshotBlocks::readableText(const QString &text) {
    QString result=text;auto it=token.globalMatch(text);int delta=0;
    while(it.hasNext()){auto m=it.next();auto o=parseMarker(m.captured());if(o.isEmpty())continue;
        QString replacement=o["title"].toString();if(replacement.isEmpty())replacement=tr("Скриншот");
        replacement+=' '+o["description"].toString().replace('\n',' ');
        result.replace(m.capturedStart()+delta,m.capturedLength(),replacement);delta+=replacement.size()-m.capturedLength();}
    return result;
}
QString ScreenshotBlocks::withoutCards(const QString &text) {
    QString result=text;auto it=token.globalMatch(text);int delta=0;
    while(it.hasNext()){auto m=it.next();if(parseMarker(m.captured()).isEmpty())continue;result.remove(m.capturedStart()+delta,m.capturedLength());delta-=m.capturedLength();}
    return result;
}
QString ScreenshotBlocks::serialize(const QTextCursor &selection) {
    auto *doc=selection.document();if(!doc)return {};
    const int start=selection.selectionStart(),end=selection.selectionEnd();QString out;
    for(auto block=doc->findBlock(start);block.isValid()&&block.position()<end;block=block.next()) {
        for(auto it=block.begin();!it.atEnd();++it){auto fragment=it.fragment();int a=qMax(start,fragment.position()),b=qMin(end,fragment.position()+fragment.length());if(a>=b)continue;
            auto text=fragment.text().mid(a-fragment.position(),b-a);
            if(fragment.charFormat().objectType()==ObjectType){auto o=QJsonDocument::fromJson(fragment.charFormat().property(Metadata).toByteArray()).object();text.replace(QChar::ObjectReplacementCharacter,marker(o));}
            out+=text;
        }
        if(block.position()+block.length()-1<end && block.next().isValid())out+='\n';
    }
    return out;
}
void ScreenshotBlocks::insertSource(QTextCursor &cursor,const QString &text,bool cloneIds) {
    auto it=token.globalMatch(text);int last=0;
    while(it.hasNext()){auto m=it.next();auto o=parseMarker(m.captured());if(o.isEmpty())continue;
        cursor.insertText(text.mid(last,m.capturedStart()-last),QTextCharFormat());
        if(cloneIds){o["id"]=QUuid::createUuid().toString(QUuid::WithoutBraces);if(cursor.positionInBlock())cursor.insertBlock(QTextBlockFormat(),QTextCharFormat());}
        cursor.insertText(QString(QChar::ObjectReplacementCharacter),objectFormat(o));
        if(cloneIds && (m.capturedEnd()==text.size() || text[m.capturedEnd()]!='\n'))cursor.insertBlock(QTextBlockFormat(),QTextCharFormat());last=m.capturedEnd();}
    cursor.insertText(text.mid(last),QTextCharFormat());
}
ScreenshotBlocks::ScreenshotBlocks(CustomDocument *editor):QObject(editor),m_editor(editor) {
    m_imagePool.setMaxThreadCount(2);
    editor->document()->documentLayout()->registerHandler(ObjectType,this);
    connect(editor->verticalScrollBar(),&QScrollBar::valueChanged,this,[this]{if(m_popup&&m_popup->isVisible())return;if(m_inlineDescription&&m_inlineDescription->hasFocus())return;closeEditor();});
    connect(editor,&CustomDocument::resized,this,[this]{
        m_editor->document()->markContentsDirty(0,m_editor->document()->characterCount());
        QTimer::singleShot(0,this,[this]{if(m_editCursor.isNull())return;const auto rect=cardRect(m_editCursor);const int left=rect.left()+qMin(120,int(rect.width()*.25))+20;
            if(m_inlineDescription)m_inlineDescription->setGeometry(left,rect.top()+36,qMax(24,rect.right()-left-38),52);
            if(m_title)m_title->setGeometry(left,rect.top()+10,qMax(24,rect.width()-qMin(120,int(rect.width()*.25))-140),26);
        });
    });
    connect(this,&ScreenshotBlocks::imageError,editor,[editor](const QString &error){QMessageBox::warning(editor,tr("Изображение не сохранено"),error);});
    connect(this,&ScreenshotBlocks::copyError,editor,[editor](const QString &error){QMessageBox::warning(editor,tr("Копирование изображения"),error);});
}
ScreenshotBlocks::~ScreenshotBlocks(){for(auto &f:m_writes)f.waitForFinished();}
bool ScreenshotBlocks::waitForWrites(){
    for(auto &f:m_writes)f.waitForFinished();
    for(auto it=m_imports.constBegin();it!=m_imports.constEnd();++it)if(!QFileInfo::exists(safePath(it->metadata["original"].toString()))){
        emit imageError(tr("Оригинал ещё не сохранён. Закрытие/экспорт отменены. Освободите место и нажмите на миниатюру для повторной попытки; исходное изображение пока остаётся в памяти программы."));return false;
    }
    return true;
}
void ScreenshotBlocks::load(const QString &text) {
    closeEditor();
    if(!text.contains(prefix)){m_editor->QTextEdit::setPlainText(text);return;}
    m_editor->QTextEdit::setPlainText(QString());
    auto c=m_editor->textCursor();c.beginEditBlock();insertSource(c,text);c.endEditBlock();m_editor->setTextCursor(QTextCursor(m_editor->document()));
    m_editor->document()->clearUndoRedoStacks();
}
QSizeF ScreenshotBlocks::intrinsicSize(QTextDocument *doc,int,const QTextFormat &) {
    return {qMax(180.0,doc->textWidth()-2*doc->documentMargin()-2),100};
}
QColor ScreenshotBlocks::surface(bool hovered) const {
    if(m_theme==Theme::Dark)return hovered?QColor(39,49,59):QColor(31,35,40);
    if(m_theme==Theme::Sepia)return hovered?QColor(237,224,198):QColor(244,233,211);
    return hovered?QColor(230,238,246):QColor(240,242,245);
}
void ScreenshotBlocks::setTheme(Theme::Value theme,QColor foreground){closeEditor();m_theme=theme;m_foreground=foreground;m_editor->viewport()->update();}
QJsonObject ScreenshotBlocks::metadata(const QTextCursor &c) const {
    auto cursor=c;cursor.setPosition(c.selectionStart());cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor);
    return QJsonDocument::fromJson(cursor.charFormat().property(Metadata).toByteArray()).object();
}
QList<QTextCursor> ScreenshotBlocks::cards() const {
    QList<QTextCursor> result;
    for(auto block=m_editor->document()->begin();block.isValid();block=block.next())for(auto it=block.begin();!it.atEnd();++it){auto f=it.fragment();if(f.charFormat().objectType()!=ObjectType)continue;
        for(int i=0;i<f.length();++i)if(f.text()[i]==QChar::ObjectReplacementCharacter){QTextCursor c(m_editor->document());c.setPosition(f.position()+i);c.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor);result<<c;}}
    return result;
}
QRect ScreenshotBlocks::cardRect(const QTextCursor &cursor) const {
    auto c=cursor;c.setPosition(cursor.selectionStart());auto r=m_editor->cursorRect(c);
    // An inline object's baseline is at its bottom, unlike a normal text cursor.
    return QRect(r.x(),r.bottom()-99,qMax(180,int(m_editor->document()->textWidth()-2*m_editor->document()->documentMargin()-2)),96);
}
void ScreenshotBlocks::drawObject(QPainter *p,const QRectF &rect,QTextDocument *,int position,const QTextFormat &format) {
    auto o=QJsonDocument::fromJson(format.property(Metadata).toByteArray()).object();if(!valid(o))return;
    const QString id=o["original"].toString();const bool hover=rect.translated(0,-m_editor->verticalScrollBar()->value()).contains(m_editor->viewport()->mapFromGlobal(QCursor::pos()));
    p->save();p->setRenderHint(QPainter::Antialiasing);QRectF r=rect.adjusted(0,2,0,-2);p->setPen(QPen(m_theme==Theme::Dark?QColor("#53575c"):QColor("#a5a5a5"),1));p->setBrush(Qt::NoBrush);p->drawRoundedRect(r.adjusted(.5,.5,-.5,-.5),4,4);
    const QRectF preview(r.left()+8,r.top()+8,qMin(120.0,r.width()*.25),80);
    if(auto image=m_thumbnails.object(id)){auto size=image->size().scaled(preview.size().toSize(),Qt::KeepAspectRatio);p->drawImage(QRectF(preview.center()-QPointF(size.width()/2.0,size.height()/2.0),size),*image);}
    else {p->setPen(m_foreground);p->drawText(preview,Qt::AlignCenter,m_missing.contains(id)?tr("Нет файла\nВосстановить"):tr("Загрузка…"));requestThumbnail(o);}
    auto font=m_editor->font();font.setPointSizeF(qMin(12.0,font.pointSizeF()>0?font.pointSizeF():11.0));font.setWeight(QFont::Bold);p->setFont(font);
    QRectF title(preview.right()+12,r.top()+10,qMax(24.0,r.right()-preview.right()-132),24);
    p->setPen(displayTitle(o).isEmpty()?QColor(m_foreground.red(),m_foreground.green(),m_foreground.blue(),145):m_foreground);
    p->drawText(title,Qt::AlignVCenter,QFontMetrics(font).elidedText(displayTitle(o).isEmpty()?tr("Название скриншота"):displayTitle(o),Qt::ElideRight,int(title.width())));
    font.setWeight(QFont::Normal);p->setFont(font);p->setPen(QColor(m_foreground.red(),m_foreground.green(),m_foreground.blue(),160));
    const auto description=o["description"].toString();bool overflow=false;auto summary=twoLines(description,font,title.width(),&overflow);
    if(!m_inlineDescription || m_editCursor.selectionStart()!=position)p->drawText(QRectF(title.left(),title.bottom()+3,title.width(),48),Qt::AlignLeft|Qt::AlignTop,description.isEmpty()?tr("Описание"):summary);
    if(hover){
        const auto button=photoOpenButton(rect);const auto mouse=m_editor->viewport()->mapFromGlobal(QCursor::pos());
        p->save();p->setPen(Qt::NoPen);p->setBrush(button.translated(0,-m_editor->verticalScrollBar()->value()).contains(mouse)?QColor(68,138,201,220):QColor(30,30,30,190));p->drawRoundedRect(button,3,3);
        p->translate(button.center()-QPointF(7,7));p->setPen(QPen(QColor("#eeeeee"),1.4,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
        p->drawLine(8,1,13,1);p->drawLine(13,1,13,6);p->drawLine(13,1,7,7);p->drawLine(6,13,1,13);p->drawLine(1,13,1,8);p->drawLine(1,13,7,7);p->restore();
    }
    if(hover || overflow)p->drawText(QRectF(r.right()-28,r.bottom()-29,24,24),Qt::AlignCenter,QStringLiteral("⌄"));
    if(hover || !m_dragCursor.isNull()){
        p->setPen(Qt::NoPen);p->setBrush(QColor(m_foreground.red(),m_foreground.green(),m_foreground.blue(),28));p->drawRoundedRect(QRectF(r.left()+2,r.top()+28,12,36),3,3);
        p->setBrush(m_foreground);for(int y=0;y<3;++y)for(int x=0;x<2;++x)p->drawEllipse(QPointF(r.left()+6+x*4,r.top()+37+y*7),1.25,1.25);
    }
    if(hover || (m_editor->textCursor().selectionStart()<=position && m_editor->textCursor().selectionEnd()>position)){
        const auto mouse=m_editor->viewport()->mapFromGlobal(QCursor::pos());
        for(int action=0;action<4;++action){
            const QRectF button(r.right()-28-action*28,r.top()+5,24,24);
            const bool active=button.translated(0,-m_editor->verticalScrollBar()->value()).contains(mouse);
            p->save();
            if(active){p->setPen(Qt::NoPen);p->setBrush(QColor(m_foreground.red(),m_foreground.green(),m_foreground.blue(),28));p->drawRoundedRect(button,3,3);}
            QColor ink=action==0&&active?QColor("#d16c6c"):m_foreground;
            if(action==1&&description.isEmpty())ink.setAlpha(80);
            p->translate(button.center()-QPointF(7,7));p->setPen(QPen(ink,1.4,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));p->setBrush(Qt::NoBrush);
            if(action==0){ // Trash: 14px, same stroke and canvas as its neighbors.
                p->drawLine(1,3,13,3);p->drawLine(5,1,9,1);p->drawRoundedRect(QRectF(3,4,8,9),1,1);p->drawLine(6,6,6,11);p->drawLine(8,6,8,11);
            }else if(action==1){ // Description copy.
                p->drawRoundedRect(QRectF(3,1,10,11),1,1);p->drawLine(1,4,1,14);p->drawLine(1,14,10,14);p->drawLine(6,4,10,4);p->drawLine(6,7,10,7);p->drawLine(6,10,9,10);
            }else if(action==2){ // Original image copy.
                p->drawRoundedRect(QRectF(3,1,10,11),1,1);p->drawLine(1,4,1,14);p->drawLine(1,14,10,14);p->drawEllipse(QPointF(6,4),.7,.7);p->drawPolyline(QPolygonF{QPointF(4,10),QPointF(7,7),QPointF(9,9),QPointF(11,7),QPointF(12,9)});
            }else { // Show original in its folder.
                QPainterPath folder;folder.moveTo(1,12);folder.lineTo(1,3);folder.lineTo(6,3);folder.lineTo(8,5);folder.lineTo(13,5);folder.lineTo(13,12);folder.closeSubpath();p->drawPath(folder);p->drawLine(2,6,12,6);
            }
            p->restore();
        }
    }
    p->restore();
}
void ScreenshotBlocks::startJob(const QJsonObject &o,const QImage &image,const QString &file,bool importing) {
    const QString id=o["original"].toString();if(m_pending.contains(id))return;m_pending.insert(id);m_missing.remove(id);
    const auto original=safePath(o["original"].toString()),thumb=safePath(o["thumbnail"].toString());
    if(importing)m_imports.insert(id,{image,file,o});
    auto future=QtConcurrent::run(&m_imagePool,[original,thumb,image,file,importing]{
        Result result;
        if(original.isEmpty()||thumb.isEmpty()||!QDir().mkpath(QFileInfo(original).absolutePath())||!QDir().mkpath(QFileInfo(thumb).absolutePath())){result.error=tr("Нет доступа к папке изображений.");return result;}
        if(importing){bool saved=file.isEmpty()?writeImage(original,image):copyAtomic(file,original);if(!saved){result.error=tr("Не удалось записать оригинал. Проверьте свободное место и права доступа. Карточка сохранена; нажмите на неё, чтобы восстановить файл.");return result;}}
        if(!importing && QFileInfo::exists(thumb)){QImageReader cached(thumb);const auto size=cached.size();if(size.isValid() && size.width()<=240 && size.height()<=160)result.thumbnail=cached.read();}
        if(result.thumbnail.isNull()){
            QImageReader reader(original);reader.setAutoTransform(true);auto size=reader.size();
            if(!size.isValid()||qint64(size.width())*size.height()>160000000){result.error=tr("Изображение отсутствует, повреждено или слишком велико. Нажмите на миниатюру для восстановления.");return result;}
            reader.setScaledSize(size.scaled(240,160,Qt::KeepAspectRatio));result.thumbnail=reader.read().scaled(240,160,Qt::KeepAspectRatio,Qt::SmoothTransformation);
            if(result.thumbnail.isNull()||!writeImage(thumb,result.thumbnail))result.error=tr("Не удалось создать миниатюру. Оригинал сохранён.");
        }
        return result;
    });
    if(importing){for(int i=m_writes.size()-1;i>=0;--i)if(m_writes[i].isFinished())m_writes.removeAt(i);m_writes<<future;}
    auto *watcher=new QFutureWatcher<Result>(this);
    connect(watcher,&QFutureWatcher<Result>::finished,this,[this,watcher,id,importing,original]{auto result=watcher->result();m_pending.remove(id);
        if(importing && QFileInfo::exists(original))m_imports.remove(id);
        if(!result.thumbnail.isNull())m_thumbnails.insert(id,new QImage(result.thumbnail),qMax(1,int(result.thumbnail.sizeInBytes()/1024)));
        if(!result.error.isEmpty()){m_missing.insert(id);if(importing)emit imageError(result.error);}
        m_editor->viewport()->update();watcher->deleteLater();});watcher->setFuture(future);
}
void ScreenshotBlocks::requestThumbnail(const QJsonObject &o){if(!m_missing.contains(o["original"].toString()))startJob(o,{},QString(),false);}
void ScreenshotBlocks::insertImage(const QImage &image,const QString &file) {
    if(m_editor->isReadOnly() || (file.isEmpty()&&image.isNull()))return;
    QString extension=file.isEmpty()?"png":QFileInfo(file).suffix().toLower();if(extension=="jpe")extension="jpg";
    if(!QStringList{"png","jpg","jpeg","webp"}.contains(extension)){emit imageError(tr("Поддерживаются PNG, JPEG и WebP."));return;}
    const QByteArray codec=(extension=="jpg"?QStringLiteral("jpeg"):extension).toLatin1();
    if(!file.isEmpty()&&!QImageReader::supportedImageFormats().contains(codec)){emit imageError(tr("Формат изображения не поддерживается этой сборкой Qt."));return;}
    QString id=QUuid::createUuid().toString(QUuid::WithoutBraces);
    QJsonObject o{{"id",id},{"original","Attachments/"+id+"."+extension},{"thumbnail","Thumbnails/"+id+".png"},{"title",""},{"description",""},{"created",QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}};
    startJob(o,image,file,true);
    closeEditor();auto c=m_editor->textCursor();const bool quoted=inCallout(c.block());c.beginEditBlock();c.removeSelectedText();
    if(c.positionInBlock()!=0)c.insertBlock(QTextBlockFormat(),QTextCharFormat());
    if(quoted)c.insertText("> ",QTextCharFormat());
    const int objectPosition=c.position();c.insertText(QString(QChar::ObjectReplacementCharacter),objectFormat(o));c.insertBlock(QTextBlockFormat(),QTextCharFormat());if(quoted&&!c.block().text().startsWith("> "))c.insertText("> ",QTextCharFormat());c.endEditBlock();m_editor->setTextCursor(c);
    QTextCursor inserted(m_editor->document());inserted.setPosition(objectPosition);inserted.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor);editDescriptionInline(inserted);
}
void ScreenshotBlocks::chooseImage(){if(m_editor->isReadOnly())return;const auto file=QFileDialog::getOpenFileName(m_editor,tr("Добавить изображение"),{},imageFilter());if(!file.isEmpty())insertImage({},file);}
bool ScreenshotBlocks::cancelDrag(){const bool active=!m_dragCursor.isNull();m_dragCursor=QTextCursor();m_dropPosition=-1;if(m_dropLine)m_dropLine->hide();return active;}
void ScreenshotBlocks::closeEditor(){
    cancelDrag();
    if(m_inlineDescription){auto *field=m_inlineDescription.data();m_inlineDescription=nullptr;field->setObjectName({});disconnect(field,nullptr,this,nullptr);field->hide();field->deleteLater();}
    if(m_title){auto *field=m_title.data();m_title=nullptr;field->setObjectName({});field->hide();field->deleteLater();}
    if(m_popup){auto *popup=m_popup.data();m_popup=nullptr;popup->setObjectName({});for(auto *field:popup->findChildren<QTextEdit*>()){field->setObjectName({});disconnect(field,nullptr,this,nullptr);}popup->close();popup->deleteLater();}
    m_editor->viewport()->update();
}
bool ScreenshotBlocks::eventFilter(QObject *object,QEvent *event) {
    if(object->property("screenshotField").toBool()) {
        if(event->type()==QEvent::ShortcutOverride){event->accept();return true;}
        if(event->type()==QEvent::KeyPress && static_cast<QKeyEvent*>(event)->key()==Qt::Key_Escape){closeEditor();m_editor->setFocus();return true;}
    }
    return QObject::eventFilter(object,event);
}
void ScreenshotBlocks::updateMetadata(const QTextCursor &selection,const QJsonObject &o){if(!valid(o)||!selection.hasSelection())return;auto c=selection;c.beginEditBlock();c.setCharFormat(objectFormat(o));c.endEditBlock();}
void ScreenshotBlocks::removeCard(QTextCursor c){closeEditor();c.beginEditBlock();c.removeSelectedText();c.endEditBlock();m_editor->setTextCursor(c);}
void ScreenshotBlocks::showNotice(const QString &text) {
    if(m_notice)delete m_notice.data();
    auto *notice=new QLabel(text,m_editor->viewport());m_notice=notice;notice->setObjectName("screenshotClipboardNotice");
    notice->setAttribute(Qt::WA_TransparentForMouseEvents);notice->setAttribute(Qt::WA_ShowWithoutActivating);
    auto font=m_editor->font();font.setPointSizeF(10);notice->setFont(font);
    notice->setStyleSheet(QString("background:%1;color:%2;padding:6px 10px;border-radius:4px;").arg(surface(true).name(),m_foreground.name()));notice->adjustSize();
    notice->move(qMax(8,m_editor->viewport()->width()-notice->width()-12),qMax(8,m_editor->viewport()->height()-notice->height()-12));notice->show();
    QTimer::singleShot(1800,notice,&QObject::deleteLater);emit clipboardNotice(text);
}
bool ScreenshotBlocks::revealImage(const QTextCursor &cursor) {
    const auto path=safePath(metadata(cursor)["original"].toString());
    if(path.isEmpty() || !QFileInfo(path).isFile()){showNotice(tr("Оригинал изображения отсутствует"));return false;}
#ifdef Q_OS_WIN
    const bool opened=QProcess::startDetached(QStringLiteral("explorer.exe"),{QStringLiteral("/select,"),QDir::toNativeSeparators(path)});
#else
    const bool opened=QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
#endif
    if(!opened)showNotice(tr("Не удалось открыть папку изображения"));
    return opened;
}
void ScreenshotBlocks::copyDescription(const QTextCursor &cursor) {
    const auto description=metadata(cursor)["description"].toString();
    if(description.isEmpty()){showNotice(tr("Описание отсутствует"));return;}
    publishClipboard({}, {}, description, ++m_clipboardGeneration);
}
void ScreenshotBlocks::publishClipboard(QImage image,QByteArray png,QString text,quint64 generation,int attempt) {
    if(attempt==0)++m_copyJobs;
    if(generation!=m_clipboardGeneration){--m_copyJobs;return;}
    auto *clipboard=QGuiApplication::clipboard();
    if(image.isNull())clipboard->setText(text);
    else {auto *mime=new QMimeData;mime->setImageData(image);mime->setData("image/png",png);clipboard->setMimeData(mime);}
    QTimer::singleShot(60,this,[this,image,png,text,generation,attempt]{
        if(generation!=m_clipboardGeneration){--m_copyJobs;return;}
        auto *clipboard=QGuiApplication::clipboard();auto *mime=clipboard->mimeData();
        const bool matches=image.isNull() ? clipboard->text().replace("\r\n","\n")==QString(text).replace("\r\n","\n")&&!mime->hasImage()
            : mime->hasImage()&&!mime->hasText()&&mime->data("image/png")==png;
        if(matches){--m_copyJobs;showNotice(image.isNull()?tr("Описание скопировано"):tr("Изображение скопировано"));return;}
        if(attempt<9){publishClipboard(image,png,text,generation,attempt+1);return;}
        --m_copyJobs;emit copyError(tr("Буфер обмена занят другим приложением. Повторите копирование."));
    });
}
void ScreenshotBlocks::copyImage(const QTextCursor &cursor) {
    const auto original=safePath(metadata(cursor)["original"].toString());
    const auto generation=++m_clipboardGeneration;++m_copyJobs;
    showNotice(tr("Копирование…"));
    auto future=QtConcurrent::run(&m_imagePool,[original]{
        Original result;QImageReader reader(original);reader.setAutoTransform(false);const auto size=reader.size();
        if(original.isEmpty()||!size.isValid()||qint64(size.width())*size.height()>160000000){result.error=tr("Оригинал отсутствует, повреждён или превышает допустимый размер. Буфер обмена не изменён.");return result;}
        result.image=reader.read();
        if(result.image.isNull()){result.error=tr("Не удалось прочитать оригинальное изображение. Буфер обмена не изменён.");return result;}
        QBuffer buffer(&result.png);buffer.open(QIODevice::WriteOnly);
        if(!result.image.save(&buffer,"PNG")){result.error=tr("Не удалось подготовить изображение для буфера обмена.");result.image={};}
        return result;
    });
    auto *watcher=new QFutureWatcher<Original>(this);
    connect(watcher,&QFutureWatcher<Original>::finished,this,[this,watcher,generation]{
        const auto result=watcher->result();--m_copyJobs;watcher->deleteLater();
        if(generation!=m_clipboardGeneration)return;
        if(!result.error.isEmpty()){emit copyError(result.error);return;}
        publishClipboard(result.image,result.png,{},generation);
    });watcher->setFuture(future);
}
bool ScreenshotBlocks::descriptionOverflows(const QString &text,qreal width) const {bool overflow=false;auto font=m_editor->font();font.setPointSizeF(qMin(12.0,font.pointSizeF()>0?font.pointSizeF():11.0));twoLines(text,font,width,&overflow);return overflow;}
void ScreenshotBlocks::editCard(QTextCursor c,bool description) {
    closeEditor();m_editCursor=c;auto o=metadata(c);if(!valid(o))return;const auto rect=cardRect(c);
    QString style=QString("background:%1;color:%2;border:1px solid #448ac9;selection-background-color:#448ac9;").arg(surface(false).name(),m_foreground.name());
    if(!description){auto *line=new QLineEdit(o["title"].toString(),m_editor->viewport());m_title=line;line->setObjectName("screenshotTitleEdit");auto titleFont=m_editor->font();titleFont.setBold(true);line->setFont(titleFont);line->setPlaceholderText(displayTitle(o).isEmpty()?tr("Название скриншота"):displayTitle(o));line->setStyleSheet(style);line->setGeometry(rect.left()+qMin(120,int(rect.width()*.25))+20,rect.top()+10,qMax(24,rect.width()-qMin(120,int(rect.width()*.25))-140),26);
        line->setProperty("screenshotField",true);line->installEventFilter(this);
        connect(line,&QLineEdit::textEdited,this,[this,c](const QString &value){auto o=metadata(c);o["title"]=value;updateMetadata(c,o);});connect(line,&QLineEdit::returnPressed,line,&QWidget::hide);connect(line,&QLineEdit::editingFinished,line,&QWidget::hide);line->show();line->setFocus();line->selectAll();
    }else{auto *popup=new QFrame(m_editor,Qt::Popup|Qt::FramelessWindowHint);m_popup=popup;popup->setObjectName("screenshotDescriptionPopup");popup->setAttribute(Qt::WA_DeleteOnClose);popup->setStyleSheet(QString("QFrame#screenshotDescriptionPopup{background:%1;border:1px solid #777;}").arg(surface(false).name()));auto *layout=new QVBoxLayout(popup);layout->setContentsMargins(8,8,8,8);auto *edit=new QTextEdit(popup);edit->setAcceptRichText(false);edit->setObjectName("screenshotDescriptionEdit");edit->setPlaceholderText(tr("Описание скриншота"));edit->setStyleSheet(QString("QTextEdit{background:%1;color:%2;border:none;selection-background-color:#448ac9;}").arg(surface(false).name(),m_foreground.name()));edit->setPlainText(o["description"].toString());layout->addWidget(edit);
        connect(edit,&QTextEdit::textChanged,this,[this,c,edit]{auto o=metadata(c);o["description"]=edit->toPlainText();updateMetadata(c,o);});
        edit->setProperty("screenshotField",true);edit->installEventFilter(this);edit->setFont(m_editor->font());
        QFile scrollStyle(":/styles/components/custom-scrollbar.css");if(scrollStyle.open(QIODevice::ReadOnly))edit->verticalScrollBar()->setStyleSheet(QString::fromUtf8(scrollStyle.readAll()));
        edit->verticalScrollBar()->setProperty("class",m_theme==Theme::Dark?"dark":m_theme==Theme::Sepia?"sepia":"light");
        edit->verticalScrollBar()->style()->unpolish(edit->verticalScrollBar());edit->verticalScrollBar()->style()->polish(edit->verticalScrollBar());
        auto screen=m_editor->screen()->availableGeometry();const QSize size(qMin(420,screen.width()-16),qMin(280,screen.height()-32));auto pos=m_editor->viewport()->mapToGlobal(rect.bottomLeft());pos.setX(qBound(screen.left()+8,pos.x(),screen.right()-size.width()-8));pos.setY(qBound(screen.top()+8,pos.y(),screen.bottom()-size.height()-8));popup->resize(size);popup->move(pos);popup->show();edit->setFocus();}
}
QString ScreenshotBlocks::displayTitle(const QJsonObject &o) {
    if(!o["title"].toString().trimmed().isEmpty())return o["title"].toString();
    for(const auto &line:o["description"].toString().split('\n'))if(!line.trimmed().isEmpty())return line.trimmed().left(160);
    return {};
}
void ScreenshotBlocks::editDescriptionInline(QTextCursor c) {
    closeEditor();m_editCursor=c;const auto o=metadata(c);if(!valid(o))return;
    auto *edit=new CardDescriptionEdit(m_editor->viewport());m_inlineDescription=edit;edit->setObjectName("screenshotInlineDescription");edit->setAcceptRichText(false);auto font=m_editor->font();font.setPointSizeF(qMin(12.0,font.pointSizeF()>0?font.pointSizeF():11.0));edit->setFont(font);edit->setPlaceholderText(tr("Описание"));
    edit->setStyleSheet(QString("QTextEdit{background:transparent;color:%1;border:none;selection-background-color:#448ac9;}").arg(QColor(m_foreground.red(),m_foreground.green(),m_foreground.blue(),160).name(QColor::HexArgb)));
    const auto rect=cardRect(c);const int left=rect.left()+qMin(120,int(rect.width()*.25))+20;
    edit->setGeometry(left,rect.top()+36,qMax(24,rect.right()-left-38),52);edit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);edit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);edit->setPlainText(o["description"].toString());
    connect(edit,&QTextEdit::textChanged,this,[this,c,edit]{auto value=metadata(c);value["description"]=edit->toPlainText();updateMetadata(c,value);});
    edit->setProperty("screenshotField",true);edit->installEventFilter(this);edit->show();edit->setFocus();
}
void ScreenshotBlocks::moveCard(QTextCursor c,int destination,bool intoCallout) {
    if(destination>=c.selectionStart()&&destination<=c.selectionEnd())return;
    closeEditor();QTextCursor target(m_editor->document());target.setPosition(qBound(0,destination,m_editor->document()->characterCount()-1));
    intoCallout=intoCallout||(inCallout(target.block())&&!target.block().text().startsWith("> [!NOTE]"));
    const auto fragment=c.selection();c.beginEditBlock();c.removeSelectedText();
    if(target.positionInBlock())target.insertBlock(QTextBlockFormat(),QTextCharFormat());
    if(intoCallout)target.insertText("> ",QTextCharFormat());
    target.insertFragment(fragment);target.insertBlock(QTextBlockFormat(),QTextCharFormat());c.endEditBlock();m_editor->setTextCursor(target);
}
bool ScreenshotBlocks::handleMouse(QMouseEvent *event) {
    if(m_editor->isReadOnly())return false;
    if(!m_dragCursor.isNull()){
        if(event->type()==QEvent::MouseMove){
            m_editor->viewport()->setCursor(Qt::ClosedHandCursor);
            auto target=m_editor->cursorForPosition(event->pos());auto block=target.block();QTextCursor start(block);
            const auto bounds=m_editor->document()->documentLayout()->blockBoundingRect(block);
            const int top=qFloor(bounds.top())-m_editor->verticalScrollBar()->value();const int height=qCeil(bounds.height());
            const bool after=event->pos().y()>top+height/2;m_dropPosition=after?(block.next().isValid()?block.next().position():m_editor->document()->characterCount()-1):block.position();
            m_dropInCallout=inCallout(block)&&(after||!block.text().startsWith("> [!NOTE]"));
            if(!m_dropLine){m_dropLine=new QLabel(m_editor->viewport());m_dropLine->setObjectName("screenshotDropIndicator");m_dropLine->setAttribute(Qt::WA_TransparentForMouseEvents);m_dropLine->setStyleSheet("background:#448ac9;border:none;");}
            m_dropLine->setGeometry(int(m_editor->document()->documentMargin()),after?top+height:top,qMax(24,int(m_editor->document()->textWidth()-2*m_editor->document()->documentMargin()-2)),3);m_dropLine->show();m_dropLine->raise();return true;
        }
        if(event->type()==QEvent::MouseButtonRelease){auto source=m_dragCursor;m_dragCursor=QTextCursor();if(m_dropLine)m_dropLine->hide();if(m_dropPosition>=0&&(event->pos()-m_dragStart).manhattanLength()>QApplication::startDragDistance())moveCard(source,m_dropPosition,m_dropInCallout);m_dropPosition=-1;return true;}
    }
    // Only inspect visible paragraphs, independent of total card count.
    auto block=m_editor->cursorForPosition(QPoint(0,event->pos().y())).block();
    for(int d=-1;d<=1;++d){auto b=d<0?block.previous():d>0?block.next():block;if(!b.isValid())continue;
        for(auto it=b.begin();!it.atEnd();++it){auto f=it.fragment();if(f.charFormat().objectType()!=ObjectType)continue;QTextCursor c(m_editor->document());c.setPosition(f.position());c.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor);auto r=cardRect(c);if(!r.contains(event->pos()))continue;
            const bool actions=event->pos().x()>r.right()-114 && event->pos().y()<r.top()+34;
            const bool expand=event->pos().x()>r.right()-32&&event->pos().y()>r.bottom()-32;
            const bool grip=event->pos().x()<r.left()+16;
            const bool image=event->pos().x()<r.left()+qMin(120,int(r.width()*.25))+16;
            const bool openPhoto=photoOpenButton(r).contains(event->pos());
            if(event->type()==QEvent::MouseButtonPress && event->button()==Qt::LeftButton && grip){closeEditor();m_dragCursor=c;m_dragStart=event->pos();return true;}
            if(event->type()==QEvent::MouseMove){
                QString tooltip=image?QString():tr("Изменить карточку");
                if(openPhoto)tooltip=tr("Открыть фотографию");
                if(actions)tooltip=event->pos().x()>r.right()-30?tr("Удалить скриншот"):event->pos().x()>r.right()-58?tr("Копировать описание"):event->pos().x()>r.right()-86?tr("Копировать изображение"):tr("Показать в Проводнике");
                if(grip)tooltip=tr("Переместить блок");else if(expand)tooltip=tr("Открыть полное описание");
                m_editor->viewport()->setCursor(grip?Qt::OpenHandCursor:(actions||expand||openPhoto)?Qt::PointingHandCursor:image?Qt::ArrowCursor:Qt::IBeamCursor);m_editor->viewport()->setToolTip(tooltip);m_editor->viewport()->update();return true;
            }
            if(event->type()==QEvent::MouseButtonPress && event->button()==Qt::LeftButton && (actions||image||expand))return true;
            if(event->type()!=QEvent::MouseButtonRelease||event->button()!=Qt::LeftButton)return false;
            auto o=metadata(c);if(actions){
                if(event->pos().x()>r.right()-30)removeCard(c);
                else if(event->pos().x()>r.right()-58)copyDescription(c);
                else if(event->pos().x()>r.right()-86)copyImage(c);
                else revealImage(c);
            }
            else if(openPhoto){const QString original=safePath(o["original"].toString());if(original.isEmpty()||!QFileInfo::exists(original)){const auto retry=m_imports.value(o["original"].toString());if(!retry.image.isNull()||!retry.file.isEmpty())startJob(o,retry.image,retry.file,true);else {const auto file=QFileDialog::getOpenFileName(m_editor,tr("Восстановить оригинал"),{},imageFilter());if(!file.isEmpty())startJob(o,{},file,true);}}else if(!QDesktopServices::openUrl(QUrl::fromLocalFile(original)))QMessageBox::warning(m_editor,tr("Просмотр изображения"),tr("Не удалось открыть изображение в стандартной программе Windows."));}
            else if(image)return true;
            else if(expand)editCard(c,true);else if(event->pos().y()>r.top()+36){editDescriptionInline(c);if(m_inlineDescription)m_inlineDescription->setTextCursor(m_inlineDescription->cursorForPosition(event->pos()-m_inlineDescription->pos()));}else editCard(c,false);return true;
        }
    }
    return false;
}
QString ScreenshotBlocks::exportMarkdown(const QString &source,const QString &directory,QString *error) {
    QString result=source;auto it=token.globalMatch(source);int delta=0;
    while(it.hasNext()){auto m=it.next();auto o=parseMarker(m.captured());if(o.isEmpty())continue;auto original=safePath(o["original"].toString());auto relative=o["original"].toString();auto target=QDir(directory).filePath(relative);
        if(!QDir().mkpath(QFileInfo(target).absolutePath())||(!QFileInfo::exists(target)&&!copyAtomic(original,target))){if(error)*error=tr("Не удалось экспортировать оригинал %1").arg(relative);return {};}
        QString title=o["title"].toString();title.replace(']',' ');QString replacement="!["+title+"]("+relative+")";if(!o["description"].toString().isEmpty())replacement+="\n\n"+o["description"].toString();
        result.replace(m.capturedStart()+delta,m.capturedLength(),replacement);delta+=replacement.size()-m.capturedLength();}
    return result;
}

bool ScreenshotBlocks::copyAssets(const QString &fromData,const QString &toData,QString *error) {
    for(const auto &folder:{QStringLiteral("Attachments"),QStringLiteral("Thumbnails")}) {
        QDir source(QDir(fromData).filePath(folder));
        if(!source.exists())continue;
        QDir target(QDir(toData).filePath(folder));
        if(QFileInfo(source.path()).isSymLink() || QFileInfo(target.path()).isSymLink() || !QDir().mkpath(target.path())){if(error)*error=tr("Нет доступа к папке изображений резервной копии.");return false;}
        for(const auto &name:source.entryList(QDir::Files)) {
            const auto from=source.filePath(name),to=target.filePath(name);
            if(QFileInfo(from).isSymLink()||QFileInfo(to).isSymLink()){if(error)*error=tr("Символические ссылки в папке изображений не поддерживаются.");return false;}
            if(QFileInfo::exists(to)){
                QFile a(from),b(to);if(!a.open(QIODevice::ReadOnly)||!b.open(QIODevice::ReadOnly)){if(error)*error=tr("Не удалось проверить изображение.");return false;}
                QCryptographicHash ah(QCryptographicHash::Sha256),bh(QCryptographicHash::Sha256);ah.addData(&a);bh.addData(&b);
                if(ah.result()!=bh.result()){if(error)*error=tr("Конфликт файла %1. Существующий оригинал не перезаписан.").arg(name);return false;}
            }else if(!copyAtomic(from,to)){if(error)*error=tr("Не удалось скопировать изображение %1.").arg(name);return false;}
        }
    }
    return true;
}
