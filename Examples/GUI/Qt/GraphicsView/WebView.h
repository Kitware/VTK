
#ifndef WebView_h
#define WebView_h

#include <QFrame>
#include <QWebView>
#include <QLineEdit>

class WebView : public QFrame
{
  Q_OBJECT
  public:
    WebView(QWidget* p=0);
    ~WebView();

    QWebView* webview();

  public Q_SLOTS:
    void go();

  protected Q_SLOTS:
    void updateUrl(const QUrl& str);

  protected:
    QWebView* mWebView;
    QLineEdit* mAddress;


};

#endif
