#include "mainwindow.h"
#include "customDocument.h"
#include "screenshotblocks.h"
#include "noteeditorlogic.h"
#include "notelistview.h"
#include "notelistmodel.h"
#include "nodetreeview.h"
#include "nodetreemodel.h"
#include "isolatedtest.h"
#include "dbmanager.h"
#include <QApplication>
#include <QTest>
#include <QMimeData>
#include <QClipboard>
#include <QFile>
#include <QTextStream>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QLineEdit>
#include <QElapsedTimer>
#include <QPushButton>
#include <QCryptographicHash>
#include <QScrollBar>
#include <QTextBlock>
#include <QUrl>
#include <QFileDialog>
#include <QTimer>
#include <QMessageBox>
#include <QSignalSpy>
#include <QJsonDocument>
#include <windows.h>
#include <psapi.h>

static QByteArray hash(const QString &path){QFile f(path);if(!f.open(QIODevice::ReadOnly))return {};return QCryptographicHash::hash(f.readAll(),QCryptographicHash::Sha256);}
static void waitJobs(ScreenshotBlocks *s){for(int i=0;i<400&&s->pendingJobs();++i)QTest::qWait(25);}
int main(int argc,char **argv) {
 QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
 QApplication app(argc,argv);app.setApplicationVersion("2.3.1");if(!validatePortablePaths())return 2;
 QFile report(QCoreApplication::applicationDirPath()+"/ui-test-report.txt");report.open(QIODevice::WriteOnly|QIODevice::Text);QTextStream out(&report);
 int failures=0;auto check=[&](bool ok,QString name){out<<(ok?"PASS ":"FAIL ")<<name<<'\n';out.flush();if(!ok)++failures;};
 if(app.arguments().contains("--storage-only")) {
   QFile f(QCoreApplication::applicationDirPath()+"/expected-screenshot.json");f.open(QIODevice::ReadOnly);auto expected=QJsonDocument::fromJson(f.readAll()).object();
   DBManager manager;manager.onOpenDBManagerRequested(portableDatabase(),false);auto note=manager.getNode(expected["note"].toInt());auto source=note.content();
   NodeData folder;folder.setId(manager.nextAvailableNodeId());folder.setNodeType(NodeData::Type::Folder);folder.setParentId(ROOT_FOLDER_ID);folder.setFullTitle("Screenshot storage test");folder.setAbsolutePath(manager.getNode(ROOT_FOLDER_ID).absolutePath()+"/"+QString::number(folder.id()));folder.setCreationDateTime(QDateTime::currentDateTime());folder.setLastModificationDateTime(QDateTime::currentDateTime());manager.addNode(folder);
   manager.moveNode(note.id(),folder);auto moved=manager.getNode(note.id());check(moved.content()==source&&moved.parentId()==folder.id()&&moved.absolutePath()==folder.absolutePath()+"/"+QString::number(note.id()),"folder move preserves screenshot source and valid path");
   moved.setFullTitle("Renamed screenshot note");moved.setHasManualTitle(true);manager.onCreateUpdateRequestedNoteContent(moved);auto renamed=manager.getNode(note.id());check(renamed.content()==source&&renamed.fullTitle()=="Renamed screenshot note","note rename leaves screenshot metadata intact");
   const QString backup=QCoreApplication::applicationDirPath()+"/storage.nbk";manager.onExportNotesRequested(backup);check(QFileInfo::exists(backup)&&hash(backup+".assets/"+expected["original"].toString())==hash(ScreenshotBlocks::dataRoot()+"/"+expected["original"].toString()),"nbk backup has matching original sidecar");
   QSqlQuery readOnly(QSqlDatabase::database("default_database"));readOnly.exec("PRAGMA query_only=ON");bool warned=false;QObject::connect(&manager,&DBManager::showErrorMessage,&manager,[&](const QString &,const QString &){warned=true;});renamed.setContent(source+"\nUnsaved recovery test");manager.onCreateUpdateRequestedNoteContent(renamed);
   check(warned&&manager.getNode(note.id()).content()==source,"SQLite write failure reported without partial content");
   bool recovered=false;for(const auto &name:QDir(ScreenshotBlocks::dataRoot()+"/Recovery").entryList({"*.json"},QDir::Files)){QFile recovery(ScreenshotBlocks::dataRoot()+"/Recovery/"+name);recovery.open(QIODevice::ReadOnly);recovered |= QJsonDocument::fromJson(recovery.readAll()).object()["content"]==renamed.content();}check(recovered,"SQLite failure retains full content in Recovery");
   readOnly.exec("PRAGMA query_only=OFF");bool migrationBackup=backupPortableFiles({portableDatabase(),portableSettings()});bool migrationAsset=false;for(const auto &name:QDir(ScreenshotBlocks::dataRoot()+"/Backups").entryList(QDir::Dirs|QDir::NoDotAndDotDot))migrationAsset |= !hash(ScreenshotBlocks::dataRoot()+"/Backups/"+name+"/"+expected["original"].toString()).isEmpty();check(migrationBackup&&migrationAsset,"pre-migration backup also includes image originals");out<<"FAILURES="<<failures<<'\n';out.flush();return failures?1:0;
 }
 auto *window=new MainWindow;window->resize(1250,800);window->show();QTest::qWait(1500);
 auto *tree=window->findChild<NodeTreeView*>("treeView");auto *tm=qobject_cast<NodeTreeModel*>(tree->model());tree->setCurrentIndexC(tm->getAllNotesButtonIndex());QTest::qWait(300);
 auto *logic=window->findChild<NoteEditorLogic*>();auto *editor=window->findChild<CustomDocument*>("textEdit");auto *cards=editor->screenshots();
 auto *list=window->findChild<NoteListView*>("listView");auto *lm=qobject_cast<NoteListModel*>(list->model());
 if(app.arguments().contains("--verify-screenshot")){
   QFile f(QCoreApplication::applicationDirPath()+"/expected-screenshot.json");f.open(QIODevice::ReadOnly);auto expected=QJsonDocument::fromJson(f.readAll()).object();
   auto ix=lm->getNoteIndex(expected["note"].toInt());for(int attempt=0;attempt<5 && logic->currentEditingNoteId()!=expected["note"].toInt();++attempt){list->setCurrentIndexC(ix);emit list->notePressed({ix});QTest::qWait(400);}
   out<<"RESTART index="<<ix.isValid()<<" editing="<<logic->currentEditingNoteId()<<" expected="<<expected["note"].toInt()<<" cards="<<cards->cards().size()<<" source_size="<<editor->toPlainText().size()<<" expected_size="<<expected["source"].toString().size()<<"\n";check(ix.isValid()&&editor->toPlainText()==expected["source"].toString(),"full process restart preserves source and order");
   check(cards->cards().size()==2,"restart restores two live card objects");waitJobs(cards);
   check(!hash(ScreenshotBlocks::dataRoot()+"/"+expected["original"].toString()).isEmpty(),"restart resolves portable original");
   out<<"FAILURES="<<failures<<'\n';out.flush();QMetaObject::invokeMethod(window,"quitApplication");return failures?1:0;
 }
 auto *newNote=window->findChild<QPushButton*>("newNoteButton");for(int i=0;i<3&&(logic->currentEditingNoteId()==INVALID_NODE_ID||!list->currentIndex().isValid());++i){QTest::mouseClick(newNote,Qt::LeftButton);QTest::qWait(300);}
 check(logic->currentEditingNoteId()!=INVALID_NODE_ID,"valid isolated note");
 editor->insertPlainText("Before screenshot\nAfter screenshot\n# Tasks\n- [ ] Keep Markdown and Kanban\n");
 auto cursor=editor->textCursor();cursor.setPosition(QString("Before screenshot\n").size());editor->setTextCursor(cursor);editor->setFocus();
 QImage original(3840,2160,QImage::Format_RGB32);original.fill(QColor("#2785bc"));QPainter painter(&original);painter.setPen(Qt::white);painter.setFont(QFont("Segoe UI",90));painter.drawText(original.rect(),Qt::AlignCenter,"Screenshot Blocks — 4K");painter.end();
 app.clipboard()->setImage(original);QTest::keyClick(editor,Qt::Key_V,Qt::ControlModifier);QTest::qWait(100);waitJobs(cards);
 check(cards->cards().size()==1,"Ctrl+V image creates one inline card");
 auto c=cards->cards().value(0);auto meta=cards->metadata(c);auto file=ScreenshotBlocks::dataRoot()+"/"+meta["original"].toString();
 check(QImage(file).size()==original.size(),"4K original retains full dimensions");
 check(QImage(ScreenshotBlocks::dataRoot()+"/"+meta["thumbnail"].toString()).size()==QSize(240,135),"thumbnail preserves aspect without cropping");
 check(editor->toPlainText().startsWith("Before screenshot\n[!notes-screenshot:v1:")&&editor->toPlainText().contains("\nAfter screenshot"),"card inserted between paragraphs");
 check(meta["title"].toString().isEmpty()&&meta["description"].toString().isEmpty(),"placeholder is not stored as title");
 check(!editor->toPlainText().contains("C:")&&!editor->toPlainText().contains("D:"),"serialized references are relative");
 int moves=0;auto moveConnection=QObject::connect(editor,&CustomDocument::mouseMoved,[&]{++moves;});auto hoverPoint=cards->cardRect(cards->cards().first()).center();QMouseEvent hover(QEvent::MouseMove,QPointF(hoverPoint),QPointF(editor->viewport()->mapToGlobal(hoverPoint)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(editor->viewport(),&hover);check(moves>0,"card hover preserves editor toolbar wake-up signal");QObject::disconnect(moveConnection);
 auto source=editor->toPlainText();editor->setText(source);check(editor->toPlainText()==source&&cards->cards().size()==1,"source roundtrip keeps metadata and object");
 c=cards->cards().first();const auto beforeHeight=cards->cardRect(c).height();
 cards->editCard(c,false);auto *title=editor->findChild<QLineEdit*>("screenshotTitleEdit");QTest::keyClicks(title,"Same title");QTest::keyClick(title,Qt::Key_Return);
 check(cards->metadata(c)["title"]=="Same title","inline title Enter autosaves");
 cards->editCard(c,true);auto *description=window->findChild<QTextEdit*>("screenshotDescriptionEdit");check(description!=nullptr,"anchored description editor opens");
 QString longDescription="Paragraph one\n\n";for(int i=0;i<80;++i)longDescription+="Long description line "+QString::number(i)+"\n";
 if(description){description->setPlainText(longDescription);QTest::qWait(80);QTest::keyClick(description,Qt::Key_Escape);}cards->closeEditor();
 check(cards->metadata(c)["description"]==longDescription,"long multi-paragraph description autosaves");
 auto firstImageMeta=cards->metadata(c);auto firstImageTitle=NoteEditorLogic::getFirstLine(ScreenshotBlocks::marker(firstImageMeta));firstImageMeta["title"]="Different screenshot title";check(NoteEditorLogic::getFirstLine(ScreenshotBlocks::marker(firstImageMeta))==firstImageTitle,"screenshot title cannot rename an image-only note");
 cards->editCard(c,true);description=window->findChild<QTextEdit*>("screenshotDescriptionEdit");auto contentBeforeShortcut=editor->toPlainText();if(description)QTest::keyClick(description,Qt::Key_B,Qt::ControlModifier);check(editor->toPlainText()==contentBeforeShortcut,"popup shortcuts do not modify note body");cards->closeEditor();
 check(cards->cardRect(c).height()==beforeHeight&&beforeHeight==96,"long description keeps 96px card");
 QTest::qWait(180);auto beforeCopySource=editor->toPlainText();QSignalSpy changedByCopy(editor,&QTextEdit::textChanged);QSignalSpy savedByCopy(logic,&NoteEditorLogic::requestCreateUpdateNote);
 bool imageClipboard=false,descriptionClipboard=false,onlyImage=false;
 auto copiedConnection=QObject::connect(cards,&ScreenshotBlocks::clipboardNotice,[&](const QString &notice){
   if(notice==QString::fromUtf8("Изображение скопировано")){
     auto copied=app.clipboard()->image().convertToFormat(QImage::Format_ARGB32);auto expected=QImage(file).convertToFormat(QImage::Format_ARGB32);
     imageClipboard=copied.size()==expected.size()&&copied==expected;
     auto *mime=app.clipboard()->mimeData();onlyImage=mime->hasImage()&&!mime->hasText()&&!mime->hasUrls()&&!mime->hasFormat("application/x-notes-screenshot-blocks-v1");
     check(QImage::fromData(mime->data("image/png")).convertToFormat(QImage::Format_ARGB32)==expected,"clipboard PNG contains full original pixels");
   }
   if(notice==QString::fromUtf8("Описание скопировано"))descriptionClipboard=app.clipboard()->text().replace("\r\n","\n")==longDescription&&!app.clipboard()->mimeData()->hasImage();
 });
 auto clickCardButton=[&](int fromRight){auto r=cards->cardRect(cards->cards().first());QPoint pos(r.right()-fromRight,r.top()+17);QMouseEvent press(QEvent::MouseButtonPress,QPointF(pos),QPointF(editor->viewport()->mapToGlobal(pos)),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);QMouseEvent release(QEvent::MouseButtonRelease,QPointF(pos),QPointF(editor->viewport()->mapToGlobal(pos)),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(editor->viewport(),&press);QApplication::sendEvent(editor->viewport(),&release);};
 QElapsedTimer clipboardTime;clipboardTime.start();clickCardButton(72);waitJobs(cards);out<<"COPY_4K_MS="<<clipboardTime.elapsed()<<'\n';check(imageClipboard&&onlyImage,"copy image button copies only original image at 3840x2160");
 clickCardButton(44);waitJobs(cards);QTest::qWait(100);out<<"DESCRIPTION_COPY notice_match="<<descriptionClipboard<<" length="<<app.clipboard()->text().size()<<" expected="<<longDescription.size()<<" has_image="<<app.clipboard()->mimeData()->hasImage()<<"\n";descriptionClipboard=app.clipboard()->text().replace("\r\n","\n")==longDescription&&!app.clipboard()->mimeData()->hasImage();check(descriptionClipboard,"copy description button keeps all paragraphs without title or image");
 cards->copyImage(cards->cards().first());cards->copyDescription(cards->cards().first());waitJobs(cards);check(app.clipboard()->text().replace("\r\n","\n")==longDescription&&!app.clipboard()->mimeData()->hasImage(),"newer description copy wins over a pending image copy");
 check(editor->toPlainText()==beforeCopySource&&changedByCopy.count()==0&&savedByCopy.count()==0,"both copy buttons leave note and autosave untouched");QObject::disconnect(copiedConnection);
 auto empty=cards->metadata(cards->cards().first());empty["description"]="";CustomDocument emptyDescription(nullptr);emptyDescription.setText(ScreenshotBlocks::marker(empty));auto clipboardBeforeEmpty=app.clipboard()->text();emptyDescription.screenshots()->copyDescription(emptyDescription.screenshots()->cards().first());check(app.clipboard()->text()==clipboardBeforeEmpty,"empty description leaves clipboard unchanged");
 QImage alpha(13,9,QImage::Format_ARGB32);for(int y=0;y<alpha.height();++y)for(int x=0;x<alpha.width();++x)alpha.setPixel(x,y,qRgba(x*17,y*23,(x+y)*9,(x+y)*11));auto alphaMeta=empty;auto alphaId=QUuid::createUuid().toString(QUuid::WithoutBraces);alphaMeta["id"]=alphaId;alphaMeta["original"]="Attachments/"+alphaId+".png";alphaMeta["thumbnail"]="Thumbnails/"+alphaId+".png";alpha.save(ScreenshotBlocks::dataRoot()+"/"+alphaMeta["original"].toString());CustomDocument alphaDocument(nullptr);alphaDocument.setText(ScreenshotBlocks::marker(alphaMeta));bool alphaCopied=false;QObject::connect(alphaDocument.screenshots(),&ScreenshotBlocks::clipboardNotice,[&](const QString &notice){if(notice==QString::fromUtf8("Изображение скопировано"))alphaCopied=QImage::fromData(app.clipboard()->mimeData()->data("image/png")).convertToFormat(QImage::Format_ARGB32)==alpha;});alphaDocument.screenshots()->copyImage(alphaDocument.screenshots()->cards().first());waitJobs(alphaDocument.screenshots());check(alphaCopied,"lossless clipboard PNG retains RGBA pixels including transparency");
 check(cards->descriptionOverflows(longDescription,180)&&!cards->descriptionOverflows("short",180),"expansion only for actual overflow");
 check(!NoteEditorLogic::getFirstLine(editor->toPlainText()).contains("notes-screenshot"),"title extraction never exposes marker");
 editor->setTextCursor(cards->cards().first());auto formattedSource=editor->toPlainText();QMetaObject::invokeMethod(window,"makeBold",Qt::DirectConnection);QMetaObject::invokeMethod(window,"setHeading",Qt::DirectConnection,Q_ARG(int,2));check(editor->toPlainText()==formattedSource,"Markdown formatting protects card object metadata");
 check(logic->checkForTasksInEditor()==false,"existing task parser accepts image paragraph");logic->checkTaskInLine(editor->document()->find("- [ ] Keep Markdown").blockNumber());check(editor->toPlainText().contains("- [x] Keep Markdown"),"Kanban checkbox preserves card and text");
 auto saved=editor->toPlainText();cards->removeCard(cards->cards().first());check(cards->cards().isEmpty()&&QFileInfo::exists(file),"delete detaches card and retains original");editor->undo();check(editor->toPlainText()==saved&&cards->cards().size()==1,"Ctrl+Z restores metadata original and position");editor->redo();check(cards->cards().isEmpty(),"Ctrl+Y removes card again");editor->undo();
 c=cards->cards().first();editor->setTextCursor(c);QTest::keyClick(editor,Qt::Key_X,Qt::ControlModifier);check(cards->cards().isEmpty(),"selection cut removes card atomically");editor->undo();check(cards->cards().size()==1,"cut undo restores object format");
 c=cards->cards().first();auto adjacent=editor->toPlainText();auto at=c;at.setPosition(c.selectionEnd());editor->setTextCursor(at);QTest::keyClick(editor,Qt::Key_Backspace);check(cards->cards().isEmpty(),"Backspace removes object as one character");editor->undo();check(editor->toPlainText()==adjacent,"Backspace undo restores surrounding text");
 c=cards->cards().first();at.setPosition(c.selectionStart());editor->setTextCursor(at);QTest::keyClick(editor,Qt::Key_Delete);check(cards->cards().isEmpty(),"Delete removes object as one character");editor->undo();check(editor->toPlainText()==adjacent,"Delete undo restores surrounding text");
 editor->setTextCursor(cards->cards().first());QTest::keyClick(editor,Qt::Key_C,Qt::ControlModifier);editor->moveCursor(QTextCursor::End);QTest::keyClick(editor,Qt::Key_V,Qt::ControlModifier);check(cards->cards().size()==2,"copy paste retains card metadata");
 for(auto theme:{Theme::Dark,Theme::Light,Theme::Sepia}){auto before=editor->toPlainText();window->setTheme(theme);QTest::qWait(80);check(editor->toPlainText()==before&&cards->cardRect(cards->cards().first()).height()==96,"theme preserves content and compact geometry "+QString::number(theme));auto heading=editor->document()->find("# Tasks").block();bool contrast=false;if(heading.isValid())for(const auto &range:heading.layout()->formats())if(range.start<=2&&range.start+range.length>2){int light=range.format.foreground().color().lightness();contrast=theme==Theme::Dark?light>100:light<100;}check(contrast,"Markdown heading contrast updates with card theme "+QString::number(theme));}
 window->setTheme(Theme::Dark);
 check(cards->metadata(cards->cards()[0])["id"]!=cards->metadata(cards->cards()[1])["id"],"copied cards have independent UUIDs");
 auto encodedBeforePlain=editor->toPlainText();editor->moveCursor(QTextCursor::End);app.clipboard()->setText("plain text paste");QTest::keyClick(editor,Qt::Key_V,Qt::ControlModifier);check(editor->toPlainText().endsWith("plain text paste"),"ordinary text paste unchanged");editor->undo();check(editor->toPlainText()==encodedBeforePlain && cards->cards().size()==2,"text paste undo does not remove previous card");
 auto png=QCoreApplication::applicationDirPath()+"/source.png";original.save(png);auto *mime=new QMimeData;mime->setUrls({QUrl::fromLocalFile(png)});editor->moveCursor(QTextCursor::End);editor->setFocus();QTest::qWait(50);app.clipboard()->setMimeData(mime);out<<"FILE_FORMATS="<<app.clipboard()->mimeData()->formats().join(",")<<"\n";QTest::keyClick(editor,Qt::Key_V,Qt::ControlModifier);out<<"FILE_IMMEDIATE="<<cards->cards().size()<<" pending="<<cards->pendingJobs()<<" readonly="<<editor->isReadOnly()<<"\n";waitJobs(cards);out<<"FILE_AFTER="<<cards->cards().size()<<"\n";check(cards->cards().size()==3,"file clipboard creates third card");auto third=cards->metadata(cards->cards().last());check(hash(png)==hash(ScreenshotBlocks::dataRoot()+"/"+third["original"].toString()),"file original copied without re-encoding");if(cards->cards().size()==3)cards->removeCard(cards->cards().last());
 auto jpeg=QCoreApplication::applicationDirPath()+"/source.jpg";original.save(jpeg,"JPEG");editor->moveCursor(QTextCursor::End);cards->insertImage({},jpeg);waitJobs(cards);check(cards->cards().size()==3&&hash(jpeg)==hash(ScreenshotBlocks::dataRoot()+"/"+cards->metadata(cards->cards().last())["original"].toString()),"JPEG original preserves exact file bytes");if(cards->cards().size()==3)cards->removeCard(cards->cards().last());out<<"IMAGE_FORMATS="<<QImageReader::supportedImageFormats().join(",")<<"\n";
 QString error;auto markdown=ScreenshotBlocks::exportMarkdown(editor->toPlainText(),QCoreApplication::applicationDirPath()+"/Export",&error);
 check(error.isEmpty()&&markdown.contains("![Same title](Attachments/")&&QFileInfo::exists(QCoreApplication::applicationDirPath()+"/Export/"+meta["original"].toString()),"Markdown export includes original and description");
 error.clear();check(ScreenshotBlocks::copyAssets(ScreenshotBlocks::dataRoot(),QCoreApplication::applicationDirPath()+"/backup.nbk.assets",&error)&&error.isEmpty(),"backup includes original and thumbnail trees");
 auto thumb=ScreenshotBlocks::dataRoot()+"/"+meta["thumbnail"].toString();QFile::remove(thumb);CustomDocument recovery(nullptr);recovery.resize(800,600);recovery.show();recovery.setText(editor->toPlainText());recovery.viewport()->update();QTest::qWait(100);waitJobs(recovery.screenshots());check(QFileInfo::exists(thumb),"missing thumbnail rebuilt from original");recovery.hide();
 auto missing=meta;auto missingId=QUuid::createUuid().toString(QUuid::WithoutBraces);missing["id"]=missingId;missing["original"]="Attachments/"+missingId+".png";missing["thumbnail"]="Thumbnails/"+missingId+".png";CustomDocument absent(nullptr);absent.resize(700,300);absent.show();absent.setText(ScreenshotBlocks::marker(missing));absent.viewport()->repaint();QTest::qWait(100);waitJobs(absent.screenshots());check(absent.screenshots()->cards().size()==1&&absent.toPlainText()==ScreenshotBlocks::marker(missing),"missing original keeps recoverable card without crash");
 auto *copyErrorDismiss=new QTimer(&app);QObject::connect(copyErrorDismiss,&QTimer::timeout,[]{for(auto *w:QApplication::topLevelWidgets())if(auto *box=qobject_cast<QMessageBox*>(w))box->accept();});copyErrorDismiss->start(40);
 QSignalSpy missingCopy(absent.screenshots(),&ScreenshotBlocks::copyError);auto clipboardBeforeError=app.clipboard()->text();absent.screenshots()->copyImage(absent.screenshots()->cards().first());waitJobs(absent.screenshots());check(missingCopy.count()==1&&app.clipboard()->text()==clipboardBeforeError,"missing original copy reports error without changing clipboard");
 auto damagedMeta=missing;auto damagedId=QUuid::createUuid().toString(QUuid::WithoutBraces);damagedMeta["id"]=damagedId;damagedMeta["original"]="Attachments/"+damagedId+".png";damagedMeta["thumbnail"]="Thumbnails/"+damagedId+".png";QFile damagedFile(ScreenshotBlocks::dataRoot()+"/"+damagedMeta["original"].toString());damagedFile.open(QIODevice::WriteOnly);damagedFile.write("broken PNG");damagedFile.close();CustomDocument damaged(nullptr);damaged.setText(ScreenshotBlocks::marker(damagedMeta));QSignalSpy damagedCopy(damaged.screenshots(),&ScreenshotBlocks::copyError);damaged.screenshots()->copyImage(damaged.screenshots()->cards().first());waitJobs(damaged.screenshots());check(damagedCopy.count()==1&&damaged.toPlainText()==ScreenshotBlocks::marker(damagedMeta),"damaged original copy reports error and keeps card metadata");copyErrorDismiss->stop();delete copyErrorDismiss;
 auto *choose=new QTimer(&app);QObject::connect(choose,&QTimer::timeout,[png]{for(auto *widget:QApplication::topLevelWidgets())if(auto *dialog=qobject_cast<QFileDialog*>(widget))if(dialog->isVisible()){if(auto *name=dialog->findChild<QLineEdit*>("fileNameEdit"))name->setText(png);QMetaObject::invokeMethod(dialog,"accept");}});choose->start(100);
 auto missingRect=absent.screenshots()->cardRect(absent.screenshots()->cards().first());QTest::mouseClick(absent.viewport(),Qt::LeftButton,Qt::NoModifier,missingRect.topLeft()+QPoint(40,40));choose->stop();delete choose;waitJobs(absent.screenshots());check(hash(png)==hash(ScreenshotBlocks::dataRoot()+"/"+missing["original"].toString()),"missing original restored through card file chooser");absent.hide();
 // Actual file-write failure in a synthetic, runner-owned Data directory.
 CustomDocument retry(nullptr);retry.resize(700,300);retry.show();QDir data(ScreenshotBlocks::dataRoot());bool held=data.rename("Attachments","Attachments-test-held");QFile blocker(data.filePath("Attachments"));bool blocked=held&&blocker.open(QIODevice::WriteOnly);blocker.close();check(blocked,"prepare isolated attachment write failure");
 auto *dismiss=new QTimer(&app);QObject::connect(dismiss,&QTimer::timeout,[]{for(auto *w:QApplication::topLevelWidgets())if(auto *box=qobject_cast<QMessageBox*>(w))box->accept();});dismiss->start(40);
 retry.screenshots()->insertImage(original);waitJobs(retry.screenshots());check(retry.screenshots()->cards().size()==1&&!retry.screenshots()->waitForWrites(),"failed original write keeps card and blocks silent shutdown");
 dismiss->stop();delete dismiss;if(blocked)QFile::remove(data.filePath("Attachments"));if(held)data.rename("Attachments-test-held","Attachments");
 auto retryMeta=retry.screenshots()->metadata(retry.screenshots()->cards().first());auto retryRect=retry.screenshots()->cardRect(retry.screenshots()->cards().first());QTest::mouseClick(retry.viewport(),Qt::LeftButton,Qt::NoModifier,retryRect.topLeft()+QPoint(40,40));waitJobs(retry.screenshots());check(retry.screenshots()->waitForWrites()&&QImage(ScreenshotBlocks::dataRoot()+"/"+retryMeta["original"].toString()).size()==original.size(),"retained clipboard image succeeds after write access restored");retry.hide();
 auto finalSource=editor->toPlainText();const int noteId=logic->currentEditingNoteId();logic->saveNoteToDB();QTest::qWait(200);
 QJsonObject expected{{"note",noteId},{"source",finalSource},{"original",meta["original"]}};QFile expectedFile(QCoreApplication::applicationDirPath()+"/expected-screenshot.json");expectedFile.open(QIODevice::WriteOnly);expectedFile.write(QJsonDocument(expected).toJson());expectedFile.close();
 check(lm->getNote(list->currentIndex()).content()==finalSource,"live model autosave matches serialized source");
 // Reopening the same note and rapid note switches must flush every metadata edit.
 QTest::mouseClick(newNote,Qt::LeftButton);QTest::qWait(250);editor->insertPlainText("Plain note unchanged\n**bold**\n- [ ] Task");list->setCurrentIndexC(lm->getNoteIndex(noteId));emit list->notePressed({lm->getNoteIndex(noteId)});QTest::qWait(150);check(editor->toPlainText()==finalSource,"rapid note switch retains cards and descriptions");
 auto db=QSqlDatabase::addDatabase("QSQLITE","screenshot-check");db.setDatabaseName(portableDatabase());db.open();QSqlQuery query(db);query.prepare("SELECT content FROM node_table WHERE id=?");query.addBindValue(noteId);query.exec();check(query.next()&&query.value(0).toString()==finalSource,"SQLite stores text order and metadata together");query.finish();db.close();
 // Measure rendering and opening using shared assets; no original is decoded per card.
 QTest::mouseClick(newNote,Qt::LeftButton);QTest::qWait(250);CustomDocument &benchmark=*editor;emit logic->hideKanbanView();benchmark.setFocus();QTest::qWait(100);
 for(int n:{0,1,10,50,100}){QString text="Benchmark text\n";for(int i=0;i<n;++i){auto o=meta;auto id=QUuid::createUuid().toString(QUuid::WithoutBraces);o["id"]=id;o["original"]="Attachments/"+id+".png";o["thumbnail"]="Thumbnails/"+id+".png";QFile::copy(file,ScreenshotBlocks::dataRoot()+"/"+o["original"].toString());QFile::copy(thumb,ScreenshotBlocks::dataRoot()+"/"+o["thumbnail"].toString());text+=ScreenshotBlocks::marker(o)+"\ntext\n";}
   IO_COUNTERS ioBefore{};GetProcessIoCounters(GetCurrentProcess(),&ioBefore);FILETIME created,exited,kernelBefore,userBefore,kernelAfter,userAfter;GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernelBefore,&userBefore);
   QElapsedTimer timer;timer.start();benchmark.setText(text);benchmark.viewport()->repaint();QApplication::processEvents();const auto open=timer.elapsed();timer.restart();benchmark.insertPlainText("typing");benchmark.viewport()->repaint();const auto typing=timer.elapsed();timer.restart();for(int i=0;i<25;++i){benchmark.verticalScrollBar()->setValue(benchmark.verticalScrollBar()->maximum()*i/24);benchmark.viewport()->repaint();}const auto scroll=timer.elapsed();PROCESS_MEMORY_COUNTERS counters{};K32GetProcessMemoryInfo(GetCurrentProcess(),&counters,sizeof(counters));IO_COUNTERS ioAfter{};GetProcessIoCounters(GetCurrentProcess(),&ioAfter);GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernelAfter,&userAfter);auto ticks=[](FILETIME f){return (quint64(f.dwHighDateTime)<<32)|f.dwLowDateTime;};out<<"CPU_ms="<<(ticks(kernelAfter)+ticks(userAfter)-ticks(kernelBefore)-ticks(userBefore))/10000<<" read_bytes="<<(ioAfter.ReadTransferCount-ioBefore.ReadTransferCount)<<" write_bytes="<<(ioAfter.WriteTransferCount-ioBefore.WriteTransferCount)<<" ";out<<"BENCH cards="<<n<<" open_ms="<<open<<" type_ms="<<typing<<" scroll25_ms="<<scroll<<" working_set="<<counters.WorkingSetSize<<'\n';out.flush();check(n<=1 || (benchmark.isVisible() && benchmark.verticalScrollBar()->maximum()>0),"visible benchmark scrollbar "+QString::number(n));check(benchmark.screenshots()->cards().size()==n,"benchmark object count "+QString::number(n));waitJobs(benchmark.screenshots());}
 list->setCurrentIndexC(lm->getNoteIndex(noteId));emit list->notePressed({lm->getNoteIndex(noteId)});QTest::qWait(150);
 out<<"RESTART_SCREENSHOT_ID="<<noteId<<'\n'<<"FAILURES="<<failures<<'\n';out.flush();QMetaObject::invokeMethod(window,"quitApplication");return failures?1:0;
}
