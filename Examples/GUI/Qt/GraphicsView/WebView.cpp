
#include "WebView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QLineEdit>
#include <QStyle>

WebView::WebView(QWidget* p)
: QFrame(p)
{
  QVBoxLayout* l = new QVBoxLayout(this);
  QHBoxLayout* hl = new QHBoxLayout;

  QToolButton* left = new QToolButton(this);
  left->setIcon(left->style()->standardIcon(QStyle::SP_ArrowLeft));
  QToolButton* right = new QToolButton(this);
  right->setIcon(right->style()->standardIcon(QStyle::SP_ArrowRight));
  mAddress = new QLineEdit(this);

  mWebView = new QWebView(this);
  mWebView->load(QUrl("http://www.google.com"));
  hl->addWidget(left);
  hl->addWidget(right);
  hl->addWidget(mAddress);
  l->addLayout(hl);
  l->addWidget(mWebView);

  QObject::connect(left, SIGNAL(clicked()), mWebView, SLOT(back()));
  QObject::connect(right, SIGNAL(clicked()), mWebView, SLOT(forward()));
  QObject::connect(mAddress, SIGNAL(editingFinished()), this, SLOT(go()));
  QObject::connect(mWebView, SIGNAL(urlChanged(const QUrl&)), this, SLOT(updateUrl(const QUrl&)));
}

WebView::~WebView()
{
}

void WebView::go()
{
  mWebView->load(QUrl(mAddress->text()));
}

void WebView::updateUrl(const QUrl& url)
{
  QString s = url.toString();
  mAddress->setText(s);
}
