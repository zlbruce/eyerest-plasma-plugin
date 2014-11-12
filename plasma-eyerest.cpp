#include "plasma-eyerest.h"
 
#include <QPainter>
#include <QFontMetrics>
#include <QSizeF>
#include <QAction>
#include <QTime>
#include <KConfigDialog>
#include <KGlobalSettings>
#include <QGraphicsLinearLayout>
 
#include <plasma/theme.h>
#include <plasma/widgets/lineedit.h>
#include <plasma/widgets/label.h>
#include <plasma/widgets/pushbutton.h>


PlasmaEyerest::PlasmaEyerest(QObject *parent, const QVariantList &args)
    : Plasma::Applet(parent, args),
    m_time_text("Eyerest"),
    m_menu_state(NULL),
    m_eye_proxy(NULL),
    m_font(KGlobalSettings::generalFont()),
    m_color(Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor))
{
    // this will get us the standard applet background, for free!
    setBackgroundHints(DefaultBackground);
    setHasConfigurationInterface(true);
    resize(250, 100);
}


PlasmaEyerest::~PlasmaEyerest()
{
    if (hasFailedToLaunch()) {
        // Do some cleanup here
    } else {
        // Save settings
    }
}

QList<QAction*> PlasmaEyerest::contextualActions()
{
    m_menu_state = new QAction(this);
    m_menu_state->setText(i18n("State: "));

    QAction* sep0 = new QAction(this);
    sep0->setSeparator(true);

    QAction* delay3min = new QAction(this);
    delay3min->setText(i18n("Delay 3 min"));

    QAction* delay5min = new QAction(this);
    delay5min->setText(i18n("Delay 5 min"));

    QSignalMapper* signalMapper = new QSignalMapper (this);
    connect (delay3min, SIGNAL(triggered()), signalMapper, SLOT(map())) ;
    connect (delay5min, SIGNAL(triggered()), signalMapper, SLOT(map())) ;

    signalMapper->setMapping (delay3min, 180) ;
    signalMapper->setMapping (delay5min, 300) ;

    connect (signalMapper, SIGNAL(mapped(int)), this, SLOT(on_delay(int))) ;


    QAction* sep1 = new QAction(this);
    sep1->setSeparator(true);

    QAction* pause = new QAction(this);
    pause->setText(i18n("Pause"));
    connect(pause, SIGNAL(triggered()), this, SLOT(on_pause()));

    QAction* unpause = new QAction(this);
    unpause->setText(i18n("Continue"));
    connect(unpause, SIGNAL(triggered()), this, SLOT(on_unpause()));

    QAction* rest_now = new QAction(this);
    rest_now->setText(i18n("Rest now"));
    connect(rest_now, SIGNAL(triggered()), this, SLOT(on_rest_now()));

    QList<QAction*> contextualActions;
    contextualActions << m_menu_state
                      << sep0
                      << delay3min
                      << delay5min
                      << sep1
                      << pause
                      << unpause
                      << rest_now;

    return contextualActions;
}


void PlasmaEyerest::init()
{
    KConfigGroup cg = config();

    m_format = cg.readEntry("format", "mm:ss");
    m_font = cg.readEntry("font", m_font);
    m_color = cg.readEntry("color", m_color);

    m_eye_proxy = new org::zlbruce::eyerest::basic("org.zlbruce.eyerest",
            "/",
            QDBusConnection::sessionBus(),
            this);

    // 让 eyerest-daemon 运行
    m_eye_proxy->unpause();

    connect(m_eye_proxy, SIGNAL(status(uint, const QString)), this, SLOT(on_status_change(uint, const QString)));

} 

void PlasmaEyerest::prepare_font(QFont &font, QRect &rect, const QString &text)
{
    QRect tmpRect;
    bool first = true;

    // 从最小的字体开始，知道填满整个区域
    const int smallest = KGlobalSettings::smallestReadableFont().pointSize();
    font.setPointSize(smallest);

    do {
        if (first) {
            first = false;
        } else  {
            font.setPointSize(font.pointSize() + 1);
        }

        const QFontMetrics fm(font);        
        tmpRect = fm.boundingRect(rect, Qt::TextSingleLine | Qt::AlignCenter, text);
        if (tmpRect.width() >= rect.width() || tmpRect.height() > rect.height())
        {
            break;
        }
    } while (true);

    rect = tmpRect;
}


void PlasmaEyerest::paintInterface(QPainter *p,
        const QStyleOptionGraphicsItem *option, const QRect &contentsRect)
{
    Q_UNUSED(option);

    p->setRenderHint(QPainter::SmoothPixmapTransform);
    p->setRenderHint(QPainter::Antialiasing);

    QRect tmpRect = contentsRect;
    QFont tmpFont = m_font;
    const QString fake_time_string = QTime(23,59,59).toString(m_format);

    prepare_font(tmpFont, tmpRect, fake_time_string);

    p->setFont(tmpFont);
    p->setPen(QPen(m_color));

    //qDebug() << "contentsRect: " << contentsRect << ", tmpRect: " << tmpRect;
    //qDebug() << "m_font: " << m_font << ", tmpFont: " << tmpFont;

    p->drawText(tmpRect, Qt::TextSingleLine | Qt::AlignCenter, m_time_text);

}
void PlasmaEyerest::on_status_change(uint time_remain, const QString state)
{
    QTime tm = QTime().addSecs(time_remain);
    m_time_text = tm.toString(m_format);
    if(m_menu_state)
    {
        m_menu_state->setText(i18n("State: ") + state);
    }

    update();
}

void PlasmaEyerest::on_delay(int time)
{
    m_eye_proxy->delay(time);
}
void PlasmaEyerest::on_pause()
{
    m_eye_proxy->pause();
}
void PlasmaEyerest::on_unpause()
{
    m_eye_proxy->unpause();
}
void PlasmaEyerest::on_rest_now()
{
    m_eye_proxy->rest_now();
}

void PlasmaEyerest::createConfigurationInterface(KConfigDialog* parent)
{
    QWidget *widget = new QWidget();
    m_config.setupUi(widget);

    m_config.format->setText(m_format);
    m_config.font->setCurrentFont(m_font);
    m_config.color->setColor(m_color);

    parent->addPage(widget, "Eyerest", "eyerest");

    connect(parent, SIGNAL(applyClicked()), this, SLOT(on_config_accepted()));
    connect(parent, SIGNAL(okClicked()), this, SLOT(on_config_accepted()));

    connect(m_config.format, SIGNAL(textChanged(QString)), parent, SLOT(settingsModified()));
    connect(m_config.font, SIGNAL(currentFontChanged(QFont)), parent, SLOT(settingsModified()));
    connect(m_config.color, SIGNAL(changed(QColor)), parent, SLOT(settingsModified()));
}

void PlasmaEyerest::on_config_accepted()
{
    KConfigGroup cg = config();

    m_format = m_config.format->text();
    m_font = m_config.font->currentFont();
    m_color = m_config.color->color();

    cg.writeEntry("format", m_format);
    cg.writeEntry("font", m_font);
    cg.writeEntry("color", m_color);
}


// This is the command that links your applet to the .desktop file
K_EXPORT_PLASMA_APPLET(eyerest, PlasmaEyerest)

#include "plasma-eyerest.moc"


