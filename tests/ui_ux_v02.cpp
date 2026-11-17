#include <QQuickView>
#include <QScrollBar>
#include <QStyleOptionSlider>
#include <QElapsedTimer>
#include "noteeditorlogic.h"
#include "taglistdelegate.h"
#include "taglistmodel.h"
#include "noterowlayout.h"
#include "utils.h"
#include "notelistdelegateeditor.h"
#include "notelistdelegate.h"
#include "taglistview.h"
#include <QLineEdit>
#include "mainwindow.h"
#include "nodetreeview.h"
#include "nodetreemodel.h"
#include "treeviewlogic.h"
#include "foldertreedelegateeditor.h"
#include "labeledittype.h"
#include "treefoldericon.h"
#include <QPointer>
#include "rowcounterbutton.h"
#include "rowactionbutton.h"
#include "tagtreedelegateeditor.h"
#include <QMenu>
#include <QAction>
#include <QProcess>
#include <QStandardItemModel>
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
 if(app.arguments().contains("--verify-persisted-color")) {
   const int at=app.arguments().indexOf("--verify-persisted-color");
   if(app.arguments().size()<=at+2) return 10;
   QSettings saved(portableSettings(),QSettings::IniFormat);
   NodeTreeView colorTree;colorTree.setFolderColorSettings(&saved);
   QStandardItemModel probe;auto *item=new QStandardItem;
   item->setData(app.arguments()[at+1].toInt(),NodeItem::NodeId);probe.appendRow(item);
   return colorTree.folderIconColor(probe.index(0,0))==QColor(app.arguments()[at+2])?0:11;
 }
 QFontDatabase::addApplicationFont(":/fonts/fontawesome/fa-solid-900.ttf");
 QFontDatabase::addApplicationFont(":/fonts/material/material-symbols-outlined.ttf");
 if(app.arguments().contains("--verify-heading")) {
   const int at=app.arguments().indexOf("--verify-heading");if(app.arguments().size()<=at+1)return 12;
   auto *restarted=new MainWindow;restarted->show();QTest::qWait(900);
   auto *t=restarted->findChild<NodeTreeView*>("treeView");auto *m=qobject_cast<NodeTreeModel*>(t->model());
   t->setCurrentIndexC(m->getAllNotesButtonIndex());QTest::qWait(400);
   auto *l=restarted->findChild<NoteListView*>("listView");auto *lm=qobject_cast<NoteListModel*>(l->model());
   auto ix=lm->getNoteIndex(app.arguments()[at+1].toInt());l->setCurrentIndexC(ix);QTest::qWait(200);
   auto *h=restarted->findChild<QLineEdit*>("noteTitleEdit");auto *body=restarted->findChild<QTextEdit*>("textEdit");
   const bool ok=ix.isValid() && h->text()=="Persistent heading" && body->toPlainText().contains("Kanban edited") && lm->getNote(ix).hasManualTitle();
   QMetaObject::invokeMethod(restarted,"quitApplication",Qt::DirectConnection);return ok?0:13;
 }
 QFile report(QCoreApplication::applicationDirPath()+"/ui-test-report.txt"); report.open(QIODevice::WriteOnly|QIODevice::Text); QTextStream out(&report);
 const QFont fa = QFontDatabase::font("Font Awesome 6 Free Solid","",9);
 out<<"FONT="<<QFontInfo(fa).family()<<" FAMILIES="<<QFontDatabase::families().filter("Awesome").join(",")<<"\n";
 for(uint glyph:{0xf65e,0xf303,0xf1f8,0xf53f})out<<"GLYPH="<<QString::number(glyph,16)<<":"<<QRawFont::fromFont(fa).supportsCharacter(glyph)<<"\n";out.flush();
 out<<"SUBFOLDER_GLYPH="<<QRawFont::fromFont(QFont("Material Symbols Outlined",12)).supportsCharacter(uint(0xe2c8))<<"\n";out.flush();
 int fails=0; auto check=[&](bool ok,const QString &name){out<<(ok?"PASS ":"FAIL ")<<name<<'\n';out.flush(); if(!ok)++fails;};
 if(app.arguments().contains("--feedback-only")){QSettings startSettings(portableSettings(),QSettings::IniFormat);startSettings.setValue("theme","Dark");startSettings.sync();}
 const auto clockProbe=QDateTime(QDate::currentDate(),QTime(15,4));
 check(utils::parseDateTime(clockProbe)=="15:04","list time uses 24-hour HH:mm");
 const auto editorClock=NoteEditorLogic::getNoteDateEditor(clockProbe.toString(Qt::ISODate));
 check(editorClock.endsWith("15:04") && !editorClock.contains("PM"),"editor time uses 24-hour HH:mm");
 MainWindow *window=new MainWindow; window->resize(1250,800); window->show();window->raise();window->activateWindow(); QTest::qWait(1200);
 auto *tree=window->findChild<NodeTreeView*>("treeView"); auto *model=qobject_cast<NodeTreeModel*>(tree->model()); auto *logic=window->findChild<TreeViewLogic*>();
 if(!tree||!model||!logic) return 3;
 window->findChild<QSplitter*>("splitter")->setSizes({300,250,700}); QTest::qWait(100);
 if(app.arguments().contains("--scroll-popup-only")) {
   window->setTheme(Theme::Dark);tree->setCurrentIndexC(model->getAllNotesButtonIndex());QTest::qWait(150);
   QTest::qWait(500);
   auto *noteLogic=window->findChild<NoteEditorLogic*>();auto *readyList=window->findChild<NoteListView*>("listView");
   for(int attempt=0;attempt<3;++attempt){window->findChild<QPushButton*>("newNoteButton")->click();QTest::qWait(350);if(noteLogic->currentEditingNoteId()!=INVALID_NODE_ID && readyList->currentIndex().isValid())break;}
   check(noteLogic->currentEditingNoteId()!=INVALID_NODE_ID && readyList->currentIndex().isValid(),"scroll fixture has a valid new note");
   auto *text=window->findChild<QTextEdit*>("textEdit");QString body="Sample heading\nMuted preview text\n";for(int i=0;i<600;++i)body+=QString("Line %1: long document scroll test\n").arg(i);text->insertPlainText(body);QTest::qWait(300);
   auto *bar=text->verticalScrollBar();check(bar->maximum()>0 && bar->isVisible(),"editor scrollbar visible before wheel input");QTest::qWait(1300);
   check(bar->isVisible(),"editor scrollbar remains visible after inactivity");check(bar->width()==14,"editor scrollbar has 14px hit target");
   auto *list=window->findChild<NoteListView*>("listView");const QRect row=list->visualRect(list->currentIndex());auto image=list->viewport()->grab().toImage();
   const auto maxLight=[&](QRect rect){int value=0;for(int y=qMax(0,rect.top());y<=qMin(image.height()-1,rect.bottom());++y)for(int x=qMax(0,rect.left());x<=qMin(image.width()-1,rect.right());++x)value=qMax(value,image.pixelColor(x,y).lightness());return value;};
   const int left=row.right()-20-note_row::metadataWidth(row.width());const int dateLight=maxLight(QRect(QPoint(left,row.top()+10),QPoint(row.right()-44,row.top()+27)));
   const int titleLight=maxLight(QRect(QPoint(row.left()+20,row.top()+10),QPoint(left-10,row.top()+27)));const int previewLight=maxLight(QRect(QPoint(row.left()+20,row.top()+34),QPoint(left-10,row.top()+54)));
   out<<"DATE_LIGHT="<<dateLight<<" PREVIEW_LIGHT="<<previewLight<<" TITLE_LIGHT="<<titleLight<<"\n";
   check(dateLight<titleLight-25 && dateLight<=previewLight+8,"card date muted like preview rather than title");
   for(auto theme:{Theme::Dark,Theme::Light,Theme::Sepia}){
     window->setTheme(theme);QTest::qWait(120);const QColor expected=theme==Theme::Dark?QColor("#202226"):theme==Theme::Sepia?QColor("#eee1c8"):QColor("#eceef0");
     auto pixels=bar->grab().toImage();check(pixels.pixelColor(pixels.width()/2,pixels.height()-5)==expected,"solid scrollbar track in theme "+QString::number(theme));
     QStyleOptionSlider option;option.initFrom(bar);option.rect=bar->rect();option.orientation=Qt::Vertical;option.minimum=bar->minimum();option.maximum=bar->maximum();option.pageStep=bar->pageStep();option.sliderPosition=bar->value();option.sliderValue=bar->value();option.subControls=QStyle::SC_All;option.activeSubControls=QStyle::SC_ScrollBarSlider;option.state|=QStyle::State_MouseOver;
     QImage hover(bar->size(),QImage::Format_ARGB32_Premultiplied);hover.fill(Qt::transparent);QPainter p(&hover);bar->style()->drawComplexControl(QStyle::CC_ScrollBar,&option,&p,bar);p.end();int blue=0;for(int y=0;y<hover.height();++y)for(int x=0;x<hover.width();++x)if(hover.pixelColor(x,y)==QColor("#448ac9"))++blue;
     check(blue>10,"scrollbar thumb renders accent blue on hover theme "+QString::number(theme));
   }
   window->setTheme(Theme::Dark);auto *popup=window->findChild<QWidget*>("editorSettingsPopup");QQuickView *quick=nullptr;for(auto *w:QGuiApplication::allWindows())if(w->objectName()=="editorSettingsQuickView")quick=qobject_cast<QQuickView*>(w);
   check(popup && quick && quick->rootObject(),"settings popup and QML root available");
   if(popup && quick && quick->rootObject())for(int height:{500,1200,600}){
     popup->hide();window->resize(1250,height);QTest::qWait(150);QMetaObject::invokeMethod(window,"showEditorSettings");QTest::qWait(300);auto *root=quick->rootObject();
     out<<"POPUP parent="<<height<<" container="<<popup->height()<<" view="<<quick->height()<<" root="<<root->height()<<"\n";
     check(qAbs(popup->height()-root->height())<=1 && qAbs(quick->height()-root->height())<=1,"popup container matches content height at "+QString::number(height));
     check(root->height()>0 && root->height()<=height*0.8+1,"settings popup bounded by parent at "+QString::number(height));
     check(quick->color()==QColor("#252525") && popup->windowFlags().testFlag(Qt::FramelessWindowHint),"dark popup has themed frameless native backing at "+QString::number(height));
     if(auto *scroll=root->findChild<QQuickItem*>("editorSettingsVerticalScrollBar")){scroll->setProperty("active",false);auto *content=scroll->property("contentItem").value<QQuickItem*>();if(scroll->property("size").toReal()<1)check(qAbs(scroll->width()-14)<=1 && content && content->opacity()==1,"QML scrollbar stays visible without activity");}
     popup->hide();
   }
   out<<"FAILURES="<<fails<<"\n";out.flush();QMetaObject::invokeMethod(window,"quitApplication",Qt::DirectConnection);return fails?1:0;
 }
 if(app.arguments().contains("--card-only")) {
   QMetaObject::invokeMethod(logic,"onAddTagRequested");QMetaObject::invokeMethod(logic,"onAddTagRequested");QTest::qWait(150);
   tree->setCurrentIndexC(model->getAllNotesButtonIndex());QTest::qWait(150);window->findChild<QPushButton*>("newNoteButton")->click();QTest::qWait(350);
   window->findChild<QTextEdit*>("textEdit")->insertPlainText("Tag row probe");QTest::qWait(300);
   auto *l=window->findChild<NoteListView*>("listView");const int id=l->currentIndex().data(NoteListModel::NoteID).toInt();
   for(int row=0;row<model->rowCount(model->rootIndex());++row){auto ix=model->index(row,0,model->rootIndex());if(ix.data(NodeItem::ItemType).toInt()==NodeItem::TagItem)tree->addNoteToTag(id,ix.data(NodeItem::NodeId).toInt());}QTest::qWait(300);
   auto *c=qobject_cast<NoteListDelegateEditor*>(l->indexWidget(l->currentIndex()));auto *v=c?c->findChild<TagListView*>():nullptr;
   if(v){auto a=v->visualRect(v->model()->index(0,0)),b=v->visualRect(v->model()->index(1,0));out<<"GEOMETRY view="<<v->width()<<"x"<<v->height()<<" viewport="<<v->viewport()->width()<<"x"<<v->viewport()->height()<<" items="<<a.x()<<","<<a.y()<<","<<a.width()<<","<<a.height()<<" / "<<b.x()<<","<<b.y()<<","<<b.width()<<","<<b.height()<<" frame="<<v->frameWidth()<<" spacing="<<v->spacing()<<"\n";
     check(a.y()==b.y(),"two dot items share horizontal row");
     for(auto theme:{Theme::Dark,Theme::Light,Theme::Sepia})for(bool selected:{true,false}){
       window->setTheme(theme);QTest::qWait(60);const auto noteIndex=static_cast<NoteListModel*>(l->model())->getNoteIndex(id);
       if(selected){l->setCurrentIndexC(noteIndex);l->setCurrentRowActive(true);}else l->clearSelection();
       const QRect row=l->visualRect(noteIndex);
       const auto sample=[&](QWidget *w,const QPoint &pt){return window->grab().toImage().pixelColor(w->mapTo(window,pt));};
       QCursor::setPos(window->mapToGlobal(QPoint(window->width()-30,window->height()-30)));QTest::qWait(120);
       const QColor idleBackground=sample(l->viewport(),QPoint(row.left()+6,row.top()+20));
       QCursor::setPos(l->viewport()->mapToGlobal(QPoint(row.left()+60,row.top()+15)));QTest::qWait(150);
       auto *button=l->findChild<QPushButton*>("noteRowDeleteButton");
       const QColor rowBackground=sample(l->viewport(),QPoint(row.left()+6,row.top()+20));
       if(!selected)check(rowBackground!=idleBackground,"unselected row actually highlights on hover theme "+QString::number(theme));
       check(button && button->isVisible(),"delete control visible for hovered row theme "+QString::number(theme)+" selected "+QString::number(selected));
       if(button){QCursor::setPos(button->mapToGlobal(button->rect().center()));QTest::qWait(150);
         check(sample(button,QPoint(2,6))==rowBackground,"delete background equals row theme "+QString::number(theme)+" selected "+QString::number(selected));
         check(sample(v->viewport(),QPoint(1,1))==rowBackground,"dot background equals row while over delete theme "+QString::number(theme)+" selected "+QString::number(selected));
         QCursor::setPos(v->viewport()->mapToGlobal(QPoint(8,8)));QTest::qWait(150);
         check(sample(l->viewport(),QPoint(row.left()+6,row.top()+20))==rowBackground && sample(v->viewport(),QPoint(1,1))==rowBackground,"row hover continues across dot child theme "+QString::number(theme)+" selected "+QString::number(selected));
       }
       QCursor::setPos(window->mapToGlobal(QPoint(window->width()-30,window->height()-30)));QTest::qWait(120);
     }
   }
   out<<"FAILURES="<<fails<<"\n";out.flush();QMetaObject::invokeMethod(window,"quitApplication",Qt::DirectConnection);return fails?1:0;
 }
 if(app.arguments().contains("--feedback-only")) {
   tree->setCurrentIndexC(model->getAllNotesButtonIndex());QTest::qWait(100);
   window->findChild<QPushButton*>("newNoteButton")->click();QTest::qWait(300);
   window->findChild<QTextEdit*>("textEdit")->insertPlainText("Automatic heading\nBody");QTest::qWait(150);
   auto *title=window->findChild<QLineEdit*>("noteTitleEdit");title->setFocus();QTest::keyClicks(title,"Manual heading");QTest::qWait(100);
   const auto hasVisibleText=[&](bool dark){
     auto image=window->grab(QRect(title->mapTo(window,QPoint()),title->size())).toImage();
     const int background=image.pixelColor(image.width()-10,10).lightness();int contrasting=0;
     for(int y=10;y<image.height()-12;++y)for(int x=25;x<qMin(image.width()-25,250);++x){const int light=image.pixelColor(x,y).lightness();if(dark?light>150:light<100)++contrasting;}
     return (dark?background<80:background>130) && contrasting>40;
   };
   check(hasVisibleText(true),"heading rendered contrast on dark startup");
   for(auto theme:{Theme::Dark,Theme::Light,Theme::Sepia,Theme::Dark}){
     window->setTheme(theme);QTest::qWait(80);
     check(hasVisibleText(theme==Theme::Dark),"heading rendered contrast in theme "+QString::number(theme));
   }
   const QString saved=title->text();title->setText("");title->setPlaceholderText("Automatic heading");QTest::qWait(50);
   check(hasVisibleText(true),"automatic heading placeholder visible in dark theme");title->setText(saved);
   auto noTag=model->tagIndexFromId(NO_TAG_ID);tree->scrollTo(noTag);QTest::qWait(100);
   const auto pixel=[&]{return tree->viewport()->grab().toImage().pixelColor(tree->visualRect(noTag).left()+8,tree->visualRect(noTag).center().y());};
   QCursor::setPos(window->mapToGlobal(QPoint(window->width()-50,window->height()-50)));QEvent leave(QEvent::Leave);QApplication::sendEvent(tree->viewport(),&leave);QApplication::sendEvent(tree,&leave);const QColor idle=pixel();
   QPoint pos=tree->visualRect(noTag).center();QMouseEvent hoverEvent(QEvent::MouseMove,QPointF(pos),QPointF(tree->viewport()->mapToGlobal(pos)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(tree->viewport(),&hoverEvent);const QColor hovered=pixel();
   check(hovered!=idle,"NO TAG paints hover background");
   QCursor::setPos(window->mapToGlobal(QPoint(window->width()-50,window->height()-50)));QApplication::sendEvent(tree->viewport(),&leave);QApplication::sendEvent(tree,&leave);
   check(pixel()==idle,"NO TAG clears hover on leave");
   QMimeData mime;mime.setData(NOTE_MIME,"999999");QDragEnterEvent enter(pos,Qt::MoveAction,&mime,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(tree->viewport(),&enter);
   QDragMoveEvent move(pos,Qt::MoveAction,&mime,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(tree->viewport(),&move);QTest::qWait(60);
   check(move.isAccepted() && pixel()!=idle,"NO TAG paints accepted note-drop target");
   QDragLeaveEvent dragLeave;QApplication::sendEvent(tree->viewport(),&dragLeave);QTest::qWait(60);
   check(pixel()==idle,"NO TAG clears target on drag cancellation");
   out<<"FAILURES="<<fails<<"\n";out.flush();QMetaObject::invokeMethod(window,"quitApplication",Qt::DirectConnection);return fails?1:0;
 }
 if(app.arguments().contains("--interactive")) {
   window->setTheme(Theme::Dark);tree->setCurrentIndexC(model->getAllNotesButtonIndex());QTest::qWait(500);
   window->findChild<QPushButton*>("newNoteButton")->click();QTest::qWait(300);window->findChild<QTextEdit*>("textEdit")->insertPlainText("# Drag probe\n\n**Markdown**");QTest::qWait(1000);
   if(app.arguments().contains("--multi")){auto *list=window->findChild<NoteListView*>("listView");list->selectionModel()->select(list->model()->index(1,0),QItemSelectionModel::Select);}
   QTimer watch;QObject::connect(&watch,&QTimer::timeout,[&]{for(auto *drag:window->findChildren<QDrag*>())if(!drag->property("recorded").toBool()){
     drag->setProperty("recorded",true);out<<"DRAG_SIZE="<<drag->pixmap().width()/drag->pixmap().devicePixelRatio()<<"x"<<drag->pixmap().height()/drag->pixmap().devicePixelRatio()<<"\n";out.flush();}});watch.start(100);
   return app.exec();
 }
 const auto hover=[&](const QModelIndex &ix){tree->scrollTo(ix);const QPoint pos=tree->visualRect(ix).center();QCursor::setPos(tree->viewport()->mapToGlobal(pos));QMouseEvent move(QEvent::MouseMove,QPointF(pos),QPointF(tree->viewport()->mapToGlobal(pos)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(tree->viewport(),&move);QTest::qWait(150);};
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
 RowCounterButton *counter=nullptr;if(editor)for(auto *button:editor->findChildren<QPushButton*>())if(auto *count=dynamic_cast<RowCounterButton*>(button))counter=count;
 check(counter && counter->mapTo(tree->viewport(),counter->rect().center()).x()==tree->viewport()->width()-25,"counter stays in fixed right column");
 bool hasRenameButton=false;
 if(editor)for(auto *b:editor->findChildren<QPushButton*>())if(b->toolTip().contains("Rename"))hasRenameButton=true;
 check(!hasRenameButton,"folder has no rename icon button");
 if(counter){
   const QRect beforeRect=counter->geometry();const QImage number=counter->grab().toImage();
   QEnterEvent enter(QPointF(3,3),QPointF(3,3),QPointF(counter->mapToGlobal(QPoint(3,3))));QApplication::sendEvent(counter,&enter);
   check(counter->grab().toImage()!=number && counter->geometry()==beforeRect,"counter hover shows dots without moving column");
   QEvent leave(QEvent::Leave);QApplication::sendEvent(counter,&leave);
   check(counter->grab().toImage()==number,"counter number returns after hover");
   bool menuForParent=false,menuHasRename=false;
   QTimer::singleShot(100,[&]{if(auto *menu=qobject_cast<QMenu*>(app.activePopupWidget())){
     menuForParent=tree->currentEditingIndex()==parent;
     for(auto *a:menu->actions())if(a->text()=="Rename Folder")menuHasRename=true;
     menu->close();
   }});
   counter->click();check(menuForParent && menuHasRename,"counter dots open original folder menu for exact row");QTest::qWait(150);
 }
 tree->setCurrentIndexC(parent);hover(parent);
 RowActionButton *hoverButton=nullptr;
 for(auto *e:tree->findChildren<FolderTreeDelegateEditor*>())if(e->isVisible())for(auto *b:e->findChildren<QPushButton*>())if(b->toolTip()=="Create subfolder")hoverButton=dynamic_cast<RowActionButton*>(b);
 if(hoverButton){hoverButton->setAttribute(Qt::WA_UnderMouse,true);const QColor ink=hoverButton->grab().toImage().pixelColor(10,13);check(qAbs(ink.red()-68)+qAbs(ink.green()-138)+qAbs(ink.blue()-201)>100,"hovered action remains visible on selected blue row");hoverButton->setAttribute(Qt::WA_UnderMouse,false);}else check(false,"hover action available for contrast check");
 check(tree->visualRect(parent).height()==30 && tree->visualRect(child).height()==30,"row heights unchanged");
 window->findChild<QSplitter*>("splitter")->setSizes({185,250,815});QTest::qWait(100);hover(parent);
 check(tree->visualRect(parent).height()==30,"narrow sidebar retains row height");
 window->findChild<QSplitter*>("splitter")->setSizes({300,250,700});QTest::qWait(100);
 tree->setExpanded(parent,true);QTest::qWait(50);const QPoint arrow=tree->visualRect(parent).topLeft()+QPoint(10,15);
 QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,arrow);check(!tree->isExpanded(parent),"arrow single click collapses folder");
 QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,arrow);check(tree->isExpanded(parent),"arrow single click expands folder");
 tree->setCurrentIndexC(parent);
 hover(parent);QTest::qWait(100);
 LabelEditType *folderLabel=nullptr;for(auto *label:tree->findChildren<LabelEditType*>())if(label->isVisible())folderLabel=label;
 if(folderLabel)QTest::mouseDClick(folderLabel,Qt::LeftButton);QTest::qWait(100);
 QLineEdit *rename=nullptr; for(auto *line:tree->findChildren<QLineEdit*>())if(line->isVisible())rename=line;
 check(rename!=nullptr,"rename uses inline editor"); if(rename){rename->selectAll();QTest::keyClicks(rename,"Parent renamed");QTest::keyClick(rename,Qt::Key_Return);QTest::qWait(100);}
 check(parent.data().toString()=="Parent renamed","rename targets parent");
 const int folderId=parent.data(NodeItem::NodeId).toInt();
 hover(parent);
 QPointer<FolderTreeDelegateEditor> colorEditor;
 TreeFolderIconButton *folderIcon = nullptr;
 for(auto *e:tree->findChildren<FolderTreeDelegateEditor*>()) if(e->isVisible()) {
   colorEditor=e;
   for(auto *button:e->findChildren<QPushButton*>())
     if(auto *icon=dynamic_cast<TreeFolderIconButton*>(button)) folderIcon=icon;
 }
 bool editorSurvivedDialog=false;
 QTimer::singleShot(100,[&]{if(auto *dlg=qobject_cast<QColorDialog*>(app.activeModalWidget())){
   QEvent leave(QEvent::Leave);QApplication::sendEvent(tree,&leave);
   QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
   editorSurvivedDialog=!colorEditor.isNull();
   if(!editorSurvivedDialog){dlg->reject();return;}
   dlg->setCurrentColor(QColor("#18aa55"));dlg->accept();
 }});
 if(folderIcon)QTest::mouseDClick(folderIcon,Qt::LeftButton);
 check(editorSurvivedDialog,"folder editor survives pointer leaving tree during color dialog");
 check(tree->folderIconColor(parent)==QColor("#18aa55"),"icon double-click saves chosen folder color");
 for(auto theme:{Theme::Dark,Theme::Light,Theme::Sepia}){window->setTheme(theme);QTest::qWait(100);check(tree->folderIconColor(parent)==QColor("#18aa55"),"folder color stable across theme "+QString::number(theme));
  COLORREF value=theme==Theme::Dark ? RGB(25,25,25) : theme==Theme::Sepia ? RGB(251,240,217) : RGB(247,247,247); auto hr=DwmSetWindowAttribute(reinterpret_cast<HWND>(window->winId()),DWMWA_CAPTION_COLOR,&value,sizeof(value));
  check(SUCCEEDED(hr),"DWM caption color accepted "+QString::number(theme));window->grab().save(QCoreApplication::applicationDirPath()+"/theme-"+QString::number(theme)+".png");}
 QSettings persisted(portableSettings(),QSettings::IniFormat); check(persisted.value("folderIconColors/"+QString::number(folderId)).toString()=="#18aa55","folder color persisted in Settings.ini");
 QProcess colorRestart;colorRestart.start(QCoreApplication::applicationFilePath(),{"--verify-persisted-color",QString::number(folderId),"#18aa55"});
 check(colorRestart.waitForFinished(10000)&&colorRestart.exitCode()==0,"chosen folder color reloads in a separate process");
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

 hover(tags[0]);QTest::qWait(100);
 TagTreeDelegateEditor *tagEditor=nullptr;for(auto *e:tree->findChildren<TagTreeDelegateEditor*>())if(e->isVisible())tagEditor=e;
 RowCounterButton *tagCounter=nullptr;bool tagHasRename=false;
 if(tagEditor)for(auto *b:tagEditor->findChildren<QPushButton*>()){if(auto *count=dynamic_cast<RowCounterButton*>(b))tagCounter=count;if(b->toolTip().contains("Rename"))tagHasRename=true;}
 check(tagEditor && !tagHasRename,"tag has no rename icon button");
 if(tagCounter){bool tagMenuCorrect=false;QTimer::singleShot(100,[&]{if(auto *menu=qobject_cast<QMenu*>(app.activePopupWidget())){tagMenuCorrect=tree->currentEditingIndex()==tags[0];menu->close();}});tagCounter->click();check(tagMenuCorrect,"counter dots open original menu for exact tag");QTest::qWait(150);}else check(false,"tag counter menu exists");
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
 auto *heading=window->findChild<QLineEdit*>("noteTitleEdit");
 check(heading && heading->text().isEmpty() && window->focusWidget()==text,"new note focuses body, optional heading empty");
 out<<"RESTART_HEADING_ID="<<noteIds.last()<<"\n";out.flush();
 const int titleId=noteIds.last();auto *listModel=qobject_cast<NoteListModel*>(list->model());
 heading->setFocus();QTest::keyClicks(heading,"Manual heading");QTest::qWait(150);
 text->setFocus();text->append("Body extension");QTest::qWait(300);
 check(listModel->getNote(listModel->getNoteIndex(titleId)).fullTitle()=="Manual heading","manual heading survives body editing");
 tree->setCurrentIndexC(model->getDefaultNotesIndex());QTest::qWait(300);tree->setCurrentIndexC(model->getAllNotesButtonIndex());QTest::qWait(300);
 list->setCurrentIndexC(listModel->getNoteIndex(titleId));QTest::qWait(200);
 check(heading->text()=="Manual heading","manual heading restored from database reload");
 heading->setFocus();heading->selectAll();QTest::keyClick(heading,Qt::Key_Backspace);QTest::qWait(250);
 check(!listModel->getNote(listModel->getNoteIndex(titleId)).hasManualTitle() && listModel->getNote(listModel->getNoteIndex(titleId)).fullTitle().contains("UI test 2"),"clearing heading restores automatic title");
 heading->setFocus();QTest::keyClicks(heading,"Persistent heading");QTest::qWait(250);text->setFocus();
 window->findChild<QPushButton*>("switchToKanbanViewButton")->click();QTest::qWait(300);
 QObject *kanban=nullptr;for(auto *quick:window->findChildren<QQuickWidget*>())if(quick->rootObject() && quick->rootObject()->property("isProVersion").isValid())kanban=quick->rootObject();
 check(kanban && kanban->property("isProVersion").toBool(),"Kanban Pro remains active");
 bool headingAboveBoard=false;for(auto *quick:window->findChildren<QQuickWidget*>())if(quick->rootObject()==kanban)headingAboveBoard=heading->mapTo(window,QPoint(0,heading->height())).y()<=quick->mapTo(window,QPoint(0,0)).y();
 check(headingAboveBoard,"heading remains above Kanban board");
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
 const auto noTag=model->tagIndexFromId(NO_TAG_ID);
 check(noTag.isValid() && noTag.row()<tags[0].row() && !(model->flags(noTag)&Qt::ItemIsDragEnabled),"NO TAG fixed above ordinary tags");
 for(int id : {noteIds[0],noteIds[1]}) for(const auto &tag:tags) tree->addNoteToTag(id,tag.data(NodeItem::NodeId).toInt());
 QTest::qWait(500);
 const auto tagged=listModel->getNoteIndex(noteIds[0]);
 auto *card=qobject_cast<NoteListDelegateEditor*>(list->indexWidget(tagged));
 auto *chips=card?card->findChild<TagListView*>():nullptr;
 check(chips && chips->height()>0 && chips->height()<=18 && chips->visualRect(chips->model()->index(0,0)).y()==chips->visualRect(chips->model()->index(1,0)).y(),"two tag dots occupy the same horizontal row");
 const auto *rowDelegate=static_cast<NoteListDelegate*>(list->itemDelegate());
 const auto fitsCard=[&](NoteListDelegateEditor *c,TagListView *v,const QModelIndex &ix){return c && v && c->height()==qMax(rowDelegate->minimumRowHeight()+rowDelegate->bodyTopOffset(ix,*listModel),v->geometry().bottom()+1+note_list_constants::LAST_EL_SEP_SPACE);};
 check(fitsCard(card,chips,tagged),"card height follows text and right-side dots without extra gap");
 check(card && chips && chips->x()>=card->width()-20-note_row::metadataWidth(card->width()) && chips->geometry().right()==card->width()-21,"tag dots right aligned under date and delete");
 check(chips && chips->y()<rowDelegate->minimumContentHeight()+rowDelegate->bodyTopOffset(tagged,*listModel),"tag dots start beside preview below date");
 if(chips){
   auto ix=chips->model()->index(0,0);QStyleOptionViewItem opt;opt.rect=QRect(0,0,16,16);
   QImage dot(16,16,QImage::Format_ARGB32_Premultiplied);dot.fill(Qt::transparent);QPainter p(&dot);chips->itemDelegate()->paint(&p,opt,ix);p.end();
   check(chips->itemDelegate()->sizeHint(opt,ix)==QSize(16,16) && dot.pixelColor(8,8)==QColor(ix.data(int(TagListModel::TagListRole::ColorRole)).toString()) && dot.pixelColor(0,0).alpha()==0,"tag renderer shows only a circle in its actual color");
   check(!ix.data(Qt::ToolTipRole).toString().isEmpty(),"tag name available as tooltip");
 }
 for(int extra=0;extra<19;++extra)QMetaObject::invokeMethod(logic,"onAddTagRequested");QTest::qWait(250);
 for(int row=0;row<model->rowCount(model->rootIndex());++row){auto ix=model->index(row,0,model->rootIndex());if(ix.data(NodeItem::ItemType).toInt()==NodeItem::TagItem)tree->addNoteToTag(noteIds[0],ix.data(NodeItem::NodeId).toInt());}
 QTest::qWait(500);card=qobject_cast<NoteListDelegateEditor*>(list->indexWidget(listModel->getNoteIndex(noteIds[0])));chips=card?card->findChild<TagListView*>():nullptr;
 check(chips && chips->model()->rowCount()==21 && chips->height()<=80 && fitsCard(card,chips,listModel->getNoteIndex(noteIds[0])),"21 tags wrap with bounded height and no trailing gap");
 const QString bodyBefore=listModel->getNote(listModel->getNoteIndex(noteIds[0])).content();
 check(dropNotes({noteIds[0],noteIds[1]},noTag),"NO TAG accepts multiple-note drop");
 QSqlQuery cleared(db);cleared.prepare("select count(*) from tag_relationship where node_id in (?,?)");cleared.addBindValue(noteIds[0]);cleared.addBindValue(noteIds[1]);
 check(cleared.exec()&&cleared.next()&&cleared.value(0).toInt()==0,"NO TAG removes all tags in SQLite");cleared.finish();
 check(listModel->getNote(listModel->getNoteIndex(noteIds[0])).tagIds().isEmpty() && listModel->getNote(listModel->getNoteIndex(noteIds[0])).content()==bodyBefore,"NO TAG updates UI without changing note body");
 tree->setCurrentIndexC(noTag);QTest::qWait(300);
 bool onlyUntagged=true;for(int row=0;row<listModel->rowCount();++row)onlyUntagged &= listModel->getNote(listModel->index(row,0)).tagIds().isEmpty();
 check(onlyUntagged && listModel->rowCount()>=3,"NO TAG selects untagged notes");
 tree->setCurrentIndexC(model->getAllNotesButtonIndex());QTest::qWait(300);
 window->setTheme(Theme::Dark);QTest::qWait(100);window->findChild<QPushButton*>("newNoteButton")->click();QTest::qWait(250);text->insertPlainText("Delete button probe");QTest::qWait(350);
 const auto deleteIndex=list->currentIndex();const int deleteId=deleteIndex.data(NoteListModel::NoteID).toInt();
 const QRect deleteRow=list->visualRect(deleteIndex);QCursor::setPos(list->viewport()->mapToGlobal(deleteRow.center()));QTest::qWait(180);
 auto *deleteButton=list->findChild<QPushButton*>("noteRowDeleteButton");
 check(deleteButton && deleteButton->isVisible() && deleteButton->property("noteIndex").value<QPersistentModelIndex>()==deleteIndex,"hover delete targets exact note");
 check(deleteButton && deleteButton->y()>=deleteRow.top() && deleteButton->y()<deleteRow.top()+32,"delete button shares title/date line");
 if(deleteButton){auto image=window->grab(QRect(deleteButton->mapTo(window,QPoint()),deleteButton->size())).toImage();int visible=0;for(int y=3;y<21;++y)for(int x=3;x<21;++x)if(image.pixelColor(x,y).lightness()>150)++visible;check(visible>10 && visible<200,"delete icon renders light strokes on dark background");}
 if(deleteButton)deleteButton->click();QTest::qWait(350);
 QElapsedTimer deleteWait;deleteWait.start();while(deleteWait.elapsed()<3000 && (listModel->getNoteIndex(deleteId).isValid() || !assertParent(deleteId,TRASH_FOLDER_ID)))QTest::qWait(20);
 check(assertParent(deleteId,TRASH_FOLDER_ID) && !listModel->getNoteIndex(deleteId).isValid(),"hover delete moves note to Trash through existing handler");
 window->findChild<QPushButton*>("newNoteButton")->click();QTest::qWait(700);
 const int emptyId=list->currentIndex().data(NoteListModel::NoteID).toInt();QCursor::setPos(list->viewport()->mapToGlobal(list->visualRect(list->currentIndex()).center()));QTest::qWait(180);
 if(deleteButton)deleteButton->click();QTest::qWait(600);
 check(!listModel->getNoteIndex(emptyId).isValid(),"hover delete also discards empty temporary note safely");
 check(dropNotes({noteIds[0]},child),"drop single note to nested folder accepted");check(assertParent(noteIds[0],child.data(NodeItem::NodeId).toInt()),"single note moved to nested folder in SQLite");
 check(dropNotes({noteIds[1],noteIds[2]},parent),"drop multiple notes accepted");check(assertParent(noteIds[1],folderId)&&assertParent(noteIds[2],folderId),"multiple notes moved in SQLite");
 QSqlQuery count(db);count.exec("select count(*) from node_table where node_type=0");check(count.next()&&count.value(0).toInt()>=3,"note records retained after moves");
 count.finish(); db.close();
 check(dropNotes({titleId},model->getDefaultNotesIndex()),"retain heading fixture outside deleted test folder");
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
