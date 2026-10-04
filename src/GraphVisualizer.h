#pragma once
#include <QString>
#include <QByteArray>
#include <QQuickPaintedItem>
#include <QPainter>
#include <QImage>
#include <QSvgRenderer>

class GraphVisualizer : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QString dotSource READ dotSource WRITE setDotSource NOTIFY dotSourceChanged)
    Q_PROPERTY(qreal zoomFactor READ zoomFactor WRITE setZoomFactor NOTIFY zoomFactorChanged)
    Q_PROPERTY(qreal panX READ panX WRITE setPanX NOTIFY panChanged)
    Q_PROPERTY(qreal panY READ panY WRITE setPanY NOTIFY panChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool hasGraphviz READ hasGraphviz CONSTANT)

public:
    explicit GraphVisualizer(QQuickItem *parent = nullptr);

    QString dotSource() const { return m_dotSource; }
    void setDotSource(const QString &source);

    qreal zoomFactor() const { return m_zoomFactor; }
    void setZoomFactor(qreal factor);

    qreal panX() const { return m_panX; }
    void setPanX(qreal x);

    qreal panY() const { return m_panY; }
    void setPanY(qreal y);

    QString statusMessage() const { return m_statusMessage; }
    bool hasGraphviz() const { return m_hasGraphviz; }

    void paint(QPainter *painter) override;

    Q_INVOKABLE void resetView();
    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();
    Q_INVOKABLE void panBy(qreal dx, qreal dy);

signals:
    void dotSourceChanged();
    void zoomFactorChanged();
    void panChanged();
    void statusMessageChanged();

private:
    void renderDot();
    void renderFallbackGraph(QPainter *painter);

    QString m_dotSource;
    qreal m_zoomFactor = 1.0;
    qreal m_panX = 0.0;
    qreal m_panY = 0.0;
    QString m_statusMessage;
    bool m_hasGraphviz = false;
    QString m_dotExecutable;

    QByteArray m_renderedSvg;
    QSvgRenderer m_svgRenderer;
    bool m_svgLoaded = false;
};
