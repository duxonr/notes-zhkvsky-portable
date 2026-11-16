#include "mainwindow.h"
#include "nodetreeview.h"
#include "nodetreemodel.h"
#include "treeviewlogic.h"
#include "foldertreedelegateeditor.h"
#include "labeledittype.h"
#include "isolatedtest.h"
#include <QApplication>
#include <cstdio>
#include <QTest>
#include <QPushButton>
#include <QLineEdit>
#include <QSplitter>
#include <QDrag>
#include <QQuickWidget>
#include <QQuickItem>
#include <QLabel>
#include <QFontInfo>
#include <QRawFont>
#include "notelistview.h"
#include "notelistmodel.h"
#include <QTextEdit>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QColorDialog>
#include <QMessageBox>
#include <QSettings>
#include <QFontDatabase>
#include <QFile>
#include <QTextStream>
#include <qt_windows.h>
#include <dwmapi.h>

int main(int argc, char **argv) {
 qInstallMessageHandler([](QtMsgType,const QMessageLogContext &,const QString &message){fprintf(stderr,"%s\n",qPrintable(message));});
 QApplication app(argc,argv); app.setApplicationVersion("2.3.1");
 if (!validatePortablePaths()) return 2;
 QFontDatabase::addApplicationFont(":/fonts/fontawesome/fa-solid-900.ttf");
 QFontDatabase::addApplicationFont(":/fonts/material/material-symbols-outlined.ttf");
 QFile report(QCoreApplication::applicationDirPath()+"/ui-test-report.txt"); report.open(QIODevice::WriteOnly|QIODevice::Text); QTextStream out(&report);
 const QFont fa = QFontDatabase::font("Font Awesome 6 Free Solid","",9);
 out<<"FONT="<<QFontInfo(fa).family()<<" FAMILIES="<<QFontDatabase::families().filter("Awesome").join(",")<<"\n";
 for(uint glyph:{0xf65e,0xf303,0xf1f8,0xf53f})out<<"GLYPH="<<QString::number(glyph,16)<<":"<<QRawFont::fromFont(fa).supportsCharacter(glyph)<<"\n";out.flush();
 out<<"SUBFOLDER_GLYPH="<<QRawFont::fromFont(QFont("Material Symbols Outlined",12)).supportsCharacter(uint(0xe2c8))<<"\n";out.flush();
 int fails=0; auto check=[&](bool ok,const QString &name){out<<(ok?"PASS ":"FAIL ")<<name<<'\n';out.flush(); if(!ok)++fails;};
 MainWindow *window=new MainWindow; window->resize(1250,800); window->show(); QTest::qWait(1200);
 auto *tree=window->findChild<NodeTreeView*>("treeView"); auto *model=qobject_cast<NodeTreeModel*>(tree->model()); auto *logic=window->findChild<TreeViewLogic*>();
 if(!tree||!model||!logic) return 3;
 window->findChild<QSplitter*>("splitter")->setSizes({300,250,700}); QTest::qWait(100);
 if(app.arguments().contains("--interactive")) {
   window->setTheme(Theme::Dark);tree->setCurrentIndexC(model->getAllNotesButtonIndex());QTest::qWait(500);
   window->findChild<QPushButton*>("newNoteButton")->click();QTest::qWait(300);window->findChild<QTextEdit*>("textEdit")->insertPlainText("# Drag probe\n\n**Markdown**");QTest::qWait(1000);
   if(app.arguments().contains("--multi")){auto *list=window->findChild<NoteListView*>("listView");list->selectionModel()->select(list->model()->index(1,0),QItemSelectionModel::Select);}
   QTimer watch;QObject::connect(&watch,&QTimer::timeout,[&]{for(auto *drag:window->findChildren<QDrag*>())if(!drag->property("recorded").toBool()){
     drag->setProperty("recorded",true);out<<"DRAG_SIZE="<<drag->pixmap().width()/drag->pixmap().devicePixelRatio()<<"x"<<drag->pixmap().height()/drag->pixmap().devicePixelRatio()<<"\n";out.flush();}});watch.start(100);
   return app.exec();
 }
 const auto hover=[&](const QModelIndex &ix){tree->scrollTo(ix);const QPoint pos=tree->visualRect(ix).center();QMouseEvent move(QEvent::MouseMove,QPointF(pos),QPointF(tree->viewport()->mapToGlobal(pos)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(tree->viewport(),&move);QTest::qWait(150);};
 tree->setCurrentIndexC(model->getAllNotesButtonIndex()); tree->addFolderRequested(); QTest::qWait(100);
 QPersistentModelIndex parent;
 for(int r=0;r<model->rowCount(model->rootIndex());++r){auto ix=model->index(r,0,model->rootIndex()); if(ix.data(NodeItem::ItemType).toInt()==NodeItem::FolderItem&&ix.data(NodeItem::NodeId).toInt()>1 && (!parent.isValid() || ix.data(NodeItem::NodeId).toInt()>parent.data(NodeItem::NodeId).toInt())) parent=ix;}
 check(parent.isValid(),"create test root folder");
 if(!parent.isValid()) return 4;
 hover(parent); QTest::qWait(200);
 auto editors=tree->findChildren<FolderTreeDelegateEditor*>(); FolderTreeDelegateEditor *editor=nullptr;
 for(auto *e:editors) if(e->isVisible()) editor=e;
 check(editor!=nullptr,"folder hover opens row controls");
 QPushButton *add=nullptr;
 if(editor)for(auto *b:editor->findChildren<QPushButton*>())if(b->toolTip()=="Create subfolder")add=b;
 check(add && add->isVisible(),"subfolder button visible");
 if(add)QTest::mouseClick(add,Qt::LeftButton); else tree->triggerRowAction(parent,NodeTreeView::RowAction::AddSubfolder);
 QTest::qWait(150); check(model->rowCount(parent)==1,"button creates child under target parent");
 QPersistentModelIndex child=model->index(0,0,parent);
 check(child.parent()==parent,"child has correct model parent");
 tree->triggerRowAction(child,NodeTreeView::RowAction::AddSubfolder); QTest::qWait(100); check(model->rowCount(child)==1,"third level supported");
 hover(parent);
 editor=nullptr;for(auto *e:tree->findChildren<FolderTreeDelegateEditor*>())if(e->isVisible())editor=e;
 QLabel *counter=nullptr;if(editor)for(auto *label:editor->findChildren<QLabel*>())if(label->width()==32)counter=label;
 check(counter && counter->mapTo(tree->viewport(),counter->rect().center()).x()==tree->viewport()->width()-25,"counter stays in fixed right column");
 check(tree->visualRect(parent).height()==30 && tree->visualRect(child).height()==30,"row heights unchanged");
 window->findChild<QSplitter*>("splitter")->setSizes({185,250,815});QTest::qWait(100);hover(parent);
 check(tree->visualRect(parent).height()==30,"narrow sidebar retains row height");
 window->findChild<QSplitter*>("splitter")->setSizes({300,250,700});QTest::qWait(100);
 tree->setExpanded(parent,true);QTest::qWait(50);const QPoint arrow=tree->visualRect(parent).topLeft()+QPoint(10,15);
 QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,arrow);check(!tree->isExpanded(parent),"arrow single click collapses folder");
 QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,arrow);check(tree->isExpanded(parent),"arrow single click expands folder");
 tree->triggerRowAction(parent,NodeTreeView::RowAction::Select);
 hover(parent);QTest::qWait(100);
 LabelEditType *folderLabel=nullptr;for(auto *label:tree->findChildren<LabelEditType*>())if(label->isVisible())folderLabel=label;
 if(folderLabel)QTest::mouseDClick(folderLabel,Qt::LeftButton);QTest::qWait(100);
 QLineEdit *rename=nullptr; for(auto *line:tree->findChildren<QLineEdit*>())if(line->isVisible())rename=line;
 check(rename!=nullptr,"rename uses inline editor"); if(rename){rename->selectAll();QTest::keyClicks(rename,"Parent renamed");QTest::keyClick(rename,Qt::Key_Return);QTest::qWait(100);}
 check(parent.data().toString()=="Parent renamed","rename targets parent");
 const int folderId=parent.data(NodeItem::NodeId).toInt();
 QTimer::singleShot(100,[&]{if(auto *dlg=qobject_cast<QColorDialog*>(app.activeModalWidget())){dlg->setCurrentColor(QColor("#18aa55"));dlg->accept();}});
 tree->changeFolderIconColor(parent);check(tree->folderIconColor(parent)==QColor("#18aa55"),"folder color saved by ID");
 for(auto theme:{Theme::Dark,Theme::Light,Theme::Sepia}){window->setTheme(theme);QTest::qWait(100);check(tree->folderIconColor(parent)==QColor("#18aa55"),"folder color stable across theme "+QString::number(theme));
  COLORREF value=theme==Theme::Dark ? RGB(25,25,25) : theme==Theme::Sepia ? RGB(251,240,217) : RGB(247,247,247); auto hr=DwmSetWindowAttribute(reinterpret_cast<HWND>(window->winId()),DWMWA_CAPTION_COLOR,&value,sizeof(value));
  check(SUCCEEDED(hr),"DWM caption color accepted "+QString::number(theme));window->grab().save(QCoreApplication::applicationDirPath()+"/theme-"+QString::number(theme)+".png");}
 QSettings persisted(portableSettings(),QSettings::IniFormat); check(persisted.value("folderIconColors/"+QString::number(folderId)).toString()=="#18aa55","folder color persisted in Settings.ini");
 const int childId=child.data(NodeItem::NodeId).toInt();
 QTimer::singleShot(100,[&]{if(auto *dlg=qobject_cast<QColorDialog*>(app.activeModalWidget())){dlg->setCurrentColor(QColor("#18aa55"));dlg->accept();}});tree->changeFolderIconColor(child);
 QMimeData folderMime;folderMime.setData(FOLDER_MIME,child.data(NodeItem::AbsPath).toString().toUtf8());
 check(model->dropMimeData(&folderMime,Qt::MoveAction,model->getSeparatorIndex()[1].row(),0,QModelIndex()),"move colored child to root");QTest::qWait(200);
 child=model->folderIndexFromIdPath(NodePath("/0/"+QString::number(childId)));parent=model->folderIndexFromIdPath(NodePath("/0/"+QString::number(folderId)));
 check(child.isValid() && tree->folderIconColor(child)==QColor("#18aa55"),"folder color follows stable ID after move");
 folderMime.setData(FOLDER_MIME,child.data(NodeItem::AbsPath).toString().toUtf8());check(model->dropMimeData(&folderMime,Qt::MoveAction,0,0,parent),"move colored folder back to parent");QTest::qWait(200);
 parent=model->folderIndexFromIdPath(NodePath("/0/"+QString::number(folderId)));child=model->folderIndexFromIdPath(NodePath("/0/"+QString::number(folderId)+"/"+QString::number(childId)));
 check(child.isValid() && tree->folderIconColor(child)==QColor("#18aa55"),"folder color preserved on nesting");
 QTimer::singleShot(100,[&]{if(auto *dlg=qobject_cast<QColorDialog*>(app.activeModalWidget()))for(auto *b:dlg->findChildren<QPushButton*>())if(b->text()=="Use theme color"){b->click();break;}});
 tree->changeFolderIconColor(parent);check(!tree->folderIconColor(parent).isValid(),"reset folder color to theme");
 QMetaObject::invokeMethod(logic,"onAddTagRequested");QMetaObject::invokeMethod(logic,"onAddTagRequested");QTest::qWait(100);
 QList<QPersistentModelIndex> tags;for(int r=0;r<model->rowCount(model->rootIndex());++r){auto ix=model->index(r,0,model->rootIndex());if(ix.data(NodeItem::ItemType).toInt()==NodeItem::TagItem)tags<<ix;}
 check(tags.size()>=2,"create test tags");
 if(tags.size()>=2){QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualRect(tags[0]).center());QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualRect(tags[1]).center());check(tree->selectionModel()->selectedIndexes().size()==1 && tree->selectionModel()->isSelected(tags[1]),"plain tag click replaces selection");
 QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::ControlModifier,tree->visualRect(tags[0]).center());check(tree->selectionModel()->selectedIndexes().size()==2,"Ctrl explicitly selects multiple tags");
 QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualRect(tags[0]).center());check(tree->selectionModel()->selectedIndexes().size()==1,"plain click clears multiselection");}

 if(!tags.isEmpty()) {
   hover(tags[0]);QTest::qWait(100);
   QTimer::singleShot(100,[&]{if(auto *dlg=qobject_cast<QColorDialog*>(app.activeModalWidget())){dlg->setCurrentColor(QColor("#f07020"));dlg->accept();}});
   tree->triggerRowAction(tags[0],NodeTreeView::RowAction::Color);QTest::qWait(100);
   check(tags[0].data(NodeItem::TagColor).toString()=="#f07020","tag palette updates target color");
   hover(tags[0]);QTest::qWait(100);
   tree->triggerRowAction(tags[0],NodeTreeView::RowAction::Rename);QTest::qWait(100);
   QLineEdit *tagLine=nullptr;for(auto *line:tree->findChildren<QLineEdit*>())if(line->isVisible())tagLine=line;
   check(tagLine!=nullptr,"tag rename inline editor");if(tagLine){tagLine->selectAll();QTest::keyClicks(tagLine,"Tag renamed");QTest::keyClick(tagLine,Qt::Key_Return);QTest::qWait(100);}
   check(tags[0].data().toString()=="Tag renamed","tag rename saved in model");
 }
 auto *list=window->findChild<NoteListView*>("listView");
 auto *text=window->findChild<QTextEdit*>("textEdit");
 QList<int> noteIds;
 tree->setCurrentIndexC(model->getAllNotesButtonIndex());QTest::qWait(500);
 for(int i=0;i<3;++i){window->findChild<QPushButton*>("newNoteButton")->click();QTest::qWait(350);text->setFocus();text->insertPlainText(QString("# UI test %1\n\n**Markdown**\n\n## To Do\n- [ ] task").arg(i));QTest::qWait(1000);noteIds<<list->currentIndex().data(NoteListModel::NoteID).toInt();}
 for(int id:noteIds)out<<"TEST_NOTE_ID="<<id<<'\n';out.flush();
 window->findChild<QPushButton*>("switchToKanbanViewButton")->click();QTest::qWait(300);
 QObject *kanban=nullptr;for(auto *quick:window->findChildren<QQuickWidget*>())if(quick->rootObject() && quick->rootObject()->property("isProVersion").isValid())kanban=quick->rootObject();
 check(kanban && kanban->property("isProVersion").toBool(),"Kanban Pro remains active");
 if(kanban){check(!kanban->property("isReadOnlyMode").toBool(),"Kanban is editable");
   bool invoked=QMetaObject::invokeMethod(kanban,"checkTaskInLine",Q_ARG(int,5));QTest::qWait(100);check(invoked && text->toPlainText().contains("[x]"),"Kanban checks task through QML handler");
   invoked=QMetaObject::invokeMethod(kanban,"updateTaskText",Q_ARG(int,5),Q_ARG(int,5),Q_ARG(QString,QString("Kanban edited")));QTest::qWait(700);check(invoked && text->toPlainText().contains("Kanban edited"),"Kanban edits task text through QML handler");}
 window->findChild<QPushButton*>("switchToTextViewButton")->click();QTest::qWait(100);
 check(noteIds.size()==3 && noteIds[0]!=noteIds[1],"create independent test notes via editor");
 auto db=QSqlDatabase::addDatabase("QSQLITE","uiAssertions");db.setDatabaseName(portableDatabase());db.setConnectOptions("QSQLITE_OPEN_READONLY");check(db.open(),"read-only SQLite verification");
 const auto assertParent=[&](int id,int parentId){QSqlQuery q(db);q.prepare("select parent_id from node_table where id=?");q.addBindValue(id);return q.exec()&&q.next()&&q.value(0).toInt()==parentId;};
 out<<"MANUAL_CHECK native OLE drag preview/cancellation requires real mouse input\n";out.flush();
 const auto dropNotes=[&](const QList<int> &ids,const QModelIndex &target){
   QMimeData mime;QStringList values;for(int id:ids)values<<QString::number(id);mime.setData(NOTE_MIME,values.join(PATH_SEPARATOR).toUtf8());
   const QPoint pos=tree->visualRect(target).center();QDragEnterEvent enter(pos,Qt::MoveAction,&mime,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(tree->viewport(),&enter);
   QDropEvent drop(QPointF(pos),Qt::MoveAction,&mime,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(tree->viewport(),&drop);QTest::qWait(500);return drop.isAccepted();};
 check(dropNotes({noteIds[0]},child),"drop single note to nested folder accepted");check(assertParent(noteIds[0],child.data(NodeItem::NodeId).toInt()),"single note moved to nested folder in SQLite");
 check(dropNotes({noteIds[1],noteIds[2]},parent),"drop multiple notes accepted");check(assertParent(noteIds[1],folderId)&&assertParent(noteIds[2],folderId),"multiple notes moved in SQLite");
 QSqlQuery count(db);count.exec("select count(*) from node_table where node_type=0");check(count.next()&&count.value(0).toInt()>=3,"note records retained after moves");
 count.finish(); db.close();
 if(!tags.isEmpty()){tree->triggerRowAction(tags[0],NodeTreeView::RowAction::Delete);QTest::qWait(100);check(!tags[0].isValid(),"delete tag uses existing handler");}
 // Removing a top-level tag resets the model; resolve the folder by its stable ID again.
 parent = model->folderIndexFromIdPath(NodePath("/0/" + QString::number(folderId)));
 check(parent.isValid(), "resolve folder after model reset");
 // Delete only our newly created test folder, through the original confirmation path.
 QTimer::singleShot(100,[&]{if(auto *dlg=qobject_cast<QMessageBox*>(app.activeModalWidget()))dlg->button(QMessageBox::Yes)->click();});
 tree->triggerRowAction(parent,NodeTreeView::RowAction::Delete);QTest::qWait(100);check(!parent.isValid(),"delete folder uses existing handler and confirmation");
 QTest::qWait(500);db.open();QSqlQuery deleted(db);deleted.prepare("select count(*) from node_table where id=?");deleted.addBindValue(folderId);check(deleted.exec()&&deleted.next()&&deleted.value(0).toInt()==0,"folder deletion persisted in SQLite");deleted.finish();db.close();
 out<<"FAILURES="<<fails<<'\n';out.flush();
 QMetaObject::invokeMethod(window,"quitApplication",Qt::DirectConnection);QTest::qWait(100);
 return fails?1:0;
}
