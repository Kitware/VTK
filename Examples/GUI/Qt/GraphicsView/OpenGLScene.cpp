
#include "OpenGLScene.hpp"
#include "TreeRingViewItem.h"
#include "GraphLayoutViewItem.h"
#include "WebView.h"
#include <QGraphicsProxyWidget>
#include <QGraphicsLinearLayout>
#include <QGraphicsSceneMouseEvent>
#include <QSignalTransition>
#include <QPropertyAnimation>
#include "QBoolAnimation.h"

OpenGLScene::OpenGLScene(QGLContext* ctx, QObject* p)
  : QGraphicsScene(p), mContext(ctx)
{
  int sz = 128;
  int gap = 10;
  QRectF activeRect(sz+2*gap, gap, 512, 512);

  mGraphLayoutView = new GraphLayoutViewItem(ctx);
  this->addItem(mGraphLayoutView);

  mTreeRingView = new TreeRingViewItem(ctx);
  this->addItem(mTreeRingView);

  QGraphicsProxyWidget* tmp = new QGraphicsProxyWidget;
  tmp->setWidget(new WebView);
  mWebView = tmp;
  tmp->setOpacity(0.8);
  this->addItem(mWebView);


  QState* state1 = new QState(&machine);
  QState* state2 = new QState(&machine);
  QState* state3 = new QState(&machine);
  QState* state4 = new QState(&machine);

  machine.setInitialState(state3);
  CurrentState = 2;

  // states
  state1->assignProperty(mGraphLayoutView, "geometry", activeRect);
  state1->assignProperty(mGraphLayoutView, "enabled", true);
  state1->assignProperty(mTreeRingView, "geometry", QRectF(gap,2*gap+sz,sz,sz));
  state1->assignProperty(mTreeRingView, "enabled", false);
  state1->assignProperty(mWebView, "geometry", QRectF(gap,4*gap+sz+sz,sz,sz));
  state1->assignProperty(mWebView, "enabled", false);

  state2->assignProperty(mGraphLayoutView, "geometry", QRectF(gap,gap,sz,sz));
  state2->assignProperty(mGraphLayoutView, "enabled", false);
  state2->assignProperty(mTreeRingView, "geometry", activeRect);
  state2->assignProperty(mTreeRingView, "enabled", true);
  state2->assignProperty(mWebView, "geometry", QRectF(gap,4*gap+sz+sz,sz,sz));
  state2->assignProperty(mWebView, "enabled", false);

  state3->assignProperty(mGraphLayoutView, "geometry", QRectF(gap,gap,sz,sz));
  state3->assignProperty(mGraphLayoutView, "enabled", false);
  state3->assignProperty(mTreeRingView, "geometry", QRectF(gap,2*gap+sz,sz,sz));
  state3->assignProperty(mTreeRingView, "enabled", false);
  state3->assignProperty(mWebView, "geometry", activeRect);
  state3->assignProperty(mWebView, "enabled", true);

  state4->assignProperty(mGraphLayoutView, "geometry", QRectF(gap,gap,sz,sz));
  state4->assignProperty(mGraphLayoutView, "enabled", false);
  state4->assignProperty(mTreeRingView, "geometry", QRectF(gap,2*gap+sz,sz,sz));
  state4->assignProperty(mTreeRingView, "enabled", false);
  state4->assignProperty(mWebView, "geometry", QRectF(gap,4*gap+sz+sz,sz,sz));
  state4->assignProperty(mWebView, "enabled", false);

  // transitions
  QAbstractTransition* trans;

  // 1 -> 3
  trans = state1->addTransition(this, SIGNAL(enterState3()), state3);
  trans->addAnimation(new QPropertyAnimation(mGraphLayoutView, "geometry"));
  trans->addAnimation(new QPropertyAnimation(mWebView, "geometry"));
  trans->addAnimation(new QBoolAnimation(1.0, mWebView, "enabled"));  // enable at end of transition

  // 1 -> 2
  trans = state1->addTransition(this, SIGNAL(enterState2()), state2);
  trans->addAnimation(new QPropertyAnimation(mGraphLayoutView, "geometry"));
  trans->addAnimation(new QPropertyAnimation(mTreeRingView, "geometry"));
  trans->addAnimation(new QBoolAnimation(1.0, mTreeRingView, "enabled"));  // enable at end of transition

  // 2 -> 3
  trans = state2->addTransition(this, SIGNAL(enterState3()), state3);
  trans->addAnimation(new QPropertyAnimation(mTreeRingView, "geometry"));
  trans->addAnimation(new QPropertyAnimation(mWebView, "geometry"));
  trans->addAnimation(new QBoolAnimation(1.0, mWebView, "enabled"));  // enable at end of transition

  // 2 -> 1
  trans = state2->addTransition(this, SIGNAL(enterState1()), state1);
  trans->addAnimation(new QPropertyAnimation(mTreeRingView, "geometry"));
  trans->addAnimation(new QPropertyAnimation(mGraphLayoutView, "geometry"));
  trans->addAnimation(new QBoolAnimation(1.0, mGraphLayoutView, "enabled"));  // enable at end of transition

  // 3 -> 1
  trans = state3->addTransition(this, SIGNAL(enterState1()), state1);
  trans->addAnimation(new QPropertyAnimation(mWebView, "geometry"));
  trans->addAnimation(new QPropertyAnimation(mGraphLayoutView, "geometry"));
  trans->addAnimation(new QBoolAnimation(1.0, mGraphLayoutView, "enabled"));  // enable at end of transition

  // 3 -> 2
  trans = state3->addTransition(this, SIGNAL(enterState2()), state2);
  trans->addAnimation(new QPropertyAnimation(mWebView, "geometry"));
  trans->addAnimation(new QPropertyAnimation(mTreeRingView, "geometry"));
  trans->addAnimation(new QBoolAnimation(1.0, mTreeRingView, "enabled")); // enable at end of transition

  // non animated transitions
  trans = state1->addTransition(this, SIGNAL(enterState4()), state4);
  trans = state2->addTransition(this, SIGNAL(enterState4()), state4);
  trans = state3->addTransition(this, SIGNAL(enterState4()), state4);
  trans = state4->addTransition(this, SIGNAL(enterState1()), state1);
  trans = state4->addTransition(this, SIGNAL(enterState2()), state2);
  trans = state4->addTransition(this, SIGNAL(enterState3()), state3);

  machine.start();

}

OpenGLScene::~OpenGLScene()
{
}

void OpenGLScene::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
  QGraphicsScene::mousePressEvent(e);

  // see if its under one our our deactivated items
  QGraphicsItem* item = itemAt(e->scenePos());
  if(item == mGraphLayoutView && CurrentState != 0)
  {
    e->accept();
    CurrentState = 0;
    emit enterState1();
  }
  else if(item == mTreeRingView && CurrentState != 1)
  {
    e->accept();
    CurrentState = 1;
    emit enterState2();
  }
  else if(item == mWebView && CurrentState != 2)
  {
    e->accept();
    CurrentState = 2;
    emit enterState3();
  }
  else if(!item)
  {
    CurrentState = 3;
    emit enterState4();
  }
}
