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

namespace {
const QString prefix = QStringLiteral("[!notes-screenshot:v1:");
const QRegularExpression token(QStringLiteral("\\[!notes-screenshot:v1:([A-Za-z0-9_-]+)\\]"));
const QRegularExpression uuid(QStringLiteral("^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$"));
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
    editor->document()->documentLayout()->registerHandler(ObjectType,this);
    connect(editor->verticalScrollBar(),&QScrollBar::valueChanged,this,[this]{closeEditor();});
    connect(editor,&CustomDocument::resized,this,[this]{closeEditor();m_editor->document()->markContentsDirty(0,m_editor->document()->characterCount());});
    connect(this,&ScreenshotBlocks::imageError,editor,[editor](const QString &error){QMessageBox::warning(editor,tr("Изображение не сохранено"),error);});
}
ScreenshotBlocks::~ScreenshotBlocks(){waitForWrites();}
void ScreenshotBlocks::waitForWrites(){for(auto &f:m_writes)f.waitForFinished();}
void ScreenshotBlocks::load(const QString &text) {
    closeEditor();m_editor->QTextEdit::setPlainText(QString());
    auto c=m_editor->textCursor();insertSource(c,text);m_editor->setTextCursor(QTextCursor(m_editor->document()));
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
    p->save();QRectF r=rect.adjusted(0,2,0,-2);p->setPen(Qt::NoPen);p->setBrush(surface(hover));p->drawRoundedRect(r,4,4);
    const QRectF preview(r.left()+8,r.top()+8,qMin(120.0,r.width()*.30),80);
    if(auto image=m_thumbnails.object(id)){auto size=image->size().scaled(preview.size().toSize(),Qt::KeepAspectRatio);p->drawImage(QRectF(preview.center()-QPointF(size.width()/2.0,size.height()/2.0),size),*image);}
    else {p->setPen(m_foreground);p->drawText(preview,Qt::AlignCenter,m_missing.contains(id)?tr("Нет файла\nВосстановить"):tr("Загрузка…"));requestThumbnail(o);}
    auto font=m_editor->font();font.setPointSizeF(qMin(12.0,font.pointSizeF()>0?font.pointSizeF():11.0));font.setWeight(QFont::DemiBold);p->setFont(font);
    QRectF title(preview.right()+12,r.top()+10,qMax(24.0,r.right()-preview.right()-48),24);
    p->setPen(o["title"].toString().isEmpty()?QColor(m_foreground.red(),m_foreground.green(),m_foreground.blue(),145):m_foreground);
    p->drawText(title,Qt::AlignVCenter,QFontMetrics(font).elidedText(o["title"].toString().isEmpty()?tr("Название скриншота"):o["title"].toString(),Qt::ElideRight,int(title.width())));
    font.setWeight(QFont::Normal);p->setFont(font);p->setPen(QColor(m_foreground.red(),m_foreground.green(),m_foreground.blue(),160));
    const auto description=o["description"].toString();bool overflow=false;auto summary=twoLines(description,font,title.width(),&overflow);
    p->drawText(QRectF(title.left(),title.bottom()+3,title.width(),48),Qt::AlignLeft|Qt::AlignTop,description.isEmpty()?tr("Описание"):summary);
    if(overflow)p->drawText(QRectF(r.right()-28,r.bottom()-29,24,24),Qt::AlignCenter,QStringLiteral("⌄"));
    QTextCursor c(m_editor->document());c.setPosition(position);
    if(hover || m_editor->textCursor().selectionStart()<=position && m_editor->textCursor().selectionEnd()>position){
        p->setPen(m_foreground);p->drawText(QRectF(r.right()-28,r.top()+5,24,24),Qt::AlignCenter,QStringLiteral("×"));}
    p->restore();
}
void ScreenshotBlocks::startJob(const QJsonObject &o,const QImage &image,const QString &file,bool importing) {
    const QString id=o["original"].toString();if(m_pending.contains(id))return;m_pending.insert(id);m_missing.remove(id);
    const auto original=safePath(o["original"].toString()),thumb=safePath(o["thumbnail"].toString());
    auto future=QtConcurrent::run([original,thumb,image,file,importing]{
        Result result;
        if(original.isEmpty()||thumb.isEmpty()||!QDir().mkpath(QFileInfo(original).absolutePath())||!QDir().mkpath(QFileInfo(thumb).absolutePath())){result.error=tr("Нет доступа к папке изображений.");return result;}
        if(importing){bool saved=file.isEmpty()?writeImage(original,image):copyAtomic(file,original);if(!saved){result.error=tr("Не удалось записать оригинал. Проверьте свободное место и права доступа. Карточка сохранена; нажмите на неё, чтобы восстановить файл.");return result;}}
        if(!importing && QFileInfo::exists(thumb)){QImageReader cached(thumb);result.thumbnail=cached.read();}
        if(result.thumbnail.isNull()){
            QImageReader reader(original);reader.setAutoTransform(true);auto size=reader.size();
            if(!size.isValid()||qint64(size.width())*size.height()>160000000){result.error=tr("Изображение отсутствует, повреждено или слишком велико. Нажмите на миниатюру для восстановления.");return result;}
            reader.setScaledSize(size.scaled(240,160,Qt::KeepAspectRatio));result.thumbnail=reader.read().scaled(240,160,Qt::KeepAspectRatio,Qt::SmoothTransformation);
            if(result.thumbnail.isNull()||!writeImage(thumb,result.thumbnail))result.error=tr("Не удалось создать миниатюру. Оригинал сохранён.");
        }
        return result;
    });
    if(importing)m_writes<<future;
    auto *watcher=new QFutureWatcher<Result>(this);
    connect(watcher,&QFutureWatcher<Result>::finished,this,[this,watcher,id,importing]{auto result=watcher->result();m_pending.remove(id);
        if(!result.thumbnail.isNull())m_thumbnails.insert(id,new QImage(result.thumbnail),qMax(1,int(result.thumbnail.sizeInBytes()/1024)));
        if(!result.error.isEmpty()){m_missing.insert(id);if(importing)emit imageError(result.error);}
        m_editor->viewport()->update();watcher->deleteLater();});watcher->setFuture(future);
}
void ScreenshotBlocks::requestThumbnail(const QJsonObject &o){if(!m_missing.contains(o["original"].toString()))startJob(o,{},QString(),false);}
void ScreenshotBlocks::insertImage(const QImage &image,const QString &file) {
    if(m_editor->isReadOnly() || (file.isEmpty()&&image.isNull()))return;
    QString extension=file.isEmpty()?"png":QFileInfo(file).suffix().toLower();if(extension=="jpe")extension="jpg";
    if(!QStringList{"png","jpg","jpeg","webp"}.contains(extension)){emit imageError(tr("Поддерживаются PNG, JPEG и WebP."));return;}
    QString id=QUuid::createUuid().toString(QUuid::WithoutBraces);
    QJsonObject o{{"id",id},{"original","Attachments/"+id+"."+extension},{"thumbnail","Thumbnails/"+id+".png"},{"title",""},{"description",""},{"created",QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}};
    closeEditor();auto c=m_editor->textCursor();c.beginEditBlock();c.removeSelectedText();
    if(c.positionInBlock()!=0)c.insertBlock(QTextBlockFormat(),QTextCharFormat());
    c.insertText(QString(QChar::ObjectReplacementCharacter),objectFormat(o));c.insertBlock(QTextBlockFormat(),QTextCharFormat());c.endEditBlock();m_editor->setTextCursor(c);
    startJob(o,image,file,true);
}
void ScreenshotBlocks::chooseImage(){if(m_editor->isReadOnly())return;const auto file=QFileDialog::getOpenFileName(m_editor,tr("Добавить изображение"),{},tr("Изображения (*.png *.jpg *.jpeg *.webp)"));if(!file.isEmpty())insertImage({},file);}
void ScreenshotBlocks::closeEditor(){if(m_title){m_title->clearFocus();delete m_title.data();}if(m_popup){m_popup->close();delete m_popup.data();}}
void ScreenshotBlocks::updateMetadata(const QTextCursor &selection,const QJsonObject &o){if(!valid(o)||!selection.hasSelection())return;auto c=selection;c.beginEditBlock();c.setCharFormat(objectFormat(o));c.endEditBlock();}
void ScreenshotBlocks::removeCard(QTextCursor c){closeEditor();c.beginEditBlock();c.removeSelectedText();c.endEditBlock();m_editor->setTextCursor(c);}
bool ScreenshotBlocks::descriptionOverflows(const QString &text,qreal width) const {bool overflow=false;auto font=m_editor->font();font.setPointSizeF(qMin(12.0,font.pointSizeF()>0?font.pointSizeF():11.0));twoLines(text,font,width,&overflow);return overflow;}
void ScreenshotBlocks::editCard(QTextCursor c,bool description) {
    closeEditor();m_editCursor=c;auto o=metadata(c);if(!valid(o))return;const auto rect=cardRect(c);
    QString style=QString("background:%1;color:%2;border:1px solid #448ac9;selection-background-color:#448ac9;").arg(surface(false).name(),m_foreground.name());
    if(!description){auto *line=new QLineEdit(o["title"].toString(),m_editor->viewport());m_title=line;line->setObjectName("screenshotTitleEdit");line->setPlaceholderText(tr("Название скриншота"));line->setStyleSheet(style);line->setGeometry(rect.left()+qMin(120,int(rect.width()*.30))+20,rect.top()+10,qMax(24,rect.width()-qMin(120,int(rect.width()*.30))-56),26);
        connect(line,&QLineEdit::textEdited,this,[this,c](const QString &value){auto o=metadata(c);o["title"]=value;updateMetadata(c,o);});connect(line,&QLineEdit::returnPressed,line,&QWidget::hide);connect(line,&QLineEdit::editingFinished,line,&QWidget::hide);line->show();line->setFocus();line->selectAll();
    }else{auto *popup=new QFrame(m_editor,Qt::Popup|Qt::FramelessWindowHint);m_popup=popup;popup->setObjectName("screenshotDescriptionPopup");popup->setAttribute(Qt::WA_DeleteOnClose);popup->setStyleSheet("QFrame#screenshotDescriptionPopup{"+style+"}");auto *layout=new QVBoxLayout(popup);layout->setContentsMargins(8,8,8,8);auto *edit=new QTextEdit(popup);edit->setAcceptRichText(false);edit->setObjectName("screenshotDescriptionEdit");edit->setPlaceholderText(tr("Описание скриншота"));edit->setStyleSheet(style);edit->setPlainText(o["description"].toString());layout->addWidget(edit);
        connect(edit,&QTextEdit::textChanged,this,[this,c,edit]{auto o=metadata(c);o["description"]=edit->toPlainText();updateMetadata(c,o);});
        auto screen=m_editor->screen()->availableGeometry();const QSize size(qMin(420,screen.width()-16),qMin(280,screen.height()-32));auto pos=m_editor->viewport()->mapToGlobal(rect.bottomLeft());pos.setX(qBound(screen.left()+8,pos.x(),screen.right()-size.width()-8));pos.setY(qBound(screen.top()+8,pos.y(),screen.bottom()-size.height()-8));popup->resize(size);popup->move(pos);popup->show();edit->setFocus();}
}
bool ScreenshotBlocks::handleMouse(QMouseEvent *event) {
    if(m_editor->isReadOnly())return false;
    // Only inspect visible paragraphs, independent of total card count.
    auto block=m_editor->cursorForPosition(QPoint(0,event->pos().y())).block();
    for(int d=-1;d<=1;++d){auto b=d<0?block.previous():d>0?block.next():block;if(!b.isValid())continue;
        for(auto it=b.begin();!it.atEnd();++it){auto f=it.fragment();if(f.charFormat().objectType()!=ObjectType)continue;QTextCursor c(m_editor->document());c.setPosition(f.position());c.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor);auto r=cardRect(c);if(!r.contains(event->pos()))continue;
            if(event->type()==QEvent::MouseMove){m_editor->viewport()->setCursor(Qt::PointingHandCursor);m_editor->viewport()->setToolTip(event->pos().x()>r.right()-30?tr("Удалить скриншот"):tr("Изменить карточку / открыть оригинал"));m_editor->viewport()->update();return true;}
            if(event->type()!=QEvent::MouseButtonRelease||event->button()!=Qt::LeftButton)return false;
            auto o=metadata(c);if(event->pos().x()>r.right()-30&&event->pos().y()<r.top()+34)removeCard(c);
            else if(event->pos().x()<r.left()+qMin(120,int(r.width()*.30))+16){const QString original=safePath(o["original"].toString());if(original.isEmpty()||!QFileInfo::exists(original)){const auto file=QFileDialog::getOpenFileName(m_editor,tr("Восстановить оригинал"),{},tr("Изображения (*.png *.jpg *.jpeg *.webp)"));if(!file.isEmpty())startJob(o,{},file,true);}else if(!QDesktopServices::openUrl(QUrl::fromLocalFile(original)))QMessageBox::warning(m_editor,tr("Просмотр изображения"),tr("Не удалось открыть изображение в стандартной программе Windows."));}
            else editCard(c,event->pos().y()>r.top()+36);return true;
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
