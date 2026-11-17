#include "mainwindow.h"
#include "customDocument.h"
#include "noteeditorlogic.h"
#include "notelistview.h"
#include "isolatedtest.h"
#include <QApplication>
#include <QTest>
#include <QFile>
#include <QTextStream>
#include <QElapsedTimer>
#include <QPushButton>
#include <QScrollBar>
#include <windows.h>
#include <psapi.h>

int main(int argc,char **argv) {
 QApplication app(argc,argv);app.setApplicationVersion("2.3.1");if(!validatePortablePaths())return 2;
 QFile report(QCoreApplication::applicationDirPath()+"/ui-test-report.txt");report.open(QIODevice::WriteOnly|QIODevice::Text);QTextStream out(&report);
 auto *window=new MainWindow;window->resize(1250,800);window->show();QTest::qWait(1000);
 auto *logic=window->findChild<NoteEditorLogic*>();auto *editor=window->findChild<CustomDocument*>("textEdit");
 auto *list=window->findChild<NoteListView*>("listView");auto *button=window->findChild<QPushButton*>("newNoteButton");
 for(int i=0;i<3&&(logic->currentEditingNoteId()==INVALID_NODE_ID||!list->currentIndex().isValid());++i){QTest::mouseClick(button,Qt::LeftButton);QTest::qWait(250);}
 emit logic->hideKanbanView();editor->setFocus();
 QString text="Plain note benchmark\n";for(int i=0;i<600;++i)text+=QString("Line %1: existing Markdown note without screenshot blocks.\n").arg(i);
 for(int run=0;run<5;++run){QElapsedTimer timer;timer.start();editor->setText(text);editor->viewport()->repaint();QApplication::processEvents();const auto open=timer.elapsed();timer.restart();editor->insertPlainText("typing");editor->viewport()->repaint();const auto type=timer.elapsed();timer.restart();for(int i=0;i<25;++i){editor->verticalScrollBar()->setValue(editor->verticalScrollBar()->maximum()*i/24);editor->viewport()->repaint();}const auto scroll=timer.elapsed();PROCESS_MEMORY_COUNTERS counters{};K32GetProcessMemoryInfo(GetCurrentProcess(),&counters,sizeof(counters));out<<"PLAIN run="<<run<<" open_ms="<<open<<" type_ms="<<type<<" scroll25_ms="<<scroll<<" working_set="<<counters.WorkingSetSize<<'\n';out.flush();QTest::qWait(100);}
 const bool ok=editor->isVisible()&&editor->verticalScrollBar()->maximum()>0;out<<(ok?"PASS ":"FAIL ")<<"visible plain note benchmark\nFAILURES="<<(ok?0:1)<<'\n';out.flush();QMetaObject::invokeMethod(window,"quitApplication");return ok?0:1;
}
