#include "mainwindow.h"
#include "customDocument.h"
#include "screenshotblocks.h"
#include "noteeditorlogic.h"
#include "notelistview.h"
#include "notelistmodel.h"
#include "nodetreeview.h"
#include "nodetreemodel.h"
#include "isolatedtest.h"
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
#include <QJsonDocument>
#include <windows.h>
#include <psapi.h>

static QByteArray hash(const QString &path){QFile f(path);if(!f.open(QIODevice::ReadOnly))return {};return QCryptographicHash::hash(f.readAll(),QCryptographicHash::Sha256);}
static void waitJobs(ScreenshotBlocks *s){for(int i=0;i<400&&s->pendingJobs();++i)QTest::qWait(25);}
int main(int argc,char **argv) {
 QApplication app(argc,argv);app.setApplicationVersion("2.3.1");if(!validatePortablePaths())return 2;
 QFile report(QCoreApplication::applicationDirPath()+"/ui-test-report.txt");report.open(QIODevice::WriteOnly|QIODevice::Text);QTextStream out(&report);
 int failures=0;auto check=[&](bool ok,QString name){out<<(ok?"PASS ":"FAIL ")<<name<<'\n';out.flush();if(!ok)++failures;};
 auto *window=new MainWindow;window->resize(1250,800);window->show();QTest::qWait(900);
 auto *tree=window->findChild<NodeTreeView*>("treeView");auto *tm=qobject_cast<NodeTreeModel*>(tree->model());tree->setCurrentIndexC(tm->getAllNotesButtonIndex());QTest::qWait(300);
 auto *logic=window->findChild<NoteEditorLogic*>();auto *editor=window->findChild<CustomDocument*>("textEdit");auto *cards=editor->screenshots();
 auto *list=window->findChild<NoteListView*>("listView");auto *lm=qobject_cast<NoteListModel*>(list->model());
 if(app.arguments().contains("--verify-screenshot")){
   QFile f(QCoreApplication::applicationDirPath()+"/expected-screenshot.json");f.open(QIODevice::ReadOnly);auto expected=QJsonDocument::fromJson(f.readAll()).object();
   auto ix=lm->getNoteIndex(expected["note"].toInt());list->setCurrentIndexC(ix);emit list->notePressed({ix});QTest::qWait(250);
   check(ix.isValid()&&editor->toPlainText()==expected["source"].toString(),"full process restart preserves source and order");
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
 auto source=editor->toPlainText();editor->setText(source);check(editor->toPlainText()==source&&cards->cards().size()==1,"source roundtrip keeps metadata and object");
 c=cards->cards().first();const auto beforeHeight=cards->cardRect(c).height();
 cards->editCard(c,false);auto *title=editor->findChild<QLineEdit*>("screenshotTitleEdit");QTest::keyClicks(title,"Same title");QTest::keyClick(title,Qt::Key_Return);
 check(cards->metadata(c)["title"]=="Same title","inline title Enter autosaves");
 cards->editCard(c,true);auto *description=window->findChild<QTextEdit*>("screenshotDescriptionEdit");check(description!=nullptr,"anchored description editor opens");
 QString longDescription="Paragraph one\n\n";for(int i=0;i<80;++i)longDescription+="Long description line "+QString::number(i)+"\n";
 if(description){description->setPlainText(longDescription);QTest::qWait(80);QTest::keyClick(description,Qt::Key_Escape);}cards->closeEditor();
 check(cards->metadata(c)["description"]==longDescription,"long multi-paragraph description autosaves");
 cards->editCard(c,true);description=window->findChild<QTextEdit*>("screenshotDescriptionEdit");auto contentBeforeShortcut=editor->toPlainText();if(description)QTest::keyClick(description,Qt::Key_B,Qt::ControlModifier);check(editor->toPlainText()==contentBeforeShortcut,"popup shortcuts do not modify note body");cards->closeEditor();
 check(cards->cardRect(c).height()==beforeHeight&&beforeHeight==96,"long description keeps 96px card");
 check(cards->descriptionOverflows(longDescription,180)&&!cards->descriptionOverflows("short",180),"expansion only for actual overflow");
 check(!NoteEditorLogic::getFirstLine(editor->toPlainText()).contains("notes-screenshot"),"title extraction never exposes marker");
 check(logic->checkForTasksInEditor()==false,"existing task parser accepts image paragraph");logic->checkTaskInLine(editor->document()->find("- [ ] Keep Markdown").blockNumber());check(editor->toPlainText().contains("- [x] Keep Markdown"),"Kanban checkbox preserves card and text");
 auto saved=editor->toPlainText();cards->removeCard(cards->cards().first());check(cards->cards().isEmpty()&&QFileInfo::exists(file),"delete detaches card and retains original");editor->undo();check(editor->toPlainText()==saved&&cards->cards().size()==1,"Ctrl+Z restores metadata original and position");editor->redo();check(cards->cards().isEmpty(),"Ctrl+Y removes card again");editor->undo();
 c=cards->cards().first();editor->setTextCursor(c);QTest::keyClick(editor,Qt::Key_X,Qt::ControlModifier);check(cards->cards().isEmpty(),"selection cut removes card atomically");editor->undo();check(cards->cards().size()==1,"cut undo restores object format");
 c=cards->cards().first();auto adjacent=editor->toPlainText();auto at=c;at.setPosition(c.selectionEnd());editor->setTextCursor(at);QTest::keyClick(editor,Qt::Key_Backspace);check(cards->cards().isEmpty(),"Backspace removes object as one character");editor->undo();check(editor->toPlainText()==adjacent,"Backspace undo restores surrounding text");
 c=cards->cards().first();at.setPosition(c.selectionStart());editor->setTextCursor(at);QTest::keyClick(editor,Qt::Key_Delete);check(cards->cards().isEmpty(),"Delete removes object as one character");editor->undo();check(editor->toPlainText()==adjacent,"Delete undo restores surrounding text");
 editor->setTextCursor(cards->cards().first());QTest::keyClick(editor,Qt::Key_C,Qt::ControlModifier);editor->moveCursor(QTextCursor::End);QTest::keyClick(editor,Qt::Key_V,Qt::ControlModifier);check(cards->cards().size()==2,"copy paste retains card metadata");
 for(auto theme:{Theme::Dark,Theme::Light,Theme::Sepia}){auto before=editor->toPlainText();window->setTheme(theme);QTest::qWait(80);check(editor->toPlainText()==before&&cards->cardRect(cards->cards().first()).height()==96,"theme preserves content and compact geometry "+QString::number(theme));}
 window->setTheme(Theme::Dark);
 check(cards->metadata(cards->cards()[0])["id"]!=cards->metadata(cards->cards()[1])["id"],"copied cards have independent UUIDs");
 auto encodedBeforePlain=editor->toPlainText();editor->moveCursor(QTextCursor::End);app.clipboard()->setText("plain text paste");QTest::keyClick(editor,Qt::Key_V,Qt::ControlModifier);check(editor->toPlainText().endsWith("plain text paste"),"ordinary text paste unchanged");editor->undo();
 auto png=QCoreApplication::applicationDirPath()+"/source.png";original.save(png);auto *mime=new QMimeData;mime->setUrls({QUrl::fromLocalFile(png)});app.clipboard()->setMimeData(mime);editor->moveCursor(QTextCursor::End);QTest::keyClick(editor,Qt::Key_V,Qt::ControlModifier);waitJobs(cards);check(cards->cards().size()==3,"file clipboard creates third card");auto third=cards->metadata(cards->cards().last());check(hash(png)==hash(ScreenshotBlocks::dataRoot()+"/"+third["original"].toString()),"file original copied without re-encoding");cards->removeCard(cards->cards().last());
 QString error;auto markdown=ScreenshotBlocks::exportMarkdown(editor->toPlainText(),QCoreApplication::applicationDirPath()+"/Export",&error);
 check(error.isEmpty()&&markdown.contains("![Same title](Attachments/")&&QFileInfo::exists(QCoreApplication::applicationDirPath()+"/Export/"+meta["original"].toString()),"Markdown export includes original and description");
 error.clear();check(ScreenshotBlocks::copyAssets(ScreenshotBlocks::dataRoot(),QCoreApplication::applicationDirPath()+"/backup.nbk.assets",&error)&&error.isEmpty(),"backup includes original and thumbnail trees");
 auto thumb=ScreenshotBlocks::dataRoot()+"/"+meta["thumbnail"].toString();QFile::remove(thumb);CustomDocument recovery(nullptr);recovery.resize(800,600);recovery.show();recovery.setText(editor->toPlainText());recovery.viewport()->update();QTest::qWait(100);waitJobs(recovery.screenshots());check(QFileInfo::exists(thumb),"missing thumbnail rebuilt from original");recovery.hide();
 auto finalSource=editor->toPlainText();const int noteId=logic->currentEditingNoteId();logic->saveNoteToDB();QTest::qWait(200);
 QJsonObject expected{{"note",noteId},{"source",finalSource},{"original",meta["original"]}};QFile expectedFile(QCoreApplication::applicationDirPath()+"/expected-screenshot.json");expectedFile.open(QIODevice::WriteOnly);expectedFile.write(QJsonDocument(expected).toJson());expectedFile.close();
 check(lm->getNote(list->currentIndex()).content()==finalSource,"live model autosave matches serialized source");
 // Reopening the same note and rapid note switches must flush every metadata edit.
 QTest::mouseClick(newNote,Qt::LeftButton);QTest::qWait(250);editor->insertPlainText("Plain note unchanged\n**bold**\n- [ ] Task");list->setCurrentIndexC(lm->getNoteIndex(noteId));emit list->notePressed({lm->getNoteIndex(noteId)});QTest::qWait(150);check(editor->toPlainText()==finalSource,"rapid note switch retains cards and descriptions");
 auto db=QSqlDatabase::addDatabase("QSQLITE","screenshot-check");db.setDatabaseName(portableDatabase());db.open();QSqlQuery query(db);query.prepare("SELECT content FROM node_table WHERE id=?");query.addBindValue(noteId);query.exec();check(query.next()&&query.value(0).toString()==finalSource,"SQLite stores text order and metadata together");query.finish();db.close();
 // Measure rendering and opening using shared assets; no original is decoded per card.
 CustomDocument benchmark(nullptr);benchmark.resize(1000,700);benchmark.show();benchmark.screenshots()->setTheme(Theme::Dark,Qt::white);
 for(int n:{0,1,10,50,100}){QString text="Benchmark text\n";for(int i=0;i<n;++i){auto o=meta;auto id=QUuid::createUuid().toString(QUuid::WithoutBraces);o["id"]=id;o["original"]="Attachments/"+id+".png";o["thumbnail"]="Thumbnails/"+id+".png";QFile::copy(file,ScreenshotBlocks::dataRoot()+"/"+o["original"].toString());QFile::copy(thumb,ScreenshotBlocks::dataRoot()+"/"+o["thumbnail"].toString());text+=ScreenshotBlocks::marker(o)+"\ntext\n";}
   QElapsedTimer timer;timer.start();benchmark.setText(text);benchmark.viewport()->repaint();QApplication::processEvents();const auto open=timer.elapsed();timer.restart();benchmark.insertPlainText("typing");benchmark.viewport()->repaint();const auto typing=timer.elapsed();timer.restart();for(int i=0;i<25;++i){benchmark.verticalScrollBar()->setValue(benchmark.verticalScrollBar()->maximum()*i/24);benchmark.viewport()->repaint();}const auto scroll=timer.elapsed();PROCESS_MEMORY_COUNTERS counters{};K32GetProcessMemoryInfo(GetCurrentProcess(),&counters,sizeof(counters));out<<"BENCH cards="<<n<<" open_ms="<<open<<" type_ms="<<typing<<" scroll25_ms="<<scroll<<" working_set="<<counters.WorkingSetSize<<'\n';out.flush();check(benchmark.screenshots()->cards().size()==n,"benchmark object count "+QString::number(n));waitJobs(benchmark.screenshots());}
 benchmark.hide();
 out<<"RESTART_SCREENSHOT_ID="<<noteId<<'\n'<<"FAILURES="<<failures<<'\n';out.flush();QMetaObject::invokeMethod(window,"quitApplication");return failures?1:0;
}
