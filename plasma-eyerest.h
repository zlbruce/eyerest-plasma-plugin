// Here we avoid loading the header multiple times
#ifndef __PLASMA_EYEREST_H__
#define __PLASMA_EYEREST_H__
// We need the Plasma Applet headers
#include <KIcon>

#include <Plasma/Applet>
#include <Plasma/Svg>
#include <Plasma/PopupApplet>
#include <QDBusInterface>
#include <QString>

#include "ui_eyerest-config.h"

#include "eyerest_dbus.h"

class QSizeF;
class QAction;

namespace Plasma
{
    class Label;
}

// Define our plasma Applet
class PlasmaEyerest: public Plasma::Applet
{
    Q_OBJECT
public:
        // Basic Create/Destroy
        PlasmaEyerest(QObject *parent, const QVariantList &args);
        ~PlasmaEyerest();

        // The paintInterface procedure paints the applet to screen
        void paintInterface(QPainter *p,
                const QStyleOptionGraphicsItem *option,
                const QRect& contentsRect);
        virtual void init();

        virtual void createConfigurationInterface(KConfigDialog* parent);

        QList<QAction*>  contextualActions ();

private:
        QString m_time_text;
        QAction *m_menu_state;

        org::zlbruce::eyerest::basic* m_eye_proxy;
        void prepare_font(QFont &font, QRect &rect, const QString &text);

        // config
        Ui::Config m_config;
        QString m_format;
        QFont m_font;
        QColor m_color;

private Q_SLOTS: 
        void on_status_change(uint time_remain, const QString state);
        void on_delay(int time);
        void on_pause();
        void on_unpause();
        void on_rest_now();

        void on_config_accepted();
};

#endif
