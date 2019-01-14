
#ifndef QBoolAnimation_h
#define QBoolAnimation_h

#include "QPropertyAnimation"

class QBoolAnimation : public QPropertyAnimation
{
  Q_OBJECT
public:
    QBoolAnimation(double tipping_point, QObject* target, const QByteArray & prop, QObject * p = 0 )
      : QPropertyAnimation(target, prop, p), mTippingPoint(tipping_point)
      {}
protected:
    QVariant interpolated(const QVariant& from, const QVariant& to, qreal progress) const
    {
      double f = from.toDouble();
      double t = to.toDouble();
      double i = QPropertyAnimation::interpolated(f, t, progress).toDouble();

      if(f < t)
        return i >= mTippingPoint;
      return i <= mTippingPoint;
    }
    double mTippingPoint;
};

#endif
