#ifndef QMLED_H
#define QMLED_H

#include <QObject>

namespace Maemo
{
/**
 * @brief Provides information and actions on device LED.
 */
class QmLED : public QObject
{
    Q_OBJECT

public:
    QmLED(QObject *parent = 0);
    ~QmLED();

    /**
     * Activate pattern in device LED.
     * @param pattern Pattern to activate.
     * @return True on success, false otherwise.
     * @todo Add description of strings for patterns.
     */
    bool activate(const QString &pattern);

    /**
     * Deactivate pattern in device LED.
     * @param pattern Pattern to deactivate.
     * @return True on success, false otherwise.
     * @todo Add description of string for patterns.
     */
    bool deactivate(const QString &pattern);

};

} //namespace Maemo


#endif // QMLED_H
